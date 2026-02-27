// ============================================
// Game types and constants — Exact match of game_types.hpp/cpp
// ============================================

pub const BOARD_SIZE: usize = 19;
pub const BOARD_CENTER: i32 = 9;
pub const PLAYER1: i32 = 1; // Human
pub const PLAYER2: i32 = 2; // AI
pub const EMPTY: i32 = 0;
pub const WIN_CAPTURES_NORMAL: i32 = 10;

/// 4 main directions (horizontal, vertical, 2 diagonals)
pub const MAIN_DIRECTIONS: [(i32, i32); 4] = [
    (0, 1),   // Horizontal →
    (1, 0),   // Vertical ↓
    (1, 1),   // Diagonal ↘
    (1, -1),  // Diagonal ↗
];

/// All 8 directions
pub const ALL_DIRECTIONS: [(i32, i32); 8] = [
    (-1, -1), (-1, 0), (-1, 1),
    (0, -1),           (0, 1),
    (1, -1),  (1, 0),  (1, 1),
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
        self.x >= 0 && self.y >= 0
            && self.x < BOARD_SIZE as i32
            && self.y < BOARD_SIZE as i32
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
            board: [[EMPTY; BOARD_SIZE]; BOARD_SIZE],
            current_player: PLAYER1,
            turn_count: 0,
            captures: [0, 0],
            last_human_move: Move::invalid(),
            zobrist_hash: 0,
        }
    }

    #[inline]
    pub fn is_valid(&self, x: i32, y: i32) -> bool {
        x >= 0 && y >= 0 && x < BOARD_SIZE as i32 && y < BOARD_SIZE as i32
    }

    #[inline]
    pub fn is_empty(&self, x: i32, y: i32) -> bool {
        self.is_valid(x, y) && self.board[x as usize][y as usize] == EMPTY
    }

    #[inline]
    pub fn get_piece(&self, x: i32, y: i32) -> i32 {
        if !self.is_valid(x, y) {
            -1
        } else {
            self.board[x as usize][y as usize]
        }
    }

    #[inline]
    pub fn get_opponent(&self, player: i32) -> i32 {
        if player == PLAYER1 { PLAYER2 } else { PLAYER1 }
    }
}
