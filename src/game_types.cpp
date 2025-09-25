/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   game_types.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/14 21:23:56 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/25 18:34:24 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/game_types.hpp"
#include "../include/zobrist_hasher.hpp"
#include <iostream>

// Inicialización del hasher estático
const ZobristHasher* GameState::hasher = nullptr;

GameState::GameState() {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            board[i][j] = EMPTY;
        }
    }
    
    // Inicializar último movimiento humano como inválido
    lastHumanMove = Move(-1, -1);
    
    // Calcular hash inicial si el hasher está disponible
    if (hasher) {
        zobristHash = hasher->computeFullHash(*this);
    }
}

GameState::GameState(const GameState& other) {
    *this = other;
}

GameState& GameState::operator=(const GameState& other) {
    if (this != &other) {
        for (int i = 0; i < BOARD_SIZE; i++) {
            for (int j = 0; j < BOARD_SIZE; j++) {
                board[i][j] = other.board[i][j];
            }
        }
        captures[0] = other.captures[0];
        captures[1] = other.captures[1];
        currentPlayer = other.currentPlayer;
        turnCount = other.turnCount;
        zobristHash = other.zobristHash; // Copiar hash también
        lastHumanMove = other.lastHumanMove; // Copiar último movimiento humano
    }
    return *this;
}

bool GameState::isValid(int x, int y) const {
    return x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE;
}

bool GameState::isEmpty(int x, int y) const {
    return isValid(x, y) && board[x][y] == EMPTY;
}

int GameState::getPiece(int x, int y) const {
    return isValid(x, y) ? board[x][y] : -1;
}

int GameState::getOpponent(int player) const {
    return (player == PLAYER1) ? PLAYER2 : PLAYER1;
}

void GameState::initializeHasher() {
    if (!hasher) {
        hasher = new ZobristHasher();
        // Hasher Zobrist inicializado - se loggeará desde main
    }
}

void GameState::updateHashAfterMove(const Move& move, int player, 
                                   const std::vector<Move>& capturedPieces,
                                   int oldCaptures) {
    if (!hasher) {
        std::cerr << "ERROR: Hasher no inicializado. Llama GameState::initializeHasher() primero." << std::endl;
        return;
    }
    
    int playerIndex = player - 1;
    int newCaptures = captures[playerIndex];
    
    zobristHash = hasher->updateHashAfterMove(zobristHash, move, player, 
                                            capturedPieces, oldCaptures, newCaptures);
}

void GameState::recalculateHash() {
    if (!hasher) {
        std::cerr << "ERROR: Hasher no inicializado. Llama GameState::initializeHasher() primero." << std::endl;
        return;
    }
    
    zobristHash = hasher->computeFullHash(*this);
    
    // Debug del hash eliminado para evitar spam en consola
}
