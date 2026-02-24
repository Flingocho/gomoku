// ============================================
// RULES_CORE.CPP
// Main rule engine functions
// Move application and basic validation
// ============================================

#include "../../include/rules/rule_engine.hpp"
#include "../../include/utils/zobrist_hasher.hpp"
#include <iostream>

RuleEngine::MoveResult RuleEngine::applyMove(GameState &state, const Move &move)
{
    MoveResult result;

    // 1. Verify the move is valid
    if (!state.isEmpty(move.x, move.y))
    {
        return result; // success = false
    }

    // 2. Check double free-three before placing
    if (createsDoubleFreeThree(state, move, state.currentPlayer))
    {
        return result; // success = false
    }

    // Save state for Zobrist hash
    int oldMyCaptures = state.captures[state.currentPlayer - 1];
    int currentPlayer = state.currentPlayer;

    // 3. Place the piece
    state.board[move.x][move.y] = state.currentPlayer;

    // 4. Find captures made by the current player
    CaptureInfo captureInfo = findAllCaptures(state, move, state.currentPlayer);
    result.myCapturedPieces = captureInfo.myCapturedPieces;

    // 5. Apply the current player's captures
    for (const Move &captured : result.myCapturedPieces)
    {
        state.board[captured.x][captured.y] = GameState::EMPTY;
    }
    state.captures[state.currentPlayer - 1] += result.myCapturedPieces.size() / 2;
    if (state.captures[state.currentPlayer - 1] > 10)
        state.captures[state.currentPlayer - 1] = 10;

    // Note: opponentCapturedPieces remains empty - no opponent captures are applied

    // 6. Check for win
    result.createsWin = checkWin(state, state.currentPlayer);

    // 7. Update Zobrist hash
    int newMyCaptures = state.captures[currentPlayer - 1];
    
    if (state.hasher) {
        state.zobristHash = state.hasher->updateHashAfterMove(
            state.zobristHash,
            move,
            currentPlayer,
            result.myCapturedPieces,
            result.opponentCapturedPieces, // Empty
            oldMyCaptures,
            newMyCaptures,
            state.captures[state.getOpponent(currentPlayer) - 1], // Unchanged
            state.captures[state.getOpponent(currentPlayer) - 1]  // Unchanged
        );
    }

    // 8. Advance turn
    state.currentPlayer = state.getOpponent(state.currentPlayer);
    state.turnCount++;

    result.success = true;
    return result;
}

bool RuleEngine::isLegalMove(const GameState &state, const Move &move)
{
	if (!state.isEmpty(move.x, move.y))
		return false;
	return !createsDoubleFreeThree(state, move, state.currentPlayer);
}

int RuleEngine::countInDirection(const GameState &state, const Move &start,
								 int dx, int dy, int player)
{
	int count = 0;
	int x = start.x + dx;
	int y = start.y + dy;

	while (state.isValid(x, y) && state.getPiece(x, y) == player)
	{
		count++;
		x += dx;
		y += dy;
	}

	return count;
}
