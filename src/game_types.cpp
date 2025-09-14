/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   game_types.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/14 21:23:56 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/14 21:24:00 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/game_types.hpp"

GameState::GameState() {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            board[i][j] = EMPTY;
        }
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