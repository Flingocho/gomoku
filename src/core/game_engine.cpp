#include "../../include/core/game_engine.hpp"
#include "../../include/core/display.hpp"
#include "../../include/debug/debug_analyzer.hpp"
#include <chrono>
#include <iostream>
#include <algorithm>
#include <map>
#include <utility>

void GameEngine::newGame()
{
	state = GameState(); // Reset to initial state
	lastHumanMove = Move(-1, -1); // Also reset the local field
}

bool GameEngine::makeHumanMove(const Move& move) {
    // In VS_HUMAN_SUGGESTED mode, both players are human
    if (currentMode == GameMode::VS_AI && state.currentPlayer != GameState::PLAYER1) {
        return false;
    }
    
    // OPTIONAL CAPTURE: If there's a pending 5-in-a-row that can be broken by capture,
    // the player CAN choose to capture, but it's not mandatory.
    // If they don't capture, they simply lose the game.
    bool ignoredCapture = false;

    if (!state.forcedCaptureMoves.empty() && state.forcedCapturePlayer == state.currentPlayer) {
        bool isCapturingMove = false;
        for (const Move& capturePos : state.forcedCaptureMoves) {
            if (capturePos.x == move.x && capturePos.y == move.y) {
                isCapturingMove = true;
                break;
            }
        }
        
        if (isCapturingMove) {
            if (g_debugAnalyzer && g_debugAnalyzer->isEnabled()) {
                std::cout << "CAPTURE MADE: Player " << state.currentPlayer 
                          << " broke the 5-in-a-row at (" << move.x << "," << move.y << ")" << std::endl;
            }
        } else {
            if (g_debugAnalyzer && g_debugAnalyzer->isEnabled()) {
                std::cout << "CAPTURE IGNORED: Player " << state.currentPlayer 
                          << " chose not to break the 5-in-a-row. Player " 
                          << state.pendingWinPlayer << " wins!" << std::endl;
            }
            ignoredCapture = true;
        }
    }
    
    lastHumanMove = move;
    state.lastHumanMove = move;
    
    RuleEngine::MoveResult result = RuleEngine::applyMove(state, move);
    
    if (result.success) {
        if (ignoredCapture) {
            // Player chose not to break the 5-in-a-row — pendingWinPlayer
            // remains set so isGameOver() will detect the loss.
            state.forcedCaptureMoves.clear();
            state.forcedCapturePlayer = 0;
        } else {
            checkAndSetForcedCaptures();
        }
    }
    
    return result.success;
}

Move GameEngine::makeAIMove() {
    if (state.currentPlayer != GameState::PLAYER2) return Move();
    
    bool hadCaptureOpportunity = !state.forcedCaptureMoves.empty() && 
                                  state.forcedCapturePlayer == state.currentPlayer;
    
    // CHECK FOR CAPTURE OPPORTUNITY: AI CAN capture to break a 5-in-a-row (optional)
    if (hadCaptureOpportunity && g_debugAnalyzer && g_debugAnalyzer->isEnabled()) {
        std::cout << "AI CAPTURE OPPORTUNITY: Can capture at " << state.forcedCaptureMoves.size() 
                  << " positions to prevent opponent win" << std::endl;
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    Move bestMove = ai.getBestMove(state);
    auto end = std::chrono::high_resolution_clock::now();
    lastAITime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    DEBUG_LOG_STATS("AI Stats: " + std::to_string(ai.getLastNodesEvaluated()) + 
                   " nodes, " + std::to_string(lastAITime) + "ms");
    
    if (bestMove.isValid()) {
        // Check if AI chose to capture when opportunity existed
        bool isCapture = false;
        for (const Move& captureMove : state.forcedCaptureMoves) {
            if (captureMove.x == bestMove.x && captureMove.y == bestMove.y) {
                isCapture = true;
                break;
            }
        }
        
        if (hadCaptureOpportunity && g_debugAnalyzer && g_debugAnalyzer->isEnabled()) {
            if (isCapture) {
                std::cout << "AI chose to CAPTURE at (" << bestMove.x << "," << bestMove.y 
                          << ") - preventing opponent win" << std::endl;
            } else {
                std::cout << "AI chose NOT to capture (will lose) - AI played at (" 
                          << bestMove.x << "," << bestMove.y << ")" << std::endl;
            }
        }
        
        RuleEngine::applyMove(state, bestMove);
        
        if (hadCaptureOpportunity && !isCapture) {
            // AI chose not to break the 5-in-a-row — pendingWinPlayer
            // remains set so isGameOver() will detect the loss.
            state.forcedCaptureMoves.clear();
            state.forcedCapturePlayer = 0;
        } else {
            checkAndSetForcedCaptures();
        }
    }
    
    return bestMove;
}

bool GameEngine::isGameOver() const
{
	// If there's a pending win that wasn't broken by capture, game is over
	if (state.pendingWinPlayer != 0) {
		return true;
	}
	
	return RuleEngine::checkWin(state, GameState::PLAYER1) ||
		   RuleEngine::checkWin(state, GameState::PLAYER2);
}

int GameEngine::getWinner() const
{
	// If there's a pending win that wasn't broken by capture
	if (state.pendingWinPlayer != 0) {
		return state.pendingWinPlayer;
	}
	
	if (RuleEngine::checkWin(state, GameState::PLAYER1))
		return GameState::PLAYER1;
	if (RuleEngine::checkWin(state, GameState::PLAYER2))
		return GameState::PLAYER2;
	return 0; // No winner
}

std::vector<Move> GameEngine::findWinningLine() const {
    std::vector<Move> line;
    
    // Determine the winner
    int winner = 0;
    if (RuleEngine::checkWin(state, GameState::PLAYER1)) {
        winner = GameState::PLAYER1;
    } else if (RuleEngine::checkWin(state, GameState::PLAYER2)) {
        winner = GameState::PLAYER2;
    } else if (state.pendingWinPlayer != 0) {
        // Pending win (5-in-a-row not broken by capture)
        winner = state.pendingWinPlayer;
    } else {
        return line; // No alignment winner found
    }
    
    // Search for the winning line of 5
    int directions[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};
    
    for (int i = 0; i < GameState::BOARD_SIZE; i++) {
        for (int j = 0; j < GameState::BOARD_SIZE; j++) {
            if (state.board[i][j] != winner) continue;
            
            for (int d = 0; d < 4; d++) {
                int dx = directions[d][0];
                int dy = directions[d][1];
                
                // Count consecutive pieces in this direction
                int count = 0;
                std::vector<Move> tempLine;
                
                for (int k = 0; k < 5; k++) {
                    int ni = i + k * dx;
                    int nj = j + k * dy;
                    
                    if (state.isValid(ni, nj) && state.board[ni][nj] == winner) {
                        tempLine.push_back(Move(ni, nj));
                        count++;
                    } else {
                        break;
                    }
                }
                
                if (count >= 5) {
                    return tempLine; // Found
                }
            }
        }
    }
    
    return line;
}

void GameEngine::checkAndSetForcedCaptures() {
    // Clear any previous forced captures
    state.forcedCaptureMoves.clear();
    state.forcedCapturePlayer = 0;
    state.pendingWinPlayer = 0;
    
    // Check if the PREVIOUS player (who just moved) created a 5-in-a-row
    int previousPlayer = state.getOpponent(state.currentPlayer);
    
    // Search for 5-in-a-row for the previous player
    int directions[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};
    
    for (int i = 0; i < GameState::BOARD_SIZE; i++) {
        for (int j = 0; j < GameState::BOARD_SIZE; j++) {
            if (state.board[i][j] != previousPlayer) continue;
            
            for (int d = 0; d < 4; d++) {
                int dx = directions[d][0];
                int dy = directions[d][1];
                
                // Check if there's a line of 5 starting here
                int count = 0;
                for (int k = 0; k < 5; k++) {
                    int ni = i + k * dx;
                    int nj = j + k * dy;
                    if (state.isValid(ni, nj) && state.board[ni][nj] == previousPlayer) {
                        count++;
                    } else {
                        break;
                    }
                }
                
                if (count >= 5) {
                    // Found 5-in-a-row! Check if current player can break it
                    Move lineStart(i, j);
                    std::vector<Move> captureMoves;
                    
                    if (RuleEngine::canBreakLineByCapture(state, lineStart, dx, dy, 
                                                          previousPlayer, &captureMoves)) {
                        // Current player CAN break it - set capture opportunities
                        state.forcedCaptureMoves = captureMoves;
                        state.forcedCapturePlayer = state.currentPlayer;
                        state.pendingWinPlayer = previousPlayer;
                        
                        std::cout << "CAPTURE OPPORTUNITY: Player " << state.currentPlayer 
                                  << " CAN capture at one of " << captureMoves.size() 
                                  << " positions to prevent Player " << previousPlayer 
                                  << " from winning! (or choose to lose)" << std::endl;
                        for (const Move& m : captureMoves) {
                            std::cout << "  - Capture position: (" << m.x << "," << m.y << ")" << std::endl;
                        }
                        return; // Only need to find one
                    }
                }
            }
        }
    }
}
