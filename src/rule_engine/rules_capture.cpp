// ============================================
// RULES_CAPTURE.CPP
// Capture detection and application logic
// Includes line break verification by capture
// ============================================

#include "../../include/rules/rule_engine.hpp"
#include <iostream>

using namespace Directions;

RuleEngine::CaptureInfo RuleEngine::findAllCaptures(const GameState &state, const Move &move, int player)
{
    CaptureInfo info;
    int opponent = state.getOpponent(player);

    // Search all 8 directions for captures by the current player (X-O-O-X)
    for (int d = 0; d < ALL_COUNT; d++)
    {
        int dx = ALL[d][0];
        int dy = ALL[d][1];

        // Pattern: move-OPP-OPP-MINE
        // All 8 directions cover both directions of each axis,
        // so a single forward pattern is sufficient.
        Move opp1(move.x + dx, move.y + dy);
        Move opp2(move.x + 2 * dx, move.y + 2 * dy);
        Move myOther(move.x + 3 * dx, move.y + 3 * dy);

        if (state.isValid(opp1.x, opp1.y) && 
            state.isValid(opp2.x, opp2.y) && 
            state.isValid(myOther.x, myOther.y))
        {
            if (state.getPiece(opp1.x, opp1.y) == opponent &&
                state.getPiece(opp2.x, opp2.y) == opponent &&
                state.getPiece(myOther.x, myOther.y) == player)
            {
                info.myCapturedPieces.push_back(opp1);
                info.myCapturedPieces.push_back(opp2);
            }
        }
    }

    // opponentCapturedPieces remains empty
    // (Opponent captures are not applied - only computed for heuristics if needed)
    
    return info;
}

std::vector<Move> RuleEngine::findCaptures(const GameState &state, const Move &move, int player)
{
	std::vector<Move> allCaptures;

	// Search all 8 directions
	for (int d = 0; d < ALL_COUNT; d++)
	{
		auto dirCaptures = findCapturesInDirection(state, move, player,
												   ALL[d][0], ALL[d][1]);
		allCaptures.insert(allCaptures.end(), dirCaptures.begin(), dirCaptures.end());
	}

	return allCaptures;
}

std::vector<Move> RuleEngine::findCapturesInDirection(const GameState &state,
													  const Move &move, int player,
													  int dx, int dy)
{
	std::vector<Move> captures;
	int opponent = state.getOpponent(player);

	// Patrón: PLAYER + OPPONENT + OPPONENT + PLAYER
	Move pos1(move.x + dx, move.y + dy);
	Move pos2(move.x + 2 * dx, move.y + 2 * dy);
	Move pos3(move.x + 3 * dx, move.y + 3 * dy);

	if (state.isValid(pos1.x, pos1.y) && state.isValid(pos2.x, pos2.y) &&
		state.isValid(pos3.x, pos3.y))
	{

		if (state.getPiece(pos1.x, pos1.y) == opponent &&
			state.getPiece(pos2.x, pos2.y) == opponent &&
			state.getPiece(pos3.x, pos3.y) == player)
		{

			captures.push_back(pos1);
			captures.push_back(pos2);
		}
	}

	return captures;
}

bool RuleEngine::canBreakLineByCapture(
    const GameState &state, 
    const Move &lineStart,  // First piece of the line of 5
    int dx, int dy,         // Direction of the line
    int winningPlayer,
    std::vector<Move>* outCaptureMoves  // OUT: positions where opponent can capture
) {
    int opponent = state.getOpponent(winningPlayer);
    
    // Collect all positions in the line of 5
    std::vector<Move> linePositions;
    for (int i = 0; i < 5; i++) {
        linePositions.push_back(Move(lineStart.x + i*dx, lineStart.y + i*dy));
    }
    
    bool foundCapture = false;
    
    // For each piece in the line, check if the opponent can capture it
    // in any direction (not just the line direction)
    for (const Move& piece : linePositions) {
        
        // Try all 8 directions for captures
        for (int d = 0; d < ALL_COUNT; d++) {
            int cdx = ALL[d][0];
            int cdy = ALL[d][1];
            
            // Look for pattern X-O-O-? where X=opponent, O=current piece
            // The current piece must be the first O of the pair
            Move secondPiece(piece.x + cdx, piece.y + cdy);
            Move before(piece.x - cdx, piece.y - cdy);
            Move after(secondPiece.x + cdx, secondPiece.y + cdy);
            
            // Check pattern: OPP-PIECE-SECOND-EMPTY
            if (state.isValid(before.x, before.y) && 
                state.getPiece(before.x, before.y) == opponent &&
                state.isValid(secondPiece.x, secondPiece.y) &&
                state.getPiece(secondPiece.x, secondPiece.y) == winningPlayer &&
                state.isValid(after.x, after.y) &&
                state.isEmpty(after.x, after.y)) {
                
                foundCapture = true;
                if (outCaptureMoves) {
                    outCaptureMoves->push_back(after);
                }
            }
            
            // Also check pattern: EMPTY-PIECE-SECOND-OPP
            if (state.isValid(after.x, after.y) &&
                state.getPiece(after.x, after.y) == opponent &&
                state.isValid(secondPiece.x, secondPiece.y) &&
                state.getPiece(secondPiece.x, secondPiece.y) == winningPlayer &&
                state.isValid(before.x, before.y) &&
                state.isEmpty(before.x, before.y)) {
                
                foundCapture = true;
                if (outCaptureMoves) {
                    outCaptureMoves->push_back(before);
                }
            }
        }
    }
    
    return foundCapture;
}

bool RuleEngine::opponentCanCaptureNextTurn(
    const GameState &state, 
    int opponent
) {
    // Try each empty cell
    for (int i = 0; i < GameState::BOARD_SIZE; i++) {
        for (int j = 0; j < GameState::BOARD_SIZE; j++) {
            if (state.isEmpty(i, j)) {
                Move testMove(i, j);
                
                // Does this move create captures?
                auto captures = findCaptures(state, testMove, opponent);
                if (!captures.empty()) {
                    return true;  // Opponent can capture on the next turn
                }
            }
        }
    }
    
    return false;
}
