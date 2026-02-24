// ============================================
// RULES_VALIDATION.CPP
// Move validation: double free-three
// Valid free-three pattern detection
// ============================================

#include "../../include/rules/rule_engine.hpp"

using namespace Directions;

bool RuleEngine::createsDoubleFreeThree(const GameState &state, const Move &move, int player)
{
	// Create temporary copy for testing
	GameState tempState = state;
	tempState.board[move.x][move.y] = player;

	auto freeThrees = findFreeThrees(tempState, move, player);
	return freeThrees.size() >= 2;
}

std::vector<Move> RuleEngine::findFreeThrees(const GameState &state, const Move &move, int player)
{
	std::vector<Move> freeThrees;

	// Check the 4 main directions
	for (int d = 0; d < MAIN_COUNT; d++)
	{
		if (isFreeThree(state, move, MAIN[d][0], MAIN[d][1], player))
		{
			freeThrees.push_back(Move(MAIN[d][0], MAIN[d][1])); // Direction as identifier
		}
	}

	return freeThrees;
}

bool RuleEngine::isFreeThree(const GameState &state, const Move &move,
							 int dx, int dy, int player)
{
	// A free-three is any pattern of 3 pieces in a window of 5 positions
	// where both ends are free and a threat of 4 can be formed
	// Includes patterns with gaps such as -XX-X- or -X-XX-
	
	// Search all windows of 5 positions containing the move
	for (int offset = -4; offset <= 0; offset++)
	{
		Move windowStart(move.x + offset * dx, move.y + offset * dy);
		
		// Verify the window of 5 is within the board
		bool validWindow = true;
		for (int i = 0; i < 5; i++)
		{
			Move pos(windowStart.x + i * dx, windowStart.y + i * dy);
			if (!state.isValid(pos.x, pos.y))
			{
				validWindow = false;
				break;
			}
		}
		
		if (!validWindow) continue;
		
		// Verify the move is within this window
		bool moveInWindow = false;
		for (int i = 0; i < 5; i++)
		{
			Move pos(windowStart.x + i * dx, windowStart.y + i * dy);
			if (pos.x == move.x && pos.y == move.y)
			{
				moveInWindow = true;
				break;
			}
		}
		
		if (!moveInWindow) continue;
		
		// Create array of window states
		int windowState[5];
		for (int i = 0; i < 5; i++)
		{
			Move pos(windowStart.x + i * dx, windowStart.y + i * dy);
			
			if (pos.x == move.x && pos.y == move.y)
			{
				windowState[i] = player; // El movimiento cuenta como ficha del jugador
			}
			else
			{
				windowState[i] = state.getPiece(pos.x, pos.y);
			}
		}
		
		// Count player and opponent pieces
		int playerPieces = 0;
		int opponentPieces = 0;
		int emptySpaces = 0;
		
		for (int i = 0; i < 5; i++)
		{
			if (windowState[i] == player)
				playerPieces++;
			else if (windowState[i] == state.getOpponent(player))
				opponentPieces++;
			else
				emptySpaces++;
		}
		
		// Free-three requires: exactly 3 player pieces, 0 opponent pieces, 2 empty spaces
		if (playerPieces == 3 && opponentPieces == 0 && emptySpaces == 2)
		{
			// Verify both ends are free
			Move leftExtreme(windowStart.x - dx, windowStart.y - dy);
			Move rightExtreme(windowStart.x + 5 * dx, windowStart.y + 5 * dy);
			
			bool leftFree = state.isValid(leftExtreme.x, leftExtreme.y) && 
							state.isEmpty(leftExtreme.x, leftExtreme.y);
			bool rightFree = state.isValid(rightExtreme.x, rightExtreme.y) && 
							 state.isEmpty(rightExtreme.x, rightExtreme.y);
			
			// Both ends must be free to qualify as "free"
			if (leftFree && rightFree)
			{
				// Verify it's a valid free-three pattern
				// Must be possible to form four consecutive by filling the spaces
				if (isValidFreeThreePattern(windowState, player))
				{
					return true;
				}
			}
		}
	}
	
	return false;
}

bool RuleEngine::isValidFreeThreePattern(const int windowState[5], int player)
{
    // Define all valid free-three patterns with 3 pieces and 2 empty spaces
    // Where X = player piece, - = empty space
    
    // Valid free-three patterns:
    // XXX-- , XX-X- , XX--X , X-XX- , X-X-X , X--XX , -XXX- , -XX-X , -X-XX , --XXX
    
    int empty = 0; // Represent empty space as 0
    
    // Create array of valid patterns
    int validPatterns[][5] = {
        {player, player, player, empty, empty},  // XXX--
        {player, player, empty, player, empty},  // XX-X-
        {player, player, empty, empty, player},  // XX--X
        {player, empty, player, player, empty},  // X-XX-
        {player, empty, player, empty, player},  // X-X-X
        {player, empty, empty, player, player},  // X--XX
        {empty, player, player, player, empty},  // -XXX-
        {empty, player, player, empty, player},  // -XX-X
        {empty, player, empty, player, player},  // -X-XX
        {empty, empty, player, player, player}   // --XXX
    };
    
    int numPatterns = sizeof(validPatterns) / sizeof(validPatterns[0]);
    
    // Check if windowState matches any valid pattern
    for (int p = 0; p < numPatterns; p++)
    {
        bool matches = true;
        for (int i = 0; i < 5; i++)
        {
            if (windowState[i] != validPatterns[p][i])
            {
                matches = false;
                break;
            }
        }
        
        if (matches)
        {
            // Verify this pattern can form a real threat
            // (can be completed to 4 in a row)
            return canFormThreat(validPatterns[p], player);
        }
    }
    
    return false;
}

bool RuleEngine::canFormThreat(const int pattern[5], int player)
{
    // A pattern can form a threat if:
    // 1. It has exactly 3 player pieces and 2 empty spaces
    // 2. At least one empty position can complete a sequence of 4
    
    int playerCount = 0;
    int emptyCount = 0;
    
    for (int i = 0; i < 5; i++)
    {
        if (pattern[i] == player) playerCount++;
        else if (pattern[i] == 0) emptyCount++;
    }
    
    // Must have exactly 3 pieces and 2 spaces
    if (playerCount != 3 || emptyCount != 2) return false;
    
    // Check if we can form threats of 4
    // Simulate filling each empty space and check for 4 consecutive
    for (int i = 0; i < 5; i++)
    {
        if (pattern[i] == 0) // Empty space
        {
            // Simulate placing a piece here
            int tempPattern[5];
            for (int j = 0; j < 5; j++)
            {
                tempPattern[j] = (j == i) ? player : pattern[j];
            }
            
            // Check if it forms 4 consecutive
            if (hasFourConsecutive(tempPattern, player))
            {
                return true;
            }
        }
    }
    
    return false;
}

bool RuleEngine::hasFourConsecutive(const int pattern[5], int player)
{
    // Search for 4 consecutive player pieces in the pattern
    for (int i = 0; i <= 1; i++) // Can only start at position 0 or 1 to have 4 consecutive
    {
        bool consecutive = true;
        for (int j = 0; j < 4; j++)
        {
            if (pattern[i + j] != player)
            {
                consecutive = false;
                break;
            }
        }
        
        if (consecutive) return true;
    }
    
    return false;
}
