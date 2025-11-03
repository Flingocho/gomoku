// Evaluator - Similar to evaluator.hpp/cpp

use crate::game_types::*;

// Evaluator score constants matching C++
pub const WIN: i32 = 600000;
pub const FOUR_OPEN: i32 = 50000;
pub const FOUR_HALF: i32 = 25000;
pub const THREE_OPEN: i32 = 10000;
pub const THREE_HALF: i32 = 1500;
pub const TWO_OPEN: i32 = 100;

// These are kept for potential future use
#[allow(dead_code)]
pub const CAPTURE_OPPORTUNITY: i32 = 5000;
#[allow(dead_code)]
pub const CAPTURE_THREAT: i32 = 6000;

// Pattern information for evaluation
#[derive(Clone, Copy, Debug)]
pub struct PatternInfo {
    pub consecutive_count: i32,
    pub total_pieces: i32,
    pub free_ends: i32,
    pub has_gaps: bool,
    pub total_span: i32,
    pub gap_count: i32,
}

// Capture opportunity for evaluation (unused for now, but keeping for future enhancements)
#[derive(Clone)]
#[allow(dead_code)]
pub struct CaptureOpportunity {
    pub position: Move,
    pub captured: Vec<Move>,
}

pub struct Evaluator;

impl Evaluator {
    pub fn evaluate(state: &GameState, max_depth: i32, current_depth: i32) -> i32 {
        let mate_distance = max_depth - current_depth;

        // Check win conditions with mate distance
        if state.is_game_over() {
            // Determine winner
            if state.captures[PLAYER2 as usize - 1] >= 10 {
                return WIN - mate_distance;
            } else if state.captures[PLAYER1 as usize - 1] >= 10 {
                return -WIN + mate_distance;
            }
            
            // Check for 5-in-a-row (need to check who won based on last move)
            // Since game is over, the player who just moved must have won
            let last_player = 3 - state.current_player;
            if last_player == PLAYER2 {
                return WIN - mate_distance;
            } else {
                return -WIN + mate_distance;
            }
        }

        let ai_score = Self::evaluate_for_player(state, PLAYER2);
        let human_score = Self::evaluate_for_player(state, PLAYER1);

        ai_score - human_score
    }

    pub fn evaluate_for_player(state: &GameState, player: i32) -> i32 {
        let mut score = 0;

        // Immediate threats
        score += Self::evaluate_immediate_threats(state, player);

        // Position analysis (patterns)
        score += Self::analyze_position(state, player);

        score
    }

    pub fn evaluate_immediate_threats(state: &GameState, player: i32) -> i32 {
        let opponent = state.get_opponent(player);
        let mut threat_score = 0;

        let my_has_win_threats = Self::has_winning_threats(state, player);
        let opp_has_win_threats = Self::has_winning_threats(state, opponent);

        if my_has_win_threats {
            threat_score += 90000;
        }
        if opp_has_win_threats {
            threat_score -= 105000;
        }

        // Count 4-patterns
        let my_four_open = Self::count_pattern_type(state, player, 4, 2);
        let opp_four_open = Self::count_pattern_type(state, opponent, 4, 2);
        let my_four_half = Self::count_pattern_type(state, player, 4, 1);
        let opp_four_half = Self::count_pattern_type(state, opponent, 4, 1);

        if opp_four_open > 0 {
            threat_score -= 80000;
        }
        if opp_four_half > 0 {
            threat_score -= 60000;
        }
        if my_four_open > 0 {
            threat_score += 70000;
        }
        if my_four_half > 0 {
            threat_score += 40000;
        }

        threat_score
    }

    pub fn has_winning_threats(state: &GameState, player: i32) -> bool {
        let four_open = Self::count_pattern_type(state, player, 4, 2);
        if four_open > 0 {
            return true;
        }

        let four_half = Self::count_pattern_type(state, player, 4, 1);
        if four_half > 0 {
            return true;
        }

        let three_open = Self::count_pattern_type(state, player, 3, 2);
        if three_open >= 2 {
            return true;
        }

        false
    }

    pub fn count_pattern_type(state: &GameState, player: i32, consecutive_count: i32, free_ends: i32) -> i32 {
        let mut count = 0;
        let mut evaluated = vec![vec![vec![false; 4]; BOARD_SIZE]; BOARD_SIZE];

        for i in 0..BOARD_SIZE {
            for j in 0..BOARD_SIZE {
                if state.board[i][j] != player {
                    continue;
                }

                for d in 0..4 {
                    if evaluated[i][j][d] {
                        continue;
                    }

                    let (dx, dy) = MAIN_DIRECTIONS[d];
                    
                    if Self::is_line_start(state, i as i32, j as i32, dx, dy, player) {
                        let pattern = Self::analyze_line(state, i as i32, j as i32, dx, dy, player);

                        let matches = if consecutive_count == 4 {
                            (pattern.consecutive_count == 4 && pattern.free_ends == free_ends) ||
                            (pattern.total_pieces == 4 && pattern.has_gaps && pattern.free_ends == free_ends)
                        } else {
                            pattern.consecutive_count == consecutive_count && pattern.free_ends == free_ends
                        };

                        if matches {
                            count += 1;
                        }

                        // Mark evaluated
                        let mut mark_x = i as i32;
                        let mut mark_y = j as i32;
                        for _ in 0..pattern.consecutive_count {
                            if mark_x >= 0 && mark_x < BOARD_SIZE as i32 && mark_y >= 0 && mark_y < BOARD_SIZE as i32 {
                                evaluated[mark_x as usize][mark_y as usize][d] = true;
                            }
                            mark_x += dx;
                            mark_y += dy;
                        }
                    }
                }
            }
        }

        count
    }

    pub fn is_line_start(state: &GameState, x: i32, y: i32, dx: i32, dy: i32, player: i32) -> bool {
        let prev_x = x - dx;
        let prev_y = y - dy;
        !state.is_valid(prev_x, prev_y) || state.get_piece(prev_x, prev_y) != player
    }

    pub fn analyze_line(state: &GameState, x: i32, y: i32, dx: i32, dy: i32, player: i32) -> PatternInfo {
        let max_scan = 6;
        let mut sequence = vec![0; max_scan];
        let mut actual_positions = 0;

        // Fill sequence array
        for i in 0..max_scan {
            let check_x = x + (i as i32) * dx;
            let check_y = y + (i as i32) * dy;

            if !state.is_valid(check_x, check_y) {
                break;
            }

            sequence[i] = state.get_piece(check_x, check_y);
            actual_positions = i + 1;
        }

        // Count consecutive from start
        let mut consecutive_from_start = 0;
        while consecutive_from_start < actual_positions && sequence[consecutive_from_start] == player {
            consecutive_from_start += 1;
        }

        let mut info = PatternInfo {
            consecutive_count: consecutive_from_start as i32,
            total_pieces: 0,
            free_ends: 0,
            has_gaps: false,
            total_span: 0,
            gap_count: 0,
        };

        // Quick win check
        if info.consecutive_count >= 5 {
            info.total_pieces = info.consecutive_count;
            info.total_span = info.consecutive_count;
            info.free_ends = 2;
            return info;
        }

        // Analyze patterns with gaps
        let mut total_pieces = 0;
        let mut gap_count = 0;
        let mut last_piece_pos = -1;

        for i in 0..actual_positions.min(6) {
            if sequence[i] == player {
                total_pieces += 1;
                last_piece_pos = i as i32;
            } else if sequence[i] != EMPTY {
                break; // Opponent piece
            } else if total_pieces > 0 {
                gap_count += 1;
            }
        }

        let total_span = last_piece_pos + 1;
        let has_gaps = gap_count > 0 && total_pieces > info.consecutive_count;

        // Calculate free ends
        let mut free_ends = 0;
        let back_x = x - dx;
        let back_y = y - dy;
        if state.is_valid(back_x, back_y) && state.is_empty(back_x, back_y) {
            free_ends += 1;
        }

        let front_x = x + total_span * dx;
        let front_y = y + total_span * dy;
        if state.is_valid(front_x, front_y) && state.is_empty(front_x, front_y) {
            free_ends += 1;
        }

        info.total_pieces = total_pieces;
        info.total_span = total_span;
        info.has_gaps = has_gaps;
        info.gap_count = gap_count;
        info.free_ends = free_ends;

        info
    }

    pub fn pattern_to_score(pattern: &PatternInfo) -> i32 {
        // Victory patterns
        if pattern.consecutive_count >= 5 {
            return WIN;
        }

        if pattern.total_pieces >= 5 && pattern.has_gaps && pattern.free_ends >= 1 {
            return WIN;
        }

        // 4-piece patterns
        if pattern.total_pieces == 4 {
            if pattern.consecutive_count == 4 {
                if pattern.free_ends == 2 {
                    return FOUR_OPEN;
                }
                if pattern.free_ends == 1 {
                    return FOUR_HALF;
                }
            } else if pattern.has_gaps {
                if pattern.free_ends == 2 {
                    return FOUR_OPEN;
                }
                if pattern.free_ends == 1 {
                    return FOUR_HALF;
                }
            }
        }

        // 3-piece patterns
        if pattern.total_pieces == 3 {
            if pattern.consecutive_count == 3 {
                if pattern.free_ends == 2 {
                    return THREE_OPEN;
                }
                if pattern.free_ends == 1 {
                    return THREE_HALF;
                }
            } else if pattern.has_gaps {
                if pattern.free_ends == 2 {
                    return THREE_OPEN;
                }
                if pattern.free_ends == 1 {
                    return THREE_HALF;
                }
            }
        }

        // 2-piece patterns
        if pattern.total_pieces == 2 && pattern.free_ends == 2 {
            return TWO_OPEN;
        }

        0
    }

    pub fn analyze_position(state: &GameState, player: i32) -> i32 {
        let mut total_score = 0;
        let mut evaluated = vec![vec![vec![false; 4]; BOARD_SIZE]; BOARD_SIZE];

        // Pattern evaluation
        for i in 0..BOARD_SIZE {
            for j in 0..BOARD_SIZE {
                if state.board[i][j] == player {
                    for d in 0..4 {
                        if evaluated[i][j][d] {
                            continue;
                        }

                        let (dx, dy) = MAIN_DIRECTIONS[d];

                        if Self::is_line_start(state, i as i32, j as i32, dx, dy, player) {
                            let pattern = Self::analyze_line(state, i as i32, j as i32, dx, dy, player);
                            total_score += Self::pattern_to_score(&pattern);

                            // Mark as evaluated
                            let mut mark_x = i as i32;
                            let mut mark_y = j as i32;
                            for _ in 0..pattern.consecutive_count {
                                if mark_x >= 0 && mark_x < BOARD_SIZE as i32 && mark_y >= 0 && mark_y < BOARD_SIZE as i32 {
                                    evaluated[mark_x as usize][mark_y as usize][d] = true;
                                }
                                mark_x += dx;
                                mark_y += dy;
                            }
                        }
                    }
                }
            }
        }

        // Capture evaluation (simplified for now)
        let my_captures = state.captures[player as usize - 1];
        let opponent = state.get_opponent(player);
        let opp_captures = state.captures[opponent as usize - 1];

        if my_captures >= 9 {
            total_score += 300000;
        } else if my_captures >= 8 {
            total_score += 200000;
        } else if my_captures >= 6 {
            total_score += 15000;
        } else if my_captures >= 4 {
            total_score += 6000;
        } else {
            total_score += my_captures * 500;
        }

        if opp_captures >= 9 {
            total_score -= 400000;
        } else if opp_captures >= 8 {
            total_score -= 300000;
        } else if opp_captures >= 6 {
            total_score -= 20000;
        } else if opp_captures >= 4 {
            total_score -= 8000;
        } else {
            total_score -= opp_captures * 800;
        }

        total_score
    }
}
