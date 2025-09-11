/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ai.cpp                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/03 19:23:51 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/11 22:38:36 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/ai.hpp"
#include <iostream>

int AI::evaluatePosition(const Board &board) const
{
	int aiScore = evaluatePlayerPatterns(board, aiPlayer);
	int humanScore = evaluatePlayerPatterns(board, humanPlayer);

	return aiScore - humanScore;
}

// VERSIÓN OPTIMIZADA: Solo evalúa patrones relevantes cerca del último movimiento
int AI::evaluatePlayerPatterns(const Board &board, int player) const {
    int score = 0;
    std::set<std::pair<int, int>> capturesAlreadyFound;
    std::set<std::pair<int, int>> threatsAlreadyFound;
    
    int directions[4][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};
    
    // OPTIMIZACIÓN 1: Solo evaluar piezas en área relevante (radio 3 del último movimiento)
    std::vector<std::pair<int, int>> relevantPieces = getRelevantPieces(board, player);
    
    for (const auto& piece : relevantPieces) {
        int i = piece.first;
        int j = piece.second;
        
        // OPTIMIZACIÓN 2: Early continue si no hay piezas cercanas
        if (!hasNearbyActivity(board, i, j, player)) continue;
        
        // Bonificación posicional (solo para centro)
        if (abs(i - 9) <= 3 && abs(j - 9) <= 3) {
            score += CENTRAL_POSITION;
        }
        
        // OPTIMIZACIÓN 3: Evaluar patrones solo en direcciones prometedoras
        for (int d = 0; d < 4; d++) {
            int dx = directions[d][0];
            int dy = directions[d][1];
            
            // Quick check: ¿hay alguna pieza en esta dirección?
            if (!hasLineActivity(board, i, j, dx, dy, player)) continue;
            
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
            
            // OPTIMIZACIÓN 4: Evaluar capturas solo si hay oponentes cercanos
            if (hasOpponentNearby(board, i, j, dx, dy, player)) {
                std::pair<int, int> captureKey = getCaptureKey(board, i, j, dx, dy, player);
                if (captureKey.first != -1 && capturesAlreadyFound.find(captureKey) == capturesAlreadyFound.end()) {
                    capturesAlreadyFound.insert(captureKey);
                    score += evaluateCapturesInDirection(board, i, j, dx, dy, player);
                }
                
                std::pair<int, int> threatKey = getThreatKey(board, i, j, dx, dy, player);
                if (threatKey.first != -1 && threatsAlreadyFound.find(threatKey) == threatsAlreadyFound.end()) {
                    threatsAlreadyFound.insert(threatKey);
                    score -= CAPTURE_THREAT;
                }
            }
        }
    }
    
    return score;
}

// FUNCIONES AUXILIARES PARA OPTIMIZACIÓN

std::vector<std::pair<int, int>> AI::getRelevantPieces(const Board& board, int player) const {
    std::vector<std::pair<int, int>> pieces;
    
    // OPTIMIZACIÓN: Usar turns en lugar de recorrer el tablero
    int totalPieces = board.getTurns();
    
    if (totalPieces < 20) {
        // Early game: evaluar todas las piezas del jugador
        for (int i = 0; i < board.getSize(); i++) {
            for (int j = 0; j < board.getSize(); j++) {
                if (board.getPiece(i, j) == player) {
                    pieces.push_back({i, j});
                }
            }
        }
    } else {
        // Mid/late game: solo piezas con actividad reciente (radio del centro de acción)
        std::pair<int, int> center = findCenterOfActivity(board);
        int radius = 6; // Radio adaptable
        
        for (int i = std::max(0, center.first - radius); 
             i < std::min(board.getSize(), center.first + radius + 1); i++) {
            for (int j = std::max(0, center.second - radius); 
                 j < std::min(board.getSize(), center.second + radius + 1); j++) {
                if (board.getPiece(i, j) == player) {
                    pieces.push_back({i, j});
                }
            }
        }
    }
    
    return pieces;
}

bool AI::hasNearbyActivity(const Board& board, int x, int y, int player) const {
    // Quick check: ¿hay piezas en radio 2 que justifiquen evaluación completa?
    int opponent = (player == 1) ? 2 : 1;
    
    for (int dx = -2; dx <= 2; dx++) {
        for (int dy = -2; dy <= 2; dy++) {
            int nx = x + dx, ny = y + dy;
            if (board.isValid(nx, ny)) {
                int piece = board.getPiece(nx, ny);
                if (piece == player || piece == opponent) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool AI::hasLineActivity(const Board& board, int x, int y, int dx, int dy, int player) const {
    // Quick check: ¿hay piezas en esta dirección en las próximas 4 posiciones?
	if (player < -1)
		return 0;
    for (int i = 1; i <= 4; i++) {
        int nx = x + i * dx, ny = y + i * dy;
        if (board.isValid(nx, ny) && board.getPiece(nx, ny) != 0) {
            return true;
        }
        nx = x - i * dx; ny = y - i * dy;
        if (board.isValid(nx, ny) && board.getPiece(nx, ny) != 0) {
            return true;
        }
    }
    return false;
}

bool AI::hasOpponentNearby(const Board& board, int x, int y, int dx, int dy, int player) const {
    int opponent = (player == 1) ? 2 : 1;
    
    // Check 3 posiciones en la dirección para capturas
    for (int i = 1; i <= 3; i++) {
        int nx = x + i * dx, ny = y + i * dy;
        if (board.isValid(nx, ny) && board.getPiece(nx, ny) == opponent) {
            return true;
        }
        nx = x - i * dx; ny = y - i * dy;
        if (board.isValid(nx, ny) && board.getPiece(nx, ny) == opponent) {
            return true;
        }
    }
    return false;
}

std::pair<int, int> AI::findCenterOfActivity(const Board& board) const {
    int sumX = 0, sumY = 0, count = 0;
    
    for (int i = 0; i < board.getSize(); i++) {
        for (int j = 0; j < board.getSize(); j++) {
            if (board.getPiece(i, j) != 0) {
                sumX += i;
                sumY += j;
                count++;
            }
        }
    }
    
    if (count == 0) return {9, 9}; // Centro del tablero
    return {sumX / count, sumY / count};
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
			score += 25000; // Entre DOUBLE_THREE_OPEN (25k) y FOUR_SIMPLE (10k)
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
			score -= 25000; // Entre DOUBLE_THREE_OPEN (25k) y FOUR_SIMPLE (10k)
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
	// Patrón hacia adelante: X(x,y) + O + O + _
	int opp1X = x + dx, opp1Y = y + dy;			  // Primera O
	int opp2X = x + 2 * dx, opp2Y = y + 2 * dy;	  // Segunda O
	int emptyX = x + 3 * dx, emptyY = y + 3 * dy; // Donde podemos jugar

	if (player < -1)
		return 0;

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

	if (player < -1)
		return 0;
		
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

Move AI::getBestMoveWithTree(Board &board) {
    // Si no tenemos raíz, crear una nueva
    if (root == nullptr) {
        root = new GameNode(board, aiPlayer, this);
        std::cout << "DEBUG: Created initial root in getBestMoveWithTree" << std::endl;
    }

    // Verificación opcional: ¿el root está sincronizado?
    if (rootNeedsSync(board)) {
        std::cout << "DEBUG: Root desincronizado, recreando en getBestMoveWithTree" << std::endl;
        delete root;
        root = new GameNode(board, aiPlayer, this);
    } else {
        std::cout << "DEBUG: Root correctamente sincronizado" << std::endl;
    }

    // Buscar mejor movimiento con profundidad configurable
    Move bestMove = root->getBestMove(maxDepth);
    
    // Después de elegir el movimiento, actualizar la raíz al nodo correspondiente
    if (bestMove.isValid()) {
        moveToChild(bestMove);
    }

    return bestMove;
}

void AI::moveToChild(const Move& move) {
    if (!root) return;
    
    // Buscar el hijo que corresponde al movimiento de la IA
    GameNode* childNode = root->findChild(move);
    
    if (childNode) {
        std::cout << "DEBUG: Navegación exitosa en moveToChild" << std::endl;
        // Promover hijo a nueva raíz
        Board newBoard = childNode->getBoard();
        int nextPlayer = humanPlayer; // Después de IA, turno del humano
        
        delete root;
        root = new GameNode(newBoard, nextPlayer, this);
    } else {
        std::cout << "DEBUG: No se encontró hijo en moveToChild, recreando" << std::endl;
        // Recrear manualmente
        Board newBoard = root->getBoard();
        if (newBoard.placePiece(move.x, move.y, aiPlayer)) {
            delete root;
            root = new GameNode(newBoard, humanPlayer, this);
        }
    }
}

void AI::updatePlayerTurn(const Move& playerMove) {
    if (!root) {
        std::cerr << "Warning: updatePlayerTurn called but no root exists!" << std::endl;
        return;
    }
    
    // Generar hijos si no existen para buscar el movimiento del jugador
    if (root->getChildrenCount() == 0) {
        root->generateChildren();
        std::cout << "DEBUG: Generated children for navigation" << std::endl;
    }
    
    // Buscar el hijo correspondiente al movimiento del humano
    GameNode* child = root->findChild(playerMove);
    if (child) {
        std::cout << "DEBUG: Navegación exitosa en updatePlayerTurn" << std::endl;
        // Promover hijo a nueva raíz
        Board childBoard = child->getBoard();
        int nextPlayer = aiPlayer; // Después del humano, turno de IA
        
        delete root;
        root = new GameNode(childBoard, nextPlayer, this);
        return;
    }
    
    // Si no encontramos el hijo, recrear (pero esto debería ser raro)
    std::cout << "DEBUG: Recreación forzada en updatePlayerTurn - movimiento no encontrado" << std::endl;
    Board newBoard = root->getBoard();
    if (newBoard.placePiece(playerMove.x, playerMove.y, humanPlayer)) {
        delete root;
        root = new GameNode(newBoard, aiPlayer, this);
    }
}

bool AI::rootNeedsSync(const Board& currentBoard) const {
    if (!root) return true;
    
    const Board& rootBoard = root->getBoard();
    
    // Verificación rápida: turnos y capturas
    if (rootBoard.getTurns() != currentBoard.getTurns()) {
        return true;
    }
    
    if (rootBoard.getCaptures(1) != currentBoard.getCaptures(1) ||
        rootBoard.getCaptures(2) != currentBoard.getCaptures(2)) {
        return true;
    }
    
    // Si pasan las verificaciones básicas, asumir que está sincronizado
    // (Una verificación completa sería muy costosa)
    return false;
}

bool AI::rootIsCompletelyWrong(const Board& board) const {
    const Board& rootBoard = root->getBoard();
    
    // Diferencia masiva de turnos
    if (abs(rootBoard.getTurns() - board.getTurns()) > 5) {
        return true;
    }
    
    // Verificar algunas posiciones clave (muestreo)
    int differences = 0;
    for (int i = 0; i < Board::getSize(); i += 3) { // Muestreo cada 3 posiciones
        for (int j = 0; j < Board::getSize(); j += 3) {
            if (rootBoard.getPiece(i, j) != board.getPiece(i, j)) {
                differences++;
                if (differences > 3) return true; // Demasiadas diferencias
            }
        }
    }
    
    return false;
}

// REEMPLAZAR updateTree() en ai.cpp
void AI::updateTree(const Board& board) {
    if (!root) {
        root = new GameNode(board, aiPlayer, this);
        std::cout << "DEBUG: Created root in updateTree (fallback)" << std::endl;
        return;
    }
    
    // Solo recrear en casos realmente excepcionales
    if (rootIsCompletelyWrong(board)) {
        std::cout << "DEBUG: Root completamente incorrecto, recreando" << std::endl;
        delete root;
        root = new GameNode(board, aiPlayer, this);
    }
}

bool AI::boardsAreSignificantlyDifferent(const Board& board1, const Board& board2) const {
    // Quick check: diferencia de turnos
    int turnDiff = abs(board1.getTurns() - board2.getTurns());
    if (turnDiff > 2) return true;
    
    // Quick check: diferentes capturas
    if (board1.getCaptures(1) != board2.getCaptures(1) || 
        board1.getCaptures(2) != board2.getCaptures(2)) {
        return turnDiff > 0; // Si hay diferencia de capturas Y turnos, recrear
    }
    
    // Si son muy similares, no recrear
    return false;
}

// NUEVA FUNCIÓN: Intenta navegar por el árbol existente
bool AI::tryNavigateToCurrentState(const Board& currentBoard) {
    // Quick check usando getTurns() 
    int currentTurns = currentBoard.getTurns();
    int rootTurns = root->getBoard().getTurns();
    int turnDifference = currentTurns - rootTurns;
    
    std::cout << "currentTurns:" << currentTurns << " rootTurns:" << rootTurns << " turnDifference:" << turnDifference << "\n";

    // Si hay demasiada diferencia de turnos, mejor recrear
    if (abs(turnDifference) > 3) {
        return false;
    }
    // Caso más común: exactamente un turno de diferencia
    if (abs(turnDifference) == 1) {
        return navigateOneMove(currentBoard, turnDifference > 0);
    }
    
    // Caso capturas: mismo número de turnos pero tablero diferente
    if (turnDifference == 0) {
        return navigateWithCaptures(currentBoard);
    }
    
    // Casos complejos (2-3 turnos): intentar navegación multi-paso
    if (abs(turnDifference) <= 3) {
        return false;
    }
    
    return false;
}

// NUEVA FUNCIÓN: Navegar un movimiento hacia adelante o atrás
bool AI::navigateOneMove(const Board& targetBoard, bool forward) {
    if (forward) {
        // Buscar hijo que coincida con targetBoard
        Move lastMove = findLastMove(root->getBoard(), targetBoard);
        if (lastMove.isValid()) {
            GameNode* child = root->findChild(lastMove);
            if (child) {
                promoteChildToRoot(child);
                return true;
            }
        }
    } else {
        // Navegar hacia atrás (menos común, pero posible con undo)
        // Por simplicidad, recrear en este caso
        return false;
    }
    
    return false;
}

// NUEVA FUNCIÓN: Manejar casos con capturas (mismo número turnos, tablero diferente)
bool AI::navigateWithCaptures(const Board& targetBoard) {
    // Verificar si la diferencia es solo por capturas
    std::vector<Move> differences = findBoardDifferences(root->getBoard(), targetBoard);
    
    // Si hay pocas diferencias y son consistentes con capturas
    if (differences.size() <= 4 && looksLikeCaptures(differences)) {
        // Intentar encontrar el movimiento que causó las capturas
        Move causingMove = findCaptureCausingMove(root->getBoard(), targetBoard);
        if (causingMove.isValid()) {
            GameNode* child = root->findChild(causingMove);
            if (child) {
                promoteChildToRoot(child);
                return true;
            }
        }
    }
    
    return false;
}

// NUEVA FUNCIÓN: Encontrar diferencias específicas entre tableros
std::vector<Move> AI::findBoardDifferences(const Board& board1, const Board& board2) const {
    std::vector<Move> differences;
    
    for (int i = 0; i < Board::getSize(); i++) {
        for (int j = 0; j < Board::getSize(); j++) {
            if (board1.getPiece(i, j) != board2.getPiece(i, j)) {
                differences.push_back(Move(i, j));
            }
        }
    }
    
    return differences;
}

// NUEVA FUNCIÓN: Verificar si las diferencias parecen capturas
bool AI::looksLikeCaptures(const std::vector<Move>& differences) const {
    // Capturas típicas: 1 nueva pieza + 2 piezas removidas
    // O patrones similares
    return differences.size() >= 1 && differences.size() <= 4;
}

// NUEVA FUNCIÓN: Encontrar movimiento que causó capturas
Move AI::findCaptureCausingMove(const Board& oldBoard, const Board& newBoard) const {
    for (int i = 0; i < Board::getSize(); i++) {
        for (int j = 0; j < Board::getSize(); j++) {
            // Buscar nueva pieza colocada
            if (oldBoard.isEmpty(i, j) && newBoard.getPiece(i, j) != 0) {
                return Move(i, j);
            }
        }
    }
    return Move(); // No válido
}

// OPTIMIZADA: Encontrar último movimiento usando getTurns()
Move AI::findLastMove(const Board& oldBoard, const Board& newBoard) const {
    // Si newBoard tiene más turnos, buscar la nueva pieza
    if (newBoard.getTurns() > oldBoard.getTurns()) {
        for (int i = 0; i < Board::getSize(); i++) {
            for (int j = 0; j < Board::getSize(); j++) {
                if (oldBoard.isEmpty(i, j) && newBoard.getPiece(i, j) != 0) {
                    return Move(i, j);
                }
            }
        }
    }
    
    return Move(); // No válido
}

// NUEVA FUNCIÓN: Promover hijo a raíz manteniendo trabajo útil
void AI::promoteChildToRoot(GameNode* child) {
    if (!child) return;
    
    // IMPORTANT: Copy data from child BEFORE deleting root
    Board childBoard = child->getBoard();
    int childPlayer = child->getPlayer();
        
    // Now delete root and create new one
    delete root;
    root = new GameNode(childBoard, childPlayer, this);
    
    // NOTA: En una implementación más sofisticada, podríamos transferir
    // los sub-hijos del nodo promovido para mantener más trabajo
}

// NUEVA FUNCIÓN: Navegación multi-paso (casos complejos)
// bool AI::tryMultiStepNavigation(const Board& targetBoard) {
//     // Para casos donde hubo múltiples movimientos o capturas complejas
//     // Por ahora, implementación conservadora: recrear
//     // TODO: Implementar lógica más sofisticada si es necesario
    
//     return false;
// }