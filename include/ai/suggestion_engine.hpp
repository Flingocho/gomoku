#ifndef SUGGESTION_ENGINE_HPP
#define SUGGESTION_ENGINE_HPP

#include "../core/game_types.hpp"
#include "../rules/rule_engine.hpp"
#include "ai.hpp"
#include <vector>

/**
 * Suggestion engine for hotseat mode
 * Uses the main AI with reduced depth for fast suggestions
 */
class SuggestionEngine {
public:
    /**
     * Get the best suggestion using the main AI engine
     * @param state Current game state
     * @param depth Search depth (default 6 for speed/quality balance)
     * @return Best suggested move
     */
    static Move getSuggestion(const GameState& state, int depth = 6);
    
    /**
     * Get a quick suggestion using simple heuristics
     * Useful when speed is preferred over quality
     */
    static Move getQuickSuggestion(const GameState& state);
    
private:
    // Evaluate a move with simple heuristics
    static int evaluateMove(const GameState& state, const Move& move, int player);
    
    // Generate candidate moves (local area)
    static std::vector<Move> generateCandidates(const GameState& state);
    
    // Specific heuristics
    static int checkWinningMove(const GameState& state, const Move& move, int player);
    static int checkBlockingMove(const GameState& state, const Move& move, int player);
    static int checkCaptureMove(const GameState& state, const Move& move, int player);
    static int checkPatternValue(const GameState& state, const Move& move, int player);
    static bool createsFourInRow(const GameState& state, const Move& move, int player);
    static bool createsThreeOpen(const GameState& state, const Move& move, int player);
    static int calculateConnectivity(const GameState& state, const Move& move, int player);
};

#endif
