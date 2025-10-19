/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ai.cpp                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/14 21:26:15 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/30 19:54:03 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/ai.hpp"
#include "../include/rust_ai_wrapper.hpp"

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

int AI::getDepthForGamePhase(const GameState& state) {
    if (state.turnCount < 6)
        return 6;
    else if (state.turnCount >= 6 && state.turnCount <= 12)
        return 8;
    else
        return 10;
}

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

std::vector<Move> AI::generateOrderedMoves(const GameState& state) {
    // For now, both implementations use the same move generation
    return searchEngine.generateOrderedMoves(state);
}

int AI::quickEvaluateMove(const GameState& state, const Move& move) {
    if (implementation == RUST_IMPLEMENTATION) {
        // For Rust implementation, use the position evaluation
        return RustAIWrapper::evaluatePosition(state);
    } else {
        return searchEngine.quickEvaluateMove(state, move);
    }
}