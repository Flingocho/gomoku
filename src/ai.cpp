/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ai.cpp                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/03 19:23:51 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/10 19:05:15 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/ai.hpp"

int AI::evaluatePosition(const Board &board) const
{
	// 1. Verificar condiciones de victoria inmediata
	if (board.checkWin(aiPlayer))
		return STRAIGHTVICTORY;
	if (board.checkWin(humanPlayer))
		return -STRAIGHTVICTORY;

	// 2. Evaluar patrones para ambos jugadores
	int aiScore = evaluatePlayerPatterns(board, aiPlayer);
	int humanScore = evaluatePlayerPatterns(board, humanPlayer);

	return aiScore - humanScore;
}

int AI::evaluatePlayerPatterns(const Board &board, int player) const {
    int score = 0;
    std::set<std::pair<int, int>> capturesAlreadyFound; // Capturas únicas encontradas
    std::set<std::pair<int, int>> threatsAlreadyFound;  // Amenazas únicas encontradas
    
    // Direcciones: horizontal, vertical, diagonal \, diagonal /
    int directions[4][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};
    
    for (int i = 0; i < board.getSize(); i++) {
        for (int j = 0; j < board.getSize(); j++) {
            if (board.getPiece(i, j) == player) {
                
                // Bonificación posicional
                if (abs(i - 9) <= 3 && abs(j - 9) <= 3) {
                    score += CENTRAL_POSITION;
                }
                
                // Evaluar patrones
                for (int d = 0; d < 4; d++) {
                    int dx = directions[d][0];
                    int dy = directions[d][1];
                    
                    if (isLineStart(board, i, j, dx, dy, player)) {
                        int patternScore = checkPatternInDirection(board, i, j, dx, dy, player);
                        
                        if (patternScore == THREE_OPEN) {
                            if (isLegalThreePattern(board, i, j, dx, dy, player)) {
                                score += patternScore;
                            }
                        } else {
                            score += patternScore;
                        }
                    }
                }
                
                // Evaluar capturas únicas
                for (int d = 0; d < 4; d++) {
                    int dx = directions[d][0];
                    int dy = directions[d][1];
                    
                    // Verificar captura ofensiva
                    std::pair<int, int> captureKey = getCaptureKey(board, i, j, dx, dy, player);
                    if (captureKey.first != -1 && capturesAlreadyFound.find(captureKey) == capturesAlreadyFound.end()) {
                        capturesAlreadyFound.insert(captureKey);
                        score += evaluateCapturesInDirection(board, i, j, dx, dy, player);
                    }
                    
                    // Verificar amenaza defensiva (similar lógica)
                    std::pair<int, int> threatKey = getThreatKey(board, i, j, dx, dy, player);
                    if (threatKey.first != -1 && threatsAlreadyFound.find(threatKey) == threatsAlreadyFound.end()) {
                        threatsAlreadyFound.insert(threatKey);
                        score -= CAPTURE_THREAT; // Penalizar amenaza
                    }
                }
            }
        }
    }
    
    return score;
}

std::pair<int, int> AI::getThreatKey(const Board& board, int x, int y, int dx, int dy, int player) const {
    int opponent = (player == 1) ? 2 : 1;
    
    // Patrón amenaza: O + X(x,y) + _ + O
    int emptyX = x + dx, emptyY = y + dy;
    int opp1X = x - dx, opp1Y = y - dy;
    int opp2X = x + 2*dx, opp2Y = y + 2*dy;
    
    if (board.isValid(opp1X, opp1Y) && board.isValid(emptyX, emptyY) && board.isValid(opp2X, opp2Y) &&
        board.getPiece(opp1X, opp1Y) == opponent &&
        board.isEmpty(emptyX, emptyY) &&
        board.getPiece(opp2X, opp2Y) == opponent) {
        
        return {emptyX, emptyY}; // Posición peligrosa
    }
    
    // Patrón amenaza: O + _ + X(x,y) + O
    emptyX = x - dx; emptyY = y - dy;
    opp1X = x - 2*dx; opp1Y = y - 2*dy;
    opp2X = x + dx; opp2Y = y + dy;
    
    if (board.isValid(opp1X, opp1Y) && board.isValid(emptyX, emptyY) && board.isValid(opp2X, opp2Y) &&
        board.getPiece(opp1X, opp1Y) == opponent &&
        board.isEmpty(emptyX, emptyY) &&
        board.getPiece(opp2X, opp2Y) == opponent) {
        
        return {emptyX, emptyY};
    }
    
    return {-1, -1}; // No hay amenaza
}

std::pair<int, int> AI::getCaptureKey(const Board& board, int x, int y, int dx, int dy, int player) const {
    int opponent = (player == 1) ? 2 : 1;
    
    // Verificar patrón hacia adelante: X(x,y) + O + O + _
    int opp1X = x + dx, opp1Y = y + dy;
    int opp2X = x + 2*dx, opp2Y = y + 2*dy;
    int emptyX = x + 3*dx, emptyY = y + 3*dy;
    
    if (board.isValid(opp1X, opp1Y) && board.isValid(opp2X, opp2Y) && board.isValid(emptyX, emptyY) &&
        board.getPiece(opp1X, opp1Y) == opponent &&
        board.getPiece(opp2X, opp2Y) == opponent &&
        board.isEmpty(emptyX, emptyY)) {
        
        // Devolver posición donde se puede capturar (única para esta captura)
        return {emptyX, emptyY};
    }
    
    // Verificar patrón hacia atrás: _ + O + O + X(x,y)
    emptyX = x - 3*dx; emptyY = y - 3*dy;
    opp1X = x - 2*dx; opp1Y = y - 2*dy;
    opp2X = x - dx; opp2Y = y - dy;
    
    if (board.isValid(emptyX, emptyY) && board.isValid(opp1X, opp1Y) && board.isValid(opp2X, opp2Y) &&
        board.isEmpty(emptyX, emptyY) &&
        board.getPiece(opp1X, opp1Y) == opponent &&
        board.getPiece(opp2X, opp2Y) == opponent) {
        
        return {emptyX, emptyY};
    }
    
    return {-1, -1}; // No hay captura
}

bool AI::isLegalThreePattern(const Board &board, int x, int y, int dx, int dy, int player) const
{
	// Encontrar los extremos del patrón de 3
	int startX = x, startY = y;
	int endX = x, endY = y;

	// Buscar el inicio real del patrón
	while (board.isValid(startX - dx, startY - dy) && board.getPiece(startX - dx, startY - dy) == player)
	{
		startX -= dx;
		startY -= dy;
	}

	// Buscar el final real del patrón
	while (board.isValid(endX + dx, endY + dy) && board.getPiece(endX + dx, endY + dy) == player)
	{
		endX += dx;
		endY += dy;
	}

	// Posiciones donde se podría completar el three
	int beforeX = startX - dx, beforeY = startY - dy;
	int afterX = endX + dx, afterY = endY + dy;

	bool canPlayBefore = board.isValid(beforeX, beforeY) && board.isEmpty(beforeX, beforeY);
	bool canPlayAfter = board.isValid(afterX, afterY) && board.isEmpty(afterX, afterY);

	// Verificar si colocar en cualquier extremo crearía double-three
	if (canPlayBefore && board.isDoubleFree(beforeX, beforeY, player))
	{
		canPlayBefore = false; // Esta jugada sería ilegal
	}

	if (canPlayAfter && board.isDoubleFree(afterX, afterY, player))
	{
		canPlayAfter = false; // Esta jugada sería ilegal
	}

	// El patrón solo es valioso si al menos un extremo es jugable legalmente
	return canPlayBefore || canPlayAfter;
}

// Verificar si esta posición es el inicio de una línea (evita doble conteo)
bool AI::isLineStart(const Board &board, int x, int y, int dx, int dy, int player) const
{
	int prevX = x - dx;
	int prevY = y - dy;

	// Es el inicio si la posición anterior no es válida o no es del mismo jugador
	return !board.isValid(prevX, prevY) || board.getPiece(prevX, prevY) != player;
}

// Detectar patrón en una dirección específica
int AI::checkPatternInDirection(const Board &board, int x, int y, int dx, int dy, int player) const
{
	int consecutive = countConsecutive(board, x, y, dx, dy, player);
	int withgaps = countGaps(board, x, y, dx, dy, player);
	int freeEnds = findFreeEnds(board, x, y, dx, dy, player);

	// 1. Patrones consecutivos
	if (consecutive >= 5)
		return STRAIGHTVICTORY;

	if (consecutive == 4)
	{
		if (freeEnds == 2)
			return FOUR_OPEN;
		if (freeEnds == 1)
			return FOUR_SIMPLE;
	}

	// 2. CORREGIDO: Patrones críticos con gaps
	if (withgaps == 4 && consecutive < 4)
	{						// Como OO_OO
		return FOUR_SIMPLE; // Una jugada para ganar
	}

	if (withgaps == 3 && consecutive < 3)
	{ // Como OO_O o O_OO
		if (freeEnds == 2)
			return THREE_OPEN; // _OO_O_
		if (freeEnds == 1)
			return THREE_SIMPLE; // XOO_O_
	}

	// 3. Resto de patrones consecutivos
	if (consecutive == 3)
	{
		if (freeEnds == 2)
			return THREE_OPEN;
		if (freeEnds == 1)
			return THREE_SIMPLE;
	}

	if (consecutive == 2)
	{
		if (freeEnds == 2)
			return TWO_OPEN;
	}

	return 0;
}

// Contar piezas consecutivas en ambas direcciones
int AI::countConsecutive(const Board &board, int x, int y, int dx, int dy, int player) const
{
	int count = 0;

	// Contar hacia una dirección (incluyendo la pieza inicial)
	int nx = x, ny = y;
	while (board.isValid(nx, ny) && board.getPiece(nx, ny) == player)
	{
		count++;
		nx += dx;
		ny += dy;
	}

	// Contar hacia la dirección opuesta (sin contar la pieza inicial otra vez)
	nx = x - dx;
	ny = y - dy;
	while (board.isValid(nx, ny) && board.getPiece(nx, ny) == player)
	{
		count++;
		nx -= dx;
		ny -= dy;
	}

	return count;
}

// Encontrar cuántos extremos están libres
int AI::findFreeEnds(const Board &board, int x, int y, int dx, int dy, int player) const
{
	int freeEnds = 0;

	// Buscar extremo hacia adelante
	int nx = x, ny = y;
	while (board.isValid(nx, ny) && board.getPiece(nx, ny) == player)
	{
		nx += dx;
		ny += dy;
	}
	if (board.isValid(nx, ny) && board.isEmpty(nx, ny))
	{
		freeEnds++;
	}

	// Buscar extremo hacia atrás
	nx = x;
	ny = y;
	while (board.isValid(nx, ny) && board.getPiece(nx, ny) == player)
	{
		nx -= dx;
		ny -= dy;
	}
	if (board.isValid(nx, ny) && board.isEmpty(nx, ny))
	{
		freeEnds++;
	}

	return freeEnds;
}

int AI::evaluateCapturesInDirection(const Board &board, int x, int y, int dx, int dy, int player) const
{
	int score = 0;
	int myCaptures = board.getCaptures(player);
	int OppCaptures = board.getCaptures((player == 1) ? 2 : 1);

	// Amenazas de captura inmediatas
	int myOpportunities = canCaptureInDirection(board, x, y, dx, dy, player, (player == 1) ? 2 : 1);
	int opponentOpportunities = isCaptureThreatenDirection(board, x, y, dx, dy, player, (player == 1) ? 2 : 1);

	// Solo aplicar bonificación por capturas existentes si hay actividad de captura relevante
	bool captureRelevant = (myOpportunities != 0) || (opponentOpportunities != 0);

	if (captureRelevant)
	{
		// Según cercanía a las 10 capturas (5 pares) - solo cuando hay actividad de captura
		if (myCaptures >= 8 && myOpportunities > 0)
		{					// 1 par para ganar - MUY GRAVE pero defendible
			score += 20000; // Entre DOUBLE_THREE_OPEN (25k) y FOUR_SIMPLE (10k)
		}
		else if (myCaptures >= 6 && myOpportunities > 0)
		{				   // 2 pares para ganar - Grave
			score += 8000; // Un poco menos que FOUR_SIMPLE
		}
		else if (myCaptures >= 4 && myOpportunities > 0)
		{				   // 3 pares para ganar - Preocupante
			score += 3000; // Más que IMMEDIATE_CAPTURE
		}
		else
		{
			score += myCaptures * 200; // Progreso gradual
		}

		score += myOpportunities * IMMEDIATE_CAPTURE;

		if (OppCaptures >= 8 && opponentOpportunities > 0)
		{					// 1 par para ganar - MUY GRAVE pero defendible
			score -= 20000; // Entre DOUBLE_THREE_OPEN (25k) y FOUR_SIMPLE (10k)
		}
		else if (OppCaptures >= 6 && opponentOpportunities > 0)
		{				   // 2 pares para ganar - Grave
			score -= 8000; // Un poco menos que FOUR_SIMPLE
		}
		else if (OppCaptures >= 4 && opponentOpportunities > 0)
		{				   // 3 pares para ganar - Preocupante
			score -= 3000; // Más que IMMEDIATE_CAPTURE
		}
		else
		{
			score -= OppCaptures * 200; // Progreso gradual
		}

		score -= opponentOpportunities * CAPTURE_THREAT;
		
	}

	return score;
}

bool AI::canCaptureInDirection(const Board &board, int x, int y, int dx, int dy, int player, int opponent) const
{
	// Empezamos desde una pieza nuestra X en (x,y)
	// Buscamos patrones: X(x,y) + 00 + _ para capturar hacia adelante
	if (player < -1)
		return 0;
	// Patrón hacia adelante: X(x,y) + O + O + _
	int opp1X = x + dx, opp1Y = y + dy;			  // Primera O
	int opp2X = x + 2 * dx, opp2Y = y + 2 * dy;	  // Segunda O
	int emptyX = x + 3 * dx, emptyY = y + 3 * dy; // Donde podemos jugar

	bool forwardPattern =
		board.isValid(opp1X, opp1Y) && board.isValid(opp2X, opp2Y) && board.isValid(emptyX, emptyY) &&
		board.getPiece(opp1X, opp1Y) == opponent &&
		board.getPiece(opp2X, opp2Y) == opponent &&
		board.isEmpty(emptyX, emptyY);

	// Patrón hacia atrás: _ + O + O + X(x,y)
	emptyX = x - 3 * dx;
	emptyY = y - 3 * dy; // Donde podemos jugar
	opp1X = x - 2 * dx;
	opp1Y = y - 2 * dy; // Primera O
	opp2X = x - dx;
	opp2Y = y - dy; // Segunda O

	bool backwardPattern =
		board.isValid(emptyX, emptyY) && board.isValid(opp1X, opp1Y) && board.isValid(opp2X, opp2Y) &&
		board.isEmpty(emptyX, emptyY) &&
		board.getPiece(opp1X, opp1Y) == opponent &&
		board.getPiece(opp2X, opp2Y) == opponent;

	return forwardPattern || backwardPattern;
}

bool AI::isCaptureThreatenDirection(const Board &board, int x, int y, int dx, int dy, int player, int opponent) const
{
	// Empezamos desde una pieza nuestra X en (x,y)
	// Buscamos patrones donde estamos vulnerables a captura: O-X(x,y)-_-O o O-_-X(x,y)-O
	if (player < -1)
		return 0;
	// Patrón 1: O + X(x,y) + _ + O
	// Si colocamos otra pieza nuestra en _, nos capturan
	int emptyX = x + dx, emptyY = y + dy;		// Donde podríamos jugar (peligroso)
	int opp1X = x - dx, opp1Y = y - dy;			// Opponent antes
	int opp2X = x + 2 * dx, opp2Y = y + 2 * dy; // Opponent después

	bool threat1 =
		board.isValid(opp1X, opp1Y) && board.isValid(emptyX, emptyY) && board.isValid(opp2X, opp2Y) &&
		board.getPiece(opp1X, opp1Y) == opponent &&
		board.isEmpty(emptyX, emptyY) &&
		board.getPiece(opp2X, opp2Y) == opponent;

	// Patrón 2: O + _ + X(x,y) + O
	// Si colocamos otra pieza nuestra en _, nos capturan
	emptyX = x - dx;
	emptyY = y - dy; // Donde podríamos jugar (peligroso)
	opp1X = x - 2 * dx;
	opp1Y = y - 2 * dy; // Opponent antes
	opp2X = x + dx;
	opp2Y = y + dy; // Opponent después

	bool threat2 =
		board.isValid(opp1X, opp1Y) && board.isValid(emptyX, emptyY) && board.isValid(opp2X, opp2Y) &&
		board.getPiece(opp1X, opp1Y) == opponent &&
		board.isEmpty(emptyX, emptyY) &&
		board.getPiece(opp2X, opp2Y) == opponent;

	return threat1 || threat2;
}

int AI::countGaps(const Board &board, int x, int y, int dx, int dy, int player) const
{
	int count = 0;

	// Contar hacia adelante (incluyendo la pieza inicial)
	int nx = x, ny = y;
	while (board.isValid(nx, ny) && board.getPiece(nx, ny) == player)
	{
		count++;
		nx += dx;
		ny += dy;
	}

	// Si hay UN gap seguido de más piezas nuestras, contarlas
	if (board.isValid(nx, ny) && board.getPiece(nx, ny) == 0)
	{
		nx += dx;
		ny += dy; // Saltar el gap
		while (board.isValid(nx, ny) && board.getPiece(nx, ny) == player)
		{
			count++;
			nx += dx;
			ny += dy;
		}
	}

	// Contar hacia atrás (sin la pieza inicial)
	nx = x - dx;
	ny = y - dy;
	while (board.isValid(nx, ny) && board.getPiece(nx, ny) == player)
	{
		count++;
		nx -= dx;
		ny -= dy;
	}

	// Gap hacia atrás también
	if (board.isValid(nx, ny) && board.getPiece(nx, ny) == 0)
	{
		nx -= dx;
		ny -= dy;
		while (board.isValid(nx, ny) && board.getPiece(nx, ny) == player)
		{
			count++;
			nx -= dx;
			ny -= dy;
		}
	}

	return count; // Sin restar nada
}

Move AI::getBestMoveWithTree(Board &board)
{
	// Crear nodo raíz del árbol
	GameNode root(board, aiPlayer, this);

	// Buscar mejor movimiento con profundidad configurable
	Move bestMove = root.getBestMove(maxDepth);

	return bestMove;
}
