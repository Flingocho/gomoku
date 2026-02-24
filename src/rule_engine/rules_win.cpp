// ============================================
// RULES_WIN.CPP
// Win detection logic
// Win by five-in-a-row and by captures
// ============================================

#include "../../include/rules/rule_engine.hpp"
#include <iostream>

using namespace Directions;

bool RuleEngine::checkWin(const GameState &state, int player)
{
    int opponent = state.getOpponent(player);
    
    // 1. Win by captures
    if (state.captures[player - 1] >= 10)
    {
        return true;
    }

    // 2. Win by five in a row (with verification)
    for (int i = 0; i < GameState::BOARD_SIZE; i++) {
        for (int j = 0; j < GameState::BOARD_SIZE; j++) {
            if (state.board[i][j] == player) {
                Move pos(i, j);
                
                // Check for a line of 5 in each direction
                for (int d = 0; d < MAIN_COUNT; d++) {
                    int dx = MAIN[d][0];
                    int dy = MAIN[d][1];
                    
                    if (checkLineWinInDirection(state, pos, dx, dy, player)) {
                        // Found a line of 5
                        
                        // Verification 1: Can the opponent break it via capture?
                        // This will be handled by the game engine setting forced captures
                        std::vector<Move> captureMoves;
                        bool canBreak = canBreakLineByCapture(state, pos, dx, dy, player, &captureMoves);
                        
                        if (canBreak) {
                            // NOTE: The game engine will handle setting forced captures
                            // For now, we treat this as "not a win yet"
                            continue;
                        }
                        
                        // Verification 2: Is the winning player at risk of losing by capture?
                        if (state.captures[opponent - 1] >= 8) {
                            // 4+ pairs captured against the winning player
                            // Can the opponent capture one more?
                            if (opponentCanCaptureNextTurn(state, opponent)) {
                                return false;  // No win, opponent can win by capture
                            }
                        }
                        
                        // If we reach here, it's a legitimate win
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

bool RuleEngine::checkLineWin(const GameState &state, const Move &move, int player)
{
	// Check the 4 main directions (no duplicates)
	for (int d = 0; d < MAIN_COUNT; d++)
	{
		int dx = MAIN[d][0];
		int dy = MAIN[d][1];

		int count = 1; // The current piece
		count += countInDirection(state, move, dx, dy, player);
		count += countInDirection(state, move, -dx, -dy, player);

		if (count >= 5)
			return true;
	}

	return false;
}

bool RuleEngine::checkLineWinInDirection(const GameState &state, const Move &start, 
                                         int dx, int dy, int player)
{
    // Check if there are 5 or more in a line from 'start' in direction (dx, dy)
    
    // Only count if 'start' is the actual beginning of the line
    // (to avoid counting the same line multiple times)
    
    // Verify there is no piece of the same player before start
    Move before(start.x - dx, start.y - dy);
    if (state.isValid(before.x, before.y) && state.getPiece(before.x, before.y) == player) {
        return false;  // Not the actual start of the line
    }
    
    // Count consecutive pieces from start
    int count = 1;  // The piece at 'start'
    Move current(start.x + dx, start.y + dy);
    
    while (state.isValid(current.x, current.y) && 
           state.getPiece(current.x, current.y) == player) {
        count++;
        current.x += dx;
        current.y += dy;
    }
    
    // Are there 5 or more?
    return (count >= 5);
}
