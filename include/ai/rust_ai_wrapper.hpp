#ifndef RUST_AI_WRAPPER_HPP
#define RUST_AI_WRAPPER_HPP

#include "../core/game_types.hpp"

// Forward declarations for Rust FFI
extern "C" {
    struct RustMove {
        int x, y;
    };

    RustMove rust_ai_get_best_move(const int* board, int current_player, int turn_count, int max_depth);
    int rust_ai_evaluate_position(const int* board, int current_player, int turn_count);
}

class RustAIWrapper {
public:
    static Move getBestMove(const GameState& state, int maxDepth) {
        // Convert GameState board to flat array
        int flat_board[19 * 19];
        for (int i = 0; i < 19; ++i) {
            for (int j = 0; j < 19; ++j) {
                flat_board[i * 19 + j] = state.board[i][j];
            }
        }

        RustMove rust_move = rust_ai_get_best_move(
            flat_board,
            state.currentPlayer,
            state.turnCount,
            maxDepth
        );

        return Move(rust_move.x, rust_move.y);
    }

    static int evaluatePosition(const GameState& state) {
        // Convert GameState board to flat array
        int flat_board[19 * 19];
        for (int i = 0; i < 19; ++i) {
            for (int j = 0; j < 19; ++j) {
                flat_board[i * 19 + j] = state.board[i][j];
            }
        }

        return rust_ai_evaluate_position(flat_board, state.currentPlayer, state.turnCount);
    }
};

#endif
