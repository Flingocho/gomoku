/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ai.cpp                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/03 19:23:51 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/03 20:36:16 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/ai.hpp"

int AI::evaluatePosition(const Board &board) const
{
    // 1. Verificar condiciones de victoria inmediata
    if (board.checkWin(aiPlayer)) return STRAIGHTVICTORY;
    if (board.checkWin(humanPlayer)) return -STRAIGHTVICTORY;
    
    // 2. Evaluar patrones para ambos jugadores
    int aiScore = evaluatePlayerPatterns(board, aiPlayer);
    int humanScore = evaluatePlayerPatterns(board, humanPlayer);
    
    return aiScore - humanScore;
}

// Evaluar todos los patrones de un jugador
int AI::evaluatePlayerPatterns(const Board& board, int player) const
{
    int score = 0;
    
    // Direcciones: horizontal, vertical, diagonal \, diagonal /
    int directions[4][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};
    
    for (int i = 0; i < board.getSize(); i++) {
        for (int j = 0; j < board.getSize(); j++) {
            if (board.getPiece(i, j) == player) {
                
                // Evaluar en las 4 direcciones, pero solo desde el inicio de cada línea
                for (int d = 0; d < 4; d++) {
                    int dx = directions[d][0];
                    int dy = directions[d][1];
                    
                    if (isLineStart(board, i, j, dx, dy, player)) {
                        score += checkPatternInDirection(board, i, j, dx, dy, player);
                    }
                }
            }
        }
    }
    
    // TODO: Agregar evaluación de capturas
    // score += evaluateCaptureOpportunities(board, player);
    
    return score;
}

// Verificar si esta posición es el inicio de una línea (evita doble conteo)
bool AI::isLineStart(const Board& board, int x, int y, int dx, int dy, int player) const
{
    int prevX = x - dx;
    int prevY = y - dy;
    
    // Es el inicio si la posición anterior no es válida o no es del mismo jugador
    return !board.isValid(prevX, prevY) || board.getPiece(prevX, prevY) != player;
}

// Detectar patrón en una dirección específica
int AI::checkPatternInDirection(const Board& board, int x, int y, int dx, int dy, int player) const
{
    int consecutive = countConsecutive(board, x, y, dx, dy, player);
    int freeEnds = findFreeEnds(board, x, y, dx, dy, player);
    
    // Clasificar el patrón según tu escala híbrida
    if (consecutive >= 5) {
        return STRAIGHTVICTORY;  // Ya ganó (aunque esto debería detectarse antes)
    }
    
    if (consecutive == 4) {
        if (freeEnds == 2) return FOUR_OPEN;    // _XXXX_ (imparable)
        if (freeEnds == 1) return FOUR_SIMPLE;  // XXXX_ (forzado)
    }
    
    if (consecutive == 3) {
        if (freeEnds == 2) return THREE_OPEN;   // _XXX_ (amenaza fuerte)
        if (freeEnds == 1) return THREE_OPEN / 3; // XXX_ (amenaza menor)
    }
    
    if (consecutive == 2) {
        if (freeEnds == 2) return TWO_OPEN;     // _XX_ (desarrollo)
    }
    
    return 0;  // Patrón no relevante
}

// Contar piezas consecutivas en ambas direcciones
int AI::countConsecutive(const Board& board, int x, int y, int dx, int dy, int player) const
{
    int count = 0;
    
    // Contar hacia una dirección (incluyendo la pieza inicial)
    int nx = x, ny = y;
    while (board.isValid(nx, ny) && board.getPiece(nx, ny) == player) {
        count++;
        nx += dx;
        ny += dy;
    }
    
    // Contar hacia la dirección opuesta (sin contar la pieza inicial otra vez)
    nx = x - dx;
    ny = y - dy;
    while (board.isValid(nx, ny) && board.getPiece(nx, ny) == player) {
        count++;
        nx -= dx;
        ny -= dy;
    }
    
    return count;
}

// Encontrar cuántos extremos están libres
int AI::findFreeEnds(const Board& board, int x, int y, int dx, int dy, int player) const
{
    int freeEnds = 0;
    
    // Buscar extremo hacia adelante
    int nx = x, ny = y;
    while (board.isValid(nx, ny) && board.getPiece(nx, ny) == player) {
        nx += dx; 
        ny += dy;
    }
    if (board.isValid(nx, ny) && board.isEmpty(nx, ny)) {
        freeEnds++;
    }
    
    // Buscar extremo hacia atrás  
    nx = x; 
    ny = y;
    while (board.isValid(nx, ny) && board.getPiece(nx, ny) == player) {
        nx -= dx; 
        ny -= dy;
    }
    if (board.isValid(nx, ny) && board.isEmpty(nx, ny)) {
        freeEnds++;
    }
    
    return freeEnds;
}

bool canCaptureInDirection(const Board& board, int x, int y, int dx, int dy, int player, int opponent) {
    // Patrón: al colocar en (x,y) se debe formar XOOX
    
    // Verificar hacia adelante: (x,y) + OO + X
    int x1 = x + dx, y1 = y + dy;         // Primera O
    int x2 = x + 2*dx, y2 = y + 2*dy;     // Segunda O  
    int x3 = x + 3*dx, y3 = y + 3*dy;     // X que cierra
    
    bool forwardCapture = 
        board.isValid(x1, y1) && board.isValid(x2, y2) && board.isValid(x3, y3) &&
        board.getPiece(x1, y1) == opponent &&
        board.getPiece(x2, y2) == opponent &&
        board.getPiece(x3, y3) == player;
    
    // Verificar hacia atrás: X + OO + (x,y)
    int xb1 = x - dx, yb1 = y - dy;       // Primera O
    int xb2 = x - 2*dx, yb2 = y - 2*dy;   // Segunda O
    int xb3 = x - 3*dx, yb3 = y - 3*dy;   // X que cierra
    
    bool backwardCapture = 
        board.isValid(xb1, yb1) && board.isValid(xb2, yb2) && board.isValid(xb3, yb3) &&
        board.getPiece(xb1, yb1) == opponent &&
        board.getPiece(xb2, yb2) == opponent &&
        board.getPiece(xb3, yb3) == player;
    
    return forwardCapture || backwardCapture;
}
