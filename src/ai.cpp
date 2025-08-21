/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ai.cpp                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/21 16:35:58 by jainavas          #+#    #+#             */
/*   Updated: 2025/08/21 16:37:41 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/ai.hpp"
#include <iostream>
#include <chrono>
#include <algorithm>

AI::AI(int player, int depth) 
    : aiPlayer(player), maxDepth(depth), nodesExplored(0), 
      evaluationsPerformed(0), lastSearchTime(0.0) {
    humanPlayer = (player == 1) ? 2 : 1;
    
    std::cout << "AI inicializada: Jugador " << aiPlayer 
              << ", Profundidad " << maxDepth << std::endl;
}

Move AI::getBestMove(Board& board) {
    auto startTime = std::chrono::high_resolution_clock::now();
    resetStats();
    
    std::cout << "\n🤖 IA ANALIZANDO POSICIÓN...\n";
    std::cout << "Capturas actuales - AI: " << board.getCaptures(aiPlayer) 
              << ", Humano: " << board.getCaptures(humanPlayer) << std::endl;
    
    // === PASO 1: VERIFICAR VICTORIA INMEDIATA ===
    Move winMove = findImmediateWin(board, aiPlayer);
    if (winMove.isValid()) {
        std::cout << "🎯 ¡MOVIMIENTO GANADOR ENCONTRADO!\n";
        lastSearchTime = 0.001; // Búsqueda instantánea
        return winMove;
    }
    
    // === PASO 2: BLOQUEAR DERROTA INMEDIATA ===
    Move blockMove = findImmediateWin(board, humanPlayer);
    if (blockMove.isValid()) {
        std::cout << "🛡️ BLOQUEANDO AMENAZA CRÍTICA\n";
        lastSearchTime = 0.001; // Búsqueda instantánea
        return blockMove;
    }
    
    // === PASO 3: BÚSQUEDA COMPLETA CON MINIMAX ===
    std::cout << "🔍 Iniciando búsqueda completa (profundidad " << maxDepth << ")...\n";
    
    // Crear nodo raíz del árbol de búsqueda
    auto rootNode = std::make_unique<GameNode>(board, aiPlayer);
    
    // Ejecutar búsqueda minimax
    Move bestMove = rootNode->getBestMove(maxDepth);
    
    // Calcular tiempo transcurrido
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    lastSearchTime = duration.count() / 1000.0;
    
    // Obtener estadísticas del árbol
    auto stats = rootNode->getTreeStats();
    nodesExplored = stats.totalNodes;
    evaluationsPerformed = stats.totalEvaluations;
    
    // Mostrar resultados
    std::cout << "\n✅ BÚSQUEDA COMPLETADA\n";
    std::cout << "Mejor movimiento: (" << bestMove.x << "," << bestMove.y 
              << ") con score " << bestMove.score << std::endl;
    std::cout << "Tiempo: " << lastSearchTime << "s" << std::endl;
    std::cout << "Nodos explorados: " << nodesExplored << std::endl;
    std::cout << "Evaluaciones: " << evaluationsPerformed << std::endl;
    
    // Verificar que el movimiento es válido
    if (!bestMove.isValid()) {
        std::cerr << "❌ ERROR: Movimiento inválido generado!\n";
        // Fallback: movimiento en el centro
        int center = board.getSize() / 2;
        return Move(center, center, 0);
    }
    
    return bestMove;
}

// === DETECCIÓN DE MOVIMIENTOS CRÍTICOS ===

Move AI::findImmediateWin(const Board& board, int player) {
    std::cout << "Buscando victoria inmediata para jugador " << player << "...\n";
    
    for (int i = 0; i < board.getSize(); ++i) {
        for (int j = 0; j < board.getSize(); ++j) {
            if (board.isEmpty(i, j)) {
                // Simular movimiento
                Board testBoard = board;
                if (testBoard.placePiece(i, j, player)) {
                    // ¿Este movimiento gana la partida?
                    if (testBoard.checkWin(player)) {
                        std::cout << "Victoria encontrada en (" << i << "," << j << ")\n";
                        return Move(i, j, 10000);
                    }
                }
            }
        }
    }
    
    return Move(-1, -1, 0); // No hay victoria inmediata
}

Move AI::findCriticalBlock(const Board& board, int player) {
    // Buscar si el oponente puede ganar en el próximo turno
    return findImmediateWin(board, player);
}

// === ANÁLISIS Y DEBUG ===

void AI::analyzePosition(const Board& board, int analysisDepth) {
    std::cout << "\n=== ANÁLISIS DETALLADO DE LA POSICIÓN ===\n";
    
    // Crear nodo para análisis
    auto analysisNode = std::make_unique<GameNode>(board, aiPlayer);
    
    // Generar algunos movimientos para análisis
    analysisNode->generateChildren();
    
    std::cout << "Jugador actual: " << aiPlayer << std::endl;
    std::cout << "Movimientos candidatos: " << analysisNode->getChildrenCount() << std::endl;
    
    // Evaluar posición actual
    int currentEval = analysisNode->evaluateNode();
    std::cout << "Evaluación actual: " << currentEval << std::endl;
    
    // Analizar algunos de los mejores movimientos
    std::vector<std::pair<Move, int>> moveAnalysis;
    
    for (size_t i = 0; i < std::min(size_t(5), analysisNode->getChildrenCount()); ++i) {
        const GameNode* child = analysisNode->getChild(i);
        if (child) {
            Move move = child->getMove();
            int score = const_cast<GameNode*>(child)->minimax(analysisDepth, false);
            moveAnalysis.push_back({move, score});
        }
    }
    
    // Ordenar por score
    std::sort(moveAnalysis.begin(), moveAnalysis.end(), 
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    std::cout << "\nTop movimientos:\n";
    for (const auto& [move, score] : moveAnalysis) {
        char col = 'A' + move.y;
        int row = move.x + 1;
        std::cout << "  " << col << row << " -> Score: " << score << std::endl;
    }
    
    std::cout << "=== FIN ANÁLISIS ===\n\n";
}

void AI::printSearchStats() const {
    std::cout << "\n=== ESTADÍSTICAS DE BÚSQUEDA ===\n";
    std::cout << "Profundidad configurada: " << maxDepth << std::endl;
    std::cout << "Último tiempo de búsqueda: " << lastSearchTime << "s" << std::endl;
    std::cout << "Nodos explorados: " << nodesExplored << std::endl;
    std::cout << "Evaluaciones realizadas: " << evaluationsPerformed << std::endl;
    
    if (lastSearchTime > 0) {
        double nodesPerSecond = nodesExplored / lastSearchTime;
        double evalsPerSecond = evaluationsPerformed / lastSearchTime;
        std::cout << "Rendimiento: " << static_cast<int>(nodesPerSecond) 
                  << " nodos/s, " << static_cast<int>(evalsPerSecond) << " eval/s" << std::endl;
    }
    
    std::cout << "================================\n\n";
}

void AI::resetStats() {
    nodesExplored = 0;
    evaluationsPerformed = 0;
    lastSearchTime = 0.0;
}