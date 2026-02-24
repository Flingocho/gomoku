// ===============================================
// AI Engine - Suggestion Engine Module
// ===============================================
// Handles: Move suggestions using the main AI
// Dependencies: RuleEngine, AI
// ===============================================

#include "../../include/ai/suggestion_engine.hpp"
#include <algorithm>
#include <limits>
#include <iostream>

Move SuggestionEngine::getSuggestion(const GameState& state, int depth) {
    // Use the main AI with the specified depth
    AI suggestionAI(depth);
    
    // Get the best move from the AI
    Move bestMove = suggestionAI.getBestMove(state);
    
    if (bestMove.isValid()) {
        return bestMove;
    }
    
    // Fallback to quick suggestion if something fails
    return getQuickSuggestion(state);
}

Move SuggestionEngine::getQuickSuggestion(const GameState& state) {
    std::vector<Move> candidates = generateCandidates(state);
    
    if (candidates.empty()) {
        // Fallback: center of the board
        return Move(GameState::BOARD_CENTER, GameState::BOARD_CENTER);
    }
    
    Move bestMove = candidates[0];
    int bestScore = std::numeric_limits<int>::min();
    int currentPlayer = state.currentPlayer;
    
    // Evaluate each candidate
    for (const Move& move : candidates) {
        int score = evaluateMove(state, move, currentPlayer);
        
        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
        }
    }
    
    return bestMove;
}

int SuggestionEngine::evaluateMove(const GameState& state, const Move& move, int player) {
    int score = 0;
    int opponent = state.getOpponent(player);
    
    // Check for immediate win
    int winScore = checkWinningMove(state, move, player);
    if (winScore > 0) {
        return 10000000;  // Immediate win
    }
    
    // Block opponent's winning move
    int blockScore = checkBlockingMove(state, move, player);
    if (blockScore > 0) {
        return 5000000;  // Must block
    }
    
    // Check if it creates a four-in-a-row threat
    GameState testState = state;
    testState.board[move.x][move.y] = player;
    
    if (createsFourInRow(testState, move, player)) {
        score += 500000;  // Very strong threat
    }
    
    // Block opponent's four-in-a-row threat
    testState = state;
    testState.board[move.x][move.y] = opponent;
    if (createsFourInRow(testState, move, opponent)) {
        score += 300000;  // Block four-in-a-row threat
    }
    
    // Restore state with our piece
    testState = state;
    testState.board[move.x][move.y] = player;
    
    // Check if it creates an open three threat
    if (createsThreeOpen(testState, move, player)) {
        score += 100000;
    }
    
    // Block opponent's open three
    testState.board[move.x][move.y] = opponent;
    if (createsThreeOpen(testState, move, opponent)) {
        score += 50000;
    }
    
    // Capture bonus (less important than patterns)
    int captureScore = checkCaptureMove(state, move, player);
    score += captureScore * 10000;
    
    // General pattern value
    int patternScore = checkPatternValue(state, move, player);
    score += patternScore;
    
    // Connectivity bonus (proximity to other pieces)
    score += calculateConnectivity(state, move, player);
    
    // Centrality bonus (slight preference for central positions)
    int centerDist = std::max(std::abs(move.x - GameState::BOARD_CENTER), std::abs(move.y - GameState::BOARD_CENTER));
    score += (GameState::BOARD_CENTER - centerDist) * 10;
    
    return score;
}

std::vector<Move> SuggestionEngine::generateCandidates(const GameState& state) {
    std::vector<Move> candidates;
    
    // Generate moves within radius 2 of existing pieces
    bool hasPieces = false;
    
    for (int i = 0; i < GameState::BOARD_SIZE; i++) {
        for (int j = 0; j < GameState::BOARD_SIZE; j++) {
            if (!state.isEmpty(i, j)) {
                hasPieces = true;
                
                // Generate candidates around this piece
                for (int di = -2; di <= 2; di++) {
                    for (int dj = -2; dj <= 2; dj++) {
                        int ni = i + di;
                        int nj = j + dj;
                        
                        if (state.isValid(ni, nj) && state.isEmpty(ni, nj)) {
                            Move candidate(ni, nj);
                            
                            // Avoid duplicates
                            bool exists = false;
                            for (const Move& m : candidates) {
                                if (m.x == ni && m.y == nj) {
                                    exists = true;
                                    break;
                                }
                            }
                            
                            if (!exists && RuleEngine::isLegalMove(state, candidate)) {
                                candidates.push_back(candidate);
                            }
                        }
                    }
                }
            }
        }
    }
    
    // If no pieces on board (first move), suggest center
    if (!hasPieces) {
        candidates.push_back(Move(GameState::BOARD_CENTER, GameState::BOARD_CENTER));
    }
    
    return candidates;
}

int SuggestionEngine::checkWinningMove(const GameState& state, const Move& move, int player) {
    GameState testState = state;
    testState.board[move.x][move.y] = player;
    
    if (RuleEngine::checkWin(testState, player)) {
        return 100;  // Immediate win
    }
    
    return 0;
}

int SuggestionEngine::checkBlockingMove(const GameState& state, const Move& move, int player) {
    int opponent = state.getOpponent(player);
    
    // Simulate opponent playing here
    GameState testState = state;
    testState.board[move.x][move.y] = opponent;
    
    if (RuleEngine::checkWin(testState, opponent)) {
        return 100;  // Blocks opponent's win
    }
    
    return 0;
}

int SuggestionEngine::checkCaptureMove(const GameState& state, const Move& move, int player) {
    auto captures = RuleEngine::findCaptures(state, move, player);
    return captures.size() / 2;  // Number of captured pairs
}

int SuggestionEngine::checkPatternValue(const GameState& state, const Move& move, int player) {
    GameState testState = state;
    testState.board[move.x][move.y] = player;
    
    int score = 0;
    int directions[4][2] = {{1,0}, {0,1}, {1,1}, {1,-1}};
    
    // Count patterns in all 4 directions
    for (int d = 0; d < 4; d++) {
        int dx = directions[d][0];
        int dy = directions[d][1];
        
        int count = 1;  // The piece we just placed
        
        // Count forward
        int x = move.x + dx, y = move.y + dy;
        while (testState.isValid(x, y) && testState.getPiece(x, y) == player) {
            count++;
            x += dx;
            y += dy;
        }
        
        // Count backward
        x = move.x - dx;
        y = move.y - dy;
        while (testState.isValid(x, y) && testState.getPiece(x, y) == player) {
            count++;
            x -= dx;
            y -= dy;
        }
        
        // Score based on pattern (reduced since higher priorities are evaluated above)
        if (count == 2) score += 100;   // Two in a row
        else if (count == 1) score += 10;  // Single piece
    }
    
    return score;
}

bool SuggestionEngine::createsFourInRow(const GameState& state, const Move& move, int player) {
    int directions[4][2] = {{1,0}, {0,1}, {1,1}, {1,-1}};
    
    for (int d = 0; d < 4; d++) {
        int dx = directions[d][0];
        int dy = directions[d][1];
        
        int count = 1;  // The piece at move position
        
        // Count forward
        int x = move.x + dx, y = move.y + dy;
        int forward = 0;
        while (state.isValid(x, y) && state.getPiece(x, y) == player && forward < 4) {
            count++;
            forward++;
            x += dx;
            y += dy;
        }
        
        // Count backward
        x = move.x - dx;
        y = move.y - dy;
        int backward = 0;
        while (state.isValid(x, y) && state.getPiece(x, y) == player && backward < 4) {
            count++;
            backward++;
            x -= dx;
            y -= dy;
        }
        
        // Check if exactly 4 with at least one free end
        if (count == 4) {
            // Check if endpoints are free
            int frontX = move.x + (forward + 1) * dx;
            int frontY = move.y + (forward + 1) * dy;
            int backX = move.x - (backward + 1) * dx;
            int backY = move.y - (backward + 1) * dy;
            
            bool frontFree = state.isValid(frontX, frontY) && state.isEmpty(frontX, frontY);
            bool backFree = state.isValid(backX, backY) && state.isEmpty(backX, backY);
            
            if (frontFree || backFree) {
                return true;  // Four in a row with at least one free end
            }
        }
    }
    
    return false;
}

bool SuggestionEngine::createsThreeOpen(const GameState& state, const Move& move, int player) {
    int directions[4][2] = {{1,0}, {0,1}, {1,1}, {1,-1}};
    
    for (int d = 0; d < 4; d++) {
        int dx = directions[d][0];
        int dy = directions[d][1];
        
        int count = 1;
        
        // Count forward
        int x = move.x + dx, y = move.y + dy;
        int forward = 0;
        while (state.isValid(x, y) && state.getPiece(x, y) == player && forward < 3) {
            count++;
            forward++;
            x += dx;
            y += dy;
        }
        
        // Count backward
        x = move.x - dx;
        y = move.y - dy;
        int backward = 0;
        while (state.isValid(x, y) && state.getPiece(x, y) == player && backward < 3) {
            count++;
            backward++;
            x -= dx;
            y -= dy;
        }
        
        // Check if exactly 3 with both ends free
        if (count == 3) {
            int frontX = move.x + (forward + 1) * dx;
            int frontY = move.y + (forward + 1) * dy;
            int backX = move.x - (backward + 1) * dx;
            int backY = move.y - (backward + 1) * dy;
            
            bool frontFree = state.isValid(frontX, frontY) && state.isEmpty(frontX, frontY);
            bool backFree = state.isValid(backX, backY) && state.isEmpty(backX, backY);
            
            if (frontFree && backFree) {
                return true;  // Open three (both ends free)
            }
        }
    }
    
    return false;
}

int SuggestionEngine::calculateConnectivity(const GameState& state, const Move& move, int player) {
    int connectivity = 0;
    
    // Check all 8 adjacent directions
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            if (dx == 0 && dy == 0) continue;
            
            int adjX = move.x + dx;
            int adjY = move.y + dy;
            
            if (state.isValid(adjX, adjY)) {
                if (state.getPiece(adjX, adjY) == player) {
                    connectivity += 50;  // Bonus for proximity to own pieces
                } else if (state.getPiece(adjX, adjY) == state.getOpponent(player)) {
                    connectivity += 20;  // Minor bonus for proximity to opponent
                }
            }
        }
    }
    
    return connectivity;
}
