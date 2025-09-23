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

TranspositionSearch::SearchResult TranspositionSearch::findBestMove(const GameState &state, int maxDepth) {
    SearchResult result;
    nodesEvaluated = 0;
    cacheHits = 0;
    
    // NUEVO: Incrementar generación para aging-based replacement
    currentGeneration++;

    // Profundidad adaptativa
    int adaptiveDepth = calculateAdaptiveDepth(state, maxDepth);

    DEBUG_LOG_AI("AI usando profundidad: " + std::to_string(adaptiveDepth) + " (solicitada: " + std::to_string(maxDepth) + ")");

    // **NUEVO: Análisis de movimientos para debug**
    if (g_debugAnalyzer) {
        std::vector<Move> candidateMoves = generateOrderedMoves(state);
        
        // Analizar cada movimiento candidato
        for (const Move& move : candidateMoves) {
            if (candidateMoves.size() > 10) break; // Limitar para performance
            
            auto breakdown = DebugAnalyzer::evaluateWithBreakdown(state, move, state.currentPlayer);
            DEBUG_ROOT_MOVE(move, breakdown.totalScore, breakdown);
        }
    }

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
    if (g_debugAnalyzer) {
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
		DEBUG_LOG_STATS("Nodos evaluados: " + std::to_string(nodesEvaluated) + ", Cache hits: " + std::to_string(cacheHits));
	}

	// OPTIMIZACIÓN CLAVE: Usar Zobrist hash del estado
	uint64_t zobristKey = state.getZobristHash();

	// OPTIMIZACIÓN: Verificar transposition table PRIMERO con mejor validación
	CacheEntry entry;
	bool foundInCache = lookupTransposition(zobristKey, entry);
	
	if (foundInCache) {
		cacheHits++;
		
		// CRÍTICO: Solo usar si la profundidad es suficiente
		if (entry.depth >= depth) {
			if (entry.type == CacheEntry::EXACT) {
				if (bestMove)
					*bestMove = entry.bestMove;
				return entry.score;
			}
			else if (entry.type == CacheEntry::LOWER_BOUND && entry.score >= beta) {
				return beta; // Beta cutoff del cache
			}
			else if (entry.type == CacheEntry::UPPER_BOUND && entry.score <= alpha) {
				return alpha; // Alpha cutoff del cache
			}
		}
		
		// NUEVO: Incluso si profundidad es insuficiente, usar el mejor movimiento para ordering
		if (entry.bestMove.isValid() && bestMove && depth == originalMaxDepth) {
			// Hint para move ordering en nivel raíz
			previousBestMove = entry.bestMove;
		}
	}

	// CASOS BASE - AQUÍ ESTÁ EL CAMBIO CRÍTICO
	if (depth == 0 || RuleEngine::checkWin(state, GameState::PLAYER1) ||
		RuleEngine::checkWin(state, GameState::PLAYER2))
	{

		// ¡NUEVO! Usar evaluador que considera distancia al mate
		int score = Evaluator::evaluate(state, originalMaxDepth, originalMaxDepth - depth);
		storeTransposition(zobristKey, score, depth, Move(), CacheEntry::EXACT);
		return score;
	}

	// Generar movimientos ordenados
	std::vector<Move> moves = generateOrderedMoves(state);
	if (moves.empty())
	{
		// ¡NUEVO! También aquí usar evaluador con distancia
		int score = Evaluator::evaluate(state, originalMaxDepth, originalMaxDepth - depth);
		storeTransposition(zobristKey, score, depth, Move(), CacheEntry::EXACT);
		return score;
	}

	Move currentBestMove;
	int originalAlpha = alpha;

	if (maximizing)
	{
		int maxEval = std::numeric_limits<int>::min();
		bool isRootLevel = (depth == originalMaxDepth);

		for (const Move &move : moves)
		{
			// CRÍTICO: En nivel raíz, verificar victoria inmediata ANTES de minimax
			if (isRootLevel) {
				int quickScore = quickEvaluateMove(state, move);
				if (quickScore >= 100000) { // Victoria inmediata detectada
					// Aplicar movimiento para verificar
					GameState newState = state;
					RuleEngine::MoveResult result = RuleEngine::applyMove(newState, move);
					
					if (result.success && (result.createsWin || RuleEngine::checkWin(newState, state.currentPlayer))) {
						// VICTORIA INMEDIATA: Elegir inmediatamente sin más evaluación
						maxEval = 100000 + (originalMaxDepth - depth + 1);
						currentBestMove = move;
						
						if (g_debugAnalyzer) {
							std::ostringstream debugMsg;
							debugMsg << "ROOT LEVEL: IMMEDIATE WIN DETECTED - Move " << char('A' + move.y) << (move.x + 1)
									 << " with score " << maxEval << " (GAME OVER)\n";
							g_debugAnalyzer->logToFile(debugMsg.str());
						}
						
						// TERMINAR BÚSQUEDA INMEDIATAMENTE
						break;
					}
				}
			}
			
			GameState newState = state;
			RuleEngine::MoveResult result = RuleEngine::applyMove(newState, move);

			if (!result.success)
				continue;

			// PASAR originalMaxDepth en recursión
			int eval = minimax(newState, depth - 1, alpha, beta, false, originalMaxDepth);

			// DEBUG: Log movimientos en nivel raíz
			if (isRootLevel && g_debugAnalyzer) {
				std::ostringstream debugMsg;
				debugMsg << "ROOT LEVEL: Move " << char('A' + move.y) << (move.x + 1)
						 << " evaluated to " << eval << "\n";
				g_debugAnalyzer->logToFile(debugMsg.str());
			}

			if (eval > maxEval)
			{
				maxEval = eval;
				currentBestMove = move;
				
				// DEBUG: Log cuando cambia el mejor movimiento
				if (isRootLevel && g_debugAnalyzer) {
					std::ostringstream debugMsg;
					debugMsg << "ROOT LEVEL: NEW BEST MOVE " << char('A' + move.y) << (move.x + 1)
							 << " with score " << eval << "\n";
					g_debugAnalyzer->logToFile(debugMsg.str());
				}

				// Cutoff inmediato si superamos beta
				if (eval >= beta)
				{
					break; // Beta cutoff
				}

				alpha = std::max(alpha, eval);
			}
		}

		// Almacenar en transposition table
		CacheEntry::Type entryType;
		if (maxEval <= originalAlpha)
		{
			entryType = CacheEntry::UPPER_BOUND;
		}
		else if (maxEval >= beta)
		{
			entryType = CacheEntry::LOWER_BOUND;
		}
		else
		{
			entryType = CacheEntry::EXACT;
		}

		storeTransposition(zobristKey, maxEval, depth, currentBestMove, entryType);

		if (bestMove)
			*bestMove = currentBestMove;
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

			// PASAR originalMaxDepth en recursión
			int eval = minimax(newState, depth - 1, alpha, beta, true, originalMaxDepth);

			if (eval < minEval)
			{
				minEval = eval;
				currentBestMove = move;
			}

			beta = std::min(beta, eval);
			if (eval <= alpha)
				break; // Poda alpha-beta
		}

		// Almacenar en transposition table
		CacheEntry::Type entryType;
		if (minEval <= originalAlpha)
		{
			entryType = CacheEntry::UPPER_BOUND;
		}
		else if (minEval >= beta)
		{
			entryType = CacheEntry::LOWER_BOUND;
		}
		else
		{
			entryType = CacheEntry::EXACT;
		}

		storeTransposition(zobristKey, minEval, depth, currentBestMove, entryType);

		if (bestMove)
			*bestMove = currentBestMove;
		return minEval;
	}
}

bool TranspositionSearch::lookupTransposition(uint64_t zobristKey, CacheEntry &entry)
{
	size_t index = zobristKey & tableSizeMask; // O(1) access!
	const CacheEntry& candidate = transpositionTable[index];

	// Verificar si hay entrada válida
	if (candidate.zobristKey == 0) {
		return false; // Entrada vacía
	}

	// Verificar coincidencia exacta de hash
	if (candidate.zobristKey == zobristKey) {
		entry = candidate;
		
		// NUEVO: Actualizar generación para LRU mejorado
		if (candidate.generation != currentGeneration) {
			// Actualizar generación sin cambiar otros datos
			transpositionTable[index].generation = currentGeneration;
		}
		
		return true;
	}

	// Hash collision detectada - no usar la entrada
	return false;
}

void TranspositionSearch::storeTransposition(uint64_t zobristKey, int score, int depth,
											 Move bestMove, CacheEntry::Type type)
{
	size_t index = zobristKey & tableSizeMask;
	CacheEntry& existing = transpositionTable[index];
	
	// NUEVA ESTRATEGIA: Reemplazo inteligente basado en importancia
	bool shouldReplace = false;
	
	if (existing.zobristKey == 0) {
		// Entrada vacía - siempre reemplazar
		shouldReplace = true;
	}
	else if (existing.zobristKey == zobristKey) {
		// Misma posición - actualizar si profundidad es mayor o igual
		shouldReplace = (depth >= existing.depth);
	}
	else {
		// Colisión de hash - usar estrategia de reemplazo sofisticada
		CacheEntry newEntry(zobristKey, score, depth, bestMove, type, currentGeneration);
		
		// Calcular valores de importancia
		int existingImportance = existing.getImportanceValue();
		int newImportance = newEntry.getImportanceValue();
		
		// Factor de aging: entradas más viejas tienen menor prioridad
		uint32_t ageDiff = currentGeneration - existing.generation;
		if (ageDiff > 0) {
			existingImportance -= (ageDiff * 10); // Penalizar entradas viejas
		}
		
		// Reemplazar si la nueva entrada es más importante
		shouldReplace = (newImportance > existingImportance);
		
		// Bias hacia entradas EXACT si hay empate
		if (newImportance == existingImportance && type == CacheEntry::EXACT) {
			shouldReplace = true;
		}
	}
	
	if (shouldReplace) {
		transpositionTable[index] = CacheEntry(zobristKey, score, depth, bestMove, type, currentGeneration);
	}
}

std::vector<Move> TranspositionSearch::generateOrderedMoves(const GameState& state) {
    // NUEVO: Generar candidatos con filtrado adaptativo por fase de juego
    std::vector<Move> candidates = generateCandidatesAdaptiveRadius(state);
    
    // NUEVO: Determinar límite de candidatos según fase del juego
    int maxCandidates = getMaxCandidatesForGamePhase(state);
    
    // NUEVO: Ordenar candidatos con evaluación geométrica rápida
    orderMovesByGeometricValue(candidates, state);
    
    // NUEVO: Limitar a los mejores candidatos
    if (candidates.size() > (size_t)maxCandidates) {
        candidates.resize(maxCandidates);
    }
    
    // Aplicar move ordering con mejor movimiento de iteración anterior
    orderMovesWithPreviousBest(candidates, state);
    
    return candidates;
}

void TranspositionSearch::orderMovesWithPreviousBest(std::vector<Move>& moves, const GameState& state) {
    // Si tenemos mejor movimiento de iteración anterior, ponerlo primero
    if (previousBestMove.isValid()) {
        auto it = std::find_if(moves.begin(), moves.end(), 
                              [this](const Move& m) { 
                                  return m.x == previousBestMove.x && m.y == previousBestMove.y; 
                              });
        
        if (it != moves.end()) {
            // Mover al frente
            std::iter_swap(moves.begin(), it);
            
            // Ordenar el resto normalmente
            if (moves.size() > 1) {
                std::sort(moves.begin() + 1, moves.end(), [&](const Move& a, const Move& b) {
                    return quickEvaluateMove(state, a) > quickEvaluateMove(state, b);
                });
            }
            
            return;
        }
    }
    
    // Si no hay movimiento anterior, orden normal
    orderMoves(moves, state);
}

void TranspositionSearch::orderMoves(std::vector<Move> &moves, const GameState &state)
{
	std::sort(moves.begin(), moves.end(), [&](const Move &a, const Move &b)
			  { return quickEvaluateMove(state, a) > quickEvaluateMove(state, b); });
}

int TranspositionSearch::quickEvaluateMove(const GameState& state, const Move& move) {
    int currentPlayer = state.currentPlayer;
    int opponent = state.getOpponent(currentPlayer);
    
    // PASO 1: Evaluación ultrarrápida sin copiar estado
    GameState tempState = state;
    RuleEngine::MoveResult result = RuleEngine::applyMove(tempState, move);
    
    if (!result.success) {
        return -10000; // Movimiento ilegal
    }
    
    // PASO 2: Verificar condiciones críticas primero (alineado con evaluate())
    
    // Victoria inmediata = máxima prioridad (alineado con Evaluator::WIN)
    if (result.createsWin || RuleEngine::checkWin(tempState, currentPlayer)) {
        return 100000 + (9 - std::max(std::abs(move.x - 9), std::abs(move.y - 9))); // WIN + bonificación de centrado
    }
    
    // NUEVA LÓGICA EFICIENTE: Verificar amenazas del oponente
    bool opponentHasThreats = Evaluator::hasWinningThreats(state, opponent);
    if (opponentHasThreats) {
        // Evaluar si este movimiento reduce las amenazas
        GameState tempState = state;
        RuleEngine::MoveResult moveResult = RuleEngine::applyMove(tempState, move);
        
        if (moveResult.success) {
            bool stillHasThreats = Evaluator::hasWinningThreats(tempState, opponent);
            
            if (!stillHasThreats) {
                return 40000; // SUPERVIVENCIA - neutralizó amenazas
            } else {
                return -30000; // PARCIAL - aún hay amenazas
            }
        } else {
            return -50000; // SUICIDIO - movimiento ilegal con amenazas
        }
    }
    
    // PASO 3: Evaluar capturas (alineado con evaluateCaptures())
    int score = 0;
    
    // Capturas propias - usar escalas similares a Evaluator
    int myCaptureValue = result.myCapturedPieces.size() * 1000;
    if (tempState.captures[currentPlayer - 1] >= 8) myCaptureValue *= 10; // Cerca de ganar por capturas
    else if (tempState.captures[currentPlayer - 1] >= 6) myCaptureValue *= 5;
    else if (tempState.captures[currentPlayer - 1] >= 4) myCaptureValue *= 2;
    score += myCaptureValue;
    
    // Capturas del oponente - penalizar
    int oppCaptureValue = result.opponentCapturedPieces.size() * 1000;
    if (tempState.captures[opponent - 1] >= 8) oppCaptureValue *= 10;
    else if (tempState.captures[opponent - 1] >= 6) oppCaptureValue *= 5;
    else if (tempState.captures[opponent - 1] >= 4) oppCaptureValue *= 2;
    score -= oppCaptureValue;
    
    // PASO 4: Evaluación de amenazas inmediatas (simplificada de evaluateImmediateThreats)
    
    // Buscar amenazas de 4 en línea (cuasi-victoria)
    int directions[4][2] = {{1,0}, {0,1}, {1,1}, {1,-1}};
    
    for (int d = 0; d < 4; d++) {
        int dx = directions[d][0], dy = directions[d][1];
        
        // Contar piezas propias en ambas direcciones
        int forward = countInDirection(tempState, move.x, move.y, dx, dy, currentPlayer);
        int backward = countInDirection(tempState, move.x, move.y, -dx, -dy, currentPlayer);
        int myTotal = forward + backward + 1; // +1 por la pieza recién colocada
        
        // Contar piezas del oponente (para detectar bloqueos)
        int oppForward = countInDirection(state, move.x, move.y, dx, dy, opponent);
        int oppBackward = countInDirection(state, move.x, move.y, -dx, -dy, opponent);
        int oppTotal = oppForward + oppBackward;
        
        // Amenazas propias (alineado con Evaluator::patternToScore)
        if (myTotal >= 4) {
            // Verificar si es una amenaza abierta o semicerrada
            bool blocked1 = isBlocked(tempState, move.x, move.y, dx, dy, forward + 1, currentPlayer);
            bool blocked2 = isBlocked(tempState, move.x, move.y, -dx, -dy, backward + 1, currentPlayer);
            
            if (!blocked1 && !blocked2) {
                score += 50000; // FOUR_OPEN - imparable
            } else {
                score += 10000; // FOUR_HALF - amenaza forzada
            }
        } else if (myTotal == 3) {
            // Verificar si es una amenaza real (no bloqueada)
            bool blocked1 = isBlocked(tempState, move.x, move.y, dx, dy, forward + 1, currentPlayer);
            bool blocked2 = isBlocked(tempState, move.x, move.y, -dx, -dy, backward + 1, currentPlayer);
            
            if (!blocked1 && !blocked2) {
                score += 5000; // THREE_OPEN - muy peligroso
            } else if (!blocked1 || !blocked2) {
                score += 1500; // THREE_HALF - amenaza
            }
        } else if (myTotal == 2) {
            score += 100; // TWO_OPEN - desarrollo
        }
        
        // Bloquear amenazas del oponente (defensivo) - valores alineados
        if (oppTotal >= 4) {
            score += 40000; // Bloquear four - crítico
        } else if (oppTotal >= 3) {
            score += 8000; // Bloquear three - importante
        } else if (oppTotal >= 2) {
            score += 200; // Bloquear desarrollo - básico
        }
    }
    
    // PASO 5: Evaluación posicional básica (alineada con analyzePosition simplificado)
    
    // Proximidad al centro (tableros favorecen centro)
    int centerDist = std::max(std::abs(move.x - 9), std::abs(move.y - 9));
    score += (9 - centerDist) * 20;
    
    // Conectividad con piezas propias (importante para patrones)
    int connectivity = 0;
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            if (dx == 0 && dy == 0) continue;
            int nx = move.x + dx, ny = move.y + dy;
            if (state.isValid(nx, ny) && state.getPiece(nx, ny) == currentPlayer) {
                connectivity += 50; // Bonificación por conexión directa
            }
        }
    }
    score += connectivity;
    
    return score;
}

// Función auxiliar para verificar si una línea está bloqueada
bool TranspositionSearch::isBlocked(const GameState& state, int x, int y, int dx, int dy, int steps, int player) {
    int nx = x + dx * steps;
    int ny = y + dy * steps;
    
    if (!state.isValid(nx, ny)) {
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
			
			if (entry.type == CacheEntry::EXACT) {
				stats.exactEntries++;
			} else {
				stats.boundEntries++;
			}
		}
	}

	stats.fillRate = static_cast<double>(stats.usedEntries) / stats.totalEntries;
	stats.avgDepth = stats.usedEntries > 0 ? totalDepth / stats.usedEntries : 0.0;

	return stats;
}

void TranspositionSearch::printCacheStats() const {
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
    const GameState& state, int maxDepth) {
    
    auto startTime = std::chrono::high_resolution_clock::now();
    SearchResult bestResult;
    
    nodesEvaluated = 0;
    cacheHits = 0;
    
    // NUEVO: Incrementar generación para aging-based replacement
    currentGeneration++;
    
    std::cout << "Búsqueda iterativa hasta profundidad " << maxDepth << std::endl;
    
    // Iterative deepening loop
    for (int depth = 1; depth <= maxDepth; depth++) {
        auto iterationStart = std::chrono::high_resolution_clock::now();
        
        // Usar el mejor movimiento de la iteración anterior como primer candidato
        if (bestResult.bestMove.isValid()) {
            previousBestMove = bestResult.bestMove;
        }
        
        Move bestMove;
        int score = minimax(const_cast<GameState&>(state), depth,
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
        if (std::abs(score) > 90000) {
            std::cout << "Mate detectado en profundidad " << depth 
                      << ", completando búsqueda" << std::endl;
            break;
        }
    }
    
    auto totalTime = std::chrono::high_resolution_clock::now() - startTime;
    std::cout << "Búsqueda completada en " 
              << std::chrono::duration_cast<std::chrono::milliseconds>(totalTime).count() 
              << "ms total" << std::endl;
    
    return bestResult;
}

// En transposition_search.cpp:
std::vector<Move> TranspositionSearch::generateCandidatesAdaptiveRadius(const GameState& state) {
    std::vector<Move> candidates;
    
    int pieceCount = state.turnCount;
    int searchRadius = getSearchRadiusForGamePhase(pieceCount);
    int opponent = state.getOpponent(state.currentPlayer);
    
    // NUEVO: Detectar si el oponente tiene amenazas críticas
    bool opponentHasThreats = Evaluator::hasWinningThreats(state, opponent);
    
    for (int i = 0; i < GameState::BOARD_SIZE; i++) {
        for (int j = 0; j < GameState::BOARD_SIZE; j++) {
            if (!state.isEmpty(i, j)) continue;
            
            // Verificar si está dentro del radio de influencia de alguna pieza
            bool withinInfluenceRadius = false;
            
            for (int pi = 0; pi < GameState::BOARD_SIZE && !withinInfluenceRadius; pi++) {
                for (int pj = 0; pj < GameState::BOARD_SIZE && !withinInfluenceRadius; pj++) {
                    if (state.getPiece(pi, pj) != GameState::EMPTY) {
                        int distance = std::max(std::abs(i - pi), std::abs(j - pj));
                        if (distance <= searchRadius) {
                            withinInfluenceRadius = true;
                        }
                    }
                }
            }
            
            // NUEVO: Si hay amenazas críticas, FORZAR inclusión de movimientos defensivos
            if (!withinInfluenceRadius && opponentHasThreats) {
                // Simular movimiento para ver si bloquea amenazas
                GameState tempState = state;
                RuleEngine::MoveResult result = RuleEngine::applyMove(tempState, Move(i, j));
                
                if (result.success) {
                    bool stillHasThreats = Evaluator::hasWinningThreats(tempState, opponent);
                    if (!stillHasThreats) {
                        withinInfluenceRadius = true; // FORZAR inclusión - es un bloqueo crítico
                    }
                }
            }
            
            // En opening, también considerar movimientos centrales estratégicos
            if (!withinInfluenceRadius && isEarlyGamePhase(pieceCount)) {
                if (isCentralStrategicPosition(i, j)) {
                    withinInfluenceRadius = true;
                }
            }
            
            if (withinInfluenceRadius && RuleEngine::isLegalMove(state, Move(i, j))) {
                candidates.push_back(Move(i, j));
            }
        }
    }
    
    return candidates;
}

int TranspositionSearch::getSearchRadiusForGamePhase(int pieceCount) {
    if (pieceCount <= 2) {
        return 3; // Opening: radio amplio para explorar
    } else if (pieceCount <= 8) {
        return 2; // Early game: radio moderado
    } else {
        return 1; // Mid/late game: radio enfocado
    }
}

int TranspositionSearch::getMaxCandidatesForGamePhase(const GameState& state) {
    int pieceCount = state.turnCount;
    
    if (pieceCount <= 4) {
        return 8;  // Opening: muy selectivo para evitar explosion combinatoria
    } else if (pieceCount <= 10) {
        return 10; // Early game: moderadamente selectivo  
    } else {
        return 12; // Mid/late game: más opciones disponibles
    }
}

bool TranspositionSearch::isEarlyGamePhase(int pieceCount) {
    return pieceCount <= 4;
}

bool TranspositionSearch::isCentralStrategicPosition(int x, int y) {
    int centerDistance = std::max(std::abs(x - 9), std::abs(y - 9));
    return centerDistance <= 2; // Posiciones dentro del área central 5x5
}

void TranspositionSearch::orderMovesByGeometricValue(std::vector<Move>& moves, const GameState& state) {
    // Ordenar usando evaluación basada en patrones geométricos
    std::sort(moves.begin(), moves.end(), [&](const Move& a, const Move& b) {
        return calculateGeometricMoveValue(state, a) > calculateGeometricMoveValue(state, b);
    });
}

int TranspositionSearch::calculateGeometricMoveValue(const GameState& state, const Move& move) {
    int value = 0;
    int currentPlayer = state.currentPlayer;
    int opponent = state.getOpponent(currentPlayer);
    
    // 1. VALOR POSICIONAL: Proximidad al centro (importante en opening)
    int centralityBonus = calculateCentralityBonus(move);
    value += centralityBonus;
    
    // 2. ANÁLISIS DE PATRONES: Evaluar alineaciones potenciales en 4 direcciones
    int directions[4][2] = {{1,0}, {0,1}, {1,1}, {1,-1}};
    
    for (int d = 0; d < 4; d++) {
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

int TranspositionSearch::calculateCentralityBonus(const Move& move) {
    int centerDistance = std::max(std::abs(move.x - 9), std::abs(move.y - 9));
    return (9 - centerDistance) * 10;
}

int TranspositionSearch::calculateAlignmentValue(int alignmentLength) {
    switch (alignmentLength) {
        case 5: return 10000; // Victoria inmediata
        case 4: return 5000;  // Amenaza crítica
        case 3: return 1000;  // Amenaza fuerte
        case 2: return 100;   // Desarrollo básico
        default: return 0;
    }
}

int TranspositionSearch::calculateInterruptionValue(int interruptionLength) {
    switch (interruptionLength) {
        case 4: return 80000;  // AUMENTADO: Bloqueo crítico debe superar ataque propio (70000)
        case 3: return 15000;  // AUMENTADO: Bloqueo de amenaza fuerte
        case 2: return 1000;   // AUMENTADO: Prevención temprana
        default: return 0;
    }
}

int TranspositionSearch::calculateConnectivityBonus(const GameState& state, const Move& move, int player) {
    int connectivity = 0;
    
    // Verificar las 8 direcciones adyacentes
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            if (dx == 0 && dy == 0) continue;
            
            int adjX = move.x + dx, adjY = move.y + dy;
            if (state.isValid(adjX, adjY) && state.getPiece(adjX, adjY) == player) {
                connectivity += 30;
            }
        }
    }
    
    return connectivity;
}

// Método auxiliar optimizado para contar piezas consecutivas
int TranspositionSearch::countPiecesInDirection(const GameState& state, int x, int y, 
                                               int dx, int dy, int player) {
    int count = 0;
    x += dx; y += dy;
    
    // Limitar conteo a 4 (suficiente para todas las evaluaciones tácticas)
    while (count < 4 && state.isValid(x, y) && state.getPiece(x, y) == player) {
        count++;
        x += dx; y += dy;
    }
    
    return count;
}

