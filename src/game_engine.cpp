/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   game_engine.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/14 21:26:45 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/24 17:50:02 by jainavas         ###   ########.fr       */
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

void GameEngine::newGame() {
    state = GameState(); // Reset to initial state
}

bool GameEngine::makeHumanMove(const Move& move) {
    if (state.currentPlayer != GameState::PLAYER1) return false;
    
    RuleEngine::MoveResult result = RuleEngine::applyMove(state, move);
    return result.success;
}

// En game_engine.cpp
Move GameEngine::makeAIMove() {
    if (state.currentPlayer != GameState::PLAYER2) return Move();
    
    Move bestMove;
    
    // NUEVO: Si tenemos cache construido y el humano ya jugó
    if (hasPendingDecision && lastHumanMove.isValid()) {
        bestMove = findBestAIMoveAfterHuman(lastHumanMove);
        
        if (bestMove.isValid()) {
            lastAITime = 0; // Tiempo efectivo = 0
            DEBUG_LOG_STATS("AI Stats (conditional): Using cached conditional response");
        }
    }
    
    // Fallback: Cálculo normal
    if (!bestMove.isValid()) {
        auto start = std::chrono::high_resolution_clock::now();
        bestMove = ai.getBestMove(state);
        auto end = std::chrono::high_resolution_clock::now();
        lastAITime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        DEBUG_LOG_STATS("AI Stats (normal): " + std::to_string(ai.getLastNodesEvaluated()) + 
                       " nodes, " + std::to_string(lastAITime) + "ms");
    }
    
    if (bestMove.isValid()) {
        RuleEngine::applyMove(state, bestMove);
        
        // NUEVO: Iniciar construcción de cache condicional para el siguiente turno
        startBackgroundCalculation();
    }
    
    hasPendingDecision = false; // Resetear
    return bestMove;
}

// En game_engine.cpp - Modificar backgroundCalculationLoop()
void GameEngine::backgroundCalculationLoop() {
    std::unique_lock<std::mutex> lock(backgroundMutex);
    
    while (!shouldStop) {
        backgroundCV.wait(lock, [this] { return shouldCalculate.load() || shouldStop.load(); });
        
        if (shouldStop) break;
        
        if (shouldCalculate) {
            shouldCalculate = false;
            isCalculating = true;
            
            lock.unlock();
            
            DEBUG_LOG_AI("Background conditional cache building started...");
            
            // NUEVO: Construir cache de respuestas condicionadas
            buildConditionalCache();
            
            lock.lock();
            
            if (!shouldStop) {
                hasBackgroundResult = true;
                DEBUG_LOG_AI("Background conditional cache completed: " + 
                           std::to_string(conditionalCache.size()) + " combinations cached");
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

// En game_engine.cpp
void GameEngine::buildConditionalCache() {
    conditionalCache.clear();
    
    DEBUG_LOG_AI("Building conditional response cache...");
    
    // 1. Obtener top movimientos IA candidatos
    std::vector<Move> topAIMoves = getTopAIMoves(state);
    
    int totalCombinations = 0;
    
    // 2. Para cada movimiento IA candidato
    for (const Move& aiMove : topAIMoves) {
        DEBUG_LOG_AI("Analyzing AI move candidate: " + 
                     std::string(1, char('A' + aiMove.y)) + std::to_string(aiMove.x + 1));
        
        // Simular movimiento IA
        GameState stateAfterAI = state;
        RuleEngine::MoveResult aiResult = RuleEngine::applyMove(stateAfterAI, aiMove);
        
        if (!aiResult.success) continue;
        
        // 3. Obtener top movimientos humanos después de este movimiento IA
        std::vector<Move> topHumanMoves = getTopHumanMovesAfterAI(stateAfterAI);
        
        // 4. Para cada movimiento humano probable
        for (const Move& humanMove : topHumanMoves) {
            // Simular movimiento humano
            GameState stateAfterBoth = stateAfterAI;
            RuleEngine::MoveResult humanResult = RuleEngine::applyMove(stateAfterBoth, humanMove);
            
            if (!humanResult.success) continue;
            
            // 5. Calcular mejor respuesta IA
            Move aiResponse = ai.getBestMove(stateAfterBoth);
            int sequenceScore = ai.getLastScore();
            
            if (aiResponse.isValid()) {
                ConditionalResponse conditional(aiMove, humanMove, aiResponse, 
                                              sequenceScore, stateAfterBoth.getZobristHash());
                conditionalCache.push_back(conditional);
                totalCombinations++;
                
                DEBUG_LOG_AI("Cached: AI " + std::string(1, char('A' + aiMove.y)) + std::to_string(aiMove.x + 1) +
                           " → Human " + std::string(1, char('A' + humanMove.y)) + std::to_string(humanMove.x + 1) +
                           " → AI " + std::string(1, char('A' + aiResponse.y)) + std::to_string(aiResponse.x + 1) +
                           " (score: " + std::to_string(sequenceScore) + ")");
            }
        }
    }
    
    DEBUG_LOG_AI("Conditional cache built: " + std::to_string(totalCombinations) + " combinations");
    hasPendingDecision = true;
}

// En game_engine.cpp
std::vector<Move> GameEngine::getTopAIMoves(const GameState& state) {
	TranspositionSearch moveGenerator;
    auto allMoves = moveGenerator.generateOrderedMoves(state);
    
    std::vector<Move> topMoves;
    for (size_t i = 0; i < std::min(allMoves.size(), (size_t)TOP_AI_MOVES); i++) {
        topMoves.push_back(allMoves[i]);
    }
    
    return topMoves;
}

std::vector<Move> GameEngine::getTopHumanMovesAfterAI(const GameState& state) {
    // Crear simulador para perspectiva humana
    AI humanSimulator(4);
    
    auto allMoves = humanSimulator.generateOrderedMoves(state);
    
    // Evaluar y ordenar movimientos humanos
    std::vector<std::pair<Move, int>> scoredMoves;
    for (const Move& move : allMoves) {
        int score = humanSimulator.quickEvaluateMove(state, move);
        scoredMoves.emplace_back(move, score);
        
        if (scoredMoves.size() >= TOP_HUMAN_MOVES * 2) break; // Limitar para velocidad
    }
    
    // Ordenar por score (mejores para humano primero)
    std::sort(scoredMoves.begin(), scoredMoves.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    std::vector<Move> topMoves;
    size_t moveCount = std::min(scoredMoves.size(), static_cast<size_t>(TOP_HUMAN_MOVES));
    for (size_t i = 0; i < moveCount; i++) {
        topMoves.push_back(scoredMoves[i].first);
    }
    
    return topMoves;
}

// En game_engine.cpp
Move GameEngine::findBestAIMoveAfterHuman(const Move& humanMove) {
    // 1. Buscar en cache todas las combinaciones con este movimiento humano
    std::vector<ConditionalResponse> validResponses;
    
    for (const auto& conditional : conditionalCache) {
        if (conditional.humanMove.x == humanMove.x && conditional.humanMove.y == humanMove.y) {
            
            // Validar que el estado es el esperado
            GameState testState = state;
            RuleEngine::MoveResult result = RuleEngine::applyMove(testState, humanMove);
            
            if (result.success && testState.getZobristHash() == conditional.stateHash) {
                validResponses.push_back(conditional);
            }
        }
    }
    
    if (!validResponses.empty()) {
        // 2. Encontrar la mejor secuencia (mejor score)
        auto best = std::max_element(validResponses.begin(), validResponses.end(),
                                    [](const auto& a, const auto& b) {
                                        return a.sequenceScore < b.sequenceScore;
                                    });
        
        DEBUG_LOG_AI("✓ Found cached response for human " + 
                     std::string(1, char('A' + humanMove.y)) + std::to_string(humanMove.x + 1) +
                     " → AI plays " + std::string(1, char('A' + best->aiMove.y)) + std::to_string(best->aiMove.x + 1) +
                     " (cached score: " + std::to_string(best->sequenceScore) + ")");
        
        return best->aiMove; // Retornar el movimiento IA de la mejor secuencia
    }
    
    // 3. NUEVO: Movimiento humano inesperado - evaluación rápida
    DEBUG_LOG_AI("Human move " + std::string(1, char('A' + humanMove.y)) + std::to_string(humanMove.x + 1) + 
                 " not in cache, quick evaluation...");
    
    int quickScore = quickEvaluateUnexpectedMove(humanMove);
    
    if (quickScore < -30000) { // Umbral: movimiento problemático
        DEBUG_LOG_AI("⚠️ Unexpected move appears threatening, recalculating with higher depth");
        return Move(); // Forzar recálculo completo
    } else {
        DEBUG_LOG_AI("✓ Unexpected move appears non-threatening, can use fallback strategy");
        
        // Usar el mejor movimiento IA de nuestro cache (el más común)
        if (!conditionalCache.empty()) {
            // Encontrar el movimiento IA que aparece más frecuentemente en el cache
            std::map<std::pair<int,int>, int> aiMoveFrequency;
            for (const auto& conditional : conditionalCache) {
                std::pair<int,int> moveKey = {conditional.aiMove.x, conditional.aiMove.y};
                aiMoveFrequency[moveKey]++;
            }
            
            auto mostFrequent = std::max_element(aiMoveFrequency.begin(), aiMoveFrequency.end(),
                                               [](const auto& a, const auto& b) {
                                                   return a.second < b.second;
                                               });
            
            Move fallbackMove(mostFrequent->first.first, mostFrequent->first.second);
            DEBUG_LOG_AI("Using most frequent AI move as fallback: " + 
                         std::string(1, char('A' + fallbackMove.y)) + std::to_string(fallbackMove.x + 1));
            return fallbackMove;
        }
    }
    
    return Move(); // No encontrado, forzar recálculo
}

// En game_engine.cpp
int GameEngine::quickEvaluateUnexpectedMove(const Move& humanMove) {
    // Crear estado después del movimiento humano
    GameState stateAfterHuman = state;
    RuleEngine::MoveResult result = RuleEngine::applyMove(stateAfterHuman, humanMove);
    
    if (!result.success) {
        return -100000; // Movimiento ilegal = muy problemático
    }
    
    // Evaluación rápida usando el evaluador existente
    int currentPlayer = stateAfterHuman.currentPlayer; // Ahora es turno IA
    int opponent = stateAfterHuman.getOpponent(currentPlayer);
    
    // Verificar amenazas inmediatas del humano
    bool humanHasThreats = Evaluator::hasWinningThreats(stateAfterHuman, opponent);
    if (humanHasThreats) {
        DEBUG_LOG_AI("⚠️ Quick eval: Human created winning threats!");
        return -50000; // Muy problemático
    }
    
    // Evaluación heurística básica
    int score = Evaluator::evaluate(stateAfterHuman);
    
    DEBUG_LOG_AI("Quick eval score: " + std::to_string(score));
    return score;
}


