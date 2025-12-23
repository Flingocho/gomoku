#include "../include/ai/ai.hpp"
#include "../include/core/game_types.hpp"
#include "../include/core/game_engine.hpp"
#include <iostream>
#include <cassert>

int main() {
    std::cout << "Running AI Test..." << std::endl;

    // Initialize Zobrist Hasher (Required for AI)
    GameState::initializeHasher();

    // Initialize Game State
    GameState state;
    // Place some pieces to create a scenario
    // Player 1 (Black) at center
    state.board[9][9] = 1;
    state.turnCount = 1;
    state.currentPlayer = 2; // AI is Player 2 (White) usually if P1 starts

    AI ai(4, CPP_IMPLEMENTATION); // Depth 4, C++ impl

    std::cout << "Getting best move..." << std::endl;
    Move bestMove = ai.getBestMove(state);

    std::cout << "Best move: " << (int)bestMove.x << ", " << (int)bestMove.y << std::endl;

    assert(bestMove.isValid());

    // Test a threat scenario
    // P1 has 3 in a row
    state.board[10][9] = 1;
    state.board[11][9] = 1;
    state.currentPlayer = 2;

    // AI should block or create a counter threat
    bestMove = ai.getBestMove(state);
    std::cout << "Response to threat: " << (int)bestMove.x << ", " << (int)bestMove.y << std::endl;

    std::cout << "AI Test Passed." << std::endl;

    // Cleanup
    GameState::cleanupHasher();
    return 0;
}
