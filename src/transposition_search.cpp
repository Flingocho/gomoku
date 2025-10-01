/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   transposition_search.cpp                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+       // NUEVA LÓGICA EFICIENTE: Verificar amenazas del oponente sin iterar
	bool opponentHasThreats = Evaluator::hasWinningThreats(state, opponent);

	if (opponentHasThreats) {
		// Evaluar si este movimiento podría ayudar defensivamente
		// Simulación rápida: aplicar movimiento y re-evaluar amenazas
		GameState tempState = state;
		RuleEngine::MoveResult result = RuleEngine::applyMove(tempState, move);

		if (result.success) {
			bool stillHasThreats = Evaluator::hasWinningThreats(tempState, opponent);

			if (!stillHasThreats) {
				score += 400000; // SUPERVIVENCIA - neutralizó amenazas
			} else {
				score -= 200000; // PARCIAL - aún hay amenazas pero es mejor que nada
			}
		} else {
			score -= 500000; // SUICIDIO - movimiento ilegal con amenazas activas
		}
	}                                        +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/14 21:38:31 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/22 16:34:18 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/transposition_search.hpp"
#include "../include/debug_analyzer.hpp"
#include <algorithm>
#include <limits>
#include <iostream>
#include <sstream>
#include <cmath>
#include <iomanip>

TranspositionSearch::TranspositionSearch(size_t tableSizeMB)
	: currentGeneration(1), nodesEvaluated(0), cacheHits(0)
{
	initializeTranspositionTable(tableSizeMB);
}

void TranspositionSearch::initializeTranspositionTable(size_t sizeInMB)
{
	// Calcular número de entradas: cada CacheEntry ~40 bytes
	size_t bytesPerEntry = sizeof(CacheEntry);
	size_t totalBytes = sizeInMB * 1024 * 1024;
	size_t numEntries = totalBytes / bytesPerEntry;

	// Redondear a la potencia de 2 más cercana (para usar & en lugar de %)
	size_t powerOf2 = 1;
	while (powerOf2 < numEntries)
	{
		powerOf2 <<= 1;
	}
	if (powerOf2 > numEntries)
	{
		powerOf2 >>= 1; // Usar la potencia menor si nos pasamos
	}

	transpositionTable.resize(powerOf2);
	tableSizeMask = powerOf2 - 1; // Para hacer index = hash & tableSizeMask

	// Inicialización de TranspositionTable se loggeará desde main
}

TranspositionSearch::SearchResult TranspositionSearch::findBestMove(const GameState &state, int maxDepth)
{
	SearchResult result;
	nodesEvaluated = 0;
	cacheHits = 0;

	// NUEVO: Incrementar generación para aging-based replacement
	currentGeneration++;

	// DEBUG: Mostrar tablero inicial antes de evaluar
	if (g_debugAnalyzer)
	{
		std::ostringstream debugMsg;
		debugMsg << "\n====== EVALUANDO POSICIÓN INICIAL ======";
		debugMsg << "\nTurno: " << state.turnCount << ", Jugador: " << (state.currentPlayer == GameState::PLAYER1 ? "HUMAN (O)" : "AI (X)");
		debugMsg << "\nCapturas: HUMAN=" << state.captures[0] << " AI=" << state.captures[1];
		debugMsg << g_debugAnalyzer->formatBoard(state);
		debugMsg << "========================================\n";
		g_debugAnalyzer->logToFile(debugMsg.str());
	}

	// Profundidad adaptativa
	int adaptiveDepth = calculateAdaptiveDepth(state, maxDepth);

	DEBUG_LOG_AI("AI usando profundidad: " + std::to_string(adaptiveDepth) + " (solicitada: " + std::to_string(maxDepth) + ")");

	Move bestMove;
	auto startTime = std::chrono::high_resolution_clock::now();

	int score = minimax(const_cast<GameState &>(state), adaptiveDepth,
						std::numeric_limits<int>::min(),
						std::numeric_limits<int>::max(),
						state.currentPlayer == GameState::PLAYER2,
						adaptiveDepth,
						&bestMove);

	auto endTime = std::chrono::high_resolution_clock::now();
	int elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

	result.bestMove = bestMove;
	result.score = score;
	result.nodesEvaluated = nodesEvaluated;
	result.cacheHits = cacheHits;
	result.cacheHitRate = nodesEvaluated > 0 ? (float)cacheHits / nodesEvaluated : 0.0f;

	// **NUEVO: Debug del movimiento final elegido**
	if (g_debugAnalyzer)
	{
		DEBUG_CHOSEN_MOVE(bestMove, score);
		DEBUG_SNAPSHOT(state, elapsedTime, nodesEvaluated);
	}

	return result;
}

int TranspositionSearch::calculateAdaptiveDepth(const GameState &state, int requestedDepth)
{
	int pieceCount = state.turnCount;

	if (pieceCount <= 2 && requestedDepth > 1)
	{
		return 4; // Primeros movimientos: muy rápido
	}
	else		  // if (pieceCount <= 6) {
		return 6; // Early game: rápido
				  // } else if (pieceCount <= 15) {
				  //     return 8;  // Mid game: moderado
				  // } else {
				  //     return std::min(requestedDepth, 10);  // Late game: profundidad completa
				  // }
}

int TranspositionSearch::minimax(GameState &state, int depth, int alpha, int beta,
								 bool maximizing, int originalMaxDepth, Move *bestMove)
{
	nodesEvaluated++;

	// Debug cada 10000 nodos
	if (nodesEvaluated % 10000 == 0)
	{
		DEBUG_LOG_STATS("Nodos evaluados: " + std::to_string(nodesEvaluated) +
						", Cache hits: " + std::to_string(cacheHits));
	}

	// ZOBRIST: Verificar transposition table PRIMERO
	uint64_t zobristKey = state.getZobristHash();
	CacheEntry entry;
	if (lookupTransposition(zobristKey, entry))
	{
		cacheHits++;

		if (entry.depth >= depth)
		{
			if (entry.type == CacheEntry::EXACT)
			{
				if (bestMove && depth == originalMaxDepth)
				{
					*bestMove = entry.bestMove;
				}
				return entry.score;
			}
			else if (entry.type == CacheEntry::LOWER_BOUND && entry.score >= beta)
			{
				return beta;
			}
			else if (entry.type == CacheEntry::UPPER_BOUND && entry.score <= alpha)
			{
				return alpha;
			}
		}

		// Usar movimiento del cache para ordering si estamos en nivel raíz
		if (entry.bestMove.isValid())
		{
			previousBestMove = entry.bestMove;
		}
	}

	// CASOS BASE
	if (depth == 0 || RuleEngine::checkWin(state, GameState::PLAYER1) ||
		RuleEngine::checkWin(state, GameState::PLAYER2))
	{

		int score = Evaluator::evaluate(state, originalMaxDepth, originalMaxDepth - depth);
		storeTransposition(zobristKey, score, depth, Move(), CacheEntry::EXACT);
		return score;
	}

	// Generar y ordenar movimientos
	std::vector<Move> moves = generateOrderedMoves(state);

	if (moves.empty())
	{
		int score = Evaluator::evaluate(state, originalMaxDepth, originalMaxDepth - depth);
		storeTransposition(zobristKey, score, depth, Move(), CacheEntry::EXACT);
		return score;
	}

	Move currentBestMove;
	int originalAlpha = alpha;

	if (maximizing)
	{
		int maxEval = std::numeric_limits<int>::min();

		for (const Move &move : moves)
		{
			GameState newState = state;
			RuleEngine::MoveResult result = RuleEngine::applyMove(newState, move);
			if (!result.success)
				continue;

			// NUEVO: Debug de heurística REAL del evaluador (no re-evaluar)
			if (g_debugAnalyzer && depth == originalMaxDepth)
			{
				// Activar captura ANTES de la evaluación recursiva real
				g_evalDebug.reset();
				g_evalDebug.active = true;
				g_evalDebug.currentMove = move;
			}

			// EVALUACIÓN RECURSIVA: Esta es la única evaluación que cuenta
			int eval = minimax(newState, depth - 1, alpha, beta, false, originalMaxDepth, nullptr);

			// NUEVO: Capturar datos reales DESPUÉS de la evaluación
			if (g_debugAnalyzer && depth == originalMaxDepth && g_evalDebug.active)
			{
				std::ostringstream heuristicInfo;

				// Mostrar el tablero después del movimiento
				heuristicInfo << "\n=== EVALUANDO MOVIMIENTO " << g_debugAnalyzer->formatMove(move) << " ===\n";
				heuristicInfo << g_debugAnalyzer->formatBoard(newState);

				heuristicInfo << "Score:" << eval;
				heuristicInfo << " [REAL_DATA: 3Open:" << g_evalDebug.aiThreeOpen
							  << "(" << g_evalDebug.aiThreeOpen * Evaluator::THREE_OPEN << ")";
				heuristicInfo << " 4Half:" << g_evalDebug.aiFourHalf
							  << "(" << g_evalDebug.aiFourHalf * Evaluator::FOUR_HALF << ")";
				heuristicInfo << " 4Open:" << g_evalDebug.aiFourOpen
							  << "(" << g_evalDebug.aiFourOpen * Evaluator::FOUR_OPEN << ")";
				heuristicInfo << " 2Open:" << g_evalDebug.aiTwoOpen
							  << "(" << g_evalDebug.aiTwoOpen * Evaluator::TWO_OPEN << ")]\n";
				g_debugAnalyzer->logToFile(heuristicInfo.str());
				g_evalDebug.active = false;
			}

			// Actualizar mejor movimiento basándose SOLO en la evaluación recursiva
			if (eval > maxEval)
			{
				maxEval = eval;
				currentBestMove = move;
			}

			// Actualizar alpha con el resultado correcto
			alpha = std::max(alpha, eval);

			// Poda alfa-beta
			if (beta <= alpha)
			{
				break; // Beta cutoff
			}
		}

		// Determinar tipo de entrada para cache
		CacheEntry::Type entryType = CacheEntry::EXACT;
		if (maxEval <= originalAlpha)
		{
			entryType = CacheEntry::UPPER_BOUND;
		}
		else if (maxEval >= beta)
		{
			entryType = CacheEntry::LOWER_BOUND;
		}

		if (!currentBestMove.isValid() && !moves.empty())
		{
			currentBestMove = moves[0]; // Al menos el primer movimiento evaluado
		}
		storeTransposition(zobristKey, maxEval, depth, currentBestMove, entryType);

		if (bestMove && depth == originalMaxDepth)
		{
			*bestMove = currentBestMove;
		}

		return maxEval;
	}
	else
	{
		int minEval = std::numeric_limits<int>::max();

		for (const Move &move : moves)
		{
			GameState newState = state;
			RuleEngine::MoveResult result = RuleEngine::applyMove(newState, move);
			if (!result.success)
				continue;

			// NUEVO: Debug de heurística REAL del evaluador (no re-evaluar)
			if (g_debugAnalyzer && depth == originalMaxDepth)
			{
				// Activar captura ANTES de la evaluación recursiva real
				g_evalDebug.reset();
				g_evalDebug.active = true;
				g_evalDebug.currentMove = move;
			}

			// EVALUACIÓN RECURSIVA: Esta es la única evaluación que cuenta
			int eval = minimax(newState, depth - 1, alpha, beta, true, originalMaxDepth, nullptr);

			// NUEVO: Capturar datos reales DESPUÉS de la evaluación
			if (g_debugAnalyzer && depth == originalMaxDepth && g_evalDebug.active)
			{
				std::ostringstream heuristicInfo;

				// Mostrar el tablero después del movimiento
				heuristicInfo << "\n=== EVALUANDO MOVIMIENTO " << g_debugAnalyzer->formatMove(move) << " ===\n";
				heuristicInfo << g_debugAnalyzer->formatBoard(newState);

				heuristicInfo << "Score:" << eval;
				heuristicInfo << " [REAL_DATA: 3Open:" << g_evalDebug.humanThreeOpen
							  << "(" << g_evalDebug.humanThreeOpen * Evaluator::THREE_OPEN << ")";
				heuristicInfo << " 4Half:" << g_evalDebug.humanFourHalf
							  << "(" << g_evalDebug.humanFourHalf * Evaluator::FOUR_HALF << ")";
				heuristicInfo << " 4Open:" << g_evalDebug.humanFourOpen
							  << "(" << g_evalDebug.humanFourOpen * Evaluator::FOUR_OPEN << ")";
				heuristicInfo << " 2Open:" << g_evalDebug.humanTwoOpen
							  << "(" << g_evalDebug.humanTwoOpen * Evaluator::TWO_OPEN << ")]\n";
				g_debugAnalyzer->logToFile(heuristicInfo.str());
				g_evalDebug.active = false;
			}

			// Actualizar mejor movimiento basándose SOLO en la evaluación recursiva
			if (eval < minEval)
			{
				minEval = eval;
				currentBestMove = move;
			}

			// Actualizar beta con el resultado correcto
			beta = std::min(beta, eval);

			// Poda alfa-beta
			if (beta <= alpha)
			{
				break; // Alpha cutoff
			}
		}

		// Determinar tipo de entrada para cache
		CacheEntry::Type entryType = CacheEntry::EXACT;
		if (minEval <= originalAlpha)
		{
			entryType = CacheEntry::UPPER_BOUND;
		}
		else if (minEval >= beta)
		{
			entryType = CacheEntry::LOWER_BOUND;
		}
		if (!currentBestMove.isValid() && !moves.empty())
		{
			currentBestMove = moves[0]; // Al menos el primer movimiento evaluado
		}
		storeTransposition(zobristKey, minEval, depth, currentBestMove, entryType);

		if (bestMove && depth == originalMaxDepth)
		{
			*bestMove = currentBestMove;
		}

		return minEval;
	}
}

bool TranspositionSearch::lookupTransposition(uint64_t zobristKey, CacheEntry &entry)
{
	size_t index = zobristKey & tableSizeMask;
	const CacheEntry &candidate = transpositionTable[index];

	// OPTIMIZACIÓN: Early return si está vacía (caso más común)
	if (candidate.zobristKey == 0)
	{
		return false;
	}

	// OPTIMIZACIÓN: Verificación exacta
	if (candidate.zobristKey == zobristKey)
	{
		entry = candidate;

		// OPTIMIZACIÓN: Solo actualizar generación si es diferente
		if (candidate.generation != currentGeneration)
		{
			transpositionTable[index].generation = currentGeneration;
		}
		return true;
	}

	return false; // Hash collision
}

void TranspositionSearch::storeTransposition(uint64_t zobristKey, int score, int depth,
											 Move bestMove, CacheEntry::Type type)
{
	size_t index = zobristKey & tableSizeMask;
	CacheEntry &existing = transpositionTable[index];

	// NUEVA ESTRATEGIA: Reemplazo inteligente basado en importancia
	bool shouldReplace = false;

	if (existing.zobristKey == 0)
	{
		// Entrada vacía - siempre reemplazar
		shouldReplace = true;
	}
	else if (existing.zobristKey == zobristKey)
	{
		// Misma posición - actualizar si profundidad es mayor o igual
		shouldReplace = (depth >= existing.depth);
	}
	else
	{
		// Colisión de hash - usar estrategia de reemplazo sofisticada
		CacheEntry newEntry(zobristKey, score, depth, bestMove, type, currentGeneration);

		// Calcular valores de importancia
		int existingImportance = existing.getImportanceValue();
		int newImportance = newEntry.getImportanceValue();

		// Factor de aging: entradas más viejas tienen menor prioridad
		uint32_t ageDiff = currentGeneration - existing.generation;
		if (ageDiff > 0)
		{
			existingImportance -= (ageDiff * 10); // Penalizar entradas viejas
		}

		// Reemplazar si la nueva entrada es más importante
		shouldReplace = (newImportance > existingImportance);

		// Bias hacia entradas EXACT si hay empate
		if (newImportance == existingImportance && type == CacheEntry::EXACT)
		{
			shouldReplace = true;
		}
	}

	if (shouldReplace)
	{
		transpositionTable[index] = CacheEntry(zobristKey, score, depth, bestMove, type, currentGeneration);
	}
}

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
    
    for (int d = 0; d < 4; d++) {
        int dx = MAIN_DIRECTIONS[d][0];
        int dy = MAIN_DIRECTIONS[d][1];
        
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
    for (int d = 0; d < 8; d++) {
        int dx = CAPTURE_DIRECTIONS[d][0];
        int dy = CAPTURE_DIRECTIONS[d][1];
        
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


bool TranspositionSearch::wouldCreateFiveInRow(const GameState& state, const Move& move, int player) {
    // Chequear 4 direcciones principales
    int directions[4][2] = {{1,0}, {0,1}, {1,1}, {1,-1}};
    
    for (int d = 0; d < 4; d++) {
        int dx = directions[d][0], dy = directions[d][1];
        
        int count = 1; // La pieza que vamos a colocar
        count += countConsecutiveInDirection(state, move.x, move.y, dx, dy, player, 4);
        count += countConsecutiveInDirection(state, move.x, move.y, -dx, -dy, player, 4);
        
        if (count >= 5) return true;
    }
    
    return false;
}

bool TranspositionSearch::createsFourInRow(const GameState& state, const Move& move, int player) {
    int directions[4][2] = {{1,0}, {0,1}, {1,1}, {1,-1}};
    
    for (int d = 0; d < 4; d++) {
        int dx = directions[d][0], dy = directions[d][1];
        
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
    int directions[4][2] = {{1,0}, {0,1}, {1,1}, {1,-1}};
    
    for (int d = 0; d < 4; d++) {
        int dx = directions[d][0], dy = directions[d][1];
        
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
    int directions[8][2] = {{-1,-1}, {-1,0}, {-1,1}, {0,-1}, {0,1}, {1,-1}, {1,0}, {1,1}};
    
    for (int d = 0; d < 8; d++) {
        int dx = directions[d][0], dy = directions[d][1];
        
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

// Función auxiliar para verificar si una línea está bloqueada
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
	int directions[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};

	for (int d = 0; d < 4; d++)
	{
		int dx = directions[d][0], dy = directions[d][1];

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

void TranspositionSearch::clearCache()
{
	std::fill(transpositionTable.begin(), transpositionTable.end(), CacheEntry());
	currentGeneration = 1; // NUEVO: Reset generación
	std::cout << "TranspositionTable: Cache limpiada (" << transpositionTable.size() << " entradas)" << std::endl;
}

TranspositionSearch::CacheStats TranspositionSearch::getCacheStats() const
{
	CacheStats stats;
	stats.totalEntries = transpositionTable.size();
	stats.usedEntries = 0;
	stats.collisions = 0;
	stats.currentGeneration = currentGeneration;
	stats.exactEntries = 0;
	stats.boundEntries = 0;
	double totalDepth = 0;

	for (const auto &entry : transpositionTable)
	{
		if (entry.zobristKey != 0)
		{
			stats.usedEntries++;
			totalDepth += entry.depth;

			if (entry.type == CacheEntry::EXACT)
			{
				stats.exactEntries++;
			}
			else
			{
				stats.boundEntries++;
			}
		}
	}

	stats.fillRate = static_cast<double>(stats.usedEntries) / stats.totalEntries;
	stats.avgDepth = stats.usedEntries > 0 ? totalDepth / stats.usedEntries : 0.0;

	return stats;
}

void TranspositionSearch::printCacheStats() const
{
	CacheStats stats = getCacheStats();

	std::cout << "=== TRANSPOSITION TABLE STATS ===" << std::endl;
	std::cout << "Total entries: " << stats.totalEntries << std::endl;
	std::cout << "Used entries: " << stats.usedEntries << std::endl;
	std::cout << "Fill rate: " << std::fixed << std::setprecision(2) << (stats.fillRate * 100) << "%" << std::endl;
	std::cout << "Current generation: " << stats.currentGeneration << std::endl;
	std::cout << "Exact entries: " << stats.exactEntries << " ("
			  << std::fixed << std::setprecision(1) << (stats.usedEntries > 0 ? (double)stats.exactEntries / stats.usedEntries * 100 : 0) << "%)" << std::endl;
	std::cout << "Bound entries: " << stats.boundEntries << " ("
			  << std::fixed << std::setprecision(1) << (stats.usedEntries > 0 ? (double)stats.boundEntries / stats.usedEntries * 100 : 0) << "%)" << std::endl;
	std::cout << "Average depth: " << std::fixed << std::setprecision(1) << stats.avgDepth << std::endl;
	std::cout << "Memory usage: " << (stats.totalEntries * sizeof(CacheEntry) / 1024 / 1024) << " MB" << std::endl;
	std::cout << "================================" << std::endl;
}

TranspositionSearch::SearchResult TranspositionSearch::findBestMoveIterative(
	const GameState &state, int maxDepth)
{

	auto startTime = std::chrono::high_resolution_clock::now();
	SearchResult bestResult;

	nodesEvaluated = 0;
	cacheHits = 0;

	// NUEVO: Incrementar generación para aging-based replacement
	currentGeneration++;

	std::cout << "Búsqueda iterativa hasta profundidad " << maxDepth << std::endl;

	// Iterative deepening loop
	for (int depth = 1; depth <= maxDepth; depth++)
	{
		auto iterationStart = std::chrono::high_resolution_clock::now();

		// Usar el mejor movimiento de la iteración anterior como primer candidato
		if (bestResult.bestMove.isValid())
		{
			previousBestMove = bestResult.bestMove;
		}

		Move bestMove;
		int score = minimax(const_cast<GameState &>(state), depth,
							std::numeric_limits<int>::min(),
							std::numeric_limits<int>::max(),
							state.currentPlayer == GameState::PLAYER2,
							depth, &bestMove);

		auto iterationEnd = std::chrono::high_resolution_clock::now();
		auto iterationTime = std::chrono::duration_cast<std::chrono::milliseconds>(iterationEnd - iterationStart);

		// Actualizar resultado
		bestResult.bestMove = bestMove;
		bestResult.score = score;
		bestResult.nodesEvaluated = nodesEvaluated;
		bestResult.cacheHits = cacheHits;
		bestResult.cacheHitRate = nodesEvaluated > 0 ? (float)cacheHits / nodesEvaluated : 0.0f;

		std::cout << "Profundidad " << depth
				  << ": " << char('A' + bestMove.y) << (bestMove.x + 1)
				  << " (score: " << score << ")"
				  << " - " << iterationTime.count() << "ms"
				  << " (" << nodesEvaluated << " nodos, "
				  << std::fixed << std::setprecision(1) << (bestResult.cacheHitRate * 100) << "% cache hit)"
				  << std::endl;

		// Si encontramos mate, podemos parar (opcional)
		if (std::abs(score) > 300000)
		{
			std::cout << "Mate detectado en profundidad " << depth
					  << ", completando búsqueda" << std::endl;
			break;
		}
	}

	auto totalTime = std::chrono::high_resolution_clock::now() - startTime;
	int elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(totalTime).count();

	std::cout << "Búsqueda completada en " << elapsedTime << "ms total" << std::endl;

	// **NUEVO: Debug snapshot igual que en findBestMove**
	if (g_debugAnalyzer)
	{
		DEBUG_CHOSEN_MOVE(bestResult.bestMove, bestResult.score);
		DEBUG_SNAPSHOT(state, elapsedTime, nodesEvaluated);
	}

	return bestResult;
}

std::vector<Move> TranspositionSearch::generateCandidatesAdaptiveRadius(const GameState &state)
{
    std::vector<Move> candidates;
    int searchRadius = getSearchRadiusForGamePhase(state.turnCount);
    
    // OPTIMIZACIÓN: Pre-marcar zonas relevantes
    bool relevantZone[GameState::BOARD_SIZE][GameState::BOARD_SIZE] = {{false}};
    
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

bool TranspositionSearch::isEarlyGamePhase(int pieceCount)
{
	return pieceCount <= 4;
}

bool TranspositionSearch::isCentralStrategicPosition(int x, int y)
{
	int centerDistance = std::max(std::abs(x - 9), std::abs(y - 9));
	return centerDistance <= 2; // Posiciones dentro del área central 5x5
}

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
	int directions[4][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};

	for (int d = 0; d < 4; d++)
	{
		int dx = directions[d][0], dy = directions[d][1];

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
