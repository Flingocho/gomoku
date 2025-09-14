/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rule_engine.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/14 21:24:14 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/14 21:24:30 by jainavas         ###   ########.fr       */
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
    
    // 3. Colocar la pieza temporalmente
    state.board[move.x][move.y] = state.currentPlayer;
    
    // 4. Buscar capturas
    result.capturedPieces = findCaptures(state, move, state.currentPlayer);
    
    // 5. Aplicar capturas
    for (const Move& captured : result.capturedPieces) {
        state.board[captured.x][captured.y] = GameState::EMPTY;
    }
    state.captures[state.currentPlayer - 1] += result.capturedPieces.size();
    
    // 6. Verificar victoria
    result.createsWin = checkWin(state, state.currentPlayer);
    
    // 7. Avanzar turno
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
