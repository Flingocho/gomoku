/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ai.hpp                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/21 16:35:45 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/10 19:03:12 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef AI_HPP
#define AI_HPP

#include "board.hpp"
#include "game_node.hpp"
#include <vector>
#include <set>
#include <utility>
#include <string>
#include <iostream>
#include <cstdint>

// Condiciones de fin de juego
#define STRAIGHTVICTORY 100000      // Victoria inmediata: ±100000

// Amenazas críticas  
#define FOUR_OPEN 50000             // Cuatro abierto (_XXXX_): ±50000 - "no defendible"
#define DOUBLE_THREE_OPEN 25000     // Doble tres abierto: ±25000 - "no defendible" también

// Amenazas forzadas
#define FOUR_SIMPLE 10000           // Cuatro simple (XXXX_): ±10000 - "amenaza forzada"
#define THREE_OPEN 5000             // Tres abierto (_XXX_): ±5000 - "tres abierto" 

// Capturas y desarrollos
#define THREE_SIMPLE 1500           // Tres simple (XXX_ o OXXXO): ±1500
#define IMMEDIATE_CAPTURE 2000      // Captura inmediata: ±2000
#define CAPTURE_THREAT 500          // Amenaza de captura: ±500  
#define TWO_OPEN 100                // Dos abierto: ±100

// Posicionales (opcionales)
#define CENTRAL_POSITION 20         // Posición central: ±20 - menos crítico
#define CONNECTION 10               // Conexión: ±10

// Clase AI simplificada - lista para nuevo algoritmo
class AI {
private:
    int aiPlayer;
    int humanPlayer;
    int maxDepth;
    
    // Funciones de evaluación de patrones
    int evaluatePlayerPatterns(const Board& board, int player) const;
    bool isLineStart(const Board& board, int x, int y, int dx, int dy, int player) const;
    int checkPatternInDirection(const Board& board, int x, int y, int dx, int dy, int player) const;
    int countConsecutive(const Board& board, int x, int y, int dx, int dy, int player) const;
    int findFreeEnds(const Board& board, int x, int y, int dx, int dy, int player) const;
	bool canCaptureInDirection(const Board& board, int x, int y, int dx, int dy, int player, int opponent) const;
	bool isCaptureThreatenDirection(const Board& board, int x, int y, int dx, int dy, int player, int opponent) const;
    int evaluateCapturesInDirection(const Board& board, int x, int y, int dx, int dy, int player) const;
	bool isLegalThreePattern(const Board& board, int x, int y, int dx, int dy, int player) const;
	int countGaps(const Board& board, int x, int y, int dx, int dy, int player) const;
	std::pair<int, int> getCaptureKey(const Board& board, int x, int y, int dx, int dy, int player) const;
	std::pair<int, int> getThreatKey(const Board& board, int x, int y, int dx, int dy, int player) const;

public:
    AI(int aiplayer, int humanplayer, int depth) {aiPlayer = aiplayer, humanPlayer = humanplayer, maxDepth = depth;}
    
    // Función principal - obtener mejor movimiento
    Move getBestMoveWithTree(Board& board);
    
    // Función de evaluación principal (pública si se necesita para depuración)
    int evaluatePosition(const Board& board) const;
};

#endif