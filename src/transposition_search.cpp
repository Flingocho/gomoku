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

	// NUEVO: Determinar límite de candidatos según fase del juego
	int maxCandidates = getMaxCandidatesForGamePhase(state);

	// Aplicar move ordering con mejor movimiento de iteración anterior
	orderMovesWithPreviousBest(candidates, state);

	// NUEVO: Limitar a los mejores candidatos
	if (candidates.size() > (size_t)maxCandidates)
	{
		candidates.resize(maxCandidates);
	}

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
	if (moves.size() <= 5)
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
    // OPTIMIZACIÓN: Arrays estáticos para evitar reconstruir en cada llamada
    static constexpr int directions[4][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};
    static constexpr int captureDirections[8][2] = {
        {-1, -1}, {-1, 0}, {-1, 1}, {0, -1}, 
        {0, 1}, {1, -1}, {1, 0}, {1, 1}
    };

    int currentPlayer = state.currentPlayer;
    int opponent = state.getOpponent(currentPlayer);
    int score = 0;

    // 1. OPTIMIZACIÓN: Centralidad y conectividad básica calculadas una sola vez
    int centerDist = std::max(std::abs(move.x - 9), std::abs(move.y - 9));
    score += (9 - centerDist) * 20;

    int connectivity = 0;
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            if (dx == 0 && dy == 0)
                continue;
            int nx = move.x + dx, ny = move.y + dy;
            if (state.isValid(nx, ny)) {
                int piece = state.getPiece(nx, ny);
                if (piece == currentPlayer)
                    connectivity += 50;
                else if (piece == opponent)
                    connectivity += 20;
            }
        }
    }
    score += connectivity;

    // 2. Análisis táctico en 4 direcciones principales
    for (int d = 0; d < 4; d++) {
        int dx = directions[d][0], dy = directions[d][1];

        // Contar piezas consecutivas propias
        int myForward = 0, myBackward = 0;
        int x = move.x + dx, y = move.y + dy;
        while (myForward < 4 && state.isValid(x, y) && state.getPiece(x, y) == currentPlayer) {
            myForward++;
            x += dx;
            y += dy;
        }
        x = move.x - dx;
        y = move.y - dy;
        while (myBackward < 4 && state.isValid(x, y) && state.getPiece(x, y) == currentPlayer) {
            myBackward++;
            x -= dx;
            y -= dy;
        }
        int myTotal = myForward + myBackward + 1;

        // Contar piezas consecutivas del oponente (para bloqueos)
        int oppForward = 0, oppBackward = 0;
        x = move.x + dx;
        y = move.y + dy;
        while (oppForward < 4 && state.isValid(x, y) && state.getPiece(x, y) == opponent) {
            oppForward++;
            x += dx;
            y += dy;
        }
        x = move.x - dx;
        y = move.y - dy;
        while (oppBackward < 4 && state.isValid(x, y) && state.getPiece(x, y) == opponent) {
            oppBackward++;
            x -= dx;
            y -= dy;
        }
        int oppTotal = oppForward + oppBackward;

        // Evaluación de amenazas propias
        if (myTotal >= 5) {
            return 500000; // Victoria inmediata
        }
        else if (myTotal == 4) {
            // Verificar si es amenaza libre (extremos disponibles)
            int startX = move.x - myBackward * dx, startY = move.y - myBackward * dy;
            int endX = move.x + myForward * dx, endY = move.y + myForward * dy;

            bool startFree = state.isValid(startX - dx, startY - dy) &&
                             state.isEmpty(startX - dx, startY - dy);
            bool endFree = state.isValid(endX + dx, endY + dy) &&
                           state.isEmpty(endX + dx, endY + dy);

            if (startFree && endFree)
                score += 50000; // Four abierto
            else if (startFree || endFree)
                score += 25000; // Four semicerrado
        }
        else if (myTotal == 3) {
            // Verificar si es three libre
            int startX = move.x - myBackward * dx, startY = move.y - myBackward * dy;
            int endX = move.x + myForward * dx, endY = move.y + myForward * dy;

            bool startFree = state.isValid(startX - dx, startY - dy) &&
                             state.isEmpty(startX - dx, startY - dy);
            bool endFree = state.isValid(endX + dx, endY + dy) &&
                           state.isEmpty(endX + dx, endY + dy);

            if (startFree && endFree)
                score += 10000; // Three abierto
            else if (startFree || endFree)
                score += 1500; // Three semicerrado
        }
        else if (myTotal == 2) {
            score += 100; // Desarrollo básico
        }

        // Evaluación defensiva (bloquear amenazas del oponente)
        if (oppTotal >= 5) {
            return 500000; // Victoria inmediata
        }
        else if (oppTotal == 4) {
            // Verificar si es amenaza libre (extremos disponibles)
            int startX = move.x - oppBackward * dx, startY = move.y - oppBackward * dy;
            int endX = move.x + oppForward * dx, endY = move.y + oppForward * dy;

            bool startFree = state.isValid(startX - dx, startY - dy) &&
                             state.isEmpty(startX - dx, startY - dy);
            bool endFree = state.isValid(endX + dx, endY + dy) &&
                           state.isEmpty(endX + dx, endY + dy);

            if (startFree && endFree)
                score += 50000; // Four abierto
            else if (startFree || endFree)
                score += 25000; // Four semicerrado
        }
        else if (oppTotal == 3) {
            // Verificar si es three libre
            int startX = move.x - oppBackward * dx, startY = move.y - oppBackward * dy;
            int endX = move.x + oppForward * dx, endY = move.y + oppForward * dy;

            bool startFree = state.isValid(startX - dx, startY - dy) &&
                             state.isEmpty(startX - dx, startY - dy);
            bool endFree = state.isValid(endX + dx, endY + dy) &&
                           state.isEmpty(endX + dx, endY + dy);

            if (startFree && endFree)
                score += 10000; // Three abierto
            else if (startFree || endFree)
                score += 1500; // Three semicerrado
        }
        else if (oppTotal == 2) {
            score += 100; // Desarrollo básico
        }
    }

    // 3. Evaluación de capturas potenciales (todas las 8 direcciones)
    int captureScore = 0;

    for (int d = 0; d < 8; d++) {
        int dx = captureDirections[d][0], dy = captureDirections[d][1];

        // Patrón de captura hacia adelante: NUEVA + OPP + OPP + MIA
        if (state.isValid(move.x + dx, move.y + dy) &&
            state.isValid(move.x + 2 * dx, move.y + 2 * dy) &&
            state.isValid(move.x + 3 * dx, move.y + 3 * dy)) {

            if (state.getPiece(move.x + dx, move.y + dy) == opponent &&
                state.getPiece(move.x + 2 * dx, move.y + 2 * dy) == opponent &&
                state.getPiece(move.x + 3 * dx, move.y + 3 * dy) == currentPlayer) {
                captureScore += 2500;
            }
        }

        // Patrón de captura hacia atrás: MIA + OPP + OPP + NUEVA
        if (state.isValid(move.x - dx, move.y - dy) &&
            state.isValid(move.x - 2 * dx, move.y - 2 * dy) &&
            state.isValid(move.x - 3 * dx, move.y - 3 * dy)) {

            if (state.getPiece(move.x - dx, move.y - dy) == opponent &&
                state.getPiece(move.x - 2 * dx, move.y - 2 * dy) == opponent &&
                state.getPiece(move.x - 3 * dx, move.y - 3 * dy) == currentPlayer) {
                captureScore += 2500;
            }
        }
    }

    // OPTIMIZACIÓN: Escalar capturas según situación del juego (calculado una vez)
    int myCaptures = state.captures[currentPlayer - 1];
    if (myCaptures >= 8)
        captureScore *= 10; // Cerca de ganar por capturas
    else if (myCaptures >= 6)
        captureScore *= 5;
    else if (myCaptures >= 4)
        captureScore *= 2;

    score += captureScore;

    // 4. Evaluación de amenazas críticas del oponente
    bool opponentHasThreats = false;
    for (int d = 0; d < 4; d++) {
        int dx = directions[d][0], dy = directions[d][1];

        // Buscar patrones peligrosos del oponente alrededor de esta posición
        for (int offset = -3; offset <= 3; offset++) {
            if (offset == 0)
                continue; // Skip la posición del movimiento

            int checkX = move.x + offset * dx;
            int checkY = move.y + offset * dy;

            if (state.isValid(checkX, checkY) && state.getPiece(checkX, checkY) == opponent) {
                int oppConsecutive = 1;

                // Contar hacia adelante
                int x = checkX + dx, y = checkY + dy;
                while (oppConsecutive < 4 && state.isValid(x, y) && state.getPiece(x, y) == opponent) {
                    oppConsecutive++;
                    x += dx;
                    y += dy;
                }

                // Contar hacia atrás
                x = checkX - dx;
                y = checkY - dy;
                while (oppConsecutive < 4 && state.isValid(x, y) && state.getPiece(x, y) == opponent) {
                    oppConsecutive++;
                    x -= dx;
                    y -= dy;
                }

                if (oppConsecutive >= 3) {
                    opponentHasThreats = true;
                    break;
                }
            }
        }
        if (opponentHasThreats)
            break;
    }

    // Si el oponente tiene amenazas, priorizar movimientos defensivos
    if (opponentHasThreats) {
        // Verificar si este movimiento interrumpe alguna amenaza
        bool blocksThreats = false;
        for (int d = 0; d < 4; d++) {
            int dx = directions[d][0], dy = directions[d][1];

            int oppBefore = 0, oppAfter = 0;
            int x = move.x - dx, y = move.y - dy;
            while (oppBefore < 4 && state.isValid(x, y) && state.getPiece(x, y) == opponent) {
                oppBefore++;
                x -= dx;
                y -= dy;
            }
            x = move.x + dx;
            y = move.y + dy;
            while (oppAfter < 4 && state.isValid(x, y) && state.getPiece(x, y) == opponent) {
                oppAfter++;
                x += dx;
                y += dy;
            }

            if (oppBefore + oppAfter >= 2) {
                blocksThreats = true;
                break;
            }
        }

        if (blocksThreats)
            score += 30000; // Bloqueo defensivo crucial
        else
            score -= 30000; // Penalizar movimientos que no defienden
    }

    return score;
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
		if (std::abs(score) > 200000)
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

	int pieceCount = state.turnCount;
	int searchRadius = getSearchRadiusForGamePhase(pieceCount);
	int opponent = state.getOpponent(state.currentPlayer);

	// OPTIMIZACIÓN: Calcular una sola vez al inicio
	bool opponentHasThreats = Evaluator::hasWinningThreats(state, opponent);

	// NUEVO: Agregar todas las casillas vacías alrededor del último movimiento humano
	addCandidatesAroundLastHumanMove(candidates, state);

	for (int i = 0; i < GameState::BOARD_SIZE; i++)
	{
		for (int j = 0; j < GameState::BOARD_SIZE; j++)
		{
			if (!state.isEmpty(i, j))
				continue;

			// Verificar si está dentro del radio de influencia de alguna pieza
			bool withinInfluenceRadius = false;

			for (int pi = 0; pi < GameState::BOARD_SIZE && !withinInfluenceRadius; pi++)
			{
				for (int pj = 0; pj < GameState::BOARD_SIZE && !withinInfluenceRadius; pj++)
				{
					if (state.getPiece(pi, pj) != GameState::EMPTY)
					{
						int distance = std::max(std::abs(i - pi), std::abs(j - pj));
						if (distance <= searchRadius)
						{
							withinInfluenceRadius = true;
						}
					}
				}
			}

			// AMPLIADO: Lógica mejorada para amenazas críticas
			if (!withinInfluenceRadius && opponentHasThreats)
			{
				GameState tempState = state;

				// CASO 1: Verificar si este movimiento bloquea amenazas del oponente
				RuleEngine::MoveResult blockResult = RuleEngine::applyMove(tempState, Move(i, j));
				if (blockResult.success)
				{
					bool stillHasThreats = Evaluator::hasWinningThreats(tempState, opponent);
					if (!stillHasThreats)
					{
						withinInfluenceRadius = true; // Es un bloqueo crítico
					}
				}

				// CASO 2: NUEVO - Verificar si es un movimiento ganador del oponente
				if (!withinInfluenceRadius)
				{
					GameState opponentTempState = state;
					opponentTempState.currentPlayer = opponent; // Simular turno del oponente

					RuleEngine::MoveResult winResult = RuleEngine::applyMove(opponentTempState, Move(i, j));
					if (winResult.success && winResult.createsWin)
					{
						withinInfluenceRadius = true; // Es un movimiento ganador del oponente - DEBE ser evaluado
					}
				}

				// CASO 3: NUEVO - Verificar si crea nuevas amenazas críticas para el oponente
				if (!withinInfluenceRadius)
				{
					GameState opponentTempState = state;
					opponentTempState.currentPlayer = opponent;

					RuleEngine::MoveResult threatResult = RuleEngine::applyMove(opponentTempState, Move(i, j));
					if (threatResult.success)
					{
						// Restaurar turno para evaluación correcta
						opponentTempState.currentPlayer = state.currentPlayer;
						bool createsNewThreats = Evaluator::hasWinningThreats(opponentTempState, opponent);

						// Si el oponente ya tenía amenazas y este movimiento crea más
						if (createsNewThreats)
						{
							withinInfluenceRadius = true; // Movimiento que amplía amenazas del oponente
						}
					}
				}
			}

			// En opening, también considerar movimientos centrales estratégicos
			if (!withinInfluenceRadius && isEarlyGamePhase(pieceCount))
			{
				if (isCentralStrategicPosition(i, j))
				{
					withinInfluenceRadius = true;
				}
			}

// NUEVO: Para debugging - forzar inclusión de movimientos específicos sospechosos
// (esto se puede quitar en producción, pero ayuda a debuggear)
#ifdef DEBUG_CANDIDATE_GENERATION
			// Si estamos en una situación de 4 en línea, forzar inclusión de completadores
			if (!withinInfluenceRadius)
			{
				// Verificar si este movimiento está adyacente a secuencias largas
				bool adjacentToLongSequence = false;
				int directions[4][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};

				for (int d = 0; d < 4; d++)
				{
					int dx = directions[d][0], dy = directions[d][1];

					// Contar en ambas direcciones para cualquier jugador
					for (int player = 1; player <= 2; player++)
					{
						int count = 0;
						count += countPiecesInDirection(state, i, j, dx, dy, player);
						count += countPiecesInDirection(state, i, j, -dx, -dy, player);

						if (count >= 3)
						{ // Secuencia de 3+ piezas
							adjacentToLongSequence = true;
							break;
						}
					}
					if (adjacentToLongSequence)
						break;
				}

				if (adjacentToLongSequence)
				{
					withinInfluenceRadius = true;
				}
			}
#endif

			if (withinInfluenceRadius && RuleEngine::isLegalMove(state, Move(i, j)))
			{
				candidates.push_back(Move(i, j));
			}
		}
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
		return 5; // Early game: moderadamente selectivo
	}
	else
	{
		return 8; // Mid/late game: más opciones disponibles
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
