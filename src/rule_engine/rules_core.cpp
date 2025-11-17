// ============================================
// RULES_CORE.CPP
// Funciones principales del motor de reglas
// Aplicación de movimientos y validación básica
// ============================================

#include "../../include/rules/rule_engine.hpp"
#include "../../include/utils/zobrist_hasher.hpp"
#include <iostream>

RuleEngine::MoveResult RuleEngine::applyMove(GameState &state, const Move &move)
{
    MoveResult result;

    // 1. Verificar que el movimiento es básicamente válido
    if (!state.isEmpty(move.x, move.y))
    {
        return result; // success = false
    }

    // 2. Verificar double free-three ANTES de colocar
    if (createsDoubleFreeThree(state, move, state.currentPlayer))
    {
        return result; // success = false
    }

    // GUARDAR estado para hash Zobrist
    int oldMyCaptures = state.captures[state.currentPlayer - 1];
    int currentPlayer = state.currentPlayer;

    // 3. Colocar la pieza
    state.board[move.x][move.y] = state.currentPlayer;

    // 4. Buscar SOLO las capturas que YO hago
    CaptureInfo captureInfo = findAllCaptures(state, move, state.currentPlayer);
    result.myCapturedPieces = captureInfo.myCapturedPieces;

    // 5. Aplicar MIS capturas
    for (const Move &captured : result.myCapturedPieces)
    {
        state.board[captured.x][captured.y] = GameState::EMPTY;
    }
    state.captures[state.currentPlayer - 1] += result.myCapturedPieces.size() / 2;

    // NOTA: opponentCapturedPieces se queda vacío - no se aplica nada del oponente

    // 6. Verificar victoria
    result.createsWin = checkWin(state, state.currentPlayer);

    // 7. Actualizar hash Zobrist
    int newMyCaptures = state.captures[currentPlayer - 1];
    
    state.zobristHash = state.hasher->updateHashAfterMove(
        state.zobristHash,
        move,
        currentPlayer,
        result.myCapturedPieces,
        result.opponentCapturedPieces, // Vacío
        oldMyCaptures,
        newMyCaptures,
        state.captures[state.getOpponent(currentPlayer) - 1], // No cambió
        state.captures[state.getOpponent(currentPlayer) - 1]  // No cambió
    );

    // 8. Avanzar turno
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
