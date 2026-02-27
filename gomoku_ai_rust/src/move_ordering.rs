// ============================================
// Move Ordering — Exact match of search_ordering.cpp
// Candidate generation with adaptive radius,
// quick evaluation with history heuristic support
// ============================================

use crate::game_types::*;

pub struct MoveOrdering;

impl MoveOrdering {
    // ============================================
    // MAIN ENTRY — generateOrderedMoves
    // C++ generateOrderedMoves simply calls
    // generateCandidatesAdaptiveRadius which does
    // all the work (generate, order, truncate).
    // ============================================

    pub fn generate_ordered_moves(
        state: &GameState,
        previous_best: &Move,
        history_table: &[[i32; BOARD_SIZE]; BOARD_SIZE],
    ) -> Vec<Move> {
        Self::generate_candidates_adaptive_radius(state, previous_best, history_table)
    }

    // ============================================
    // CANDIDATE GENERATION — adaptive radius
    // Matches C++ generateCandidatesAdaptiveRadius:
    //   1. Mark zones near pieces
    //   2. Extend zone around lastHumanMove
    //   3. Collect candidates
    //   4. Order with orderMovesWithPreviousBest (quickEvaluateMove)
    //   5. Truncate to maxCandidates
    // ============================================

    fn generate_candidates_adaptive_radius(
        state: &GameState,
        previous_best: &Move,
        history_table: &[[i32; BOARD_SIZE]; BOARD_SIZE],
    ) -> Vec<Move> {
        let search_radius = Self::get_search_radius_for_game_phase(state);
        let max_candidates = Self::get_max_candidates_for_game_phase(state);

        let mut relevant_zone = [[false; BOARD_SIZE]; BOARD_SIZE];

        // Mark positions near existing pieces
        for i in 0..BOARD_SIZE as i32 {
            for j in 0..BOARD_SIZE as i32 {
                if state.get_piece(i, j) != EMPTY {
                    for di in -search_radius..=search_radius {
                        for dj in -search_radius..=search_radius {
                            let ni = i + di;
                            let nj = j + dj;
                            if state.is_valid(ni, nj) && state.is_empty(ni, nj) {
                                relevant_zone[ni as usize][nj as usize] = true;
                            }
                        }
                    }
                }
            }
        }

        // Mark zone around opponent's last move (tactical priority)
        if state.last_human_move.is_valid() {
            let lx = state.last_human_move.x;
            let ly = state.last_human_move.y;
            let extended_radius = search_radius + 1;
            for di in -extended_radius..=extended_radius {
                for dj in -extended_radius..=extended_radius {
                    let ni = lx + di;
                    let nj = ly + dj;
                    if state.is_valid(ni, nj) && state.is_empty(ni, nj) {
                        relevant_zone[ni as usize][nj as usize] = true;
                    }
                }
            }
        }

        // Collect candidates from marked zones
        let mut candidates = Vec::new();
        for i in 0..BOARD_SIZE {
            for j in 0..BOARD_SIZE {
                if relevant_zone[i][j] {
                    candidates.push(Move::new(i as i32, j as i32));
                }
            }
        }

        // If no candidates (e.g. empty board), play center
        if candidates.is_empty() {
            candidates.push(Move::new(BOARD_CENTER, BOARD_CENTER));
            return candidates;
        }

        // Sort with move ordering (matching C++ orderMovesWithPreviousBest)
        Self::order_moves_with_previous_best(&mut candidates, state, previous_best, history_table);

        // Limit number of candidates (matching C++)
        if candidates.len() > max_candidates {
            candidates.truncate(max_candidates);
        }

        candidates
    }

    fn get_search_radius_for_game_phase(state: &GameState) -> i32 {
        if state.turn_count <= 6 {
            2
        } else {
            1
        }
    }

    fn get_max_candidates_for_game_phase(state: &GameState) -> usize {
        if state.turn_count <= 4 {
            3
        } else if state.turn_count <= 10 {
            4
        } else {
            5
        }
    }

    // ============================================
    // MOVE ORDERING — matching C++ orderMovesWithPreviousBest
    // ============================================

    fn order_moves_with_previous_best(
        moves: &mut Vec<Move>,
        state: &GameState,
        previous_best: &Move,
        history_table: &[[i32; BOARD_SIZE]; BOARD_SIZE],
    ) {
        // If we have the best move from previous iteration, place it first
        if previous_best.is_valid() {
            if let Some(pos) = moves.iter().position(|m| m == previous_best) {
                // Move to front
                moves.swap(0, pos);

                // Sort the rest by quickEvaluateMove
                if moves.len() > 1 {
                    // Pre-compute scores for the rest
                    let mut scored: Vec<(i32, Move)> = moves[1..]
                        .iter()
                        .map(|mv| (Self::quick_evaluate_move(state, mv, history_table), *mv))
                        .collect();
                    scored.sort_by(|a, b| b.0.cmp(&a.0));
                    for (i, (_, mv)) in scored.into_iter().enumerate() {
                        moves[i + 1] = mv;
                    }
                }

                return;
            }
        }

        // No previous best move found, use standard ordering
        Self::order_moves(moves, state, history_table);
    }

    fn order_moves(
        moves: &mut Vec<Move>,
        state: &GameState,
        history_table: &[[i32; BOARD_SIZE]; BOARD_SIZE],
    ) {
        // For very few moves, ordering has minimal impact
        if moves.len() <= 2 {
            return;
        }

        // Pre-compute scores once
        let mut scored: Vec<(i32, Move)> = moves
            .iter()
            .map(|mv| (Self::quick_evaluate_move(state, mv, history_table), *mv))
            .collect();
        scored.sort_by(|a, b| b.0.cmp(&a.0));
        for (i, (_, mv)) in scored.into_iter().enumerate() {
            moves[i] = mv;
        }
    }

    // ============================================
    // QUICK MOVE EVALUATION — exact match of C++ quickEvaluateMove
    // ============================================

    pub fn quick_evaluate_move(
        state: &GameState,
        mv: &Move,
        history_table: &[[i32; BOARD_SIZE]; BOARD_SIZE],
    ) -> i32 {
        let mut score = 0;
        let player = state.current_player;
        let opponent = state.get_opponent(player);

        // 1. CENTRALITY (Chebyshev distance, 0-90 points)
        let center_dist = (mv.x - BOARD_CENTER).abs().max((mv.y - BOARD_CENTER).abs());
        score += (9 - center_dist) * 10;

        // 2. IMMEDIATE CONNECTIVITY
        let mut my_adjacent = 0;
        let mut opp_adjacent = 0;
        for &(ddx, ddy) in ALL_DIRECTIONS.iter() {
            let nx = mv.x + ddx;
            let ny = mv.y + ddy;
            if state.is_valid(nx, ny) {
                let piece = state.get_piece(nx, ny);
                if piece == player {
                    my_adjacent += 1;
                } else if piece == opponent {
                    opp_adjacent += 1;
                }
            }
        }
        score += my_adjacent * 50; // 0-400 points
        score += opp_adjacent * 20; // Minor bonus for blocking

        // 3. ACTIVE ZONE PRIORITY — near opponent's last move
        if state.last_human_move.is_valid() {
            let dist = (mv.x - state.last_human_move.x)
                .abs()
                .max((mv.y - state.last_human_move.y).abs());
            if dist <= 2 {
                score += 500; // Tactical response
            }
        }

        // 4. SIMPLE PATTERNS — count consecutive pieces, take MAX across directions
        let mut max_my_line = 0;
        let mut max_opp_line = 0;

        for &(ddx, ddy) in MAIN_DIRECTIONS.iter() {
            // Count own pieces: starts at 1 (the piece being placed)
            let mut my_count: i32 = 1;
            my_count +=
                Self::count_consecutive_in_direction(state, mv.x, mv.y, ddx, ddy, player, 4);
            my_count +=
                Self::count_consecutive_in_direction(state, mv.x, mv.y, -ddx, -ddy, player, 4);
            max_my_line = max_my_line.max(my_count);

            // Count opponent lines (for blocking evaluation)
            let mut opp_count: i32 = 0;
            opp_count +=
                Self::count_consecutive_in_direction(state, mv.x, mv.y, ddx, ddy, opponent, 4);
            opp_count +=
                Self::count_consecutive_in_direction(state, mv.x, mv.y, -ddx, -ddy, opponent, 4);
            max_opp_line = max_opp_line.max(opp_count);
        }

        // Exponential scoring for long lines
        if max_my_line >= 5 {
            score += 100000; // Win
        } else if max_my_line == 4 {
            score += 10000; // Very dangerous
        } else if max_my_line == 3 {
            score += 1000; // Dangerous
        } else if max_my_line == 2 {
            score += 100; // Development
        }

        // Blocking
        if max_opp_line >= 4 {
            score += 8000; // Critical block
        } else if max_opp_line == 3 {
            score += 800; // Important block
        }

        // 5. QUICK CAPTURE CHECK — pattern: NEW + OPP + OPP + OWN
        'capture: for &(ddx, ddy) in ALL_DIRECTIONS.iter() {
            let x1 = mv.x + ddx;
            let y1 = mv.y + ddy;
            let x2 = mv.x + 2 * ddx;
            let y2 = mv.y + 2 * ddy;
            let x3 = mv.x + 3 * ddx;
            let y3 = mv.y + 3 * ddy;

            if state.is_valid(x1, y1) && state.is_valid(x2, y2) && state.is_valid(x3, y3) {
                if state.get_piece(x1, y1) == opponent
                    && state.get_piece(x2, y2) == opponent
                    && state.get_piece(x3, y3) == player
                {
                    score += 2000; // Capture available
                    break 'capture; // Stop searching
                }
            }
        }

        // 6. HISTORY HEURISTIC
        score += history_table[mv.x as usize][mv.y as usize];

        score
    }

    /// Count consecutive pieces in a direction from (x,y), not including (x,y) itself.
    /// Matches C++ countConsecutiveInDirection with maxCount limit.
    fn count_consecutive_in_direction(
        state: &GameState,
        x: i32,
        y: i32,
        dx: i32,
        dy: i32,
        player: i32,
        max_count: i32,
    ) -> i32 {
        let mut count = 0;
        let mut cx = x + dx;
        let mut cy = y + dy;
        while count < max_count && state.is_valid(cx, cy) && state.get_piece(cx, cy) == player {
            count += 1;
            cx += dx;
            cy += dy;
        }
        count
    }
}
