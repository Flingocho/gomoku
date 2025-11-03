// Game types and constants - Similar to game_types.hpp/cpp

pub const BOARD_SIZE: usize = 19;
pub const PLAYER1: i32 = 1; // Human
pub const PLAYER2: i32 = 2; // AI
pub const EMPTY: i32 = 0;

// Direction constants matching C++
pub const MAIN_DIRECTIONS: [(i32, i32); 4] = [
    (0, 1),  // Horizontal →
    (1, 0),  // Vertical ↓
    (1, 1),  // Diagonal ↘
    (1, -1), // Diagonal ↗
];

pub const CAPTURE_DIRECTIONS: [(i32, i32); 8] = [
    (-1, -1), (-1, 0), (-1, 1),  // Top-left, Top, Top-right
    (0, -1),           (0, 1),    // Left, Right
    (1, -1),  (1, 0),  (1, 1),    // Bottom-left, Bottom, Bottom-right
];

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

#[derive(Clone)]
pub struct GameState {
    pub board: [[i32; BOARD_SIZE]; BOARD_SIZE],
    pub current_player: i32,
    pub turn_count: i32,
    pub captures: [i32; 2], // [PLAYER1_captures, PLAYER2_captures]
    pub last_human_move: Move,
    pub zobrist_hash: u64,
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

    pub fn is_valid(&self, x: i32, y: i32) -> bool {
        x >= 0 && y >= 0 && x < BOARD_SIZE as i32 && y < BOARD_SIZE as i32
    }

    pub fn is_empty(&self, x: i32, y: i32) -> bool {
        if !self.is_valid(x, y) {
            return false;
        }
        self.board[x as usize][y as usize] == EMPTY
    }

    pub fn get_piece(&self, x: i32, y: i32) -> i32 {
        if !self.is_valid(x, y) {
            return -1;
        }
        self.board[x as usize][y as usize]
    }

    pub fn get_opponent(&self, player: i32) -> i32 {
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
