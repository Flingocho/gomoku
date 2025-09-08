/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ai.cpp                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/03 19:23:51 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/08 20:17:39 by jainavas         ###   ########.fr       */
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
    std::cout << "Ai score only patterns: " << aiScore << " humanScore only patterns: " << humanScore << "\n";
	aiScore += evaluateCaptureAdvantage(board, aiPlayer);
    humanScore += evaluateCaptureAdvantage(board, humanPlayer);
    std::cout << "Ai score: " << aiScore << " humanScore: " << humanScore << "\n";

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
                
                for (int d = 0; d < 4; d++) {
                    int dx = directions[d][0];
                    int dy = directions[d][1];
                    
                    if (isLineStart(board, i, j, dx, dy, player)) {
                        int patternScore = checkPatternInDirection(board, i, j, dx, dy, player);
                        
                        // NUEVO: Si es THREE_OPEN, verificar que sea legal
                        if (patternScore == THREE_OPEN) {
                            if (isLegalThreePattern(board, i, j, dx, dy, player)) {
                                score += patternScore;
                            }
                            // Si no es legal, no sumar puntos por este patrón
                        } else {
                            score += patternScore;
                        }
                    }
                }
            }
        }
    }
    
    return score;
}

bool AI::isLegalThreePattern(const Board& board, int x, int y, int dx, int dy, int player) const {
    // Encontrar los extremos del patrón de 3
    int startX = x, startY = y;
    int endX = x, endY = y;
    
    // Buscar el inicio real del patrón
    while (board.isValid(startX - dx, startY - dy) && board.getPiece(startX - dx, startY - dy) == player) {
        startX -= dx;
        startY -= dy;
    }
    
    // Buscar el final real del patrón  
    while (board.isValid(endX + dx, endY + dy) && board.getPiece(endX + dx, endY + dy) == player) {
        endX += dx;
        endY += dy;
    }
    
    // Posiciones donde se podría completar el three
    int beforeX = startX - dx, beforeY = startY - dy;
    int afterX = endX + dx, afterY = endY + dy;
    
    bool canPlayBefore = board.isValid(beforeX, beforeY) && board.isEmpty(beforeX, beforeY);
    bool canPlayAfter = board.isValid(afterX, afterY) && board.isEmpty(afterX, afterY);
    
    // Verificar si colocar en cualquier extremo crearía double-three
    if (canPlayBefore && board.isDoubleFree(beforeX, beforeY, player)) {
        canPlayBefore = false; // Esta jugada sería ilegal
    }
    
    if (canPlayAfter && board.isDoubleFree(afterX, afterY, player)) {
        canPlayAfter = false; // Esta jugada sería ilegal
    }
    
    // El patrón solo es valioso si al menos un extremo es jugable legalmente
    return canPlayBefore || canPlayAfter;
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

int AI::evaluateCaptureAdvantage(const Board& board, int player) const {
    int score = 0;
    int myCaptures = board.getCaptures(player);
    
    // Según cercanía a las 10 capturas (5 pares)
    if (myCaptures >= 8) {        // 1 par para ganar - MUY GRAVE pero defendible
        score += 20000;           // Entre DOUBLE_THREE_OPEN (25k) y FOUR_SIMPLE (10k)
    }
    else if (myCaptures >= 6) {   // 2 pares para ganar - Grave
        score += 8000;            // Un poco menos que FOUR_SIMPLE
    }
    else if (myCaptures >= 4) {   // 3 pares para ganar - Preocupante
        score += 3000;            // Más que IMMEDIATE_CAPTURE
    }
    else {
        score += myCaptures * 200; // Progreso gradual
    }
    
    // Amenazas de captura inmediatas
    score += countCaptureOpportunities(board, player) * IMMEDIATE_CAPTURE;
    
    return score;
}

int AI::countCaptureOpportunities(const Board& board, int player) const {
    int opportunities = 0;
    int opponent = (player == 1) ? 2 : 1;
    
    // 8 direcciones
    int directions[8][2] = {
        {-1, -1}, {-1, 0}, {-1, 1},
        {0, -1},           {0, 1},
        {1, -1},  {1, 0},  {1, 1}
    };
    
    // Recorrer todas las posiciones vacías
    for (int i = 0; i < board.getSize(); i++) {
        for (int j = 0; j < board.getSize(); j++) {
            if (board.isEmpty(i, j)) {
                // Verificar las 8 direcciones
                for (int d = 0; d < 8; d++) {
                    if (canCaptureInDirection(board, i, j, directions[d][0], directions[d][1], player, opponent)) {
                        opportunities++;
                        break; // Una oportunidad por posición es suficiente
                    }
                }
            }
        }
    }
    
    return opportunities;
}

bool AI::canCaptureInDirection(const Board& board, int x, int y, int dx, int dy, int player, int opponent) const {
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
