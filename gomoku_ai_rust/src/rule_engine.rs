// ============================================
// Rule Engine — Exact match of rule_engine.hpp / rules_*.cpp
// Handles: move application, captures, win detection,
//          double free-three validation
// ============================================

use crate::game_types::*;
use crate::zobrist::get_zobrist;

pub struct MoveResult {
    pub success: bool,
    pub creates_win: bool,
    pub my_captured_pieces: Vec<Move>,
}

impl MoveResult {
    fn new() -> Self {
        MoveResult {
            success: false,
            creates_win: false,
            my_captured_pieces: Vec::new(),
        }
    }
}

pub struct RuleEngine;

impl RuleEngine {
    // ============================================
    // MOVE APPLICATION — mirrors rules_core.cpp
    // ============================================

    pub fn apply_move(state: &mut GameState, mv: &Move) -> MoveResult {
        let mut result = MoveResult::new();

        // 1. Verify the move is valid
        if !state.is_empty(mv.x, mv.y) {
            return result;
        }

        // 2. Check double free-three before placing
        if Self::creates_double_free_three(state, mv, state.current_player) {
            return result;
        }

        let current_player = state.current_player;
        let player_idx = (current_player - 1) as usize;
        let old_captures = state.captures[player_idx];
        let opponent = state.get_opponent(current_player);

        // 3. Place the piece
        state.board[mv.x as usize][mv.y as usize] = current_player;

        // 4. Find and apply captures
        result.my_captured_pieces = Self::find_captures(state, mv, current_player);

        for captured in &result.my_captured_pieces {
            state.board[captured.x as usize][captured.y as usize] = EMPTY;
        }
        state.captures[player_idx] += (result.my_captured_pieces.len() / 2) as i32;
        if state.captures[player_idx] > 10 {
            state.captures[player_idx] = 10;
        }

        // 5. Check for win
        result.creates_win = Self::check_win(state, current_player);

        // 6. Update Zobrist hash incrementally
        let z = get_zobrist();
        let mut hash = state.zobrist_hash;

        // Place new piece
        hash ^= z.piece_key(mv.x, mv.y, current_player);

        // Remove captured pieces
        for captured in &result.my_captured_pieces {
            hash ^= z.piece_key(captured.x, captured.y, opponent);
        }

        // Update capture hash
        let new_captures = state.captures[player_idx];
        hash ^= z.capture_key(player_idx, old_captures);
        hash ^= z.capture_key(player_idx, new_captures);

        // Switch turn
        hash ^= z.turn_key();

        state.zobrist_hash = hash;

        // 7. Advance turn
        state.current_player = state.get_opponent(state.current_player);
        state.turn_count += 1;

        result.success = true;
        result
    }

    // ============================================
    // CAPTURE DETECTION — mirrors rules_capture.cpp
    // ============================================

    pub fn find_captures(state: &GameState, mv: &Move, player: i32) -> Vec<Move> {
        let mut captures = Vec::new();
        let opponent = state.get_opponent(player);

        // Search all 8 directions: pattern PLAYER-OPP-OPP-PLAYER
        for &(dx, dy) in ALL_DIRECTIONS.iter() {
            let p1x = mv.x + dx;
            let p1y = mv.y + dy;
            let p2x = mv.x + 2 * dx;
            let p2y = mv.y + 2 * dy;
            let p3x = mv.x + 3 * dx;
            let p3y = mv.y + 3 * dy;

            if state.is_valid(p1x, p1y)
                && state.is_valid(p2x, p2y)
                && state.is_valid(p3x, p3y)
                && state.get_piece(p1x, p1y) == opponent
                && state.get_piece(p2x, p2y) == opponent
                && state.get_piece(p3x, p3y) == player
            {
                captures.push(Move::new(p1x, p1y));
                captures.push(Move::new(p2x, p2y));
            }
        }

        captures
    }

    // ============================================
    // WIN DETECTION — mirrors rules_win.cpp
    // ============================================

    pub fn check_win(state: &GameState, player: i32) -> bool {
        let opponent = state.get_opponent(player);

        // 1. Win by captures
        if state.captures[(player - 1) as usize] >= WIN_CAPTURES_NORMAL {
            return true;
        }

        // 2. Win by five in a row (with verification)
        for i in 0..BOARD_SIZE {
            for j in 0..BOARD_SIZE {
                if state.board[i][j] == player {
                    let pos = Move::new(i as i32, j as i32);

                    for &(dx, dy) in MAIN_DIRECTIONS.iter() {
                        if Self::check_line_win_in_direction(state, &pos, dx, dy, player) {
                            // Verification: can opponent break it via capture?
                            if Self::can_break_line_by_capture(state, &pos, dx, dy, player) {
                                continue;
                            }

                            // Verification: is opponent close to capture win?
                            if state.captures[(opponent - 1) as usize] >= 8
                                && Self::opponent_can_capture_next_turn(state, opponent)
                            {
                                return false;
                            }

                            return true;
                        }
                    }
                }
            }
        }

        false
    }

    fn check_line_win_in_direction(
        state: &GameState, start: &Move, dx: i32, dy: i32, player: i32,
    ) -> bool {
        // Only count if start is the actual beginning of the line
        let bx = start.x - dx;
        let by = start.y - dy;
        if state.is_valid(bx, by) && state.get_piece(bx, by) == player {
            return false;
        }

        let mut count = 1;
        let mut cx = start.x + dx;
        let mut cy = start.y + dy;
        while state.is_valid(cx, cy) && state.get_piece(cx, cy) == player {
            count += 1;
            cx += dx;
            cy += dy;
        }

        count >= 5
    }

    fn can_break_line_by_capture(
        state: &GameState, line_start: &Move, dx: i32, dy: i32, winning_player: i32,
    ) -> bool {
        let opponent = state.get_opponent(winning_player);

        // Collect first 5 positions of the line
        for i in 0..5 {
            let px = line_start.x + i * dx;
            let py = line_start.y + i * dy;

            for &(cdx, cdy) in ALL_DIRECTIONS.iter() {
                let second = Move::new(px + cdx, py + cdy);
                let before = Move::new(px - cdx, py - cdy);
                let after = Move::new(second.x + cdx, second.y + cdy);

                // Pattern: OPP - PIECE - SECOND - EMPTY
                if state.is_valid(before.x, before.y)
                    && state.get_piece(before.x, before.y) == opponent
                    && state.is_valid(second.x, second.y)
                    && state.get_piece(second.x, second.y) == winning_player
                    && state.is_valid(after.x, after.y)
                    && state.is_empty(after.x, after.y)
                {
                    return true;
                }

                // Pattern: EMPTY - PIECE - SECOND - OPP
                if state.is_valid(after.x, after.y)
                    && state.get_piece(after.x, after.y) == opponent
                    && state.is_valid(second.x, second.y)
                    && state.get_piece(second.x, second.y) == winning_player
                    && state.is_valid(before.x, before.y)
                    && state.is_empty(before.x, before.y)
                {
                    return true;
                }
            }
        }

        false
    }

    fn opponent_can_capture_next_turn(state: &GameState, opponent: i32) -> bool {
        for i in 0..BOARD_SIZE {
            for j in 0..BOARD_SIZE {
                if state.is_empty(i as i32, j as i32) {
                    let test_move = Move::new(i as i32, j as i32);
                    let captures = Self::find_captures(state, &test_move, opponent);
                    if !captures.is_empty() {
                        return true;
                    }
                }
            }
        }
        false
    }

    // ============================================
    // LEGAL MOVE VALIDATION — mirrors rules_validation.cpp
    // ============================================

    pub fn is_legal_move(state: &GameState, mv: &Move) -> bool {
        if !state.is_empty(mv.x, mv.y) {
            return false;
        }
        !Self::creates_double_free_three(state, mv, state.current_player)
    }

    pub fn creates_double_free_three(state: &GameState, mv: &Move, player: i32) -> bool {
        let mut temp = state.clone();
        temp.board[mv.x as usize][mv.y as usize] = player;

        let mut free_three_count = 0;
        for &(dx, dy) in MAIN_DIRECTIONS.iter() {
            if Self::is_free_three(&temp, mv, dx, dy, player) {
                free_three_count += 1;
            }
        }
        free_three_count >= 2
    }

    fn is_free_three(state: &GameState, mv: &Move, dx: i32, dy: i32, player: i32) -> bool {
        let opponent = state.get_opponent(player);

        // Search all windows of 5 positions containing the move
        for offset in -4..=0i32 {
            let wx = mv.x + offset * dx;
            let wy = mv.y + offset * dy;

            // Verify window of 5 is within the board
            let mut valid = true;
            for i in 0..5 {
                if !state.is_valid(wx + i * dx, wy + i * dy) {
                    valid = false;
                    break;
                }
            }
            if !valid {
                continue;
            }

            // Verify the move is within this window
            let mut in_window = false;
            for i in 0..5 {
                if wx + i * dx == mv.x && wy + i * dy == mv.y {
                    in_window = true;
                    break;
                }
            }
            if !in_window {
                continue;
            }

            // Create window state
            let mut window = [0i32; 5];
            let mut player_count = 0;
            let mut opponent_count = 0;
            let mut empty_count = 0;

            for i in 0..5 {
                let px = wx + i * dx;
                let py = wy + i * dy;
                if px == mv.x && py == mv.y {
                    window[i as usize] = player;
                } else {
                    window[i as usize] = state.get_piece(px, py);
                }
                if window[i as usize] == player {
                    player_count += 1;
                } else if window[i as usize] == opponent {
                    opponent_count += 1;
                } else {
                    empty_count += 1;
                }
            }

            // Free-three: exactly 3 player pieces, 0 opponent, 2 empty
            if player_count == 3 && opponent_count == 0 && empty_count == 2 {
                let left_x = wx - dx;
                let left_y = wy - dy;
                let right_x = wx + 5 * dx;
                let right_y = wy + 5 * dy;

                let left_free =
                    state.is_valid(left_x, left_y) && state.is_empty(left_x, left_y);
                let right_free =
                    state.is_valid(right_x, right_y) && state.is_empty(right_x, right_y);

                if left_free && right_free && Self::is_valid_free_three_pattern(&window, player) {
                    return true;
                }
            }
        }

        false
    }

    fn is_valid_free_three_pattern(window: &[i32; 5], player: i32) -> bool {
        let e = 0; // EMPTY
        let p = player;
        let patterns: [[i32; 5]; 10] = [
            [p, p, p, e, e],
            [p, p, e, p, e],
            [p, p, e, e, p],
            [p, e, p, p, e],
            [p, e, p, e, p],
            [p, e, e, p, p],
            [e, p, p, p, e],
            [e, p, p, e, p],
            [e, p, e, p, p],
            [e, e, p, p, p],
        ];

        for pattern in &patterns {
            if window == pattern {
                return Self::can_form_threat(pattern, player);
            }
        }
        false
    }

    fn can_form_threat(pattern: &[i32; 5], player: i32) -> bool {
        for i in 0..5 {
            if pattern[i] == 0 {
                let mut temp = [0i32; 5];
                temp.copy_from_slice(pattern);
                temp[i] = player;
                if Self::has_four_consecutive(&temp, player) {
                    return true;
                }
            }
        }
        false
    }

    fn has_four_consecutive(pattern: &[i32; 5], player: i32) -> bool {
        for start in 0..=1 {
            let mut ok = true;
            for j in 0..4 {
                if pattern[start + j] != player {
                    ok = false;
                    break;
                }
            }
            if ok {
                return true;
            }
        }
        false
    }

    // ============================================
    // HELPER: count consecutive in a direction
    // ============================================

    pub fn count_in_direction(
        state: &GameState, start: &Move, dx: i32, dy: i32, player: i32,
    ) -> i32 {
        let mut count = 0;
        let mut x = start.x + dx;
        let mut y = start.y + dy;
        while state.is_valid(x, y) && state.get_piece(x, y) == player {
            count += 1;
            x += dx;
            y += dy;
        }
        count
    }
}
