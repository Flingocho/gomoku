/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   game_types.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/14 21:15:50 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/14 22:07:22 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef GAME_TYPES_HPP
#define GAME_TYPES_HPP

#include <utility>
#include <vector>

struct Move {
    int x, y;
    
    Move(int row = -1, int col = -1) : x(row), y(col) {}
    
    bool isValid() const { return x >= 0 && y >= 0 && x < 19 && y < 19; }
    bool operator==(const Move& other) const { return x == other.x && y == other.y; }
};

struct GameState {
    static constexpr int BOARD_SIZE = 19;
    static constexpr int EMPTY = 0;
    static constexpr int PLAYER1 = 1;
    static constexpr int PLAYER2 = 2;
    
    int board[BOARD_SIZE][BOARD_SIZE];
    int captures[2] = {0, 0};
    int currentPlayer = PLAYER1;
    int turnCount = 0;
    
    GameState();
    GameState(const GameState& other);
    GameState& operator=(const GameState& other);
    
    bool isValid(int x, int y) const;
    bool isEmpty(int x, int y) const;
    int getPiece(int x, int y) const;
    int getOpponent(int player) const;
};

#endif