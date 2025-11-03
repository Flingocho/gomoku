// Main library file - Organizes modules similar to C++ structure
// This matches the C++ project structure:
// - game_types.rs ≈ game_types.hpp/cpp
// - evaluator.rs ≈ evaluator.hpp/cpp
// - transposition_table.rs ≈ transposition_search.hpp/cpp (table part)
// - move_ordering.rs ≈ transposition_search.hpp/cpp (move generation)
// - ai.rs ≈ ai.hpp/cpp + transposition_search.hpp/cpp (search engine)

mod game_types;
mod evaluator;
mod transposition_table;
mod move_ordering;
mod ai;

// Re-export main types for public API
pub use game_types::{Move, GameState, BOARD_SIZE};
pub use ai::AI;

// FFI interface for C++ - Similar to rust_ai_wrapper.hpp
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

    evaluator::Evaluator::evaluate(&state, 6, 0) // Use default depth for evaluation
}
