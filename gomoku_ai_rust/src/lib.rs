// ============================================
// lib.rs — Module declarations & FFI entry points
// Matches rust_ai_wrapper.hpp interface
// ============================================

pub mod game_types;
pub mod zobrist;
pub mod rule_engine;
pub mod evaluator;
pub mod transposition_table;
pub mod move_ordering;
pub mod ai;

use game_types::*;
use zobrist::get_zobrist;
use evaluator::Evaluator;
use ai::AI;

// ============================================
// FFI Entry Points
// ============================================

/// Get the best move for the current game state.
/// Called from C++ via rust_ai_wrapper.hpp.
///
/// Parameters:
///   board:          flat [19*19] i32 array (row-major)
///   current_player: 1 or 2
///   turn_count:     number of turns played
///   captures_p1:    captures for player 1
///   captures_p2:    captures for player 2
///   last_move_x:    x of last human move (-1 if none)
///   last_move_y:    y of last human move (-1 if none)
///   max_depth:      search depth
#[no_mangle]
pub extern "C" fn rust_ai_get_best_move(
    board: *const i32,
    current_player: i32,
    turn_count: i32,
    captures_p1: i32,
    captures_p2: i32,
    last_move_x: i32,
    last_move_y: i32,
    max_depth: i32,
) -> Move {
    let state = build_game_state(
        board,
        current_player,
        turn_count,
        captures_p1,
        captures_p2,
        last_move_x,
        last_move_y,
    );

    let mut ai = AI::new();
    let result = ai.find_best_move_iterative(&state, max_depth);

    if result.best_move.is_valid() {
        result.best_move
    } else {
        // Fallback: play center or first empty cell
        if state.is_empty(BOARD_CENTER, BOARD_CENTER) {
            Move::new(BOARD_CENTER, BOARD_CENTER)
        } else {
            find_first_empty(&state)
        }
    }
}

/// Evaluate the current position for use in tests/suggestions.
#[no_mangle]
pub extern "C" fn rust_ai_evaluate_position(
    board: *const i32,
    current_player: i32,
    turn_count: i32,
    captures_p1: i32,
    captures_p2: i32,
) -> i32 {
    let state = build_game_state(
        board,
        current_player,
        turn_count,
        captures_p1,
        captures_p2,
        -1,
        -1,
    );

    Evaluator::evaluate_simple(&state)
}

// ============================================
// Helpers
// ============================================

fn build_game_state(
    board: *const i32,
    current_player: i32,
    turn_count: i32,
    captures_p1: i32,
    captures_p2: i32,
    last_move_x: i32,
    last_move_y: i32,
) -> GameState {
    let mut state = GameState::new();
    state.current_player = current_player;
    state.turn_count = turn_count;
    state.captures[0] = captures_p1;
    state.captures[1] = captures_p2;
    state.last_human_move = Move::new(last_move_x, last_move_y);

    // Copy flat board into 2D array
    if !board.is_null() {
        for i in 0..BOARD_SIZE {
            for j in 0..BOARD_SIZE {
                state.board[i][j] = unsafe { *board.add(i * BOARD_SIZE + j) };
            }
        }
    }

    // Compute initial Zobrist hash
    let z = get_zobrist();
    state.zobrist_hash = z.compute_full_hash(&state);

    state
}

fn find_first_empty(state: &GameState) -> Move {
    for i in 0..BOARD_SIZE as i32 {
        for j in 0..BOARD_SIZE as i32 {
            if state.is_empty(i, j) {
                return Move::new(i, j);
            }
        }
    }
    Move::new(BOARD_CENTER, BOARD_CENTER)
}
