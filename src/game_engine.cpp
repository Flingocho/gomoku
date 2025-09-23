/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   game_engine.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/14 21:26:45 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/23 15:28:03 by jainavas         ###   ########.fr       */
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
    
    // Verificar si tenemos resultado de cálculo anticipado
    Move bestMove = getBackgroundResult();
    
    if (!bestMove.isValid()) {
        // No hay resultado anticipado, calcular normalmente
        auto start = std::chrono::high_resolution_clock::now();
        bestMove = ai.getBestMove(state);
        auto end = std::chrono::high_resolution_clock::now();
        lastAITime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        DEBUG_LOG_STATS("AI Stats (normal): " + std::to_string(ai.getLastNodesEvaluated()) + " nodes, " + std::to_string(lastAITime) + "ms");
    } else {
        DEBUG_LOG_STATS("AI Stats (background): Using pre-calculated move " + std::string(1, char('A' + bestMove.y)) + std::to_string(bestMove.x + 1));
        lastAITime = 0; // Tiempo efectivo = 0 porque ya estaba calculado
    }
    
    if (bestMove.isValid()) {
        RuleEngine::applyMove(state, bestMove);
        
        // Iniciar cálculo anticipado para el próximo turno
        startBackgroundCalculation();
    }
    
    return bestMove;
}

void GameEngine::backgroundCalculationLoop() {
    std::unique_lock<std::mutex> lock(backgroundMutex);
    
    while (!shouldStop) {
        backgroundCV.wait(lock, [this] { return shouldCalculate.load() || shouldStop.load(); });
        
        if (shouldStop) break;
        
        if (shouldCalculate) {
            shouldCalculate = false;
            isCalculating = true;
            
            // Copiar estado actual para trabajar
            GameState workingState = state;
            
            lock.unlock(); // Liberar lock durante cálculo
            
            DEBUG_LOG_AI("Background thinking started...");
            
            // Calcular con profundidad+1 (impar para absorber turno humano)
            int backgroundDepth = ai.getDepth() + 1;
            AI backgroundAI(backgroundDepth);
            Move result = backgroundAI.getBestMove(workingState);
            
            lock.lock(); // Re-adquirir lock
            
            if (!shouldStop) { // Solo guardar si no se canceló
                backgroundBestMove = result;
                hasBackgroundResult = true;
                DEBUG_LOG_AI("Background thinking completed: " + std::string(1, char('A' + result.y)) + std::to_string(result.x + 1) + " (depth " + std::to_string(backgroundDepth) + ")");
            }
            
            isCalculating = false;
        }
    }
}

void GameEngine::startBackgroundCalculation() {
    std::lock_guard<std::mutex> lock(backgroundMutex);
    
    // Cancelar cualquier cálculo previo
    hasBackgroundResult = false;
    shouldCalculate = true;
    backgroundCV.notify_one();
}

Move GameEngine::getBackgroundResult() {
    std::lock_guard<std::mutex> lock(backgroundMutex);
    
    if (!hasBackgroundResult) {
        return Move(); // No hay resultado válido
    }
    
    // Validación simple: verificar si el movimiento sigue siendo legal
    if (!RuleEngine::isLegalMove(state, backgroundBestMove)) {
        DEBUG_LOG_AI("Background move no longer legal, recalculating...");
        hasBackgroundResult = false;
        return Move();
    }
    
    // Validación de relevancia: si el humano jugó muy lejos, usar resultado
    // (Por ahora asumimos que es válido - podemos refinar después)
    
    hasBackgroundResult = false; // Consumir resultado
    return backgroundBestMove;
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
