
#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq)]
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
        self.x >= 0 && self.y >= 0 && self.x < 19 && self.y < 19
    }
}

#[derive(Clone)]
pub struct GameState {
    pub board: [[i32; 19]; 19],
    pub current_player: i32,
    pub turn_count: i32,
}

impl GameState {
    pub fn new() -> Self {
        GameState {
            board: [[0; 19]; 19],
            current_player: 1,
            turn_count: 0,
        }
    }

    pub fn make_move(&mut self, mv: Move) -> bool {
        if !mv.is_valid() || self.board[mv.x as usize][mv.y as usize] != 0 {
            return false;
        }

        self.board[mv.x as usize][mv.y as usize] = self.current_player;
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
        for i in 0..19 {
            for j in 0..19 {
                if self.board[i][j] != 0 {
                    if self.check_five_in_row(i, j) {
                        return true;
                    }
                }
            }
        }
        false
    }

    fn check_five_in_row(&self, x: usize, y: usize) -> bool {
        let player = self.board[x][y];
        let directions = [(0, 1), (1, 0), (1, 1), (1, -1)];

        for (dx, dy) in directions.iter() {
            let mut count = 1;
            // Check positive direction
            let mut i = x as i32 + dx;
            let mut j = y as i32 + dy;
            while i >= 0 && i < 19 && j >= 0 && j < 19 && self.board[i as usize][j as usize] == player {
                count += 1;
                i += dx;
                j += dy;
            }
            // Check negative direction
            i = x as i32 - dx;
            j = y as i32 - dy;
            while i >= 0 && i < 19 && j >= 0 && j < 19 && self.board[i as usize][j as usize] == player {
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
        for i in 0..19 {
            for j in 0..19 {
                if self.board[i][j] == 0 {
                    moves.push(Move::new(i as i32, j as i32));
                }
            }
        }
        moves
    }

    // Simple hash function for transposition table
    pub fn get_hash(&self) -> u64 {
        let mut hash: u64 = 0;
        for i in 0..19 {
            for j in 0..19 {
                let piece = self.board[i][j] as u64;
                hash = hash.wrapping_mul(31).wrapping_add(piece);
                hash = hash.wrapping_add((i * 19 + j) as u64);
            }
        }
        hash.wrapping_add(self.current_player as u64)
    }
}

pub struct AI {
    nodes_evaluated: u64,
    cache_hits: u64,
}

impl AI {
    pub fn new() -> Self {
        AI {
            nodes_evaluated: 0,
            cache_hits: 0,
        }
    }

    pub fn get_best_move(&mut self, state: &GameState, max_depth: i32) -> Move {
        self.nodes_evaluated = 0;
        self.cache_hits = 0;

        // First, check if opponent has immediate winning threats
        let winning_moves = self.find_winning_moves(state, 3 - state.current_player);
        if !winning_moves.is_empty() {
            // Must block the win! Return the first blocking move
            return winning_moves[0];
        }

        // Check if we have immediate winning moves
        let our_winning_moves = self.find_winning_moves(state, state.current_player);
        if !our_winning_moves.is_empty() {
            // Take the win!
            return our_winning_moves[0];
        }

        // Check for 4-in-a-row threats (opponent could win in 1 move)
        let threat_moves = self.find_threat_moves(state, 3 - state.current_player);
        if !threat_moves.is_empty() {
            // Block the threat!
            return threat_moves[0];
        }

        // Normal search
        let mut best_move = Move::invalid();
        let mut best_score = i32::MIN;

        let moves = self.order_moves(state);

        // Limit moves to first 20 for reasonable speed
        let limited_moves = if moves.len() > 20 { &moves[0..20] } else { &moves };

        for mv in limited_moves.iter() {
            let mut new_state = state.clone();
            if new_state.make_move(*mv) {
                let score = -self.minimax(&mut new_state, max_depth - 1, i32::MIN + 1, i32::MAX, false);
                new_state.unmake_move(*mv);

                if score > best_score {
                    best_score = score;
                    best_move = *mv;
                }
            }
        }

        if best_move.is_valid() {
            best_move
        } else if !moves.is_empty() {
            moves[0] // Fallback to first move
        } else {
            Move::invalid()
        }
    }

    fn find_threat_moves(&self, state: &GameState, _player: i32) -> Vec<Move> {
        let mut threat_moves = Vec::new();

        for mv in state.get_legal_moves() {
            let mut test_state = state.clone();
            if test_state.make_move(mv) {
                // Check if this creates a 4-in-a-row threat for the player
                if self.has_open_four(&test_state, 3 - state.current_player) {
                    threat_moves.push(mv);
                }
            }
        }

        threat_moves
    }

    fn find_winning_moves(&self, state: &GameState, _player: i32) -> Vec<Move> {
        let mut winning_moves = Vec::new();

        for mv in state.get_legal_moves() {
            let mut test_state = state.clone();
            if test_state.make_move(mv) {
                if test_state.is_game_over() {
                    // The move that was just made caused the game to end
                    // Since we made the move for the player we're checking,
                    // this player must have won
                    winning_moves.push(mv);
                }
                // Don't unmake_move here as we're just checking
            }
        }

        winning_moves
    }

    fn has_open_four(&self, state: &GameState, player: i32) -> bool {
        let directions = [(0, 1), (1, 0), (1, 1), (1, -1)];

        for i in 0..19 {
            for j in 0..19 {
                if state.board[i][j] == player {
                    for (dx, dy) in directions {
                        if self.can_form_window(i, j, dx, dy) {
                            let window_score = self.evaluate_window_for_player(state, i, j, dx, dy, player);
                            // If this window has 4 pieces with 1 empty, it's a threat
                            if window_score >= 50000 { // Our scoring for 4+1 empty
                                return true;
                            }
                        }
                    }
                }
            }
        }

        false
    }

    fn minimax(&mut self, state: &mut GameState, depth: i32, mut alpha: i32, mut beta: i32, maximizing: bool) -> i32 {
        self.nodes_evaluated += 1;

        if depth == 0 || state.is_game_over() {
            return self.evaluate_position(state);
        }

        let moves = self.order_moves(state);
        // Limit moves at each level too
        let limited_moves = if moves.len() > 15 { &moves[0..15] } else { &moves };

        let mut best_score = if maximizing { i32::MIN } else { i32::MAX };

        for mv in limited_moves.iter() {
            if state.make_move(*mv) {
                let score = -self.minimax(state, depth - 1, alpha, beta, !maximizing);
                state.unmake_move(*mv);

                if maximizing {
                    if score > best_score {
                        best_score = score;
                    }
                    alpha = alpha.max(score);
                    if beta <= alpha {
                        break;
                    }
                } else {
                    if score < best_score {
                        best_score = score;
                    }
                    beta = beta.min(score);
                    if beta <= alpha {
                        break;
                    }
                }
            }
        }

        best_score
    }

    fn evaluate_position(&self, state: &GameState) -> i32 {
        if state.is_game_over() {
            return if state.current_player == 1 { -10000 } else { 10000 };
        }

        let mut score = 0;

        // Evaluate all possible 5-piece windows in all directions
        let directions = [(0, 1), (1, 0), (1, 1), (1, -1)];

        for i in 0..19 {
            for j in 0..19 {
                for (dx, dy) in directions {
                    // Evaluate 5-piece windows starting from this position
                    if self.can_form_window(i, j, dx, dy) {
                        let window_score = self.evaluate_window(state, i, j, dx, dy);
                        score += window_score;
                    }
                }
            }
        }

        score
    }

    fn can_form_window(&self, x: usize, y: usize, dx: i32, dy: i32) -> bool {
        let end_x = x as i32 + 4 * dx;
        let end_y = y as i32 + 4 * dy;
        end_x >= 0 && end_x < 19 && end_y >= 0 && end_y < 19
    }

    fn evaluate_window(&self, state: &GameState, start_x: usize, start_y: usize, dx: i32, dy: i32) -> i32 {
        let mut player1_count = 0;
        let mut player2_count = 0;
        let mut empty_count = 0;

        for i in 0..5 {
            let x = (start_x as i32 + i * dx) as usize;
            let y = (start_y as i32 + i * dy) as usize;
            match state.board[x][y] {
                1 => player1_count += 1,
                2 => player2_count += 1,
                _ => empty_count += 1,
            }
        }

        // Only score if there's a chance to make 5 in a row
        if player1_count + player2_count < 2 {
            return 0;
        }

        // Score based on the stronger player's position
        let p1_score = self.score_pattern(player1_count, empty_count);
        let p2_score = self.score_pattern(player2_count, empty_count);

        p1_score - p2_score
    }

    fn evaluate_window_for_player(&self, state: &GameState, start_x: usize, start_y: usize, dx: i32, dy: i32, player: i32) -> i32 {
        let mut player_count = 0;
        let mut empty_count = 0;

        for i in 0..5 {
            let x = (start_x as i32 + i * dx) as usize;
            let y = (start_y as i32 + i * dy) as usize;
            match state.board[x][y] {
                p if p == player => player_count += 1,
                0 => empty_count += 1,
                _ => {} // Opponent piece, doesn't count for this player's threat
            }
        }

        // Only score if this player has pieces in this window
        if player_count == 0 {
            return 0;
        }

        self.score_pattern(player_count, empty_count)
    }

    fn score_pattern(&self, pieces: i32, empties: i32) -> i32 {
        match pieces {
            5 => 1000000, // Win
            4 if empties == 1 => 100000, // One move to win (very high priority)
            3 if empties == 2 => 10000, // Two moves to win
            2 if empties == 3 => 1000,  // Three moves to win
            1 if empties == 4 => 100,   // Four moves to win
            _ => 0,
        }
    }

    fn order_moves(&self, state: &GameState) -> Vec<Move> {
        let mut moves_with_scores: Vec<(Move, i32)> = Vec::new();

        for mv in state.get_legal_moves() {
            // Simple heuristic scoring without full state evaluation
            let center_dist = (mv.x - 9).abs() + (mv.y - 9).abs();
            let center_score = (20 - center_dist) as i32;

            // Check if this move creates or blocks immediate threats
            let threat_score = self.quick_threat_check(state, mv);

            let total_score = center_score + threat_score * 10;
            moves_with_scores.push((mv, total_score));
        }

        // Sort by score (highest first)
        moves_with_scores.sort_by(|a, b| b.1.cmp(&a.1));

        // Return top 20 moves
        moves_with_scores.into_iter().take(20).map(|(mv, _)| mv).collect()
    }

    fn quick_threat_check(&self, state: &GameState, mv: Move) -> i32 {
        let mut threat_score = 0;
        let directions = [(0, 1), (1, 0), (1, 1), (1, -1)];

        // Check if placing here would create a 4-in-a-row for us or block opponent's 4-in-a-row
        for (dx, dy) in directions {
            // Check our potential (positive for creating threats)
            threat_score += self.check_line_threat(state, mv.x as i32, mv.y as i32, dx, dy, state.current_player) * 2;

            // Check opponent's threat we're blocking (very high priority to block)
            let opponent_threat = self.check_line_threat(state, mv.x as i32, mv.y as i32, dx, dy, 3 - state.current_player);
            if opponent_threat >= 4 { // Blocking a win is critical
                threat_score += 1000;
            } else {
                threat_score += opponent_threat;
            }
        }

        threat_score
    }

    fn check_line_threat(&self, state: &GameState, x: i32, y: i32, dx: i32, dy: i32, player: i32) -> i32 {
        let mut count = 1; // The piece we're placing

        // Count in positive direction
        let mut i = x + dx;
        let mut j = y + dy;
        while i >= 0 && i < 19 && j >= 0 && j < 19 && state.board[i as usize][j as usize] == player {
            count += 1;
            i += dx;
            j += dy;
        }

        // Count in negative direction
        i = x - dx;
        j = y - dy;
        while i >= 0 && i < 19 && j >= 0 && j < 19 && state.board[i as usize][j as usize] == player {
            count += 1;
            i -= dx;
            j -= dy;
        }

        // Score based on consecutive pieces
        match count {
            4 => 1000, // Creates or blocks 4 in a row (very high priority)
            3 => 100,  // Creates or blocks 3 in a row
            2 => 10,   // Creates or blocks 2 in a row
            _ => 0,
        }
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
    let mut rust_board = [[0i32; 19]; 19];
    for i in 0..19 {
        for j in 0..19 {
            unsafe {
                rust_board[i][j] = *board.offset((i * 19 + j) as isize);
            }
        }
    }

    let state = GameState {
        board: rust_board,
        current_player,
        turn_count,
    };

    ai.get_best_move(&state, max_depth)
}

#[no_mangle]
pub extern "C" fn rust_ai_evaluate_position(board: *const i32, current_player: i32, turn_count: i32) -> i32 {
    let mut rust_board = [[0i32; 19]; 19];
    for i in 0..19 {
        for j in 0..19 {
            unsafe {
                rust_board[i][j] = *board.offset((i * 19 + j) as isize);
            }
        }
    }

    let state = GameState {
        board: rust_board,
        current_player,
        turn_count,
    };

    let ai = AI::new();
    ai.evaluate_position(&state)
}