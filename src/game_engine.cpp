/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   game_engine.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/14 21:26:45 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/29 19:56:20 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/game_engine.hpp"
#include "../include/display.hpp"
#include "../include/debug_analyzer.hpp"
#include <chrono>
#include <iostream>
#include <algorithm>
#include <map>
#include <utility>

void GameEngine::newGame()
{
	state = GameState(); // Reset to initial state
	lastHumanMove = Move(-1, -1); // También reiniciar el campo local
}

bool GameEngine::makeHumanMove(const Move& move) {
    // En modo VS_HUMAN_SUGGESTED, ambos jugadores son humanos
    if (currentMode == GameMode::VS_AI && state.currentPlayer != GameState::PLAYER1) {
        return false;
    }
    
    lastHumanMove = move;
    state.lastHumanMove = move;
    
    RuleEngine::MoveResult result = RuleEngine::applyMove(state, move);
    
    return result.success;
}

Move GameEngine::makeAIMove() {
    if (state.currentPlayer != GameState::PLAYER2) return Move();
    
    auto start = std::chrono::high_resolution_clock::now();
    Move bestMove = ai.getBestMove(state);
    auto end = std::chrono::high_resolution_clock::now();
    lastAITime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    DEBUG_LOG_STATS("AI Stats: " + std::to_string(ai.getLastNodesEvaluated()) + 
                   " nodes, " + std::to_string(lastAITime) + "ms");
    
    if (bestMove.isValid()) {
        RuleEngine::applyMove(state, bestMove);
    }
    
    return bestMove;
}

bool GameEngine::isGameOver() const
{
	return RuleEngine::checkWin(state, GameState::PLAYER1) ||
		   RuleEngine::checkWin(state, GameState::PLAYER2);
}

int GameEngine::getWinner() const
{
	if (RuleEngine::checkWin(state, GameState::PLAYER1))
		return GameState::PLAYER1;
	if (RuleEngine::checkWin(state, GameState::PLAYER2))
		return GameState::PLAYER2;
	return 0; // No winner
}
