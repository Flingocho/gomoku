/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ai.cpp                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/14 21:26:15 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/28 23:46:29 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/ai.hpp"

Move AI::getBestMove(const GameState& state) {
    int depth = getDepthForGamePhase(state);
    lastResult = searchEngine.findBestMoveIterative(state, depth);
    return lastResult.bestMove;
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
    lastResult = searchEngine.findBestMoveIterative(state, maxDepth);
    return lastResult;
}

std::vector<Move> AI::generateOrderedMoves(const GameState& state) {
    return searchEngine.generateOrderedMoves(state);
}

int AI::quickEvaluateMove(const GameState& state, const Move& move) {
    return searchEngine.quickEvaluateMove(state, move);
}