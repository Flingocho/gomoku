/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   game_engine.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/14 21:26:45 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/18 17:12:12 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/game_engine.hpp"
#include "../include/debug_analyzer.hpp"
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
    if (state.currentPlayer != GameState::PLAYER2) return Move();
    
    auto start = std::chrono::high_resolution_clock::now();
    
    Move bestMove = ai.getBestMove(state);
    
    auto end = std::chrono::high_resolution_clock::now();
    lastAITime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    // **MODIFICADO: Mejorar output de estadísticas**
    if (g_debugAnalyzer && g_debugAnalyzer->shouldDebug(0, 0, true)) {
        // El debug ya se mostró en findBestMove, solo mostrar estadísticas adicionales
        std::cout << "Cache utilization: " << (ai.getLastCacheHitRate() * 100) 
                  << "% hit rate, " << ai.getCacheSize() << " entries" << std::endl;
    } else {
        // Si debug está off, mostrar stats básicas
        std::cout << "AI Stats: " << ai.getLastNodesEvaluated() << " nodes, "
                  << lastAITime << "ms, "
                  << (ai.getLastCacheHitRate() * 100) << "% cache hit rate" << std::endl;
    }
    
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
