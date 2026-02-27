// ============================================
// SEARCH_MINIMAX.CPP
// Minimax algorithm with alpha-beta pruning
// Iterative deepening search
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

	// Log stats every 10000 nodes
	if (nodesEvaluated % 10000 == 0)
	{
		DEBUG_LOG_STATS("Nodes evaluated: " + std::to_string(nodesEvaluated) +
						", Cache hits: " + std::to_string(cacheHits));
	}

	// CRITICAL: Detect 5-in-a-row BEFORE transposition lookup.
	// checkWin() ignores breakable 5-in-a-row ("not a win yet"),
	// making them invisible to the search and cached with wrong scores.
	// hasFiveInARow() catches them so the AI actually blocks.
	if (RuleEngine::hasFiveInARow(state, GameState::PLAYER2)) {
		int mateDistance = originalMaxDepth - depth;
		return Evaluator::WIN - mateDistance;
	}
	if (RuleEngine::hasFiveInARow(state, GameState::PLAYER1)) {
		int mateDistance = originalMaxDepth - depth;
		return -Evaluator::WIN + mateDistance;
	}

	// Check transposition table first
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

		// Use cached move for ordering at root level
		if (entry.bestMove.isValid())
		{
			previousBestMove = entry.bestMove;
		}
	}

	// Base cases
	if (depth == 0 || RuleEngine::checkWin(state, GameState::PLAYER1) ||
		RuleEngine::checkWin(state, GameState::PLAYER2))
	{

		int score = Evaluator::evaluate(state, originalMaxDepth, originalMaxDepth - depth);
		storeTransposition(zobristKey, score, depth, Move(), CacheEntry::EXACT);
		return score;
	}

	// Generate and order moves
	std::vector<Move> moves = generateOrderedMoves(state);

	if (moves.empty())
	{
		int score = Evaluator::evaluate(state, originalMaxDepth, originalMaxDepth - depth);
		storeTransposition(zobristKey, score, depth, Move(), CacheEntry::EXACT);
		return score;
	}

	// Promote killer moves toward the front (after previous best move)
	// Killer moves are moves that caused cutoffs at this depth in sibling nodes
	if (depth < MAX_SEARCH_DEPTH && moves.size() > 2)
	{
		for (int k = 0; k < 2; k++)
		{
			if (!killerMoves[depth][k].isValid())
				continue;
			auto it = std::find_if(moves.begin() + 1, moves.end(),
				[&](const Move &m) {
					return m.x == killerMoves[depth][k].x &&
						   m.y == killerMoves[depth][k].y;
				});
			if (it != moves.end())
			{
				// Move killer to position 1 (or 2 for second killer)
				size_t targetPos = std::min((size_t)(1 + k), moves.size() - 1);
				if (it != moves.begin() + (int)targetPos)
					std::iter_swap(moves.begin() + targetPos, it);
			}
		}
	}

	Move currentBestMove;
	int originalAlpha = alpha;

	if (maximizing)
	{
		int maxEval = std::numeric_limits<int>::min();
		int moveIndex = 0;

		for (const Move &move : moves)
		{
			GameState newState = state;
			RuleEngine::MoveResult result = RuleEngine::applyMove(newState, move);
			if (!result.success)
				continue;

			// Enable debug capture before recursive evaluation
			if (g_debugAnalyzer && depth == originalMaxDepth)
			{
				g_evalDebug.reset();
				g_evalDebug.active = true;
				g_evalDebug.currentMove = move;
			}

			// Late Move Reduction: search late moves at reduced depth first
			int eval;
			bool needsFullSearch = true;
			if (moveIndex >= 2 && depth >= 3 && depth != originalMaxDepth)
			{
				// Reduced depth search (save 1 ply)
				eval = minimax(newState, depth - 2, alpha, beta, false, originalMaxDepth, nullptr);
				// Only re-search at full depth if it improves alpha
				needsFullSearch = (eval > alpha);
			}
			if (needsFullSearch)
			{
				eval = minimax(newState, depth - 1, alpha, beta, false, originalMaxDepth, nullptr);
			}

			// Capture debug data after evaluation
			if (g_debugAnalyzer && depth == originalMaxDepth && g_evalDebug.active)
			{
				std::ostringstream heuristicInfo;

				// Show board state after the move
				heuristicInfo << "\n=== EVALUATING MOVE " << g_debugAnalyzer->formatMove(move) << " ===\n";
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

			// Update best move from recursive evaluation
			if (eval > maxEval)
			{
				maxEval = eval;
				currentBestMove = move;
			}

			// Update alpha
			alpha = std::max(alpha, eval);

			// Alpha-beta pruning
			if (beta <= alpha)
			{
				// History heuristic: reward move that caused cutoff
				if (currentBestMove.isValid())
				{
					historyTable[currentBestMove.x][currentBestMove.y] += depth * depth;
					// Killer move: store non-capture cutoff move at this depth
					if (depth < MAX_SEARCH_DEPTH)
					{
						if (!(killerMoves[depth][0].x == currentBestMove.x &&
							  killerMoves[depth][0].y == currentBestMove.y))
						{
							killerMoves[depth][1] = killerMoves[depth][0];
							killerMoves[depth][0] = currentBestMove;
						}
					}
				}
				break; // Beta cutoff
			}
			moveIndex++;
		}

		// Determine cache entry type
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
			currentBestMove = moves[0]; // Fallback to first evaluated move
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
		int moveIndex = 0;

		for (const Move &move : moves)
		{
			GameState newState = state;
			RuleEngine::MoveResult result = RuleEngine::applyMove(newState, move);
			if (!result.success)
				continue;

			// Enable debug capture before recursive evaluation
			if (g_debugAnalyzer && depth == originalMaxDepth)
			{
				g_evalDebug.reset();
				g_evalDebug.active = true;
				g_evalDebug.currentMove = move;
			}

			// Late Move Reduction: search late moves at reduced depth first
			int eval;
			bool needsFullSearch = true;
			if (moveIndex >= 2 && depth >= 3 && depth != originalMaxDepth)
			{
				eval = minimax(newState, depth - 2, alpha, beta, true, originalMaxDepth, nullptr);
				needsFullSearch = (eval < beta);
			}
			if (needsFullSearch)
			{
				eval = minimax(newState, depth - 1, alpha, beta, true, originalMaxDepth, nullptr);
			}

			// Capture debug data after evaluation
			if (g_debugAnalyzer && depth == originalMaxDepth && g_evalDebug.active)
			{
				std::ostringstream heuristicInfo;

				// Show board state after the move
				heuristicInfo << "\n=== EVALUATING MOVE " << g_debugAnalyzer->formatMove(move) << " ===\n";
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

			// Update best move from recursive evaluation
			if (eval < minEval)
			{
				minEval = eval;
				currentBestMove = move;
			}

			// Update beta
			beta = std::min(beta, eval);

			// Alpha-beta pruning
			if (beta <= alpha)
			{
				// History heuristic: reward move that caused cutoff
				if (currentBestMove.isValid())
				{
					historyTable[currentBestMove.x][currentBestMove.y] += depth * depth;
					// Killer move: store non-capture cutoff move at this depth
					if (depth < MAX_SEARCH_DEPTH)
					{
						if (!(killerMoves[depth][0].x == currentBestMove.x &&
							  killerMoves[depth][0].y == currentBestMove.y))
						{
							killerMoves[depth][1] = killerMoves[depth][0];
							killerMoves[depth][0] = currentBestMove;
						}
					}
				}
				break; // Alpha cutoff
			}
			moveIndex++;
		}

		// Determine cache entry type
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
			currentBestMove = moves[0]; // Fallback to first evaluated move
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

    if (g_debugAnalyzer && g_debugAnalyzer->isEnabled()) {
        std::cout << "Iterative search up to depth " << maxDepth << std::endl;
    }

    // ============================================
    // Pre-check for immediate victory
    // ============================================
    std::vector<Move> allCandidates = generateCandidatesAdaptiveRadius(state);
    
    for (const Move& move : allCandidates) {
        GameState testState = state;
        RuleEngine::MoveResult result = RuleEngine::applyMove(testState, move);
        
        if (!result.success) continue;
        
        // Check if this move wins immediately
        if (RuleEngine::checkWin(testState, state.currentPlayer) ||
            testState.captures[state.currentPlayer - 1] >= 10) {
            
            auto endTime = std::chrono::high_resolution_clock::now();
            int elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                endTime - startTime).count();
            
            SearchResult winResult;
            winResult.bestMove = move;
            winResult.score = Evaluator::WIN;
            winResult.nodesEvaluated = allCandidates.size();
            winResult.cacheHits = 0;
            winResult.cacheHitRate = 0.0f;
            
            if (g_debugAnalyzer && g_debugAnalyzer->isEnabled()) {
                std::cout << "IMMEDIATE VICTORY detected at " 
                          << char('A' + move.y) << (move.x + 1) 
                          << " in " << elapsedTime << "ms!" << std::endl;
            }
            
            if (g_debugAnalyzer) {
                DEBUG_CHOSEN_MOVE(move, winResult.score);
                DEBUG_SNAPSHOT(state, elapsedTime, allCandidates.size());
            }
            
            return winResult;
        }
    }
    
    if (g_debugAnalyzer && g_debugAnalyzer->isEnabled()) {
        std::cout << "No immediate victory, starting iterative search..." << std::endl;
    }
    
    // ============================================
    // Iterative deepening search
    // ============================================
    for (int depth = 1; depth <= maxDepth; depth++)
    {
        auto iterationStart = std::chrono::high_resolution_clock::now();

        // Age history table: halve values between iterations so that
        // recent cutoff information dominates over stale data
        for (int i = 0; i < GameState::BOARD_SIZE; i++)
            for (int j = 0; j < GameState::BOARD_SIZE; j++)
                historyTable[i][j] >>= 1;

        if (bestResult.bestMove.isValid()) {
            previousBestMove = bestResult.bestMove;
        }

        Move bestMove;
        GameState mutableState = state; // Mutable copy for minimax
        int score = minimax(mutableState, depth,
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

        if (g_debugAnalyzer && g_debugAnalyzer->isEnabled()) {
            std::cout << "Depth " << depth
                      << ": " << char('A' + bestMove.y) << (bestMove.x + 1)
                      << " (score: " << score << ")"
                      << " - " << iterationTime.count() << "ms"
                      << " (" << nodesEvaluated << " nodes, "
                      << std::fixed << std::setprecision(1) 
                      << (bestResult.cacheHitRate * 100) << "% cache hit)"
                      << std::endl;
        }

        // Early exit on decisive score
        if (std::abs(score) > 300000) {
            if (g_debugAnalyzer && g_debugAnalyzer->isEnabled()) {
                std::cout << "Mate detected at depth " << depth
                          << ", completing search" << std::endl;
            }
            break;
        }
    }

    auto totalTime = std::chrono::high_resolution_clock::now() - startTime;
    int elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        totalTime).count();

    if (g_debugAnalyzer && g_debugAnalyzer->isEnabled()) {
        std::cout << "Search completed in " << elapsedTime << "ms total" << std::endl;
    }

    if (g_debugAnalyzer) {
        DEBUG_CHOSEN_MOVE(bestResult.bestMove, bestResult.score);
        DEBUG_SNAPSHOT(state, elapsedTime, nodesEvaluated);
    }

    return bestResult;
}
