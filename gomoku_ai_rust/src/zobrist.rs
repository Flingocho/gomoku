// ============================================
// Zobrist hashing — Matches zobrist_hasher.hpp/cpp
// Global table initialized once, used for transposition table keys
// ============================================

use crate::game_types::*;
use std::sync::Once;

static INIT: Once = Once::new();
static mut ZOBRIST: Option<ZobristTable> = None;

pub struct ZobristTable {
    piece_keys: [[[u64; 3]; BOARD_SIZE]; BOARD_SIZE], // [x][y][piece]
    turn_key: u64,
    capture_keys: [[u64; 11]; 2], // [player_index][capture_count 0..10]
}

/// Simple xorshift64 PRNG (deterministic, no external dependencies)
fn xorshift64(state: &mut u64) -> u64 {
    let mut x = *state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    *state = x;
    x
}

impl ZobristTable {
    fn new() -> Self {
        let mut rng: u64 = 0x123456789ABCDEF0; // Fixed seed for determinism
        let mut table = ZobristTable {
            piece_keys: [[[0u64; 3]; BOARD_SIZE]; BOARD_SIZE],
            turn_key: 0,
            capture_keys: [[0u64; 11]; 2],
        };

        for i in 0..BOARD_SIZE {
            for j in 0..BOARD_SIZE {
                table.piece_keys[i][j][EMPTY as usize] = 0; // EMPTY is always 0
                table.piece_keys[i][j][PLAYER1 as usize] = xorshift64(&mut rng);
                table.piece_keys[i][j][PLAYER2 as usize] = xorshift64(&mut rng);
            }
        }

        table.turn_key = xorshift64(&mut rng);

        for p in 0..2 {
            for c in 0..11 {
                table.capture_keys[p][c] = xorshift64(&mut rng);
            }
        }

        table
    }

    /// Compute full hash from scratch — O(n²), used for initialization only
    pub fn compute_full_hash(&self, state: &GameState) -> u64 {
        let mut hash: u64 = 0;

        for i in 0..BOARD_SIZE {
            for j in 0..BOARD_SIZE {
                let piece = state.board[i][j];
                if piece != EMPTY {
                    hash ^= self.piece_keys[i][j][piece as usize];
                }
            }
        }

        if state.current_player == PLAYER2 {
            hash ^= self.turn_key;
        }

        hash ^= self.capture_keys[0][(state.captures[0].min(10)) as usize];
        hash ^= self.capture_keys[1][(state.captures[1].min(10)) as usize];

        hash
    }

    #[inline]
    pub fn piece_key(&self, x: i32, y: i32, player: i32) -> u64 {
        self.piece_keys[x as usize][y as usize][player as usize]
    }

    #[inline]
    pub fn turn_key(&self) -> u64 {
        self.turn_key
    }

    #[inline]
    pub fn capture_key(&self, player_idx: usize, count: i32) -> u64 {
        self.capture_keys[player_idx][(count.min(10)) as usize]
    }
}

/// Get the global Zobrist table (thread-safe, initialized once)
#[allow(static_mut_refs)]
pub fn get_zobrist() -> &'static ZobristTable {
    INIT.call_once(|| {
        unsafe {
            ZOBRIST = Some(ZobristTable::new());
        }
    });
    unsafe { ZOBRIST.as_ref().unwrap() }
}
