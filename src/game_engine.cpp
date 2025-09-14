/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   game_engine.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/14 21:26:45 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/14 21:26:59 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/game_engine.hpp"
#include <chrono>
#include <iostream>

void GameEngine::newGame() {
    state = GameState(); // Reset to initial state
}

bool GameEngine::makeHumanMove(const Move& move) {
    if (state.currentPlayer != GameState::PLAYER1) return false;
    
    RuleEngine::MoveResult result = RuleEngine::applyMove(state, move);
    return result.success;
}

Move GameEngine::makeAIMove() {
    if (state.currentPlayer != GameState::PLAYER2) return Move(); // Invalid
    
    auto start = std::chrono::high_resolution_clock::now();
    
    Move bestMove = ai.getBestMove(state);
    
    auto end = std::chrono::high_resolution_clock::now();
    lastAITime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    // Debug info opcional
    std::cout << "AI Stats: " << ai.getLastNodesEvaluated() << " nodes, "
              << ai.getLastCacheHits() << " cache hits ("
              << (ai.getLastCacheHitRate() * 100) << "%), "
              << "Cache size: " << ai.getCacheSize() << std::endl;
    
    if (bestMove.isValid()) {
        RuleEngine::applyMove(state, bestMove);
    }
    
    return bestMove;
}

bool GameEngine::isGameOver() const {
    return RuleEngine::checkWin(state, GameState::PLAYER1) || 
           RuleEngine::checkWin(state, GameState::PLAYER2);
}

int GameEngine::getWinner() const {
    if (RuleEngine::checkWin(state, GameState::PLAYER1)) return GameState::PLAYER1;
    if (RuleEngine::checkWin(state, GameState::PLAYER2)) return GameState::PLAYER2;
    return 0; // No winner
}
