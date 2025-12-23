// ============================================
// SEARCH_ORDERING.CPP
// Generación y ordenamiento de movimientos candidatos
// Incluye heurísticas de evaluación rápida y filtrado adaptativo
// ============================================

#include "../../include/ai/transposition_search.hpp"
#include <algorithm>
#include <cmath>

using namespace Directions;

// ============================================
// MOVE GENERATION & ORDERING
// ============================================

std::vector<Move> TranspositionSearch::generateOrderedMoves(const GameState &state)
{
	// NUEVO: Generar candidatos con filtrado adaptativo por fase de juego
	std::vector<Move> candidates = generateCandidatesAdaptiveRadius(state);

	return candidates;
}

void TranspositionSearch::orderMovesWithPreviousBest(std::vector<Move> &moves, const GameState &state)
{
	// Si tenemos mejor movimiento de iteración anterior, ponerlo primero
	if (previousBestMove.isValid())
	{
		auto it = std::find_if(moves.begin(), moves.end(),
							   [this](const Move &m)
							   {
								   return m.x == previousBestMove.x && m.y == previousBestMove.y;
							   });

		if (it != moves.end())
		{
			// Mover al frente
			std::iter_swap(moves.begin(), it);

			// Ordenar el resto normalmente
			if (moves.size() > 1)
			{
				std::sort(moves.begin() + 1, moves.end(), [&](const Move &a, const Move &b)
						  { return quickEvaluateMove(state, a) > quickEvaluateMove(state, b); });
			}

			return;
		}
	}

	// Si no hay movimiento anterior, orden normal
	orderMoves(moves, state);
}

void TranspositionSearch::orderMoves(std::vector<Move> &moves, const GameState &state)
{
	// OPTIMIZACIÓN: Para pocos movimientos, el orden importa menos
	if (moves.size() <= 2)
	{
		return; // Skip sorting completamente
	}

	// OPTIMIZACIÓN: Para movimientos medianos, sorting parcial
	if (moves.size() <= 4)
	{
		// Insertion sort más rápido para arrays pequeños
		for (size_t i = 1; i < moves.size(); ++i)
		{
			Move key = moves[i];
			int keyScore = quickEvaluateMove(state, key);

			size_t j = i;
			while (j > 0 && quickEvaluateMove(state, moves[j - 1]) < keyScore)
			{
				moves[j] = moves[j - 1];
				--j;
			}
			moves[j] = key;
		}
		return;
	}

	// Para arrays grandes, usar std::sort normal
	std::sort(moves.begin(), moves.end(), [&](const Move &a, const Move &b)
			  { return quickEvaluateMove(state, a) > quickEvaluateMove(state, b); });
}

// ============================================
// QUICK MOVE EVALUATION
// ============================================

int TranspositionSearch::quickEvaluateMove(const GameState& state, const Move& move) {
    int score = 0;
    int currentPlayer = state.currentPlayer;
    int opponent = state.getOpponent(currentPlayer);
    
    // ============================================
    // 1. CENTRALIDAD (O(1) - trivial)
    // ============================================
    int centerDist = std::max(std::abs(move.x - 9), std::abs(move.y - 9));
    score += (9 - centerDist) * 10;  // 0-90 puntos
    
    // ============================================
    // 2. CONECTIVIDAD INMEDIATA (O(8) - barato)
    // ============================================
    // ¿Cuántas fichas propias hay adyacentes?
    int myAdjacent = 0;
    int oppAdjacent = 0;
    
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            if (dx == 0 && dy == 0) continue;
            int nx = move.x + dx, ny = move.y + dy;
            if (state.isValid(nx, ny)) {
                int piece = state.getPiece(nx, ny);
                if (piece == currentPlayer) myAdjacent++;
                else if (piece == opponent) oppAdjacent++;
            }
        }
    }
    
    score += myAdjacent * 50;      // 0-400 puntos
    score += oppAdjacent * 20;     // Bonus menor por bloqueo
    
    // ============================================
    // 3. PRIORIDAD POR ZONA ACTIVA (O(1) - trivial)
    // ============================================
    // ¿Está cerca del último movimiento del oponente?
    if (state.lastHumanMove.isValid()) {
        int distToLast = std::max(
            std::abs(move.x - state.lastHumanMove.x),
            std::abs(move.y - state.lastHumanMove.y)
        );
        
        if (distToLast <= 2) {
            score += 500;  // Respuesta táctica
        }
    }
    
    // ============================================
    // 4. PATRONES SIMPLES (O(4) - muy barato)
    // ============================================
    // Solo contar piezas consecutivas SIN verificar extremos libres
    int maxMyLine = 0;
    int maxOppLine = 0;
    
    for (int d = 0; d < MAIN_COUNT; d++) {
        int dx = MAIN[d][0];
        int dy = MAIN[d][1];
        
        // Contar hacia ambos lados
        int myCount = 1;  // La que voy a colocar
        myCount += countConsecutiveInDirection(state, move.x, move.y, dx, dy, currentPlayer, 4);
        myCount += countConsecutiveInDirection(state, move.x, move.y, -dx, -dy, currentPlayer, 4);
        maxMyLine = std::max(maxMyLine, myCount);
        
        // Contar líneas del oponente (para bloqueo)
        int oppCount = 0;
        oppCount += countConsecutiveInDirection(state, move.x, move.y, dx, dy, opponent, 4);
        oppCount += countConsecutiveInDirection(state, move.x, move.y, -dx, -dy, opponent, 4);
        maxOppLine = std::max(maxOppLine, oppCount);
    }
    
    // Scoring exponencial para líneas largas
    if (maxMyLine >= 5) score += 100000;      // Victoria
    else if (maxMyLine == 4) score += 10000;  // Muy peligroso
    else if (maxMyLine == 3) score += 1000;   // Peligroso
    else if (maxMyLine == 2) score += 100;    // Desarrollo
    
    // Bloqueo
    if (maxOppLine >= 4) score += 8000;       // Bloqueo crítico
    else if (maxOppLine == 3) score += 800;   // Bloqueo importante
    
    // ============================================
    // 5. CAPTURA RÁPIDA (O(8) - barato)
    // ============================================
    // Solo verificar si HAY captura, sin evaluar contexto
    for (int d = 0; d < ALL_COUNT; d++) {
        int dx = ALL[d][0];
        int dy = ALL[d][1];
        
        // Patrón: NUEVA + OPP + OPP + MIA
        int x1 = move.x + dx, y1 = move.y + dy;
        int x2 = move.x + 2*dx, y2 = move.y + 2*dy;
        int x3 = move.x + 3*dx, y3 = move.y + 3*dy;
        
        if (state.isValid(x1, y1) && state.isValid(x2, y2) && state.isValid(x3, y3)) {
            if (state.getPiece(x1, y1) == opponent &&
                state.getPiece(x2, y2) == opponent &&
                state.getPiece(x3, y3) == currentPlayer) {
                score += 2000;  // Hay captura
                break;  // No seguir buscando
            }
        }
    }
    
    return score;
}

// ============================================
// PATTERN DETECTION HELPERS
// ============================================

bool TranspositionSearch::wouldCreateFiveInRow(const GameState& state, const Move& move, int player) {
    // Chequear 4 direcciones principales
    for (int d = 0; d < MAIN_COUNT; d++) {
        int dx = MAIN[d][0], dy = MAIN[d][1];
        
        int count = 1; // La pieza que vamos a colocar
        count += countConsecutiveInDirection(state, move.x, move.y, dx, dy, player, 4);
        count += countConsecutiveInDirection(state, move.x, move.y, -dx, -dy, player, 4);
        
        if (count >= 5) return true;
    }
    
    return false;
}

bool TranspositionSearch::createsFourInRow(const GameState& state, const Move& move, int player) {
    for (int d = 0; d < MAIN_COUNT; d++) {
        int dx = MAIN[d][0], dy = MAIN[d][1];
        
        int count = 1;
        count += countConsecutiveInDirection(state, move.x, move.y, dx, dy, player, 3);
        count += countConsecutiveInDirection(state, move.x, move.y, -dx, -dy, player, 3);
        
        if (count == 4) {
            // Verificar que al menos un extremo esté libre (para que sea amenaza real)
            int startX = move.x - (count - 1) * dx;
            int startY = move.y - (count - 1) * dy;
            int endX = move.x + (count - 1) * dx;
            int endY = move.y + (count - 1) * dy;
            
            bool startFree = state.isValid(startX - dx, startY - dy) && 
                           state.isEmpty(startX - dx, startY - dy);
            bool endFree = state.isValid(endX + dx, endY + dy) && 
                         state.isEmpty(endX + dx, endY + dy);
            
            if (startFree || endFree) return true;
        }
    }
    
    return false;
}

bool TranspositionSearch::createsThreeInRow(const GameState& state, const Move& move, int player) {
    for (int d = 0; d < MAIN_COUNT; d++) {
        int dx = MAIN[d][0], dy = MAIN[d][1];
        
        int count = 1;
        count += countConsecutiveInDirection(state, move.x, move.y, dx, dy, player, 2);
        count += countConsecutiveInDirection(state, move.x, move.y, -dx, -dy, player, 2);
        
        if (count == 3) {
            // Verificar que ambos extremos estén libres (free-three)
            int forward = countConsecutiveInDirection(state, move.x, move.y, dx, dy, player, 2);
            int backward = countConsecutiveInDirection(state, move.x, move.y, -dx, -dy, player, 2);
            
            int startX = move.x - backward * dx;
            int startY = move.y - backward * dy;
            int endX = move.x + forward * dx;
            int endY = move.y + forward * dy;
            
            bool startFree = state.isValid(startX - dx, startY - dy) && 
                           state.isEmpty(startX - dx, startY - dy);
            bool endFree = state.isValid(endX + dx, endY + dy) && 
                         state.isEmpty(endX + dx, endY + dy);
            
            if (startFree && endFree) return true;
        }
    }
    
    return false;
}

bool TranspositionSearch::hasImmediateCapture(const GameState& state, const Move& move, int player) {
    int opponent = state.getOpponent(player);
    
    // Verificar patrón de captura en 8 direcciones: NUEVA + OPP + OPP + MIA
    for (int d = 0; d < ALL_COUNT; d++) {
        int dx = ALL[d][0], dy = ALL[d][1];
        
        // Patrón hacia adelante: NUEVA + OPP + OPP + MIA
        if (state.isValid(move.x + dx, move.y + dy) &&
            state.isValid(move.x + 2*dx, move.y + 2*dy) &&
            state.isValid(move.x + 3*dx, move.y + 3*dy)) {
            
            if (state.getPiece(move.x + dx, move.y + dy) == opponent &&
                state.getPiece(move.x + 2*dx, move.y + 2*dy) == opponent &&
                state.getPiece(move.x + 3*dx, move.y + 3*dy) == player) {
                return true;
            }
        }
        
        // Patrón hacia atrás: MIA + OPP + OPP + NUEVA
        if (state.isValid(move.x - dx, move.y - dy) &&
            state.isValid(move.x - 2*dx, move.y - 2*dy) &&
            state.isValid(move.x - 3*dx, move.y - 3*dy)) {
            
            if (state.getPiece(move.x - dx, move.y - dy) == opponent &&
                state.getPiece(move.x - 2*dx, move.y - 2*dy) == opponent &&
                state.getPiece(move.x - 3*dx, move.y - 3*dy) == player) {
                return true;
            }
        }
    }
    
    return false;
}

bool TranspositionSearch::isNearExistingPieces(const GameState& state, const Move& move) {
    // Verificar si hay piezas en radio 2
    for (int dx = -2; dx <= 2; dx++) {
        for (int dy = -2; dy <= 2; dy++) {
            if (dx == 0 && dy == 0) continue;
            int nx = move.x + dx, ny = move.y + dy;
            if (state.isValid(nx, ny) && state.getPiece(nx, ny) != GameState::EMPTY) {
                return true;
            }
        }
    }
    return false;
}

bool TranspositionSearch::blocksOpponentWin(const GameState& state, const Move& move, int opponent) {
    // Simular que el oponente tiene el turno y verificar si ese movimiento crearía victoria
    return wouldCreateFiveInRow(state, move, opponent);
}

bool TranspositionSearch::blocksOpponentFour(const GameState& state, const Move& move, int opponent) {
    return createsFourInRow(state, move, opponent);
}

bool TranspositionSearch::blocksOpponentThree(const GameState& state, const Move& move, int opponent) {
    return createsThreeInRow(state, move, opponent);
}

int TranspositionSearch::countConsecutiveInDirection(const GameState& state, int x, int y, 
                                                   int dx, int dy, int player, int maxCount) {
    int count = 0;
    x += dx;
    y += dy;
    
    while (count < maxCount && state.isValid(x, y) && state.getPiece(x, y) == player) {
        count++;
        x += dx;
        y += dy;
    }
    
    return count;
}

// ============================================
// THREAT COUNTING HELPERS
// ============================================

bool TranspositionSearch::isBlocked(const GameState &state, int x, int y, int dx, int dy, int steps, int player)
{
	int nx = x + dx * steps;
	int ny = y + dy * steps;

	if (!state.isValid(nx, ny))
	{
		return true; // Bloqueado por borde
	}

	int piece = state.getPiece(nx, ny);
	return (piece != GameState::EMPTY && piece != player); // Bloqueado por oponente
}

int TranspositionSearch::countThreats(const GameState &state, int player)
{
	int threats = 0;

	for (int i = 0; i < GameState::BOARD_SIZE; i += 2)
	{
		for (int j = 0; j < GameState::BOARD_SIZE; j += 2)
		{
			if (state.getPiece(i, j) == player)
			{
				threats += countLinesFromPosition(state, i, j, player);
			}
		}
	}

	return threats;
}

int TranspositionSearch::countLinesFromPosition(const GameState &state, int x, int y, int player)
{
	int lines = 0;

	for (int d = 0; d < MAIN_COUNT; d++)
	{
		int dx = MAIN[d][0], dy = MAIN[d][1];

		int count = 1; // La pieza actual
		count += countInDirection(state, x, y, dx, dy, player);
		count += countInDirection(state, x, y, -dx, -dy, player);

		if (count >= 3)
			lines++;
	}

	return lines;
}

int TranspositionSearch::countInDirection(const GameState &state, int x, int y, int dx, int dy, int player)
{
	int count = 0;
	x += dx;
	y += dy;

	while (state.isValid(x, y) && state.getPiece(x, y) == player)
	{
		count++;
		x += dx;
		y += dy;
	}

	return count;
}

// ============================================
// ADAPTIVE CANDIDATE GENERATION
// ============================================

std::vector<Move> TranspositionSearch::generateCandidatesAdaptiveRadius(const GameState &state)
{
    std::vector<Move> candidates;
    int searchRadius = getSearchRadiusForGamePhase(state.turnCount);
    
    // OPTIMIZACIÓN: Pre-marcar zonas relevantes
    bool relevantZone[GameState::BOARD_SIZE][GameState::BOARD_SIZE];
    for(int r=0; r<GameState::BOARD_SIZE; r++)
        for(int c=0; c<GameState::BOARD_SIZE; c++)
            relevantZone[r][c] = false;
    
    // PASO 1: Marcar casillas alrededor de piezas existentes
    for (int i = 0; i < GameState::BOARD_SIZE; i++) {
        for (int j = 0; j < GameState::BOARD_SIZE; j++) {
            if (state.board[i][j] != GameState::EMPTY) {
                // Marcar radio alrededor de esta pieza
                for (int di = -searchRadius; di <= searchRadius; di++) {
                    for (int dj = -searchRadius; dj <= searchRadius; dj++) {
                        int ni = i + di, nj = j + dj;
                        if (state.isValid(ni, nj) && state.isEmpty(ni, nj)) {
                            relevantZone[ni][nj] = true;
                        }
                    }
                }
            }
        }
    }
    
    // PASO 2: Marcar zona alrededor del último movimiento humano (prioridad táctica)
    if (state.lastHumanMove.isValid()) {
        int extendedRadius = searchRadius + 1; // Radio mayor para respuestas
        for (int di = -extendedRadius; di <= extendedRadius; di++) {
            for (int dj = -extendedRadius; dj <= extendedRadius; dj++) {
                int ni = state.lastHumanMove.x + di;
                int nj = state.lastHumanMove.y + dj;
                if (state.isValid(ni, nj) && state.isEmpty(ni, nj)) {
                    relevantZone[ni][nj] = true;
                }
            }
        }
    }
    
    // PASO 3: Solo agregar candidatos de zonas marcadas
    for (int i = 0; i < GameState::BOARD_SIZE; i++) {
        for (int j = 0; j < GameState::BOARD_SIZE; j++) {
            if (relevantZone[i][j]) {
                candidates.push_back(Move(i, j));
            }
        }
    }
    
    // Ordenar con move ordering
    orderMovesWithPreviousBest(candidates, state);
    
    // Limitar número de candidatos
    int maxCandidates = getMaxCandidatesForGamePhase(state);
    if (candidates.size() > (size_t)maxCandidates) {
        candidates.resize(maxCandidates);
    }
    
    return candidates;
}

int TranspositionSearch::getSearchRadiusForGamePhase(int pieceCount)
{
	if (pieceCount > 0)
		return 1;
	return 1;
}

int TranspositionSearch::getMaxCandidatesForGamePhase(const GameState &state)
{
	int pieceCount = state.turnCount;

	if (pieceCount <= 4)
	{
		return 3; // Opening: muy selectivo para evitar explosion combinatoria
	}
	else if (pieceCount <= 10)
	{
		return 4; // Early game: moderadamente selectivo
	}
	else
	{
		return 5; // Mid/late game: más opciones disponibles
	}
}

void TranspositionSearch::addCandidatesAroundLastHumanMove(std::vector<Move> &candidates, const GameState &state)
{
	// Si no hay último movimiento humano válido, no hacer nada
	if (!state.lastHumanMove.isValid())
	{
		return;
	}

	int lastX = state.lastHumanMove.x;
	int lastY = state.lastHumanMove.y;

	// Generar todas las casillas vacías en un radio de 2 alrededor del último movimiento humano
	int radius = 2; // Radio de respuesta defensiva

	for (int dx = -radius; dx <= radius; dx++)
	{
		for (int dy = -radius; dy <= radius; dy++)
		{
			if (dx == 0 && dy == 0)
				continue; // Saltar la posición del último movimiento

			int newX = lastX + dx;
			int newY = lastY + dy;

			// Verificar que esté dentro del tablero y sea una casilla vacía
			if (state.isValid(newX, newY) && state.isEmpty(newX, newY))
			{
				Move candidate(newX, newY);

				// Verificar si ya está en la lista de candidatos
				bool alreadyAdded = false;
				for (const Move &existing : candidates)
				{
					if (existing.x == newX && existing.y == newY)
					{
						alreadyAdded = true;
						break;
					}
				}

				// Si no está ya agregado y es un movimiento legal, agregarlo
				if (!alreadyAdded && RuleEngine::isLegalMove(state, candidate))
				{
					candidates.push_back(candidate);
				}
			}
		}
	}
}

// ============================================
// GEOMETRIC MOVE VALUE CALCULATION
// ============================================

void TranspositionSearch::orderMovesByGeometricValue(std::vector<Move> &moves, const GameState &state)
{
	// Ordenar usando evaluación basada en patrones geométricos
	std::sort(moves.begin(), moves.end(), [&](const Move &a, const Move &b)
			  { return calculateGeometricMoveValue(state, a) > calculateGeometricMoveValue(state, b); });
}

int TranspositionSearch::calculateGeometricMoveValue(const GameState &state, const Move &move)
{
	int value = 0;
	int currentPlayer = state.currentPlayer;
	int opponent = state.getOpponent(currentPlayer);

	// 1. VALOR POSICIONAL: Proximidad al centro (importante en opening)
	int centralityBonus = calculateCentralityBonus(move);
	value += centralityBonus;

	// 2. ANÁLISIS DE PATRONES: Evaluar alineaciones potenciales en 4 direcciones
	for (int d = 0; d < MAIN_COUNT; d++)
	{
		int dx = MAIN[d][0], dy = MAIN[d][1];

		// Contar piezas propias que se alinearían con este movimiento
		int myAlignment = 1; // La pieza que vamos a colocar
		myAlignment += countPiecesInDirection(state, move.x, move.y, dx, dy, currentPlayer);
		myAlignment += countPiecesInDirection(state, move.x, move.y, -dx, -dy, currentPlayer);

		// Contar piezas del oponente para evaluar interrupciones
		int opponentInterruption = 0;
		opponentInterruption += countPiecesInDirection(state, move.x, move.y, dx, dy, opponent);
		opponentInterruption += countPiecesInDirection(state, move.x, move.y, -dx, -dy, opponent);

		// SCORING basado en valor táctico de las alineaciones
		value += calculateAlignmentValue(myAlignment);
		value += calculateInterruptionValue(opponentInterruption);
	}

	// 3. CONECTIVIDAD: Bonus por estar adyacente a piezas propias
	int connectivityBonus = calculateConnectivityBonus(state, move, currentPlayer);
	value += connectivityBonus;

	return value;
}

int TranspositionSearch::calculateCentralityBonus(const Move &move)
{
	int centerDistance = std::max(std::abs(move.x - 9), std::abs(move.y - 9));
	return (9 - centerDistance) * 10;
}

int TranspositionSearch::calculateAlignmentValue(int alignmentLength)
{
	switch (alignmentLength)
	{
	case 5:
		return 10000; // Victoria inmediata
	case 4:
		return 5000; // Amenaza crítica
	case 3:
		return 1000; // Amenaza fuerte
	case 2:
		return 100; // Desarrollo básico
	default:
		return 0;
	}
}

int TranspositionSearch::calculateInterruptionValue(int interruptionLength)
{
	switch (interruptionLength)
	{
	case 4:
		return 80000; // AUMENTADO: Bloqueo crítico debe superar ataque propio (70000)
	case 3:
		return 15000; // AUMENTADO: Bloqueo de amenaza fuerte
	case 2:
		return 1000; // AUMENTADO: Prevención temprana
	default:
		return 0;
	}
}

int TranspositionSearch::calculateConnectivityBonus(const GameState &state, const Move &move, int player)
{
	int connectivity = 0;

	// Verificar las 8 direcciones adyacentes
	for (int dx = -1; dx <= 1; dx++)
	{
		for (int dy = -1; dy <= 1; dy++)
		{
			if (dx == 0 && dy == 0)
				continue;

			int adjX = move.x + dx, adjY = move.y + dy;
			if (state.isValid(adjX, adjY) && state.getPiece(adjX, adjY) == player)
			{
				connectivity += 30;
			}
		}
	}

	return connectivity;
}

// Método auxiliar optimizado para contar piezas consecutivas
int TranspositionSearch::countPiecesInDirection(const GameState &state, int x, int y,
												int dx, int dy, int player)
{
	// OPTIMIZACIÓN: Unroll manual para casos comunes (1-4 piezas)
	x += dx;
	y += dy;

	if (!state.isValid(x, y) || state.getPiece(x, y) != player)
		return 0;

	x += dx;
	y += dy;
	if (!state.isValid(x, y) || state.getPiece(x, y) != player)
		return 1;

	x += dx;
	y += dy;
	if (!state.isValid(x, y) || state.getPiece(x, y) != player)
		return 2;

	x += dx;
	y += dy;
	if (!state.isValid(x, y) || state.getPiece(x, y) != player)
		return 3;

	return 4; // Máximo que necesitamos
}
