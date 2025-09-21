/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ai.cpp                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/14 21:26:15 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/21 17:50:00 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/ai.hpp"

Move AI::getBestMove(const GameState& state) {
	int depth = 0;

	if (state.turnCount < 6)
		depth = 6;
	else if (state.turnCount >= 6 && state.turnCount <= 20)
		depth = 8;
	else if (state.turnCount > 20)
		depth = 10;
    lastResult = searchEngine.findBestMoveIterative(state, depth);
    return lastResult.bestMove;
}