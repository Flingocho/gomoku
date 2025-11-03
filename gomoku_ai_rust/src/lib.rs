
use std::collections::HashMap;

const BOARD_SIZE: usize = 19;
const PLAYER1: i32 = 1; // Human
const PLAYER2: i32 = 2; // AI
const EMPTY: i32 = 0;

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub struct Move {
    pub x: i32,
    pub y: i32,
}

impl Move {
    pub fn new(x: i32, y: i32) -> Self {
        Move { x, y }
    }

    pub fn invalid() -> Self {
        Move { x: -1, y: -1 }
    }

    pub fn is_valid(&self) -> bool {
        self.x >= 0 && self.y >= 0 && self.x < BOARD_SIZE as i32 && self.y < BOARD_SIZE as i32
    }
}

// Direction constants matching C++
const MAIN_DIRECTIONS: [(i32, i32); 4] = [
    (0, 1),  // Horizontal →
    (1, 0),  // Vertical ↓
    (1, 1),  // Diagonal ↘
    (1, -1), // Diagonal ↗
];

const CAPTURE_DIRECTIONS: [(i32, i32); 8] = [
    (-1, -1), (-1, 0), (-1, 1),  // Top-left, Top, Top-right
    (0, -1),           (0, 1),    // Left, Right
    (1, -1),  (1, 0),  (1, 1),    // Bottom-left, Bottom, Bottom-right
];

// Evaluator score constants matching C++
const WIN: i32 = 600000;
const FOUR_OPEN: i32 = 50000;
const FOUR_HALF: i32 = 25000;
const THREE_OPEN: i32 = 10000;
const THREE_HALF: i32 = 1500;
const TWO_OPEN: i32 = 100;

// These are kept for potential future use
#[allow(dead_code)]
const CAPTURE_OPPORTUNITY: i32 = 5000;
#[allow(dead_code)]
const CAPTURE_THREAT: i32 = 6000;


#[derive(Clone)]
pub struct GameState {
    pub board: [[i32; BOARD_SIZE]; BOARD_SIZE],
    pub current_player: i32,
    pub turn_count: i32,
    pub captures: [i32; 2], // [PLAYER1_captures, PLAYER2_captures]
    pub last_human_move: Move,
    zobrist_hash: u64,
}

impl GameState {
    pub fn new() -> Self {
        GameState {
            board: [[0; BOARD_SIZE]; BOARD_SIZE],
            current_player: 1,
            turn_count: 0,
            captures: [0, 0],
            last_human_move: Move::invalid(),
            zobrist_hash: 0,
        }
    }

    fn is_valid(&self, x: i32, y: i32) -> bool {
        x >= 0 && y >= 0 && x < BOARD_SIZE as i32 && y < BOARD_SIZE as i32
    }

    fn is_empty(&self, x: i32, y: i32) -> bool {
        if !self.is_valid(x, y) {
            return false;
        }
        self.board[x as usize][y as usize] == EMPTY
    }

    fn get_piece(&self, x: i32, y: i32) -> i32 {
        if !self.is_valid(x, y) {
            return -1;
        }
        self.board[x as usize][y as usize]
    }

    fn get_opponent(&self, player: i32) -> i32 {
        if player == PLAYER1 { PLAYER2 } else { PLAYER1 }
    }

    pub fn make_move(&mut self, mv: Move) -> bool {
        if !mv.is_valid() || self.board[mv.x as usize][mv.y as usize] != 0 {
            return false;
        }

        self.board[mv.x as usize][mv.y as usize] = self.current_player;
        
        // Track last human move
        if self.current_player == PLAYER1 {
            self.last_human_move = mv;
        }
        
        self.current_player = 3 - self.current_player; // Switch between 1 and 2
        self.turn_count += 1;
        true
    }

    pub fn unmake_move(&mut self, mv: Move) {
        if mv.is_valid() {
            self.board[mv.x as usize][mv.y as usize] = 0;
            self.current_player = 3 - self.current_player;
            self.turn_count -= 1;
        }
    }

    pub fn is_game_over(&self) -> bool {
        // Check for 5 in a row
        for i in 0..BOARD_SIZE {
            for j in 0..BOARD_SIZE {
                if self.board[i][j] != 0 {
                    if self.check_five_in_row(i, j) {
                        return true;
                    }
                }
            }
        }
        
        // Check for capture win (10+ captures)
        if self.captures[0] >= 10 || self.captures[1] >= 10 {
            return true;
        }
        
        false
    }

    fn check_five_in_row(&self, x: usize, y: usize) -> bool {
        let player = self.board[x][y];

        for (dx, dy) in MAIN_DIRECTIONS.iter() {
            let mut count = 1;
            // Check positive direction
            let mut i = x as i32 + dx;
            let mut j = y as i32 + dy;
            while i >= 0 && i < BOARD_SIZE as i32 && j >= 0 && j < BOARD_SIZE as i32 
                && self.board[i as usize][j as usize] == player {
                count += 1;
                i += dx;
                j += dy;
            }
            // Check negative direction
            i = x as i32 - dx;
            j = y as i32 - dy;
            while i >= 0 && i < BOARD_SIZE as i32 && j >= 0 && j < BOARD_SIZE as i32 
                && self.board[i as usize][j as usize] == player {
                count += 1;
                i -= dx;
                j -= dy;
            }
            if count >= 5 {
                return true;
            }
        }
        false
    }

    pub fn get_legal_moves(&self) -> Vec<Move> {
        let mut moves = Vec::new();
        for i in 0..BOARD_SIZE {
            for j in 0..BOARD_SIZE {
                if self.board[i][j] == 0 {
                    moves.push(Move::new(i as i32, j as i32));
                }
            }
        }
        moves
    }

    pub fn get_zobrist_hash(&self) -> u64 {
        self.zobrist_hash
    }
}


// Pattern information for evaluation
#[derive(Clone, Copy, Debug)]
struct PatternInfo {
    consecutive_count: i32,
    total_pieces: i32,
    free_ends: i32,
    has_gaps: bool,
    total_span: i32,
    gap_count: i32,
}

// Capture opportunity for evaluation (unused for now, but keeping for future enhancements)
#[derive(Clone)]
#[allow(dead_code)]
struct CaptureOpportunity {
    position: Move,
    captured: Vec<Move>,
}

// Cache entry types matching C++
#[derive(Clone, Copy, PartialEq)]
enum CacheEntryType {
    Exact,
    LowerBound,
    UpperBound,
}

#[derive(Clone)]
struct CacheEntry {
    zobrist_key: u64,
    score: i32,
    depth: i32,
    best_move: Move,
    generation: u32,
    entry_type: CacheEntryType,
}

impl CacheEntry {
    fn new(zobrist_key: u64, score: i32, depth: i32, best_move: Move, entry_type: CacheEntryType, generation: u32) -> Self {
        CacheEntry {
            zobrist_key,
            score,
            depth,
            best_move,
            generation,
            entry_type,
        }
    }

    fn get_importance_value(&self) -> i32 {
        let mut value = self.depth * 100;
        if self.entry_type == CacheEntryType::Exact {
            value += 50;
        } else if self.entry_type == CacheEntryType::LowerBound {
            value += 25;
        }
        value
    }
}

pub struct AI {
    nodes_evaluated: u64,
    cache_hits: u64,
    transposition_table: HashMap<usize, CacheEntry>,
    table_size_mask: usize,
    current_generation: u32,
    previous_best_move: Move,
}

impl AI {
    pub fn new() -> Self {
        let table_size_mb = 64;
        let bytes_per_entry = std::mem::size_of::<CacheEntry>();
        let total_bytes = table_size_mb * 1024 * 1024;
        let num_entries = total_bytes / bytes_per_entry;
        
        // Round to nearest power of 2
        let mut power_of_2 = 1;
        while power_of_2 < num_entries {
            power_of_2 <<= 1;
        }
        if power_of_2 > num_entries {
            power_of_2 >>= 1;
        }
        
        AI {
            nodes_evaluated: 0,
            cache_hits: 0,
            transposition_table: HashMap::new(),
            table_size_mask: power_of_2 - 1,
            current_generation: 1,
            previous_best_move: Move::invalid(),
        }
    }

    pub fn get_best_move(&mut self, state: &GameState, max_depth: i32) -> Move {
        self.find_best_move_iterative(state, max_depth)
    }

    fn find_best_move_iterative(&mut self, state: &GameState, max_depth: i32) -> Move {
        self.nodes_evaluated = 0;
        self.cache_hits = 0;
        self.current_generation += 1;

        // Pre-check for immediate win
        let all_candidates = self.generate_candidates_adaptive_radius(state);
        for mv in &all_candidates {
            let mut test_state = state.clone();
            if test_state.make_move(*mv) {
                if test_state.is_game_over() {
                    println!("Immediate win detected at ({}, {})", mv.x, mv.y);
                    return *mv;
                }
            }
        }

        let mut best_move = Move::invalid();

        // Iterative deepening
        for depth in 1..=max_depth {
            if best_move.is_valid() {
                self.previous_best_move = best_move;
            }

            let mut move_at_depth = Move::invalid();
            let score = self.minimax(
                &mut state.clone(),
                depth,
                i32::MIN,
                i32::MAX,
                state.current_player == PLAYER2,
                depth,
                Some(&mut move_at_depth)
            );

            best_move = move_at_depth;

            println!("Depth {}: score {} - {} nodes", depth, score, self.nodes_evaluated);

            // Early exit for mate
            if score.abs() > 300000 {
                println!("Mate detected at depth {}", depth);
                break;
            }
        }

        if best_move.is_valid() {
            best_move
        } else if !all_candidates.is_empty() {
            all_candidates[0]
        } else {
            Move::invalid()
        }
    }


    fn minimax(&mut self, state: &mut GameState, depth: i32, mut alpha: i32, mut beta: i32, maximizing: bool, original_max_depth: i32, best_move: Option<&mut Move>) -> i32 {
        self.nodes_evaluated += 1;

        // Transposition table lookup
        let zobrist_key = state.get_zobrist_hash();
        if let Some(entry) = self.lookup_transposition(zobrist_key) {
            self.cache_hits += 1;
            if entry.depth >= depth {
                match entry.entry_type {
                    CacheEntryType::Exact => {
                        if depth == original_max_depth {
                            if let Some(bm) = best_move {
                                *bm = entry.best_move;
                            }
                        }
                        return entry.score;
                    }
                    CacheEntryType::LowerBound => {
                        if entry.score >= beta {
                            return beta;
                        }
                    }
                    CacheEntryType::UpperBound => {
                        if entry.score <= alpha {
                            return alpha;
                        }
                    }
                }
            }
            if entry.best_move.is_valid() {
                self.previous_best_move = entry.best_move;
            }
        }

        // Base cases
        if depth == 0 || state.is_game_over() {
            let score = self.evaluate(state, original_max_depth, original_max_depth - depth);
            self.store_transposition(zobrist_key, score, depth, Move::invalid(), CacheEntryType::Exact);
            return score;
        }

        // Generate and order moves
        let moves = self.generate_ordered_moves(state);
        if moves.is_empty() {
            let score = self.evaluate(state, original_max_depth, original_max_depth - depth);
            self.store_transposition(zobrist_key, score, depth, Move::invalid(), CacheEntryType::Exact);
            return score;
        }

        let mut current_best_move = Move::invalid();
        let original_alpha = alpha;

        if maximizing {
            let mut max_eval = i32::MIN;

            for mv in &moves {
                if !state.make_move(*mv) {
                    continue;
                }

                let eval = self.minimax(state, depth - 1, alpha, beta, false, original_max_depth, None);
                state.unmake_move(*mv);

                if eval > max_eval {
                    max_eval = eval;
                    current_best_move = *mv;
                }

                alpha = alpha.max(eval);
                if beta <= alpha {
                    break; // Beta cutoff
                }
            }

            let entry_type = if max_eval <= original_alpha {
                CacheEntryType::UpperBound
            } else if max_eval >= beta {
                CacheEntryType::LowerBound
            } else {
                CacheEntryType::Exact
            };

            if !current_best_move.is_valid() && !moves.is_empty() {
                current_best_move = moves[0];
            }

            self.store_transposition(zobrist_key, max_eval, depth, current_best_move, entry_type);

            if depth == original_max_depth {
                if let Some(bm) = best_move {
                    *bm = current_best_move;
                }
            }

            max_eval
        } else {
            let mut min_eval = i32::MAX;

            for mv in &moves {
                if !state.make_move(*mv) {
                    continue;
                }

                let eval = self.minimax(state, depth - 1, alpha, beta, true, original_max_depth, None);
                state.unmake_move(*mv);

                if eval < min_eval {
                    min_eval = eval;
                    current_best_move = *mv;
                }

                beta = beta.min(eval);
                if beta <= alpha {
                    break; // Alpha cutoff
                }
            }

            let entry_type = if min_eval <= original_alpha {
                CacheEntryType::UpperBound
            } else if min_eval >= beta {
                CacheEntryType::LowerBound
            } else {
                CacheEntryType::Exact
            };

            if !current_best_move.is_valid() && !moves.is_empty() {
                current_best_move = moves[0];
            }

            self.store_transposition(zobrist_key, min_eval, depth, current_best_move, entry_type);

            if depth == original_max_depth {
                if let Some(bm) = best_move {
                    *bm = current_best_move;
                }
            }

            min_eval
        }
    }


    fn lookup_transposition(&self, zobrist_key: u64) -> Option<CacheEntry> {
        let index = (zobrist_key as usize) & self.table_size_mask;
        if let Some(entry) = self.transposition_table.get(&index) {
            if entry.zobrist_key == zobrist_key {
                return Some(entry.clone());
            }
        }
        None
    }

    fn store_transposition(&mut self, zobrist_key: u64, score: i32, depth: i32, best_move: Move, entry_type: CacheEntryType) {
        let index = (zobrist_key as usize) & self.table_size_mask;
        
        let should_replace = if let Some(existing) = self.transposition_table.get(&index) {
            if existing.zobrist_key == 0 {
                true
            } else if existing.zobrist_key == zobrist_key {
                depth >= existing.depth
            } else {
                let new_entry = CacheEntry::new(zobrist_key, score, depth, best_move, entry_type, self.current_generation);
                let existing_importance = existing.get_importance_value() - ((self.current_generation - existing.generation) as i32 * 10);
                let new_importance = new_entry.get_importance_value();
                
                new_importance > existing_importance || 
                (new_importance == existing_importance && entry_type == CacheEntryType::Exact)
            }
        } else {
            true
        };

        if should_replace {
            self.transposition_table.insert(index, CacheEntry::new(zobrist_key, score, depth, best_move, entry_type, self.current_generation));
        }
    }

    fn evaluate(&self, state: &GameState, max_depth: i32, current_depth: i32) -> i32 {
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

        let ai_score = self.evaluate_for_player(state, PLAYER2);
        let human_score = self.evaluate_for_player(state, PLAYER1);

        ai_score - human_score
    }

    fn evaluate_for_player(&self, state: &GameState, player: i32) -> i32 {
        let mut score = 0;

        // Immediate threats
        score += self.evaluate_immediate_threats(state, player);

        // Position analysis (patterns)
        score += self.analyze_position(state, player);

        score
    }

    fn evaluate_immediate_threats(&self, state: &GameState, player: i32) -> i32 {
        let opponent = state.get_opponent(player);
        let mut threat_score = 0;

        let my_has_win_threats = self.has_winning_threats(state, player);
        let opp_has_win_threats = self.has_winning_threats(state, opponent);

        if my_has_win_threats {
            threat_score += 90000;
        }
        if opp_has_win_threats {
            threat_score -= 105000;
        }

        // Count 4-patterns
        let my_four_open = self.count_pattern_type(state, player, 4, 2);
        let opp_four_open = self.count_pattern_type(state, opponent, 4, 2);
        let my_four_half = self.count_pattern_type(state, player, 4, 1);
        let opp_four_half = self.count_pattern_type(state, opponent, 4, 1);

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

    fn has_winning_threats(&self, state: &GameState, player: i32) -> bool {
        let four_open = self.count_pattern_type(state, player, 4, 2);
        if four_open > 0 {
            return true;
        }

        let four_half = self.count_pattern_type(state, player, 4, 1);
        if four_half > 0 {
            return true;
        }

        let three_open = self.count_pattern_type(state, player, 3, 2);
        if three_open >= 2 {
            return true;
        }

        false
    }

    fn count_pattern_type(&self, state: &GameState, player: i32, consecutive_count: i32, free_ends: i32) -> i32 {
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
                    
                    if self.is_line_start(state, i as i32, j as i32, dx, dy, player) {
                        let pattern = self.analyze_line(state, i as i32, j as i32, dx, dy, player);

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


    fn is_line_start(&self, state: &GameState, x: i32, y: i32, dx: i32, dy: i32, player: i32) -> bool {
        let prev_x = x - dx;
        let prev_y = y - dy;
        !state.is_valid(prev_x, prev_y) || state.get_piece(prev_x, prev_y) != player
    }

    fn analyze_line(&self, state: &GameState, x: i32, y: i32, dx: i32, dy: i32, player: i32) -> PatternInfo {
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

    fn pattern_to_score(&self, pattern: &PatternInfo) -> i32 {
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

    fn analyze_position(&self, state: &GameState, player: i32) -> i32 {
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

                        if self.is_line_start(state, i as i32, j as i32, dx, dy, player) {
                            let pattern = self.analyze_line(state, i as i32, j as i32, dx, dy, player);
                            total_score += self.pattern_to_score(&pattern);

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


    fn generate_ordered_moves(&mut self, state: &GameState) -> Vec<Move> {
        let mut candidates = self.generate_candidates_adaptive_radius(state);
        self.order_moves_with_previous_best(&mut candidates, state);
        candidates
    }

    fn generate_candidates_adaptive_radius(&self, state: &GameState) -> Vec<Move> {
        let mut candidates = Vec::new();
        let search_radius = self.get_search_radius_for_game_phase(state.turn_count);
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
        let max_candidates = self.get_max_candidates_for_game_phase(state);
        if candidates.len() > max_candidates {
            // Order first before limiting
            let mut scored: Vec<(Move, i32)> = candidates.iter()
                .map(|&mv| (mv, self.quick_evaluate_move(state, mv)))
                .collect();
            scored.sort_by(|a, b| b.1.cmp(&a.1));
            candidates = scored.into_iter().take(max_candidates).map(|(mv, _)| mv).collect();
        }

        candidates
    }

    fn get_search_radius_for_game_phase(&self, _piece_count: i32) -> usize {
        1
    }

    fn get_max_candidates_for_game_phase(&self, state: &GameState) -> usize {
        let piece_count = state.turn_count;
        if piece_count <= 4 {
            3
        } else if piece_count <= 10 {
            4
        } else {
            5
        }
    }

    fn order_moves_with_previous_best(&self, moves: &mut Vec<Move>, state: &GameState) {
        if moves.len() <= 2 {
            return;
        }

        // Check if previous best move is in the list
        if self.previous_best_move.is_valid() {
            if let Some(pos) = moves.iter().position(|m| *m == self.previous_best_move) {
                moves.swap(0, pos);
                if moves.len() > 1 {
                    let rest = &mut moves[1..];
                    rest.sort_by(|a, b| {
                        let score_a = self.quick_evaluate_move(state, *a);
                        let score_b = self.quick_evaluate_move(state, *b);
                        score_b.cmp(&score_a)
                    });
                }
                return;
            }
        }

        // Normal ordering
        moves.sort_by(|a, b| {
            let score_a = self.quick_evaluate_move(state, *a);
            let score_b = self.quick_evaluate_move(state, *b);
            score_b.cmp(&score_a)
        });
    }

    fn quick_evaluate_move(&self, state: &GameState, mv: Move) -> i32 {
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
            let my_count = 1 + self.count_consecutive_in_direction(state, mv.x, mv.y, *dx, *dy, current_player, 4)
                + self.count_consecutive_in_direction(state, mv.x, mv.y, -dx, -dy, current_player, 4);
            max_my_line = max_my_line.max(my_count);

            let opp_count = self.count_consecutive_in_direction(state, mv.x, mv.y, *dx, *dy, opponent, 4)
                + self.count_consecutive_in_direction(state, mv.x, mv.y, -dx, -dy, opponent, 4);
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

    fn count_consecutive_in_direction(&self, state: &GameState, mut x: i32, mut y: i32, dx: i32, dy: i32, player: i32, max_count: i32) -> i32 {
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

    pub fn get_stats(&self) -> (u64, u64) {
        (self.nodes_evaluated, self.cache_hits)
    }
}


// FFI interface for C++
#[no_mangle]
pub extern "C" fn rust_ai_get_best_move(
    board: *const i32,
    current_player: i32,
    turn_count: i32,
    max_depth: i32,
) -> Move {
    let mut ai = AI::new();

    // Convert C array to Rust array
    let mut rust_board = [[0i32; BOARD_SIZE]; BOARD_SIZE];
    for i in 0..BOARD_SIZE {
        for j in 0..BOARD_SIZE {
            unsafe {
                rust_board[i][j] = *board.offset((i * BOARD_SIZE + j) as isize);
            }
        }
    }

    let state = GameState {
        board: rust_board,
        current_player,
        turn_count,
        captures: [0, 0], // TODO: Pass from C++ if needed
        last_human_move: Move::invalid(), // TODO: Pass from C++ if needed
        zobrist_hash: 0, // Will be computed if needed
    };

    ai.get_best_move(&state, max_depth)
}

#[no_mangle]
pub extern "C" fn rust_ai_evaluate_position(board: *const i32, current_player: i32, turn_count: i32) -> i32 {
    let mut rust_board = [[0i32; BOARD_SIZE]; BOARD_SIZE];
    for i in 0..BOARD_SIZE {
        for j in 0..BOARD_SIZE {
            unsafe {
                rust_board[i][j] = *board.offset((i * BOARD_SIZE + j) as isize);
            }
        }
    }

    let state = GameState {
        board: rust_board,
        current_player,
        turn_count,
        captures: [0, 0],
        last_human_move: Move::invalid(),
        zobrist_hash: 0,
    };

    let ai = AI::new();
    ai.evaluate(&state, 6, 0) // Use default depth for evaluation
}
