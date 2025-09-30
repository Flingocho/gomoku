/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rule_engine.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/14 21:24:14 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/30 16:35:56 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/rule_engine.hpp"
#include "../include/zobrist_hasher.hpp"

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

	// NUEVO: Guardar estado para actualización de hash
	int oldMyCaptures = state.captures[state.currentPlayer - 1];
	int currentPlayer = state.currentPlayer;
	int oldOppCaptures = state.captures[state.getOpponent(currentPlayer) - 1];

	// 3. Colocar la pieza temporalmente
	state.board[move.x][move.y] = state.currentPlayer;

	// 4. Buscar TODAS las capturas que se forman
	CaptureInfo captureInfo = findAllCaptures(state, move, state.currentPlayer);
	result.myCapturedPieces = captureInfo.myCapturedPieces;
	result.opponentCapturedPieces = captureInfo.opponentCapturedPieces;

	// 5. Aplicar MIS capturas
	for (const Move &captured : result.myCapturedPieces)
	{
		state.board[captured.x][captured.y] = GameState::EMPTY;
	}
	state.captures[state.currentPlayer - 1] += result.myCapturedPieces.size() / 2;

	// 6. Aplicar capturas del OPONENTE (¡aunque sea mi turno!)
	for (const Move &captured : result.opponentCapturedPieces)
	{
		state.board[captured.x][captured.y] = GameState::EMPTY;
	}
	int opponentIndex = state.getOpponent(state.currentPlayer) - 1;
	state.captures[opponentIndex] += result.opponentCapturedPieces.size() / 2;

	int newMyCaptures = state.captures[currentPlayer - 1];
	int newOppCaptures = state.captures[state.getOpponent(currentPlayer) - 1];
	// 7. Verificar victoria
	result.createsWin = checkWin(state, state.currentPlayer);

	state.zobristHash = state.hasher->updateHashAfterMove(
		state.zobristHash,
		move,
		currentPlayer,
		result.myCapturedPieces,
		result.opponentCapturedPieces,
		oldMyCaptures,
		newMyCaptures,
		oldOppCaptures,
		newOppCaptures);
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

bool RuleEngine::checkWin(const GameState &state, int player)
{
    int opponent = state.getOpponent(player);
    
    // 1. Victoria por capturas (esta no cambia)
    if (state.captures[player - 1] >= 10)
        return true;

    // 2. Victoria por 5 en línea (AHORA CON VERIFICACIÓN)
    for (int i = 0; i < GameState::BOARD_SIZE; i++) {
        for (int j = 0; j < GameState::BOARD_SIZE; j++) {
            if (state.board[i][j] == player) {
                Move pos(i, j);
                
                // Buscar línea de 5 en cada dirección
                for (int d = 0; d < 4; d++) {
                    int dx = MAIN_DIRECTIONS[d][0];
                    int dy = MAIN_DIRECTIONS[d][1];
                    
                    if (checkLineWinInDirection(state, pos, dx, dy, player)) {
                        // ✅ Encontramos 5 en línea
                        
                        // NUEVA VERIFICACIÓN 1: ¿Oponente puede romperla?
                        if (canBreakLineByCapture(state, pos, dx, dy, player)) {
                            continue;  // No es victoria todavía
                        }
                        
                        // NUEVA VERIFICACIÓN 2: ¿Estoy en peligro de perder por captura?
                        if (state.captures[opponent - 1] >= 8) {
                            // Tengo 4+ pares capturados en mi contra
                            // ¿El oponente puede capturar uno más?
                            if (opponentCanCaptureNextTurn(state, opponent)) {
                                return false;  // No gano, el oponente puede ganar por captura
                            }
                        }
                        
                        // Si llegamos aquí, es victoria legítima
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

RuleEngine::CaptureInfo RuleEngine::findAllCaptures(const GameState &state, const Move &move, int player)
{
	CaptureInfo info;
	int opponent = state.getOpponent(player);

	// Buscar en las 8 direcciones
	for (int d = 0; d < 8; d++)
	{
		int dx = DIRECTIONS[d][0];
		int dy = DIRECTIONS[d][1];

		// Patrón 1: NUEVA-OPP-OPP-MIA (yo capturo hacia adelante)
		Move pos1(move.x + dx, move.y + dy);
		Move pos2(move.x + 2 * dx, move.y + 2 * dy);
		Move pos3(move.x + 3 * dx, move.y + 3 * dy);

		if (state.isValid(pos1.x, pos1.y) && state.isValid(pos2.x, pos2.y) && state.isValid(pos3.x, pos3.y))
		{
			if (state.getPiece(pos1.x, pos1.y) == opponent &&
				state.getPiece(pos2.x, pos2.y) == opponent &&
				state.getPiece(pos3.x, pos3.y) == player)
			{

				info.myCapturedPieces.push_back(pos1);
				info.myCapturedPieces.push_back(pos2);
			}
		}

		// Patrón 2: MIA-OPP-OPP-NUEVA (yo capturo hacia atrás)
		Move back1(move.x - dx, move.y - dy);
		Move back2(move.x - 2 * dx, move.y - 2 * dy);
		Move back3(move.x - 3 * dx, move.y - 3 * dy);

		if (state.isValid(back1.x, back1.y) && state.isValid(back2.x, back2.y) && state.isValid(back3.x, back3.y))
		{
			if (state.getPiece(back1.x, back1.y) == opponent &&
				state.getPiece(back2.x, back2.y) == opponent &&
				state.getPiece(back3.x, back3.y) == player)
			{

				info.myCapturedPieces.push_back(back1);
				info.myCapturedPieces.push_back(back2);
			}
		}

		// Patrón 3: OPP-NUEVA-MIA-OPP (oponente me captura hacia adelante)
		if (state.isValid(back1.x, back1.y) && state.isValid(pos1.x, pos1.y) && state.isValid(pos2.x, pos2.y))
		{
			if (state.getPiece(back1.x, back1.y) == opponent &&
				state.getPiece(pos1.x, pos1.y) == player &&
				state.getPiece(pos2.x, pos2.y) == opponent)
			{

				info.opponentCapturedPieces.push_back(move); // Mi pieza nueva
				info.opponentCapturedPieces.push_back(pos1); // Mi otra pieza
			}
		}

		// Patrón 4: OPP-MIA-NUEVA-OPP (oponente me captura hacia atrás)
		if (state.isValid(back2.x, back2.y) && state.isValid(back1.x, back1.y) && state.isValid(pos1.x, pos1.y))
		{
			if (state.getPiece(back2.x, back2.y) == opponent &&
				state.getPiece(back1.x, back1.y) == player &&
				state.getPiece(pos1.x, pos1.y) == opponent)
			{

				info.opponentCapturedPieces.push_back(back1); // Mi otra pieza
				info.opponentCapturedPieces.push_back(move);  // Mi pieza nueva
			}
		}
	}

	return info;
}

std::vector<Move> RuleEngine::findCaptures(const GameState &state, const Move &move, int player)
{
	std::vector<Move> allCaptures;

	// Buscar en las 8 direcciones
	for (int d = 0; d < 8; d++)
	{
		auto dirCaptures = findCapturesInDirection(state, move, player,
												   DIRECTIONS[d][0], DIRECTIONS[d][1]);
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

bool RuleEngine::checkLineWin(const GameState &state, const Move &move, int player)
{
	// Verificar las 4 direcciones principales (sin duplicados)
	int mainDirections[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};

	for (int d = 0; d < 4; d++)
	{
		int dx = mainDirections[d][0];
		int dy = mainDirections[d][1];

		int count = 1; // La pieza actual
		count += countInDirection(state, move, dx, dy, player);
		count += countInDirection(state, move, -dx, -dy, player);

		if (count >= 5)
			return true;
	}

	return false;
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

bool RuleEngine::createsDoubleFreeThree(const GameState &state, const Move &move, int player)
{
	// Crear copia temporal para testear
	GameState tempState = state;
	tempState.board[move.x][move.y] = player;

	auto freeThrees = findFreeThrees(tempState, move, player);
	return freeThrees.size() >= 2;
}

std::vector<Move> RuleEngine::findFreeThrees(const GameState &state, const Move &move, int player)
{
	std::vector<Move> freeThrees;

	// Verificar las 4 direcciones principales
	int mainDirections[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};

	for (int d = 0; d < 4; d++)
	{
		if (isFreeThree(state, move, mainDirections[d][0], mainDirections[d][1], player))
		{
			freeThrees.push_back(Move(mainDirections[d][0], mainDirections[d][1])); // Direction as identifier
		}
	}

	return freeThrees;
}

bool RuleEngine::isFreeThree(const GameState &state, const Move &move,
							 int dx, int dy, int player)
{
	// Un free-three es cualquier patrón de 3 fichas en una ventana de 5 posiciones
	// donde ambos extremos están libres y se puede formar una amenaza de 4
	
	// Buscar todas las ventanas de 5 posiciones que contengan el movimiento
	for (int offset = -4; offset <= 0; offset++)
	{
		Move windowStart(move.x + offset * dx, move.y + offset * dy);
		
		// Verificar que la ventana de 5 está dentro del tablero
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
		
		// Verificar que el movimiento está dentro de esta ventana
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
		
		// Contar fichas del jugador en esta ventana (incluyendo el nuevo movimiento)
		int playerPieces = 0;
		int opponentPieces = 0;
		int emptySpaces = 0;
		
		for (int i = 0; i < 5; i++)
		{
			Move pos(windowStart.x + i * dx, windowStart.y + i * dy);
			
			if (pos.x == move.x && pos.y == move.y)
			{
				playerPieces++; // El movimiento cuenta como ficha del jugador
			}
			else
			{
				int piece = state.getPiece(pos.x, pos.y);
				if (piece == player)
					playerPieces++;
				else if (piece == state.getOpponent(player))
					opponentPieces++;
				else
					emptySpaces++;
			}
		}
		
		// Para ser free-three: exactamente 3 fichas del jugador, 0 del oponente, 2 vacías
		if (playerPieces == 3 && opponentPieces == 0 && emptySpaces == 2)
		{
			// Verificar que los extremos están libres
			Move leftExtreme(windowStart.x - dx, windowStart.y - dy);
			Move rightExtreme(windowStart.x + 5 * dx, windowStart.y + 5 * dy);
			
			bool leftFree = state.isValid(leftExtreme.x, leftExtreme.y) && 
							state.isEmpty(leftExtreme.x, leftExtreme.y);
			bool rightFree = state.isValid(rightExtreme.x, rightExtreme.y) && 
							 state.isEmpty(rightExtreme.x, rightExtreme.y);
			
			// Al menos uno de los extremos debe estar libre para ser "free"
			if (leftFree && rightFree)
			{
				return true;
			}
		}
	}
	
	return false;
}

bool RuleEngine::canBreakLineByCapture(
    const GameState &state, 
    const Move &lineStart,  // Primera ficha de la línea de 5
    int dx, int dy,         // Dirección de la línea
    int winningPlayer
) {
    int opponent = state.getOpponent(winningPlayer);
    
    // Revisar cada par consecutivo en la línea de 5
    // En una línea de 5 hay 4 pares posibles: (0,1), (1,2), (2,3), (3,4)
    for (int i = 0; i < 4; i++) {
        Move pos1(lineStart.x + i*dx, lineStart.y + i*dy);
        Move pos2(lineStart.x + (i+1)*dx, lineStart.y + (i+1)*dy);
        
        // Para capturar este par, el oponente necesita flanquearlo:
        // Patrón de captura: OPP-PAIR-PAIR-OPP
        
        // Opción 1: Puede jugar ANTES del par
        Move before(pos1.x - dx, pos1.y - dy);
        Move beforeBefore(before.x - dx, before.y - dy);
        
        if (state.isValid(before.x, before.y) && state.isEmpty(before.x, before.y) &&
            state.isValid(beforeBefore.x, beforeBefore.y) && 
            state.getPiece(beforeBefore.x, beforeBefore.y) == opponent) {
            // El oponente puede jugar en "before" y tiene ficha en "beforeBefore"
            // Esto formaría: OPP-NUEVA-PAR1-PAR2
            // Falta verificar que tenga otra ficha DESPUÉS del par
            Move after(pos2.x + dx, pos2.y + dy);
            if (state.isValid(after.x, after.y) && 
                state.getPiece(after.x, after.y) == opponent) {
                return true;  // Patrón completo: OPP-NUEVA-PAR-PAR-OPP
            }
        }
        
        // Opción 2: Puede jugar DESPUÉS del par
        Move after(pos2.x + dx, pos2.y + dy);
        Move afterAfter(after.x + dx, after.y + dy);
        
        if (state.isValid(after.x, after.y) && state.isEmpty(after.x, after.y) &&
            state.isValid(afterAfter.x, afterAfter.y) && 
            state.getPiece(afterAfter.x, afterAfter.y) == opponent) {
            // El oponente puede jugar en "after" y tiene ficha en "afterAfter"
            // Esto formaría: PAR1-PAR2-NUEVA-OPP
            // Falta verificar que tenga otra ficha ANTES del par
            Move before(pos1.x - dx, pos1.y - dy);
            if (state.isValid(before.x, before.y) && 
                state.getPiece(before.x, before.y) == opponent) {
                return true;  // Patrón completo: OPP-PAR-PAR-NUEVA-OPP
            }
        }
    }
    
    return false;  // No puede romper la línea
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

bool RuleEngine::checkLineWinInDirection(const GameState &state, const Move &start, 
                                         int dx, int dy, int player)
{
    // Verificar si desde 'start' hay exactamente 5 o más en línea en dirección (dx, dy)
    
    // IMPORTANTE: Solo contar si 'start' es realmente el inicio de la línea
    // (para evitar contar la misma línea múltiples veces)
    
    // Verificar que no hay pieza del mismo jugador ANTES de start
    Move before(start.x - dx, start.y - dy);
    if (state.isValid(before.x, before.y) && state.getPiece(before.x, before.y) == player) {
        return false;  // No es el inicio real de la línea
    }
    
    // Contar cuántas fichas consecutivas hay desde start
    int count = 1;  // La ficha en 'start'
    Move current(start.x + dx, start.y + dy);
    
    while (state.isValid(current.x, current.y) && 
           state.getPiece(current.x, current.y) == player) {
        count++;
        current.x += dx;
        current.y += dy;
    }
    
    // ¿Hay 5 o más?
    return (count >= 5);
}
