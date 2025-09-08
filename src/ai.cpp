/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ai.cpp                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/03 19:23:51 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/08 20:58:19 by jainavas         ###   ########.fr       */
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
    
    std::cout << "DEBUG: evaluateCaptureAdvantage for player " << player << " with " << myCaptures << " captures\n";
    
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
    
    std::cout << "DEBUG: Base capture score: " << score << "\n";
    
    // Amenazas de captura inmediatas
    int myOpportunities = countCaptureOpportunities(board, player);
    int opponentOpportunities = countCaptureOpportunities(board, (player == 1) ? 2 : 1);
    
    std::cout << "DEBUG: My opportunities: " << myOpportunities << ", Opponent opportunities: " << opponentOpportunities << "\n";
    
    score += myOpportunities * IMMEDIATE_CAPTURE;
    score -= opponentOpportunities * CAPTURE_THREAT;
    
    std::cout << "DEBUG: Final capture advantage score: " << score << "\n";
    
    return score;
}

int AI::countCaptureOpportunities(const Board& board, int player) const {
    int opportunities = 0;
    int threats = 0;
    int opponent = (player == 1) ? 2 : 1;
    
    std::cout << "DEBUG: Counting capture opportunities for player " << player << " (opponent " << opponent << ")\n";
    
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
                bool foundOpportunity = false;
                bool foundThreat = false;
                
                // Verificar las 8 direcciones para capturas inmediatas (donde player puede capturar)
                for (int d = 0; d < 8; d++) {
                    if (canCaptureInDirection(board, i, j, directions[d][0], directions[d][1], player, opponent)) {
                        std::cout << "DEBUG: Found capture opportunity at (" << i << "," << j << ") direction " << d << "\n";
                        foundOpportunity = true;
                        break; // Una oportunidad por posición es suficiente
                    }
                }
                
                // Si no hay oportunidad inmediata, verificar amenazas (donde colocar ayuda al oponente)
                if (!foundOpportunity) {
                    for (int d = 0; d < 8; d++) {
                        if (isCaptureThreatenDirection(board, i, j, directions[d][0], directions[d][1], player, opponent)) {
                            std::cout << "DEBUG: Found capture threat at (" << i << "," << j << ") direction " << d << "\n";
                            foundThreat = true;
                            break;
                        }
                    }
                }
                
                if (foundOpportunity) opportunities++;
                if (foundThreat) threats++;
            }
        }
    }
    
    std::cout << "DEBUG: Total capture opportunities: " << opportunities << ", threats: " << threats << "\n";
    // Devolver oportunidades menos amenazas (las amenazas son negativas para el jugador)
    return opportunities - threats;
}

bool AI::canCaptureInDirection(const Board& board, int x, int y, int dx, int dy, int player, int opponent) const {
    // Patrón: al colocar en (x,y) se debe formar [MI_PIEZA][ENEMIGO][ENEMIGO][MI_PIEZA]
    // Esto coincide exactamente con getCapturesInDirection del Board
    
    // Verificar hacia adelante: (x,y) + OO + X
    int pos1x = x + dx, pos1y = y + dy;         // Primera O
    int pos2x = x + 2*dx, pos2y = y + 2*dy;     // Segunda O  
    int pos3x = x + 3*dx, pos3y = y + 3*dy;     // X que cierra
    
    bool forwardCapture = 
        board.isValid(pos1x, pos1y) && board.isValid(pos2x, pos2y) && board.isValid(pos3x, pos3y) &&
        board.getPiece(pos1x, pos1y) == opponent &&
        board.getPiece(pos2x, pos2y) == opponent &&
        board.getPiece(pos3x, pos3y) == player;
    
    // Verificar hacia atrás: X + OO + (x,y)
    pos1x = x - dx; pos1y = y - dy;       // Primera O
    pos2x = x - 2*dx; pos2y = y - 2*dy;   // Segunda O
    pos3x = x - 3*dx; pos3y = y - 3*dy;   // X que cierra
    
    bool backwardCapture = 
        board.isValid(pos1x, pos1y) && board.isValid(pos2x, pos2y) && board.isValid(pos3x, pos3y) &&
        board.getPiece(pos1x, pos1y) == opponent &&
        board.getPiece(pos2x, pos2y) == opponent &&
        board.getPiece(pos3x, pos3y) == player;
    
    if (forwardCapture || backwardCapture) {
        std::cout << "DEBUG: Capture pattern found at (" << x << "," << y << ") dir(" << dx << "," << dy << ")\n";
        if (forwardCapture) {
            std::cout << "  Forward: X(" << x << "," << y << ") O(" << pos1x << "," << pos1y 
                      << ") O(" << pos2x << "," << pos2y << ") X(" << pos3x << "," << pos3y << ")\n";
        }
        if (backwardCapture) {
            int bpos1x = x - dx, bpos1y = y - dy;
            int bpos2x = x - 2*dx, bpos2y = y - 2*dy;
            int bpos3x = x - 3*dx, bpos3y = y - 3*dy;
            std::cout << "  Backward: X(" << bpos3x << "," << bpos3y << ") O(" << bpos2x << "," << bpos2y 
                      << ") O(" << bpos1x << "," << bpos1y << ") X(" << x << "," << y << ")\n";
        }
    }
    
    return forwardCapture || backwardCapture;
}

bool AI::isCaptureThreatenDirection(const Board& board, int x, int y, int dx, int dy, int player, int opponent) const {
    // Detectar patrones de amenaza de captura donde si el jugador coloca una pieza,
    // permite al oponente hacer una captura
    
    // Verificar si colocar 'player' en (x,y) crearía un patrón que el oponente puede capturar
    
    // Patrón 1: [opponent][player][(x,y)][opponent] 
    // Si player coloca en (x,y), crea [opponent][player][player][opponent] = captura para opponent
    int prev2X = x - 2*dx, prev2Y = y - 2*dy;     // opponent
    int prevX = x - dx, prevY = y - dy;           // player
    int nextX = x + dx, nextY = y + dy;           // opponent
    
    bool threat1 = 
        board.isValid(prev2X, prev2Y) && board.isValid(prevX, prevY) && board.isValid(nextX, nextY) &&
        board.getPiece(prev2X, prev2Y) == opponent &&
        board.getPiece(prevX, prevY) == player &&
        board.getPiece(nextX, nextY) == opponent;
    
    // Patrón 2: [opponent][(x,y)][player][opponent]
    // Si player coloca en (x,y), crea [opponent][player][player][opponent] = captura para opponent  
    int prev1X = x - dx, prev1Y = y - dy;         // opponent
    int next1X = x + dx, next1Y = y + dy;         // player
    int next2X = x + 2*dx, next2Y = y + 2*dy;     // opponent
    
    bool threat2 = 
        board.isValid(prev1X, prev1Y) && board.isValid(next1X, next1Y) && board.isValid(next2X, next2Y) &&
        board.getPiece(prev1X, prev1Y) == opponent &&
        board.getPiece(next1X, next1Y) == player &&
        board.getPiece(next2X, next2Y) == opponent;
    
    if (threat1 || threat2) {
        std::cout << "DEBUG: Capture threat pattern found at (" << x << "," << y << ") dir(" << dx << "," << dy << ")\n";
        if (threat1) {
            std::cout << "  Threat1: " << opponent << "(" << prev2X << "," << prev2Y 
                      << ") " << player << "(" << prevX << "," << prevY 
                      << ") _(" << x << "," << y << ") " << opponent << "(" << nextX << "," << nextY << ") -> OXXO\n";
        }
        if (threat2) {
            std::cout << "  Threat2: " << opponent << "(" << prev1X << "," << prev1Y 
                      << ") _(" << x << "," << y << ") " << player << "(" << next1X << "," << next1Y 
                      << ") " << opponent << "(" << next2X << "," << next2Y << ") -> OXXO\n";
        }
    }
    
    return threat1 || threat2;
}
