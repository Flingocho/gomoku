/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ai.cpp                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/14 21:26:15 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/22 20:31:41 by jainavas         ###   ########.fr       */
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
    else if (state.turnCount >= 6 && state.turnCount <= 20)
        return 8;
    else
        return 10;
}