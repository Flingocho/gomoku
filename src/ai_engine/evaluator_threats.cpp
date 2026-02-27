// ===============================================
// AI Engine - Evaluator Threats Module
// ===============================================
// Handles: Immediate threats, capture evaluation, winning threats
// Dependencies: Evaluator patterns, game types
// ===============================================

#include "../../include/ai/evaluator.hpp"
#include "../../include/core/game_types.hpp"

using namespace Directions;

// ===============================================
// SINGLE-PASS PATTERN COUNTING
// ===============================================

/**
 * Scan the entire board ONCE and return all pattern counts.
 * Replaces 5 separate countPatternType calls with a single O(19*19*4) pass.
 */
Evaluator::PatternCounts Evaluator::countAllPatterns(const GameState &state, int player)
{
	PatternCounts counts = {0, 0, 0, 0, 0};

	for (int i = 0; i < GameState::BOARD_SIZE; i++)
	{
		for (int j = 0; j < GameState::BOARD_SIZE; j++)
		{
			if (state.board[i][j] != player)
				continue;

			for (int d = 0; d < MAIN_COUNT; d++)
			{
				int dx = MAIN[d][0];
				int dy = MAIN[d][1];

				if (!isLineStart(state, i, j, dx, dy, player))
					continue;

				PatternInfo pattern = analyzeLine(state, i, j, dx, dy, player);

				// Skip dead shapes
				if (pattern.maxReachable < 5 && pattern.consecutiveCount < 5)
					continue;

				int c = pattern.consecutiveCount;
				int tp = pattern.totalPieces;
				int fe = pattern.freeEnds;
				bool gaps = pattern.hasGaps;

				// Classify pattern (mirrors patternToScore logic)
				if (tp >= 4)
				{
					if (c == 4 || (tp == 4 && gaps))
					{
						if (fe == 2) counts.fourOpen++;
						else if (fe == 1) counts.fourHalf++;
					}
				}
				if (tp == 3)
				{
					if (c == 3 || gaps)
					{
						if (fe == 2) counts.threeOpen++;
						else if (fe == 1) counts.threeHalf++;
					}
				}
				if (tp == 2 && fe == 2)
				{
					counts.twoOpen++;
				}
			}
		}
	}

	return counts;
}

// ===============================================
// COMBINED THREAT + COMBINATION EVALUATION
// ===============================================

/**
 * Evaluate both immediate threats and pattern combinations
 * using pre-computed pattern counts (single scan).
 * Replaces evaluateImmediateThreats + evaluateCombinations.
 */
int Evaluator::evaluateThreatsAndCombinations(const GameState &state, int player,
											  const PatternCounts &counts)
{
	int score = 0;

	// --- Immediate threats (from evaluateImmediateThreats) ---

	if (counts.fourOpen > 0)
		score += 90000;

	if (counts.fourHalf > 0)
		score += 40000;

	if (counts.threeOpen >= 2)
		score += 50000;

	// --- Combinations / forks (from evaluateCombinations) ---

	// FOUR_HALF + THREE_OPEN fork
	if (counts.fourHalf >= 1 && counts.threeOpen >= 1)
		score += 80000;

	// Double FOUR_HALF
	if (counts.fourHalf >= 2)
		score += 70000;

	// Capture synergy
	int myCaptures = state.captures[player - 1];
	if (myCaptures >= 8 && counts.threeOpen >= 1)
		score += 60000;

	return score;
}

// ===============================================
// LEGACY IMMEDIATE THREATS (kept for hasWinningThreats)
// ===============================================

/**
 * Evaluate immediate threats for a player
 */
int Evaluator::evaluateImmediateThreats(const GameState &state, int player)
{
	int threatScore = 0;

	int fourOpen = countPatternType(state, player, 4, 2);
	int fourHalf = countPatternType(state, player, 4, 1);
	int threeOpen = countPatternType(state, player, 3, 2);

	if (fourOpen > 0)
		threatScore += 90000;
	if (fourHalf > 0)
		threatScore += 40000;
	if (threeOpen >= 2)
		threatScore += 50000;

	return threatScore;
}

// ===============================================
// WINNING THREATS DETECTION
// ===============================================

/**
 * EFFICIENT function that detects mate threats using existing patterns
 * Returns true if player has immediate winning opportunities
 */
bool Evaluator::hasWinningThreats(const GameState &state, int player)
{
	// Search for 4-consecutive patterns with at least one free end
	// This indicates a mate-in-1 threat

	// FOUR_OPEN (4 with both ends free) = unstoppable threat
	int fourOpen = countPatternType(state, player, 4, 2);
	if (fourOpen > 0)
	{
		return true; // Unstoppable threat
	}

	// FOUR_HALF (4 with one free end) = forced threat
	int fourHalf = countPatternType(state, player, 4, 1);
	if (fourHalf > 0)
	{
		return true; // Threat requiring defense
	}

	// Check multiple THREE_OPEN creating dual threats
	int threeOpen = countPatternType(state, player, 3, 2);
	if (threeOpen >= 2)
	{
		return true; // Multiple open-3 threats = likely mate
	}

	return false;
}

// ===============================================
// CAPTURE PATTERN VALIDATION
// ===============================================

/**
 * Validate if a capture pattern exists
 * Pattern: ATTACKER + VICTIM + VICTIM + EMPTY
 */
bool Evaluator::isValidCapturePattern(const GameState &state, int x, int y,
									  int dx, int dy, int attacker, int victim)
{
	// Verify pattern: NEW(x,y) + VICTIM + VICTIM + ATTACKER
	int pos1X = x + dx, pos1Y = y + dy;
	int pos2X = x + 2 * dx, pos2Y = y + 2 * dy;
	int pos3X = x + 3 * dx, pos3Y = y + 3 * dy;

	return state.isValid(pos1X, pos1Y) && state.isValid(pos2X, pos2Y) && state.isValid(pos3X, pos3Y) &&
		   state.getPiece(pos1X, pos1Y) == victim &&
		   state.getPiece(pos2X, pos2Y) == victim &&
		   state.getPiece(pos3X, pos3Y) == attacker;
}

/**
 * Check if capture breaks opponent's pattern
 * Returns true if capturing these pieces disrupts dangerous patterns
 */
bool Evaluator::captureBreaksOpponentPattern(const GameState &state,
								  const std::vector<Move> &capturedPieces,
								  int opponent)
{
	// For each captured piece
	for (const Move &captured : capturedPieces)
	{
		// Check 4 main directions from this position
		for (int d = 0; d < MAIN_COUNT; d++)
		{
			int dx = MAIN[d][0];
			int dy = MAIN[d][1];

			// Was there a dangerous opponent pattern here?
			int countBefore = countPatternThroughPosition(
				state, captured, dx, dy, opponent);

			// If there was 3+ in a row passing through here → we break it
			if (countBefore >= 3)
			{
				return true; // Valuable defensive capture!
			}
		}
	}
	return false;
}

/**
 * Count pattern length through a specific position
 * Used to evaluate pattern disruption
 */
int Evaluator::countPatternThroughPosition(const GameState &state,
								const Move &pos,
								int dx, int dy,
								int player)
{
	int count = 0;

	// Count backwards
	int x = pos.x - dx, y = pos.y - dy;
	while (state.isValid(x, y) && state.getPiece(x, y) == player)
	{
		count++;
		x -= dx;
		y -= dy;
	}

	// Count forwards
	x = pos.x + dx;
	y = pos.y + dy;
	while (state.isValid(x, y) && state.getPiece(x, y) == player)
	{
		count++;
		x += dx;
		y += dy;
	}

	return count;
}

// ===============================================
// COMBINATION DETECTION (FORKS)
// ===============================================

/**
 * Detect advantageous pattern combinations for a player.
 * Forks are positions where two or more threats exist simultaneously,
 * making them impossible to defend against in a single move.
 *
 * Detected combinations:
 *   - FOUR_HALF + THREE_OPEN  (fork: forced response + secondary threat)
 *   - Multiple FOUR_HALF       (double forced threats)
 *   - THREE_OPEN + near capture win (alignment + capture synergy)
 */
int Evaluator::evaluateCombinations(const GameState &state, int player)
{
	int comboScore = 0;

	// Gather pattern counts (these use cached countPatternType scans)
	int fourHalf = countPatternType(state, player, 4, 1);
	int threeOpen = countPatternType(state, player, 3, 2);

	// fourOpen alone is already near-unstoppable; no combination bonus needed.
	// Focus on combinations that create dual-purpose threats.

	// 1. FOUR_HALF + THREE_OPEN fork:
	//    Opponent must block the four, leaving the three free to extend.
	if (fourHalf >= 1 && threeOpen >= 1)
	{
		comboScore += 80000;
	}

	// 2. Double FOUR_HALF:
	//    Opponent can only block one four at a time → guaranteed win next move.
	if (fourHalf >= 2)
	{
		comboScore += 70000;
	}

	// 3. Capture synergy:
	//    Player is close to 10 captures (≥8) AND has an open three.
	//    Forces opponent to decide between blocking alignment and preventing capture.
	int myCaptures = state.captures[player - 1];
	if (myCaptures >= 8 && threeOpen >= 1)
	{
		comboScore += 60000;
	}

	// 4. THREE_OPEN + FOUR_HALF already covered above.
	//    Additional: multiple THREE_OPEN creates dual threats too,
	//    but that's already scored in evaluateImmediateThreats.

	return comboScore;
}

// ===============================================
// CAPTURE CONTEXT EVALUATION
// ===============================================

/**
 * Evaluate capture in context of game state
 * Considers proximity to victory, pattern disruption, tactical value
 */
int Evaluator::evaluateCaptureContext(const GameState &state, int player, 
									  const std::vector<Move> &capturedPieces, 
									  int newCaptureCount)
{
	int value = 0;
	int opponent = state.getOpponent(player);

	// 1. BASE VALUE: Proximity to capture victory
	if (newCaptureCount >= 10)
	{
		return 500000; // IMMEDIATE VICTORY!
	}
	else if (newCaptureCount == 9)
	{
		value += 100000; // One more capture and we win
	}
	else if (newCaptureCount >= 8)
	{
		value += 50000; // Very close
	}
	else if (newCaptureCount >= 6)
	{
		value += 15000; // Considerable pressure
	}
	else
	{
		value += newCaptureCount * 2000; // Normal development
	}

	// 2. DEFENSIVE VALUE: Does it break opponent patterns?
	for (const Move &captured : capturedPieces)
	{
		// Check 4 main directions
		for (int d = 0; d < MAIN_COUNT; d++)
		{
			int dx = MAIN[d][0];
			int dy = MAIN[d][1];

			// Count pattern that existed through this position
			int patternSize = countPatternThroughPosition(
				state, captured, dx, dy, opponent);

			// Scoring based on how dangerous the pattern was
			if (patternSize >= 4)
			{
				value += 30000; // Broke a 4-in-row!
			}
			else if (patternSize == 3)
			{
				value += 12000; // Broke a 3-in-row
			}
			else if (patternSize == 2)
			{
				value += 3000; // Broke a 2-in-row
			}
		}
	}

	// 3. OFFENSIVE VALUE: Does it create space for our patterns?
	// Captured positions are now empty
	// Do they improve our adjacent lines?
	for (const Move &captured : capturedPieces)
	{
		// Check if we can now extend our lines
		for (int d = 0; d < MAIN_COUNT; d++)
		{
			int dx = MAIN[d][0];
			int dy = MAIN[d][1];

			// Are there our pieces adjacent?
			int x1 = captured.x + dx, y1 = captured.y + dy;
			int x2 = captured.x - dx, y2 = captured.y - dy;

			bool hasMyPieceAdjacent =
				(state.isValid(x1, y1) && state.getPiece(x1, y1) == player) ||
				(state.isValid(x2, y2) && state.getPiece(x2, y2) == player);

			if (hasMyPieceAdjacent)
			{
				value += 1500; // Creates tactical opportunities
			}
		}
	}

	// 4. DANGER: Is opponent close to winning by capture?
	int opponentCaptures = state.captures[opponent - 1];
	if (opponentCaptures >= 8)
	{
		// Critical defensive capture if it prevents more captures
		value += 25000;
	}

	return value;
}

// ===============================================
// CAPTURE OPPORTUNITY FINDING
// ===============================================

/**
 * Find all capture opportunities for a player
 * Returns positions where player can capture opponent pairs
 */
std::vector<Evaluator::CaptureOpportunity> Evaluator::findAllCaptureOpportunities(
    const GameState& state, 
    int player) {
    
    std::vector<CaptureOpportunity> opportunities;
    int opponent = state.getOpponent(player);
    
    // Scan all positions looking for opponent PAIRS
    for (int i = 0; i < GameState::BOARD_SIZE; i++) {
        for (int j = 0; j < GameState::BOARD_SIZE; j++) {
            
            // Is there an opponent piece here?
            if (state.board[i][j] != opponent) continue;
            
            // Check 4 main directions
            for (int d = 0; d < MAIN_COUNT; d++) {
                int dx = MAIN[d][0];
                int dy = MAIN[d][1];
                
                // Is there ANOTHER opponent piece in this direction?
                int x2 = i + dx, y2 = j + dy;
                if (!state.isValid(x2, y2) || state.board[x2][y2] != opponent) 
                    continue;
                
                // Found opponent pair: (i,j) - (x2,y2)
                
                // OPTION 1: Flank from FRONT
                int xFront = x2 + dx, yFront = y2 + dy;
                if (state.isValid(xFront, yFront) && state.isEmpty(xFront, yFront)) {
                    // Do I have MY piece behind the pair?
                    int xBack = i - dx, yBack = j - dy;
                    if (state.isValid(xBack, yBack) && state.board[xBack][yBack] == player) {
                        // Pattern: PLAYER-OO-[EMPTY] → capture by playing at EMPTY
                        CaptureOpportunity opp;
                        opp.position = Move(xFront, yFront);
                        opp.captured = {Move(i, j), Move(x2, y2)};
                        opportunities.push_back(opp);
                    }
                }
                
                // OPTION 2: Flank from BEHIND
                int xBack = i - dx, yBack = j - dy;
                if (state.isValid(xBack, yBack) && state.isEmpty(xBack, yBack)) {
                    // Do I have MY piece in front of the pair?
                    int xFront2 = x2 + dx, yFront2 = y2 + dy;
                    if (state.isValid(xFront2, yFront2) && state.board[xFront2][yFront2] == player) {
                        // Pattern: [EMPTY]-OO-PLAYER → capture by playing at EMPTY
                        CaptureOpportunity opp;
                        opp.position = Move(xBack, yBack);
                        opp.captured = {Move(i, j), Move(x2, y2)};
                        opportunities.push_back(opp);
                    }
                }
            }
        }
    }
    
    return opportunities;
}
