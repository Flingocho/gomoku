/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ai.cpp                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/14 21:26:15 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/14 21:26:20 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/ai.hpp"

Move AI::getBestMove(const GameState& state) {
    lastResult = searchEngine.findBestMove(state, depth);
    return lastResult.bestMove;
}