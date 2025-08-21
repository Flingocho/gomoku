/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ai.hpp                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/21 16:35:45 by jainavas          #+#    #+#             */
/*   Updated: 2025/08/21 16:37:21 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef AI_HPP
#define AI_HPP

#include "board.hpp"
#include "game_node.hpp"
#include <memory>

class AI {
private:
    int aiPlayer;        // 1 o 2
    int humanPlayer;     // El oponente
    int maxDepth;        // Profundidad de búsqueda
    
    // Para debugging y estadísticas
    mutable int nodesExplored;
    mutable int evaluationsPerformed;
    mutable double lastSearchTime;

public:
    AI(int player, int depth = 6);
    
    // === MÉTODO PRINCIPAL ===
    
    // Encuentra el mejor movimiento usando el árbol de juego
    Move getBestMove(Board& board);
    
    // === CONFIGURACIÓN ===
    
    void setDepth(int depth) { maxDepth = depth; }
    int getDepth() const { return maxDepth; }
    
    // === ESTADÍSTICAS ===
    
    int getNodesExplored() const { return nodesExplored; }
    int getEvaluationsPerformed() const { return evaluationsPerformed; }
    double getLastSearchTime() const { return lastSearchTime; }
    
    // === DEBUG ===
    
    void printSearchStats() const;
    
    // Analiza una posición específica y muestra información detallada
    void analyzePosition(const Board& board, int analysisDepth = 4);

private:
    // === MÉTODOS AUXILIARES ===
    
    // Detecta movimientos que ganan inmediatamente
    Move findImmediateWin(const Board& board, int player);
    
    // Detecta movimientos que bloquean una derrota inmediata  
    Move findCriticalBlock(const Board& board, int player);
    
    // Resetea estadísticas para una nueva búsqueda
    void resetStats();
};

#endif