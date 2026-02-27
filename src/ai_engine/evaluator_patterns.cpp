// ===============================================
// AI Engine - Evaluator Patterns Module
// ===============================================
// Handles: Pattern analysis, line scanning, pattern scoring
// Dependencies: Evaluator core, game types
// ===============================================

#include "../../include/ai/evaluator.hpp"
#include "../../include/core/game_types.hpp"

using namespace Directions;

// ===============================================
// LINE START DETECTION
// ===============================================

/**
 * Check if position is the start of a line for pattern analysis
 * Returns true if previous position doesn't have same player's piece
 */
bool Evaluator::isLineStart(const GameState &state, int x, int y, int dx, int dy, int player)
{
	// It's a start if previous position doesn't have same player's piece
	int prevX = x - dx;
	int prevY = y - dy;

	return !state.isValid(prevX, prevY) || state.getPiece(prevX, prevY) != player;
}

// ===============================================
// LINE ANALYSIS
// ===============================================

/**
 * Analyze a line starting from (x,y) in direction (dx,dy)
 * Detects consecutive pieces, gaps, and free ends
 */
Evaluator::PatternInfo Evaluator::analyzeLine(const GameState &state, int x, int y,
											  int dx, int dy, int player)
{
	PatternInfo info = {0, 0, 0, false, 0, 0, 0};

	// STEP 1: Extended analysis - scan up to 6 positions to detect gaps
	const int MAX_SCAN = 6;
	int sequence[MAX_SCAN];
	int actualPositions = 0;

	// Fill array with content of 6 positions
	for (int i = 0; i < MAX_SCAN; i++)
	{
		int checkX = x + i * dx;
		int checkY = y + i * dy;

		if (!state.isValid(checkX, checkY))
		{
			break;
		}

		sequence[i] = state.getPiece(checkX, checkY);
		actualPositions = i + 1;
	}

	// STEP 2: Analyze consecutive patterns from start
	int consecutiveFromStart = 0;
	while (consecutiveFromStart < actualPositions &&
		   sequence[consecutiveFromStart] == player)
	{
		consecutiveFromStart++;
	}

	info.consecutiveCount = consecutiveFromStart;

	// STEP 3: If 5+ consecutive, it's immediate victory
	if (info.consecutiveCount >= 5)
	{
		info.totalPieces = info.consecutiveCount;
		info.totalSpan = info.consecutiveCount;
		info.freeEnds = 2;
		return info;
	}

	// STEP 4: Gap pattern analysis (X-XXX, XX-XX, etc.)
	int totalPieces = 0;
	int gapCount = 0;
	int lastPiecePos = -1;

	// Count total pieces and gaps in first 5-6 spaces
	for (int i = 0; i < actualPositions && i < 6; i++)
	{
		if (sequence[i] == player)
		{
			totalPieces++;
			lastPiecePos = i;
		}
		else if (sequence[i] != GameState::EMPTY)
		{
			// If opponent piece, cut analysis here
			break;
		}
		else if (totalPieces > 0)
		{
			// It's a gap (empty space after finding pieces)
			gapCount++;
		}
	}

	// STEP 5: Determine total span (from first to last piece)
	int totalSpan = lastPiecePos + 1;

	// STEP 6: Detect significant gaps
	bool hasGaps = (gapCount > 0 && totalPieces > info.consecutiveCount);

	// STEP 7: Calculate free ends
	info.freeEnds = 0;

	// Check back end (before start)
	int backX = x - dx, backY = y - dy;
	if (state.isValid(backX, backY) && state.isEmpty(backX, backY))
	{
		info.freeEnds++;
	}

	// Check front end (after last analyzed element)
	int frontX = x + totalSpan * dx;
	int frontY = y + totalSpan * dy;
	if (state.isValid(frontX, frontY) && state.isEmpty(frontX, frontY))
	{
		info.freeEnds++;
	}

	// STEP 8: Assign final values
	info.totalPieces = totalPieces;
	info.totalSpan = totalSpan;
	info.hasGaps = hasGaps;
	info.gapCount = gapCount;

	// STEP 9: Compute maxReachable — total usable cells (own + empty)
	// in this direction. If < 5, pattern can never become 5-in-a-row.
	{
		int reachable = totalSpan; // Already includes pieces + gaps in span

		// Extend backward (before start position)
		int bx = x - dx, by = y - dy;
		while (state.isValid(bx, by) && state.getPiece(bx, by) != state.getOpponent(player))
		{
			reachable++;
			bx -= dx;
			by -= dy;
		}

		// Extend forward (after span end)
		int fx = x + totalSpan * dx;
		int fy = y + totalSpan * dy;
		while (state.isValid(fx, fy) && state.getPiece(fx, fy) != state.getOpponent(player))
		{
			reachable++;
			fx += dx;
			fy += dy;
		}

		info.maxReachable = reachable;
	}

	return info;
}

// ===============================================
// PATTERN SCORING
// ===============================================

/**
 * Convert pattern information to score
 * Handles consecutive patterns, gap patterns, and various threat levels
 */
int Evaluator::patternToScore(const PatternInfo &pattern)
{
	int consecutiveCount = pattern.consecutiveCount;
	int totalPieces = pattern.totalPieces;
	int freeEnds = pattern.freeEnds;
	bool hasGaps = pattern.hasGaps;

	// SPACE VALIDATION: If not enough room to ever make 5-in-a-row,
	// this pattern is strategically worthless (dead shape).
	if (pattern.maxReachable < 5 && consecutiveCount < 5)
		return 0;

	// STEP 1: Victory patterns (5+ pieces consecutive or with valid gaps)
	if (consecutiveCount >= 5)
		return WIN;

	// Victory with gaps - X-XXXX, XX-XXX, etc.
	if (totalPieces >= 5 && hasGaps && freeEnds >= 1)
	{
		return WIN; // Also victory
	}

	// STEP 2: Critical 4-piece patterns
	if (totalPieces == 4)
	{
		// Case 1: 4 consecutive (XXXX)
		if (consecutiveCount == 4)
		{
			if (freeEnds == 2)
			{
				if (g_evalDebug.active)
				{
					if (g_evalDebug.currentPlayer == GameState::PLAYER2)
					{
						g_evalDebug.aiFourOpen++;
					}
					else
					{
						g_evalDebug.humanFourOpen++;
					}
				}
				return FOUR_OPEN; // Unstoppable
			}
			if (freeEnds == 1)
			{
				if (g_evalDebug.active)
				{
					if (g_evalDebug.currentPlayer == GameState::PLAYER2)
					{
						g_evalDebug.aiFourHalf++;
					}
					else
					{
						g_evalDebug.humanFourHalf++;
					}
				}
				return FOUR_HALF; // Forced threat
			}
		}
		// Case 2: 4 with gaps (X-XXX, XX-XX, XXX-X)
		else if (hasGaps)
		{
			if (freeEnds == 2)
			{
				if (g_evalDebug.active)
				{
					if (g_evalDebug.currentPlayer == GameState::PLAYER2)
					{
						g_evalDebug.aiFourOpen++;
					}
					else
					{
						g_evalDebug.humanFourOpen++;
					}
				}
				return FOUR_OPEN; // CRITICAL! X-XXX is unstoppable
			}
			if (freeEnds == 1)
			{
				if (g_evalDebug.active)
				{
					if (g_evalDebug.currentPlayer == GameState::PLAYER2)
					{
						g_evalDebug.aiFourHalf++;
					}
					else
					{
						g_evalDebug.humanFourHalf++;
					}
				}
				return FOUR_HALF; // Strong threat
			}
		}
	}

	// STEP 3: 3-piece patterns
	if (totalPieces == 3)
	{
		// Case 1: 3 consecutive (XXX)
		if (consecutiveCount == 3)
		{
			if (freeEnds == 2)
			{
				if (g_evalDebug.active)
				{
					if (g_evalDebug.currentPlayer == GameState::PLAYER2)
					{
						g_evalDebug.aiThreeOpen++;
					}
					else
					{
						g_evalDebug.humanThreeOpen++;
					}
				}
				return THREE_OPEN; // Very dangerous
			}
			if (freeEnds == 1)
				return THREE_HALF; // Threat
		}
		// Case 2: 3 with gaps (X-XX, XX-X)
		else if (hasGaps)
		{
			if (freeEnds == 2)
			{
				if (g_evalDebug.active)
				{
					if (g_evalDebug.currentPlayer == GameState::PLAYER2)
					{
						g_evalDebug.aiThreeOpen++;
					}
					else
					{
						g_evalDebug.humanThreeOpen++;
					}
				}
				return THREE_OPEN; // Also dangerous
			}
			if (freeEnds == 1)
				return THREE_HALF; // Split threat
		}
	}

	// STEP 4: 2-piece patterns (development)
	if (totalPieces == 2 && freeEnds == 2)
	{
		if (g_evalDebug.active)
		{
			if (g_evalDebug.currentPlayer == GameState::PLAYER2)
			{
				g_evalDebug.aiTwoOpen++;
			}
			else
			{
				g_evalDebug.humanTwoOpen++;
			}
		}
		return TWO_OPEN; // Development (XX or X-X)
	}

	return 0;
}

// ===============================================
// PATTERN COUNTING
// ===============================================

/**
 * Count specific pattern types for a player
 * Used for threat detection and position analysis
 */
int Evaluator::countPatternType(const GameState &state, int player, int consecutiveCount, int freeEnds)
{
	int count = 0;

	// Scan board for pattern starts
	for (int i = 0; i < GameState::BOARD_SIZE; i++)
	{
		for (int j = 0; j < GameState::BOARD_SIZE; j++)
		{
			if (state.board[i][j] == player)
			{
				// Check all 4 main directions
				for (int d = 0; d < MAIN_COUNT; d++)
				{
					int dx = MAIN[d][0];
					int dy = MAIN[d][1];

					// Only analyze line starts to avoid double counting
					if (isLineStart(state, i, j, dx, dy, player))
					{
						PatternInfo pattern = analyzeLine(state, i, j, dx, dy, player);

						// Check if matches requested pattern type
						if (pattern.consecutiveCount == consecutiveCount &&
							pattern.freeEnds == freeEnds)
						{
							count++;
						}
					}
				}
			}
		}
	}

	return count;
}
