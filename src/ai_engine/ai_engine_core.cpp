// ===============================================
// AI Engine - Core Module
// ===============================================
// Handles: AI wrapper, Rust/C++ dispatcher, configuration, statistics
// Dependencies: TranspositionSearch, RustAIWrapper
// ===============================================

#include "../../include/ai/ai.hpp"
#include "../../include/ai/rust_ai_wrapper.hpp"

// ===============================================
// MAIN AI INTERFACE
// ===============================================

Move AI::getBestMove(const GameState& state) {
    if (implementation == RUST_IMPLEMENTATION) {
        int maxDepth = getDepthForGamePhase(state);
        return RustAIWrapper::getBestMove(state, maxDepth);
    } else {
        // Original C++ implementation
        int depth = getDepthForGamePhase(state);
        lastResult = searchEngine.findBestMoveIterative(state, depth);
        return lastResult.bestMove;
    }
}

// ===============================================
// DEPTH CONFIGURATION
// ===============================================

int AI::getDepthForGamePhase(const GameState& state) {
    if (state.turnCount < 6)
        return 6;      // Opening: less depth for speed
    else if (state.turnCount >= 6 && state.turnCount <= 12)
        return 8;      // Midgame: moderate depth
    else
        return 10;     // Endgame: maximum depth
}

// ===============================================
// SEARCH DELEGATION
// ===============================================

TranspositionSearch::SearchResult AI::findBestMoveIterative(const GameState& state, int maxDepth) {
    if (implementation == RUST_IMPLEMENTATION) {
        // For Rust implementation, create a basic result since we don't have detailed stats yet
        Move bestMove = RustAIWrapper::getBestMove(state, maxDepth);
        TranspositionSearch::SearchResult result;
        result.bestMove = bestMove;
        result.score = 0; // TODO: Get score from Rust
        result.nodesEvaluated = 0; // TODO: Get stats from Rust
        result.cacheHits = 0;
        result.cacheHitRate = 0.0f;
        lastResult = result;
        return result;
    } else {
        lastResult = searchEngine.findBestMoveIterative(state, maxDepth);
        return lastResult;
    }
}

// ===============================================
// MOVE GENERATION
// ===============================================

std::vector<Move> AI::generateOrderedMoves(const GameState& state) {
    // For now, both implementations use the same move generation
    return searchEngine.generateOrderedMoves(state);
}

// ===============================================
// QUICK EVALUATION
// ===============================================

int AI::quickEvaluateMove(const GameState& state, const Move& move) {
    if (implementation == RUST_IMPLEMENTATION) {
        // For Rust implementation, use the position evaluation
        return RustAIWrapper::evaluatePosition(state);
    } else {
        return searchEngine.quickEvaluateMove(state, move);
    }
}
