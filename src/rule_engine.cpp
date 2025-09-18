/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rule_engine.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/14 21:24:14 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/18 16:49:24 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/rule_engine.hpp"

RuleEngine::MoveResult RuleEngine::applyMove(GameState& state, const Move& move) {
    MoveResult result;
    
    // 1. Verificar que el movimiento es básicamente válido
    if (!state.isEmpty(move.x, move.y)) {
        return result; // success = false
    }
    
    // 2. Verificar double free-three ANTES de colocar
    if (createsDoubleFreeThree(state, move, state.currentPlayer)) {
        return result; // success = false
    }
    
    // NUEVO: Guardar estado para actualización de hash
    int oldMyCaptures = state.captures[state.currentPlayer - 1];
    int currentPlayer = state.currentPlayer;
    
    // 3. Colocar la pieza temporalmente
    state.board[move.x][move.y] = state.currentPlayer;
    
    // 4. Buscar TODAS las capturas que se forman
    CaptureInfo captureInfo = findAllCaptures(state, move, state.currentPlayer);
    result.myCapturedPieces = captureInfo.myCapturedPieces;
    result.opponentCapturedPieces = captureInfo.opponentCapturedPieces;
    
    // 5. Aplicar MIS capturas
    for (const Move& captured : result.myCapturedPieces) {
        state.board[captured.x][captured.y] = GameState::EMPTY;
    }
    state.captures[state.currentPlayer - 1] += result.myCapturedPieces.size() / 2;
    
    // 6. Aplicar capturas del OPONENTE (¡aunque sea mi turno!)
    for (const Move& captured : result.opponentCapturedPieces) {
        state.board[captured.x][captured.y] = GameState::EMPTY;
    }
    int opponentIndex = state.getOpponent(state.currentPlayer) - 1;
    state.captures[opponentIndex] += result.opponentCapturedPieces.size() / 2;
    
    // 7. Verificar victoria
    result.createsWin = checkWin(state, state.currentPlayer);
    
	std::vector<Move> allCaptured = result.myCapturedPieces;
    allCaptured.insert(allCaptured.end(), result.opponentCapturedPieces.begin(), result.opponentCapturedPieces.end());
    state.updateHashAfterMove(move, currentPlayer, allCaptured, oldMyCaptures);
    // 8. Avanzar turno
    state.currentPlayer = state.getOpponent(state.currentPlayer);
    state.turnCount++;

    result.success = true;
    return result;
}

bool RuleEngine::isLegalMove(const GameState& state, const Move& move) {
    if (!state.isEmpty(move.x, move.y)) return false;
    return !createsDoubleFreeThree(state, move, state.currentPlayer);
}

bool RuleEngine::checkWin(const GameState& state, int player) {
    // Victoria por capturas (10 piezas = 5 pares)
    if (state.captures[player - 1] >= 10) return true;
    
    // Victoria por 5 en línea
    for (int i = 0; i < GameState::BOARD_SIZE; i++) {
        for (int j = 0; j < GameState::BOARD_SIZE; j++) {
            if (state.board[i][j] == player) {
                Move pos(i, j);
                if (checkLineWin(state, pos, player)) return true;
            }
        }
    }
    
    return false;
}

RuleEngine::CaptureInfo RuleEngine::findAllCaptures(const GameState& state, const Move& move, int player) {
    CaptureInfo info;
    int opponent = state.getOpponent(player);
    
    // Buscar en las 8 direcciones
    for (int d = 0; d < 8; d++) {
        int dx = DIRECTIONS[d][0];
        int dy = DIRECTIONS[d][1];
        
        // Patrón 1: NUEVA-OPP-OPP-MIA (yo capturo hacia adelante)
        Move pos1(move.x + dx, move.y + dy);
        Move pos2(move.x + 2*dx, move.y + 2*dy);
        Move pos3(move.x + 3*dx, move.y + 3*dy);
        
        if (state.isValid(pos1.x, pos1.y) && state.isValid(pos2.x, pos2.y) && state.isValid(pos3.x, pos3.y)) {
            if (state.getPiece(pos1.x, pos1.y) == opponent &&
                state.getPiece(pos2.x, pos2.y) == opponent &&
                state.getPiece(pos3.x, pos3.y) == player) {
                
                info.myCapturedPieces.push_back(pos1);
                info.myCapturedPieces.push_back(pos2);
            }
        }
        
        // Patrón 2: MIA-OPP-OPP-NUEVA (yo capturo hacia atrás)
        Move back1(move.x - dx, move.y - dy);
        Move back2(move.x - 2*dx, move.y - 2*dy);
        Move back3(move.x - 3*dx, move.y - 3*dy);
        
        if (state.isValid(back1.x, back1.y) && state.isValid(back2.x, back2.y) && state.isValid(back3.x, back3.y)) {
            if (state.getPiece(back1.x, back1.y) == opponent &&
                state.getPiece(back2.x, back2.y) == opponent &&
                state.getPiece(back3.x, back3.y) == player) {
                
                info.myCapturedPieces.push_back(back1);
                info.myCapturedPieces.push_back(back2);
            }
        }
        
        // Patrón 3: OPP-NUEVA-MIA-OPP (oponente me captura hacia adelante)
        if (state.isValid(back1.x, back1.y) && state.isValid(pos1.x, pos1.y) && state.isValid(pos2.x, pos2.y)) {
            if (state.getPiece(back1.x, back1.y) == opponent &&
                state.getPiece(pos1.x, pos1.y) == player &&
                state.getPiece(pos2.x, pos2.y) == opponent) {
                
                info.opponentCapturedPieces.push_back(move);      // Mi pieza nueva
                info.opponentCapturedPieces.push_back(pos1);     // Mi otra pieza
            }
        }
        
        // Patrón 4: OPP-MIA-NUEVA-OPP (oponente me captura hacia atrás)
        if (state.isValid(back2.x, back2.y) && state.isValid(back1.x, back1.y) && state.isValid(pos1.x, pos1.y)) {
            if (state.getPiece(back2.x, back2.y) == opponent &&
                state.getPiece(back1.x, back1.y) == player &&
                state.getPiece(pos1.x, pos1.y) == opponent) {
                
                info.opponentCapturedPieces.push_back(back1);    // Mi otra pieza
                info.opponentCapturedPieces.push_back(move);     // Mi pieza nueva
            }
        }
    }
    
    return info;
}

std::vector<Move> RuleEngine::findCaptures(const GameState& state, const Move& move, int player) {
    std::vector<Move> allCaptures;
    
    // Buscar en las 8 direcciones
    for (int d = 0; d < 8; d++) {
        auto dirCaptures = findCapturesInDirection(state, move, player, 
                                                  DIRECTIONS[d][0], DIRECTIONS[d][1]);
        allCaptures.insert(allCaptures.end(), dirCaptures.begin(), dirCaptures.end());
    }
    
    return allCaptures;
}

std::vector<Move> RuleEngine::findCapturesInDirection(const GameState& state, 
                                                     const Move& move, int player,
                                                     int dx, int dy) {
    std::vector<Move> captures;
    int opponent = state.getOpponent(player);
    
    // Patrón: PLAYER + OPPONENT + OPPONENT + PLAYER
    Move pos1(move.x + dx, move.y + dy);
    Move pos2(move.x + 2*dx, move.y + 2*dy);
    Move pos3(move.x + 3*dx, move.y + 3*dy);
    
    if (state.isValid(pos1.x, pos1.y) && state.isValid(pos2.x, pos2.y) && 
        state.isValid(pos3.x, pos3.y)) {
        
        if (state.getPiece(pos1.x, pos1.y) == opponent &&
            state.getPiece(pos2.x, pos2.y) == opponent &&
            state.getPiece(pos3.x, pos3.y) == player) {
            
            captures.push_back(pos1);
            captures.push_back(pos2);
        }
    }
    
    return captures;
}

bool RuleEngine::checkLineWin(const GameState& state, const Move& move, int player) {
    // Verificar las 4 direcciones principales (sin duplicados)
    int mainDirections[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};
    
    for (int d = 0; d < 4; d++) {
        int dx = mainDirections[d][0];
        int dy = mainDirections[d][1];
        
        int count = 1; // La pieza actual
        count += countInDirection(state, move, dx, dy, player);
        count += countInDirection(state, move, -dx, -dy, player);
        
        if (count >= 5) return true;
    }
    
    return false;
}

int RuleEngine::countInDirection(const GameState& state, const Move& start, 
                               int dx, int dy, int player) {
    int count = 0;
    int x = start.x + dx;
    int y = start.y + dy;
    
    while (state.isValid(x, y) && state.getPiece(x, y) == player) {
        count++;
        x += dx;
        y += dy;
    }
    
    return count;
}

bool RuleEngine::createsDoubleFreeThree(const GameState& state, const Move& move, int player) {
    // Crear copia temporal para testear
    GameState tempState = state;
    tempState.board[move.x][move.y] = player;
    
    auto freeThrees = findFreeThrees(tempState, move, player);
    return freeThrees.size() >= 2;
}

std::vector<Move> RuleEngine::findFreeThrees(const GameState& state, const Move& move, int player) {
    std::vector<Move> freeThrees;
    
    // Verificar las 4 direcciones principales
    int mainDirections[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};
    
    for (int d = 0; d < 4; d++) {
        if (isFreeThree(state, move, mainDirections[d][0], mainDirections[d][1], player)) {
            freeThrees.push_back(Move(mainDirections[d][0], mainDirections[d][1])); // Direction as identifier
        }
    }
    
    return freeThrees;
}

bool RuleEngine::isFreeThree(const GameState& state, const Move& move, 
                            int dx, int dy, int player) {
    // Contar piezas consecutivas en esta dirección
    int totalCount = 1; // La pieza actual
    totalCount += countInDirection(state, move, dx, dy, player);
    totalCount += countInDirection(state, move, -dx, -dy, player);
    
    if (totalCount != 3) return false; // No es exactamente 3
    
    // Verificar que al menos un extremo está libre
    // Encontrar los extremos reales
    Move start = move;
    Move end = move;
    
    // Buscar inicio real
    while (state.isValid(start.x - dx, start.y - dy) && 
           state.getPiece(start.x - dx, start.y - dy) == player) {
        start.x -= dx;
        start.y -= dy;
    }
    
    // Buscar final real
    while (state.isValid(end.x + dx, end.y + dy) && 
           state.getPiece(end.x + dx, end.y + dy) == player) {
        end.x += dx;
        end.y += dy;
    }
    
    // Verificar extremos libres
    bool startFree = state.isValid(start.x - dx, start.y - dy) && 
                     state.isEmpty(start.x - dx, start.y - dy);
    bool endFree = state.isValid(end.x + dx, end.y + dy) && 
                   state.isEmpty(end.x + dx, end.y + dy);
    
    return startFree && endFree; // Ambos extremos deben estar libres para ser "free-three"
}