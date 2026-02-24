#ifndef EVALUATOR_HPP
#define EVALUATOR_HPP

#include "../core/game_types.hpp"
#include "../utils/directions.hpp"
#include "../debug/debug_types.hpp"

class Evaluator
{
public:
	static constexpr int WIN = 600000;
	static constexpr int FOUR_OPEN = 50000;
	static constexpr int FOUR_HALF = 25000;
	static constexpr int THREE_OPEN = 10000;
	static constexpr int THREE_HALF = 1500;
	static constexpr int TWO_OPEN = 100;
	static constexpr int CAPTURE_OPPORTUNITY = 5000;
	static constexpr int CAPTURE_THREAT = 6000;
	static constexpr int CAPTURE_WIN = 500000;
	static constexpr int CAPTURE_PREVENT_LOSS = 400000;

	static int evaluateForPlayer(const GameState &state, int player);

	// Evaluate position with mate distance scoring
	static int evaluate(const GameState &state, int maxDepth, int currentDepth);

	// Evaluate position (without mate distance)
	static int evaluate(const GameState &state);

	static int evaluateImmediateThreats(const GameState &state, int player);

	// Detect mate-in-1 threats using pattern analysis
	static bool hasWinningThreats(const GameState &state, int player);

	// Count specific pattern types (public for debug access)
	static int countPatternType(const GameState &state, int player, int consecutiveCount, int freeEnds);

private:
	struct PatternInfo
	{
		int consecutiveCount; // Maximum consecutive pieces
		int totalPieces;	  // Total pieces in pattern (including gaps)
		int freeEnds;		  // Number of open ends
		bool hasGaps;		  // Whether pattern has small gaps
		int totalSpan;		  // Total span of pattern
		int gapCount;		  // Number of gaps in pattern
	};

	static int analyzePosition(const GameState &state, int player);

	static PatternInfo analyzeLine(const GameState &state, int x, int y,
								   int dx, int dy, int player);

	static int patternToScore(const PatternInfo &pattern);

	static bool isLineStart(const GameState &state, int x, int y, int dx, int dy, int player);

	static bool isValidCapturePattern(const GameState &state, int x, int y,
									  int dx, int dy, int attacker, int victim);
	static bool captureBreaksOpponentPattern(const GameState &state, const std::vector<Move> &capturedPieces, int opponent);
	struct CaptureOpportunity {
        Move position;
        std::vector<Move> captured;
        int value;
        
        CaptureOpportunity() : value(0) {}
    };
    
    static std::vector<CaptureOpportunity> findAllCaptureOpportunities(
        const GameState& state, int player);
    
    static int evaluateCaptureContext(
        const GameState& state,
        int player,
        const std::vector<Move>& capturedPieces,
        int newCaptureCount);
    
    static int countPatternThroughPosition(
        const GameState& state,
        const Move& pos,
        int dx, int dy,
        int player);
};

#endif