// Move ordering and candidate generation - Similar to transposition_search.hpp/cpp (move generation part)

use crate::game_types::*;

pub struct MoveOrdering;

impl MoveOrdering {
    pub fn generate_candidates_adaptive_radius(state: &GameState) -> Vec<Move> {
        let mut candidates = Vec::new();
        let search_radius = Self::get_search_radius_for_game_phase(state.turn_count);
        let mut relevant_zone = vec![vec![false; BOARD_SIZE]; BOARD_SIZE];

        // Mark zones around existing pieces
        for i in 0..BOARD_SIZE {
            for j in 0..BOARD_SIZE {
                if state.board[i][j] != EMPTY {
                    for di in -(search_radius as i32)..=(search_radius as i32) {
                        for dj in -(search_radius as i32)..=(search_radius as i32) {
                            let ni = i as i32 + di;
                            let nj = j as i32 + dj;
                            if state.is_valid(ni, nj) && state.is_empty(ni, nj) {
                                relevant_zone[ni as usize][nj as usize] = true;
                            }
                        }
                    }
                }
            }
        }

        // Extended zone around last human move
        if state.last_human_move.is_valid() {
            let extended_radius = search_radius + 1;
            for di in -(extended_radius as i32)..=(extended_radius as i32) {
                for dj in -(extended_radius as i32)..=(extended_radius as i32) {
                    let ni = state.last_human_move.x + di;
                    let nj = state.last_human_move.y + dj;
                    if state.is_valid(ni, nj) && state.is_empty(ni, nj) {
                        relevant_zone[ni as usize][nj as usize] = true;
                    }
                }
            }
        }

        // Collect candidates
        for i in 0..BOARD_SIZE {
            for j in 0..BOARD_SIZE {
                if relevant_zone[i][j] {
                    candidates.push(Move::new(i as i32, j as i32));
                }
            }
        }

        // Limit candidates
        let max_candidates = Self::get_max_candidates_for_game_phase(state);
        if candidates.len() > max_candidates {
            // Order first before limiting
            let mut scored: Vec<(Move, i32)> = candidates.iter()
                .map(|&mv| (mv, Self::quick_evaluate_move(state, mv)))
                .collect();
            scored.sort_by(|a, b| b.1.cmp(&a.1));
            candidates = scored.into_iter().take(max_candidates).map(|(mv, _)| mv).collect();
        }

        candidates
    }

    pub fn order_moves_with_previous_best(moves: &mut Vec<Move>, state: &GameState, previous_best_move: Move) {
        if moves.len() <= 2 {
            return;
        }

        // Check if previous best move is in the list
        if previous_best_move.is_valid() {
            if let Some(pos) = moves.iter().position(|m| *m == previous_best_move) {
                moves.swap(0, pos);
                if moves.len() > 1 {
                    let rest = &mut moves[1..];
                    rest.sort_by(|a, b| {
                        let score_a = Self::quick_evaluate_move(state, *a);
                        let score_b = Self::quick_evaluate_move(state, *b);
                        score_b.cmp(&score_a)
                    });
                }
                return;
            }
        }

        // Normal ordering
        moves.sort_by(|a, b| {
            let score_a = Self::quick_evaluate_move(state, *a);
            let score_b = Self::quick_evaluate_move(state, *b);
            score_b.cmp(&score_a)
        });
    }

    pub fn quick_evaluate_move(state: &GameState, mv: Move) -> i32 {
        let mut score = 0;
        let current_player = state.current_player;
        let opponent = state.get_opponent(current_player);

        // Centrality
        let center_dist = (mv.x - 9).abs().max((mv.y - 9).abs());
        score += (9 - center_dist) * 10;

        // Connectivity
        let mut my_adjacent = 0;
        let mut opp_adjacent = 0;
        for dx in -1..=1 {
            for dy in -1..=1 {
                if dx == 0 && dy == 0 {
                    continue;
                }
                let nx = mv.x + dx;
                let ny = mv.y + dy;
                if state.is_valid(nx, ny) {
                    let piece = state.get_piece(nx, ny);
                    if piece == current_player {
                        my_adjacent += 1;
                    } else if piece == opponent {
                        opp_adjacent += 1;
                    }
                }
            }
        }
        score += my_adjacent * 50;
        score += opp_adjacent * 20;

        // Proximity to last move
        if state.last_human_move.is_valid() {
            let dist_to_last = (mv.x - state.last_human_move.x).abs()
                .max((mv.y - state.last_human_move.y).abs());
            if dist_to_last <= 2 {
                score += 500;
            }
        }

        // Simple pattern check
        let mut max_my_line = 0;
        let mut max_opp_line = 0;

        for (dx, dy) in MAIN_DIRECTIONS.iter() {
            let my_count = 1 + Self::count_consecutive_in_direction(state, mv.x, mv.y, *dx, *dy, current_player, 4)
                + Self::count_consecutive_in_direction(state, mv.x, mv.y, -dx, -dy, current_player, 4);
            max_my_line = max_my_line.max(my_count);

            let opp_count = Self::count_consecutive_in_direction(state, mv.x, mv.y, *dx, *dy, opponent, 4)
                + Self::count_consecutive_in_direction(state, mv.x, mv.y, -dx, -dy, opponent, 4);
            max_opp_line = max_opp_line.max(opp_count);
        }

        if max_my_line >= 5 {
            score += 100000;
        } else if max_my_line == 4 {
            score += 10000;
        } else if max_my_line == 3 {
            score += 1000;
        } else if max_my_line == 2 {
            score += 100;
        }

        if max_opp_line >= 4 {
            score += 8000;
        } else if max_opp_line == 3 {
            score += 800;
        }

        // Quick capture check
        for (dx, dy) in CAPTURE_DIRECTIONS.iter() {
            let x1 = mv.x + dx;
            let y1 = mv.y + dy;
            let x2 = mv.x + 2 * dx;
            let y2 = mv.y + 2 * dy;
            let x3 = mv.x + 3 * dx;
            let y3 = mv.y + 3 * dy;

            if state.is_valid(x1, y1) && state.is_valid(x2, y2) && state.is_valid(x3, y3) {
                if state.get_piece(x1, y1) == opponent
                    && state.get_piece(x2, y2) == opponent
                    && state.get_piece(x3, y3) == current_player
                {
                    score += 2000;
                    break;
                }
            }
        }

        score
    }

    fn count_consecutive_in_direction(state: &GameState, mut x: i32, mut y: i32, dx: i32, dy: i32, player: i32, max_count: i32) -> i32 {
        let mut count = 0;
        x += dx;
        y += dy;

        while count < max_count && state.is_valid(x, y) && state.get_piece(x, y) == player {
            count += 1;
            x += dx;
            y += dy;
        }

        count
    }

    fn get_search_radius_for_game_phase(_piece_count: i32) -> usize {
        1
    }

    fn get_max_candidates_for_game_phase(state: &GameState) -> usize {
        let piece_count = state.turn_count;
        if piece_count <= 4 {
            3
        } else if piece_count <= 10 {
            4
        } else {
            5
        }
    }
}
