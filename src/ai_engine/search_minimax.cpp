// ============================================
// SEARCH_MINIMAX.CPP
// Implementa el algoritmo minimax con alpha-beta pruning
// Incluye búsqueda iterativa con deepening
// ============================================

#include "../../include/ai/transposition_search.hpp"
#include "../../include/debug/debug_analyzer.hpp"
#include <algorithm>
#include <limits>
#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>

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

TranspositionSearch::SearchResult TranspositionSearch::findBestMoveIterative(
    const GameState &state, int maxDepth)
{
    auto startTime = std::chrono::high_resolution_clock::now();
    SearchResult bestResult;

    nodesEvaluated = 0;
    cacheHits = 0;
    currentGeneration++;

    std::cout << "Búsqueda iterativa hasta profundidad " << maxDepth << std::endl;

    // ============================================
    // NUEVO: PRE-CHECK - Detectar victoria inmediata
    // ============================================
    std::vector<Move> allCandidates = generateCandidatesAdaptiveRadius(state);
    
    for (const Move& move : allCandidates) {
        GameState testState = state;
        RuleEngine::MoveResult result = RuleEngine::applyMove(testState, move);
        
        if (!result.success) continue;
        
        // ¿Este movimiento gana INMEDIATAMENTE?
        if (RuleEngine::checkWin(testState, state.currentPlayer) ||
            testState.captures[state.currentPlayer - 1] >= 10) {
            
            auto endTime = std::chrono::high_resolution_clock::now();
            int elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                endTime - startTime).count();
            
            SearchResult winResult;
            winResult.bestMove = move;
            winResult.score = Evaluator::WIN; // Usar constante del evaluador
            winResult.nodesEvaluated = allCandidates.size();
            winResult.cacheHits = 0;
            winResult.cacheHitRate = 0.0f;
            
            std::cout << "¡VICTORIA INMEDIATA detectada en " 
                      << char('A' + move.y) << (move.x + 1) 
                      << " en " << elapsedTime << "ms!" << std::endl;
            
            if (g_debugAnalyzer) {
                DEBUG_CHOSEN_MOVE(move, winResult.score);
                DEBUG_SNAPSHOT(state, elapsedTime, allCandidates.size());
            }
            
            return winResult;
        }
    }
    
    std::cout << "No hay victoria inmediata, iniciando búsqueda iterativa..." << std::endl;
    
    // ============================================
    // Iterative deepening normal
    // ============================================
    for (int depth = 1; depth <= maxDepth; depth++)
    {
        auto iterationStart = std::chrono::high_resolution_clock::now();

        if (bestResult.bestMove.isValid()) {
            previousBestMove = bestResult.bestMove;
        }

        Move bestMove;
        int score = minimax(const_cast<GameState &>(state), depth,
                            std::numeric_limits<int>::min(),
                            std::numeric_limits<int>::max(),
                            state.currentPlayer == GameState::PLAYER2,
                            depth, &bestMove);

        auto iterationEnd = std::chrono::high_resolution_clock::now();
        auto iterationTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            iterationEnd - iterationStart);

        bestResult.bestMove = bestMove;
        bestResult.score = score;
        bestResult.nodesEvaluated = nodesEvaluated;
        bestResult.cacheHits = cacheHits;
        bestResult.cacheHitRate = nodesEvaluated > 0 ?
            (float)cacheHits / nodesEvaluated : 0.0f;

        std::cout << "Profundidad " << depth
                  << ": " << char('A' + bestMove.y) << (bestMove.x + 1)
                  << " (score: " << score << ")"
                  << " - " << iterationTime.count() << "ms"
                  << " (" << nodesEvaluated << " nodos, "
                  << std::fixed << std::setprecision(1) 
                  << (bestResult.cacheHitRate * 100) << "% cache hit)"
                  << std::endl;

        // MANTENER threshold original
        if (std::abs(score) > 300000) {
            std::cout << "Mate detectado en profundidad " << depth
                      << ", completando búsqueda" << std::endl;
            break;
        }
    }

    auto totalTime = std::chrono::high_resolution_clock::now() - startTime;
    int elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        totalTime).count();

    std::cout << "Búsqueda completada en " << elapsedTime << "ms total" << std::endl;

    if (g_debugAnalyzer) {
        DEBUG_CHOSEN_MOVE(bestResult.bestMove, bestResult.score);
        DEBUG_SNAPSHOT(state, elapsedTime, nodesEvaluated);
    }

    return bestResult;
}
