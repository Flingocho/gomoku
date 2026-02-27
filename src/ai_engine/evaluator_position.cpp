// ===============================================
// AI Engine - Evaluator Position Module
// ===============================================
// Handles: Main position evaluation, score calculation
// Dependencies: Evaluator (patterns, threats), RuleEngine
// ===============================================

#include "../../include/ai/evaluator.hpp"
#include "../../include/rules/rule_engine.hpp"
#include <iostream>

using namespace Directions;

// Global debug capture instance
EvaluationDebugCapture g_evalDebug;

// ===============================================
// MAIN EVALUATION FUNCTIONS
// ===============================================

/**
 * Main evaluator with mate distance information
 * Prioritizes closer mates (higher scores for quick wins)
 */
int Evaluator::evaluate(const GameState &state, int maxDepth, int currentDepth)
{
	int mateDistance = maxDepth - currentDepth;

	// Check immediate win conditions WITH mate distance
	// Use hasFiveInARow first: checkWin ignores breakable 5-in-a-row,
	// but the AI must still treat them as wins to block properly.
	if (RuleEngine::hasFiveInARow(state, GameState::PLAYER2) ||
	    RuleEngine::checkWin(state, GameState::PLAYER2))
	{
		return WIN - mateDistance;
	}
	if (RuleEngine::hasFiveInARow(state, GameState::PLAYER1) ||
	    RuleEngine::checkWin(state, GameState::PLAYER1))
	{
		return -WIN + mateDistance;
	}

	int aiScore = evaluateForPlayer(state, GameState::PLAYER2);
	int humanScore = evaluateForPlayer(state, GameState::PLAYER1);

	return aiScore - humanScore;
}

/**
 * Evaluate position without mate distance scoring
 */
int Evaluator::evaluate(const GameState &state)
{
	// Check immediate win conditions
	// hasFiveInARow catches breakable 5-in-a-row that checkWin ignores
	if (RuleEngine::hasFiveInARow(state, GameState::PLAYER2) ||
	    RuleEngine::checkWin(state, GameState::PLAYER2))
		return WIN;
	if (RuleEngine::hasFiveInARow(state, GameState::PLAYER1) ||
	    RuleEngine::checkWin(state, GameState::PLAYER1))
		return -WIN;

	// Evaluate AI with debug capture if active
	if (g_evalDebug.active)
		g_evalDebug.currentPlayer = GameState::PLAYER2;
	int aiScore = evaluateForPlayer(state, GameState::PLAYER2);

	// Evaluate HUMAN with debug capture if active
	if (g_evalDebug.active)
		g_evalDebug.currentPlayer = GameState::PLAYER1;
	int humanScore = evaluateForPlayer(state, GameState::PLAYER1);

	// Complete debug information
	if (g_evalDebug.active)
	{
		g_evalDebug.totalScore = aiScore - humanScore;
		g_evalDebug.aiScore = aiScore;
		g_evalDebug.humanScore = humanScore;
	}

	return aiScore - humanScore;
}

/**
 * Evaluate position for a specific player
 * Combines patterns, threats, and captures
 */
int Evaluator::evaluateForPlayer(const GameState &state, int player)
{
	int score = 0;

	// Capture debug info if active for this player
	bool captureForThisPlayer = g_evalDebug.active && player == g_evalDebug.currentPlayer;

	// Single-pass threat + combination evaluation (replaces 5 separate board scans)
	PatternCounts counts = countAllPatterns(state, player);
	score += evaluateThreatsAndCombinations(state, player, counts);

	// Unified evaluation: patterns + captures in single pass
	score += analyzePosition(state, player);

	// Capture information if activated
	if (captureForThisPlayer)
	{
		if (player == GameState::PLAYER2)
		{ // AI
			g_evalDebug.aiScore = score;
		}
		else
		{ // HUMAN
			g_evalDebug.humanScore = score;
		}
	}

	return score;
}

/**
 * Analyze complete position for a player
 * Evaluates patterns and capture opportunities in optimized single pass
 */
int Evaluator::analyzePosition(const GameState& state, int player) {
    int totalScore = 0;
    int opponent = state.getOpponent(player);
    
    // Variables for captures
    int captureOpportunities = 0;
    int captureThreats = 0;
    
    // OPTIMIZATION: Mark already evaluated lines
    bool evaluated[GameState::BOARD_SIZE][GameState::BOARD_SIZE][4] = {{{false}}};
    
    // ============================================
    // PART 1: PATTERN EVALUATION
    // ============================================
    for (int i = 0; i < GameState::BOARD_SIZE; i++) {
        for (int j = 0; j < GameState::BOARD_SIZE; j++) {
            if (state.board[i][j] == player) {
                for (int d = 0; d < MAIN_COUNT; d++) {
                    if (evaluated[i][j][d]) continue;
                    
                    int dx = MAIN[d][0];
                    int dy = MAIN[d][1];
                    
                    if (isLineStart(state, i, j, dx, dy, player)) {
                        PatternInfo pattern = analyzeLine(state, i, j, dx, dy, player);
                        totalScore += patternToScore(pattern);
                        
                        // Mark evaluated positions
                        int markX = i, markY = j;
                        for (int k = 0; k < pattern.consecutiveCount && 
                                       state.isValid(markX, markY); k++) {
                            if (markX >= 0 && markX < GameState::BOARD_SIZE && 
                                markY >= 0 && markY < GameState::BOARD_SIZE) {
                                evaluated[markX][markY][d] = true;
                            }
                            markX += dx;
                            markY += dy;
                        }
                    }
                }
            }
        }
    }
    
    // ============================================
    // PART 2: CAPTURE EVALUATION (OPTIMIZED)
    // ============================================
    
    // OFFENSIVE CAPTURES: Find opponent pairs player can capture
    auto myOpportunities = findAllCaptureOpportunities(state, player);
    for (const auto& opp : myOpportunities) {
        int captureCount = opp.captured.size() / 2;
        int currentCaptures = state.captures[player - 1];
        int newTotal = currentCaptures + captureCount;
        
        captureOpportunities += evaluateCaptureContext(
            state, player, opp.captured, newTotal
        );
    }
    
    // DEFENSIVE CAPTURES: Find player's pairs opponent can capture
    auto oppThreats = findAllCaptureOpportunities(state, opponent);
    for (const auto& threat : oppThreats) {
        int captureCount = threat.captured.size() / 2;
        int oppCurrentCaptures = state.captures[opponent - 1];
        int oppNewTotal = oppCurrentCaptures + captureCount;
        
        captureThreats += evaluateCaptureContext(
            state, opponent, threat.captured, oppNewTotal
        );
    }
    
    // ============================================
    // PART 3: EXISTING CAPTURES SCORING
    // ============================================
    int myCaptures = state.captures[player - 1];
    int oppCaptures = state.captures[opponent - 1];
    
    // Score my captures (progressive scaling)
    if (myCaptures >= 9) totalScore += 300000;      // Almost winning
    else if (myCaptures >= 8) totalScore += 200000;  // Very dangerous
    else if (myCaptures >= 6) totalScore += 15000;   // Strong
    else if (myCaptures >= 4) totalScore += 6000;    // Good
    else totalScore += myCaptures * 500;             // Base value
    
    // Penalize opponent captures (higher penalty)
    if (oppCaptures >= 9) totalScore -= 400000;      // Almost losing
    else if (oppCaptures >= 8) totalScore -= 300000;  // Critical danger
    else if (oppCaptures >= 6) totalScore -= 20000;   // Serious threat
    else if (oppCaptures >= 4) totalScore -= 8000;    // Concern
    else totalScore -= oppCaptures * 800;             // Base penalty
    
    totalScore += captureOpportunities;
    totalScore -= captureThreats;
    
    return totalScore;
}
