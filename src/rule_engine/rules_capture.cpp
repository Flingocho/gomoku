// ============================================
// RULES_CAPTURE.CPP
// Lógica de detección y aplicación de capturas
// Incluye verificación de rupturas de líneas por captura
// ============================================

#include "../../include/rules/rule_engine.hpp"
#include <iostream>

using namespace Directions;

RuleEngine::CaptureInfo RuleEngine::findAllCaptures(const GameState &state, const Move &move, int player)
{
    CaptureInfo info;
    int opponent = state.getOpponent(player);

    // Buscar en las 8 direcciones SOLO capturas que YO hago (X-O-O-X)
    for (int d = 0; d < ALL_COUNT; d++)
    {
        int dx = ALL[d][0];
        int dy = ALL[d][1];

        // Patrón 1: NUEVA(move)-OPP-OPP-MIA
        // Esto es: X(nueva)-O-O-X(existente)
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
                // Captura válida hacia adelante
                info.myCapturedPieces.push_back(opp1);
                info.myCapturedPieces.push_back(opp2);
            }
        }

        // Patrón 2: MIA-OPP-OPP-NUEVA(move)
        // Esto es: X(existente)-O-O-X(nueva)
        Move opp1_back(move.x - dx, move.y - dy);
        Move opp2_back(move.x - 2 * dx, move.y - 2 * dy);
        Move myOther_back(move.x - 3 * dx, move.y - 3 * dy);

        if (state.isValid(opp1_back.x, opp1_back.y) && 
            state.isValid(opp2_back.x, opp2_back.y) && 
            state.isValid(myOther_back.x, myOther_back.y))
        {
            if (state.getPiece(opp1_back.x, opp1_back.y) == opponent &&
                state.getPiece(opp2_back.x, opp2_back.y) == opponent &&
                state.getPiece(myOther_back.x, myOther_back.y) == player)
            {
                // Captura válida hacia atrás
                info.myCapturedPieces.push_back(opp1_back);
                info.myCapturedPieces.push_back(opp2_back);
            }
        }
    }

    // opponentCapturedPieces se queda VACÍO
    // (NO se aplican capturas del oponente - solo se calculan para heurística si se necesita)
    
    return info;
}

std::vector<Move> RuleEngine::findCaptures(const GameState &state, const Move &move, int player)
{
	std::vector<Move> allCaptures;

	// Buscar en las 8 direcciones
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
    const Move &lineStart,  // Primera ficha de la línea de 5
    int dx, int dy,         // Dirección de la línea
    int winningPlayer,
    std::vector<Move>* outCaptureMoves  // OUT: positions where opponent can capture
) {
    int opponent = state.getOpponent(winningPlayer);
    
    // Recopilar todas las posiciones de la línea de 5
    std::vector<Move> linePositions;
    for (int i = 0; i < 5; i++) {
        linePositions.push_back(Move(lineStart.x + i*dx, lineStart.y + i*dy));
    }
    
    bool foundCapture = false;
    
    // Para cada pieza en la línea, verificar si el oponente puede capturarla
    // en CUALQUIER dirección (no solo en la dirección de la línea)
    for (const Move& piece : linePositions) {
        
        // Probar las 8 direcciones para capturas
        for (int d = 0; d < ALL_COUNT; d++) {
            int cdx = ALL[d][0];
            int cdy = ALL[d][1];
            
            // Buscar patrón X-O-O-? donde X=oponente, O=pieza actual
            // La pieza actual debe ser la primera O del par
            Move secondPiece(piece.x + cdx, piece.y + cdy);
            Move before(piece.x - cdx, piece.y - cdy);
            Move after(secondPiece.x + cdx, secondPiece.y + cdy);
            
            // Verificar patrón: OPP-PIECE-SECOND-EMPTY
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
            
            // También verificar patrón: EMPTY-PIECE-SECOND-OPP
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
    
    if (!foundCapture) {
        std::cout << "  -> No capture pattern found for any piece in the line" << std::endl;
    }
    
    return foundCapture;
}

bool RuleEngine::opponentCanCaptureNextTurn(
    const GameState &state, 
    int opponent
) {
    // Probar cada casilla vacía
    for (int i = 0; i < GameState::BOARD_SIZE; i++) {
        for (int j = 0; j < GameState::BOARD_SIZE; j++) {
            if (state.isEmpty(i, j)) {
                Move testMove(i, j);
                
                // ¿Este movimiento crea capturas?
                auto captures = findCaptures(state, testMove, opponent);
                if (!captures.empty()) {
                    return true;  // Sí puede capturar en su próximo turno
                }
            }
        }
    }
    
    return false;
}
