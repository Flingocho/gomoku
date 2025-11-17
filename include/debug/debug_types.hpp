#ifndef DEBUG_TYPES_HPP
#define DEBUG_TYPES_HPP

#include "../core/game_types.hpp"
#include <vector>
#include <string>

/**
 * Centralized debug-related data structures
 * Used by: DebugAnalyzer, Evaluator debug capture
 */

// ============================================
// EVALUATION DEBUG CAPTURE
// ============================================

/**
 * Simple structure to capture debug info during real evaluations
 * Used by Evaluator to expose internal scoring for analysis
 */
struct EvaluationDebugCapture
{
	bool active;
	int currentPlayer;
	Move currentMove;

	// Information captured during real evaluation
	int aiScore;
	int humanScore;
	int totalScore;
	int aiThreeOpen;
	int aiFourHalf;
	int aiFourOpen;
	int aiTwoOpen;
	int humanThreeOpen;
	int humanFourHalf;
	int humanFourOpen;
	int humanTwoOpen;

	EvaluationDebugCapture() : active(false), currentPlayer(0), aiScore(0), humanScore(0), totalScore(0),
							   aiThreeOpen(0), aiFourHalf(0), aiFourOpen(0), aiTwoOpen(0),
							   humanThreeOpen(0), humanFourHalf(0), humanFourOpen(0), humanTwoOpen(0) {}

	void reset()
	{
		active = false;
		aiScore = humanScore = totalScore = 0;
		aiThreeOpen = aiFourHalf = aiFourOpen = aiTwoOpen = 0;
		humanThreeOpen = humanFourHalf = humanFourOpen = humanTwoOpen = 0;
	}
};

// Global instance for evaluation debug capture
extern EvaluationDebugCapture g_evalDebug;

// ============================================
// DEBUG ANALYZER STRUCTURES
// ============================================

/**
 * Detailed breakdown of move evaluation
 * Contains all scoring components for analysis
 */
struct EvaluationBreakdown
{
	Move move;
	int totalScore;
	int patternScore;
	int captureScore;
	int threatScore;
	int positionScore;
	int mateDistance;
	bool isWinning;
	bool isLosing;
	bool isCriticalThreat;
	std::string explanation;

	/**
	 * Detailed heuristic debug information
	 * Pattern counts and scores for deep analysis
	 */
	struct HeuristicDebug
	{
		int threeOpenCount;
		int threeOpenScore;
		int fourHalfCount;
		int fourHalfScore;
		int fourOpenCount;
		int fourOpenScore;
		int twoOpenCount;
		int twoOpenScore;
		std::string patternDetails;

		HeuristicDebug() : threeOpenCount(0), threeOpenScore(0), fourHalfCount(0),
						   fourHalfScore(0), fourOpenCount(0), fourOpenScore(0),
						   twoOpenCount(0), twoOpenScore(0) {}
	} heuristicDebug;

	EvaluationBreakdown(const Move &m = Move()) : move(m), totalScore(0), patternScore(0), captureScore(0),
												  threatScore(0), positionScore(0), mateDistance(0),
												  isWinning(false), isLosing(false), isCriticalThreat(false) {}
};

/**
 * Analysis of a single move at root level
 * Includes evaluation, search stats, and reasoning
 */
struct MoveAnalysis
{
	Move move;
	int score;
	int depth;
	int nodesEvaluated;
	EvaluationBreakdown breakdown;
	std::string reasoning;
	bool wasChosenAsRoot;

	MoveAnalysis(const Move &m = Move()) : move(m), score(0), depth(0), nodesEvaluated(0),
										   breakdown(m), wasChosenAsRoot(false) {}
};

/**
 * Complete game state snapshot for debugging
 * Captures top moves, chosen move, and game context
 */
struct GameSnapshot
{
	GameState state;
	std::vector<MoveAnalysis> topMoves;
	Move chosenMove;
	int totalTime;
	int totalNodes;
	std::string gamePhase;
	std::string criticalThreats;

	void saveToFile(const std::string &filename) const;
	void printToConsole() const;
};

#endif // DEBUG_TYPES_HPP
