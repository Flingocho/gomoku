// ============================================
// Evaluator — Exact match of evaluator_patterns.cpp,
//   evaluator_position.cpp, evaluator_threats.cpp
// ============================================

use crate::game_types::*;
use crate::rule_engine::RuleEngine;

// ============================================
// Score constants — exact match of evaluator.hpp
// ============================================
pub const WIN: i32 = 600000;
pub const FOUR_OPEN: i32 = 50000;
pub const FOUR_HALF: i32 = 25000;
pub const THREE_OPEN: i32 = 10000;
pub const THREE_HALF: i32 = 1500;
pub const TWO_OPEN: i32 = 100;

pub const CAPTURE_OPPORTUNITY: i32 = 5000;
pub const CAPTURE_THREAT: i32 = 6000;
pub const CAPTURE_WIN: i32 = 500000;
pub const CAPTURE_PREVENT_LOSS: i32 = 400000;

// ============================================
// Types
// ============================================
struct PatternInfo {
    consecutive_count: i32,
    total_pieces: i32,
    free_ends: i32,
    has_gaps: bool,
    _total_span: i32,
    _gap_count: i32,
    max_reachable: i32,
}

pub struct PatternCounts {
    pub four_open: i32,
    pub four_half: i32,
    pub three_open: i32,
    pub three_half: i32,
    pub two_open: i32,
}

#[allow(dead_code)]
struct CaptureOpportunity {
    position: Move,
    captured: Vec<Move>,
}

pub struct Evaluator;

impl Evaluator {
    // ============================================
    // MAIN EVALUATE — evaluator_position.cpp
    // ============================================

    /// Evaluate with mate distance scoring
    pub fn evaluate(state: &GameState, max_depth: i32, current_depth: i32) -> i32 {
        let mate_distance = max_depth - current_depth;

        if RuleEngine::check_win(state, PLAYER2) {
            return WIN - mate_distance;
        }
        if RuleEngine::check_win(state, PLAYER1) {
            return -WIN + mate_distance;
        }

        let ai_score = Self::evaluate_for_player(state, PLAYER2);
        let human_score = Self::evaluate_for_player(state, PLAYER1);

        ai_score - human_score
    }

    /// Evaluate without mate distance (for static evaluation/FFI)
    pub fn evaluate_simple(state: &GameState) -> i32 {
        if RuleEngine::check_win(state, PLAYER2) {
            return WIN;
        }
        if RuleEngine::check_win(state, PLAYER1) {
            return -WIN;
        }

        let ai_score = Self::evaluate_for_player(state, PLAYER2);
        let human_score = Self::evaluate_for_player(state, PLAYER1);
        ai_score - human_score
    }

    pub fn evaluate_for_player(state: &GameState, player: i32) -> i32 {
        let mut score = 0;

        // 1. Count patterns via single-pass
        let counts = Self::count_all_patterns(state, player);

        // 2. Evaluate threats and combinations using counts
        score += Self::evaluate_threats_and_combinations(state, player, &counts);

        // 3. Unified evaluation: patterns + captures
        score += Self::analyze_position(state, player);

        score
    }

    // ============================================
    // PATTERN ANALYSIS — evaluator_patterns.cpp
    // ============================================

    fn is_line_start(state: &GameState, x: i32, y: i32, dx: i32, dy: i32, player: i32) -> bool {
        let prev_x = x - dx;
        let prev_y = y - dy;
        if !state.is_valid(prev_x, prev_y) {
            return true;
        }
        state.get_piece(prev_x, prev_y) != player
    }

    fn analyze_line(
        state: &GameState, start_x: i32, start_y: i32, dx: i32, dy: i32, player: i32,
    ) -> PatternInfo {
        let opponent = state.get_opponent(player);
        let mut consecutive_from_start = 0;
        let mut total_pieces = 0;
        let mut gap_count = 0;
        let mut total_span = 0;
        let mut in_gap = false;
        let mut pattern_broken = false;

        // Scan up to 6 positions forward
        for i in 0..6 {
            let cx = start_x + i * dx;
            let cy = start_y + i * dy;

            if !state.is_valid(cx, cy) || state.get_piece(cx, cy) == opponent {
                total_span = i;
                pattern_broken = true;
                break;
            }

            if state.get_piece(cx, cy) == player {
                if !in_gap && total_pieces == consecutive_from_start {
                    consecutive_from_start += 1;
                }
                total_pieces += 1;
                in_gap = false;
            } else {
                // Empty cell
                in_gap = true;
                gap_count += 1;
            }
            total_span = i + 1;
        }
        if !pattern_broken {
            total_span = 6;
        }

        // Count free ends
        let mut free_ends = 0;

        // Check backward end
        let before_x = start_x - dx;
        let before_y = start_y - dy;
        if state.is_valid(before_x, before_y) && state.get_piece(before_x, before_y) == EMPTY {
            free_ends += 1;
        }

        // Check forward end
        let end_x = start_x + total_span * dx;
        let end_y = start_y + total_span * dy;
        if state.is_valid(end_x, end_y) && state.get_piece(end_x, end_y) == EMPTY {
            free_ends += 1;
        }

        // Compute maxReachable — count non-opponent cells in both directions
        let mut max_reachable = total_pieces;

        // Extend backward
        let mut bx = start_x - dx;
        let mut by = start_y - dy;
        while state.is_valid(bx, by) && state.get_piece(bx, by) != opponent {
            max_reachable += 1;
            bx -= dx;
            by -= dy;
        }

        // Extend forward from the end of scanned region
        let mut fx = start_x + total_span * dx;
        let mut fy = start_y + total_span * dy;
        while state.is_valid(fx, fy) && state.get_piece(fx, fy) != opponent {
            max_reachable += 1;
            fx += dx;
            fy += dy;
        }

        PatternInfo {
            consecutive_count: consecutive_from_start,
            total_pieces,
            free_ends,
            has_gaps: gap_count > 0,
            _total_span: total_span,
            _gap_count: gap_count,
            max_reachable,
        }
    }

    fn pattern_to_score(pattern: &PatternInfo) -> i32 {
        let c = pattern.consecutive_count;
        let tp = pattern.total_pieces;
        let fe = pattern.free_ends;
        let has_gaps = pattern.has_gaps;

        // Dead shape check: maxReachable < 5 and not already 5 in a row
        if pattern.max_reachable < 5 && c < 5 {
            return 0;
        }

        // Five in a row
        if c >= 5 {
            return WIN;
        }

        // Four patterns
        if tp >= 4 {
            if c == 4 || (tp == 4 && has_gaps) {
                if fe == 2 {
                    return FOUR_OPEN;
                } else if fe == 1 {
                    return FOUR_HALF;
                }
            }
        }

        // Three patterns
        if tp == 3 {
            if c == 3 || has_gaps {
                if fe == 2 {
                    return THREE_OPEN;
                } else if fe == 1 {
                    return THREE_HALF;
                }
            }
        }

        // Two patterns
        if tp == 2 && fe == 2 {
            return TWO_OPEN;
        }

        0
    }

    // ============================================
    // PATTERN COUNTING — evaluator_threats.cpp
    // ============================================

    pub fn count_all_patterns(state: &GameState, player: i32) -> PatternCounts {
        let mut counts = PatternCounts {
            four_open: 0,
            four_half: 0,
            three_open: 0,
            three_half: 0,
            two_open: 0,
        };

        for i in 0..BOARD_SIZE as i32 {
            for j in 0..BOARD_SIZE as i32 {
                if state.get_piece(i, j) != player {
                    continue;
                }

                for &(dx, dy) in MAIN_DIRECTIONS.iter() {
                    if !Self::is_line_start(state, i, j, dx, dy, player) {
                        continue;
                    }

                    let pattern = Self::analyze_line(state, i, j, dx, dy, player);
                    let c = pattern.consecutive_count;
                    let tp = pattern.total_pieces;
                    let fe = pattern.free_ends;
                    let has_gaps = pattern.has_gaps;

                    // Skip dead shapes
                    if pattern.max_reachable < 5 && c < 5 {
                        continue;
                    }

                    // Four patterns
                    if tp >= 4 {
                        if c == 4 || (tp == 4 && has_gaps) {
                            if fe == 2 {
                                counts.four_open += 1;
                            } else if fe == 1 {
                                counts.four_half += 1;
                            }
                        }
                    }

                    // Three patterns
                    if tp == 3 && (c == 3 || has_gaps) {
                        if fe == 2 {
                            counts.three_open += 1;
                        } else if fe == 1 {
                            counts.three_half += 1;
                        }
                    }

                    // Two patterns
                    if tp == 2 && fe == 2 {
                        counts.two_open += 1;
                    }
                }
            }
        }

        counts
    }

    /// countPatternType — used by evaluateImmediateThreats and hasWinningThreats
    fn count_pattern_type(
        state: &GameState, player: i32, consecutive_count: i32, free_ends: i32,
    ) -> i32 {
        let mut count = 0;

        for i in 0..BOARD_SIZE as i32 {
            for j in 0..BOARD_SIZE as i32 {
                if state.get_piece(i, j) != player {
                    continue;
                }

                for &(dx, dy) in MAIN_DIRECTIONS.iter() {
                    if !Self::is_line_start(state, i, j, dx, dy, player) {
                        continue;
                    }

                    let pattern = Self::analyze_line(state, i, j, dx, dy, player);
                    if pattern.consecutive_count == consecutive_count
                        && pattern.free_ends == free_ends
                    {
                        count += 1;
                    }
                }
            }
        }

        count
    }

    // ============================================
    // THREAT/COMBINATION EVALUATION — evaluator_threats.cpp
    // ============================================

    fn evaluate_threats_and_combinations(
        state: &GameState, player: i32, counts: &PatternCounts,
    ) -> i32 {
        let mut score = 0;

        if counts.four_open > 0 {
            score += 90000;
        }
        if counts.four_half > 0 {
            score += 40000;
        }
        if counts.three_open >= 2 {
            score += 50000;
        }

        // Combinations
        if counts.four_half >= 1 && counts.three_open >= 1 {
            score += 80000;
        }
        if counts.four_half >= 2 {
            score += 70000;
        }

        let my_captures = state.captures[(player - 1) as usize];
        if my_captures >= 8 && counts.three_open >= 1 {
            score += 60000;
        }

        score
    }

    pub fn evaluate_immediate_threats(state: &GameState, player: i32) -> i32 {
        let mut score = 0;

        let four_open = Self::count_pattern_type(state, player, 4, 2);
        let four_half = Self::count_pattern_type(state, player, 4, 1);
        let three_open = Self::count_pattern_type(state, player, 3, 2);

        if four_open > 0 {
            score += 90000;
        }
        if four_half > 0 {
            score += 40000;
        }
        if three_open >= 2 {
            score += 50000;
        }

        score
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

    #[allow(dead_code)]
    fn evaluate_combinations(state: &GameState, player: i32) -> i32 {
        let counts = Self::count_all_patterns(state, player);
        let mut score = 0;

        // Fork detection
        if counts.four_half >= 1 && counts.three_open >= 1 {
            score += 80000;
        }
        if counts.four_half >= 2 {
            score += 70000;
        }
        if counts.three_open >= 2 {
            score += 50000;
        }

        let my_captures = state.captures[(player - 1) as usize];
        if my_captures >= 8 && counts.three_open >= 1 {
            score += 60000;
        }

        score
    }

    // ============================================
    // POSITION ANALYSIS — evaluator_position.cpp analyzePosition
    // ============================================

    fn analyze_position(state: &GameState, player: i32) -> i32 {
        let mut total_score = 0;
        let opponent = state.get_opponent(player);

        // Track evaluated positions to avoid double counting
        let mut evaluated = [[[false; 4]; BOARD_SIZE]; BOARD_SIZE];

        // PART 1: PATTERN EVALUATION
        for i in 0..BOARD_SIZE as i32 {
            for j in 0..BOARD_SIZE as i32 {
                if state.get_piece(i, j) != player {
                    continue;
                }

                for (dir_idx, &(dx, dy)) in MAIN_DIRECTIONS.iter().enumerate() {
                    if evaluated[i as usize][j as usize][dir_idx] {
                        continue;
                    }

                    if !Self::is_line_start(state, i, j, dx, dy, player) {
                        continue;
                    }

                    let pattern = Self::analyze_line(state, i, j, dx, dy, player);
                    let score = Self::pattern_to_score(&pattern);
                    total_score += score;

                    // Mark all positions in this line as evaluated in this direction
                    let mut mx = i;
                    let mut my = j;
                    for _ in 0..pattern.consecutive_count {
                        if state.is_valid(mx, my) {
                            evaluated[mx as usize][my as usize][dir_idx] = true;
                        }
                        mx += dx;
                        my += dy;
                    }
                }
            }
        }

        // PART 2: CAPTURE EVALUATION
        let mut capture_opportunities = 0;
        let mut capture_threats = 0;

        // Offensive: our capture opportunities
        let my_opps = Self::find_all_capture_opportunities(state, player);
        for opp in &my_opps {
            capture_opportunities += Self::evaluate_capture_context(
                state,
                player,
                &opp.captured,
                state.captures[(player - 1) as usize] + (opp.captured.len() / 2) as i32,
            );
        }

        // Defensive: opponent's capture opportunities (threats to us)
        let opp_threats = Self::find_all_capture_opportunities(state, opponent);
        for threat in &opp_threats {
            capture_threats += Self::evaluate_capture_context(
                state,
                opponent,
                &threat.captured,
                state.captures[(opponent - 1) as usize] + (threat.captured.len() / 2) as i32,
            );
        }

        // PART 3: EXISTING CAPTURES SCORING
        let my_captures = state.captures[(player - 1) as usize];
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

        let opp_captures = state.captures[(opponent - 1) as usize];
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

        total_score += capture_opportunities;
        total_score -= capture_threats;

        total_score
    }

    // ============================================
    // CAPTURE OPPORTUNITIES — evaluator_threats.cpp
    // ============================================

    fn find_all_capture_opportunities(state: &GameState, player: i32) -> Vec<CaptureOpportunity> {
        let mut opportunities = Vec::new();
        let opponent = state.get_opponent(player);

        for i in 0..BOARD_SIZE as i32 {
            for j in 0..BOARD_SIZE as i32 {
                if state.get_piece(i, j) != opponent {
                    continue;
                }

                for &(dx, dy) in ALL_DIRECTIONS.iter() {
                    let nx = i + dx;
                    let ny = j + dy;

                    // Need pair of opponent pieces
                    if !state.is_valid(nx, ny) || state.get_piece(nx, ny) != opponent {
                        continue;
                    }

                    // Check front flank: player at (i - dx, j - dy) and empty at (nx + dx, ny + dy)
                    let front_x = i - dx;
                    let front_y = j - dy;
                    let back_x = nx + dx;
                    let back_y = ny + dy;

                    if state.is_valid(front_x, front_y)
                        && state.get_piece(front_x, front_y) == player
                        && state.is_valid(back_x, back_y)
                        && state.is_empty(back_x, back_y)
                    {
                        let mut captured = Vec::new();
                        captured.push(Move::new(i, j));
                        captured.push(Move::new(nx, ny));
                        opportunities.push(CaptureOpportunity {
                            position: Move::new(back_x, back_y),
                            captured,
                        });
                    }

                    // Check back flank: empty at (i - dx, j - dy) and player at (nx + dx, ny + dy)
                    if state.is_valid(front_x, front_y)
                        && state.is_empty(front_x, front_y)
                        && state.is_valid(back_x, back_y)
                        && state.get_piece(back_x, back_y) == player
                    {
                        let mut captured = Vec::new();
                        captured.push(Move::new(i, j));
                        captured.push(Move::new(nx, ny));
                        opportunities.push(CaptureOpportunity {
                            position: Move::new(front_x, front_y),
                            captured,
                        });
                    }
                }
            }
        }

        opportunities
    }

    fn evaluate_capture_context(
        state: &GameState, player: i32, captured_pieces: &[Move], new_capture_count: i32,
    ) -> i32 {
        let mut value;
        let opponent = state.get_opponent(player);

        // 1. Proximity to capture victory
        if new_capture_count >= 10 {
            return CAPTURE_WIN;
        } else if new_capture_count == 9 {
            value = 100000;
        } else if new_capture_count >= 8 {
            value = 50000;
        } else if new_capture_count >= 6 {
            value = 15000;
        } else {
            value = new_capture_count * 2000;
        }

        // 2. Pattern disruption — check opponent patterns through captured positions
        for captured in captured_pieces {
            for &(dx, dy) in MAIN_DIRECTIONS.iter() {
                let pattern_size =
                    Self::count_pattern_through_position(state, captured, dx, dy, opponent);
                if pattern_size >= 4 {
                    value += 30000;
                } else if pattern_size == 3 {
                    value += 12000;
                } else if pattern_size == 2 {
                    value += 3000;
                }
            }
        }

        // 3. Offensive value — adjacency to own pieces
        for captured in captured_pieces {
            for &(dx, dy) in ALL_DIRECTIONS.iter() {
                let adj_x = captured.x + dx;
                let adj_y = captured.y + dy;
                if state.is_valid(adj_x, adj_y) && state.get_piece(adj_x, adj_y) == player {
                    value += 1500;
                }
            }
        }

        // 4. Danger: opponent close to capture win
        let opp_captures = state.captures[(opponent - 1) as usize];
        if opp_captures >= 8 {
            value += 25000;
        }

        value
    }

    fn count_pattern_through_position(
        state: &GameState, pos: &Move, dx: i32, dy: i32, player: i32,
    ) -> i32 {
        let mut count = 0;

        // Count backward
        let mut x = pos.x - dx;
        let mut y = pos.y - dy;
        while state.is_valid(x, y) && state.get_piece(x, y) == player {
            count += 1;
            x -= dx;
            y -= dy;
        }

        // Count forward
        x = pos.x + dx;
        y = pos.y + dy;
        while state.is_valid(x, y) && state.get_piece(x, y) == player {
            count += 1;
            x += dx;
            y += dy;
        }

        count
    }
}
