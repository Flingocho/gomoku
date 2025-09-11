/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   board.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/21 16:35:33 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/11 22:07:06 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef BOARD_HPP
#define BOARD_HPP

#include <vector>
#include <utility>
#include <cstdint>

class Board {
private:
    static const int SIZE = 19;
    int board[SIZE][SIZE];
    int captures[2];  // [humano, AI]
	int turns;
	int pieces;

public:
    Board();
    
    // Básicas
    bool isValid(int x, int y) const;
    bool isEmpty(int x, int y) const;
    int getPiece(int x, int y) const;
    bool placePiece(int x, int y, int player);
    bool placePieceSimple(int x, int y, int player);  // Sin reglas, solo para simulación
    void removePiece(int x, int y);  // Para deshacer movimientos
    
    // Reglas del juego
    bool checkWin(int player) const;
    std::vector<std::pair<int, int>> checkCaptures(int x, int y, int player);  
    std::vector<std::pair<int, int>> checkCapturesForOpponent(int x, int y, int opponent);
    bool isDoubleFree(int x, int y, int player) const;
    
	// Setters
	void newturn() { turns++; }
	void incPieces() { pieces++; }
	void decPieces() { pieces--; }

    // Getters
    int getCaptures(int player) const { return captures[player-1]; }
    static int getSize() { return SIZE; }
	int getTurns() const { return turns; }
	int getPieces() const { return pieces; }
    
    // Para debugging
    void printRaw() const;

private:
    // Métodos auxiliares para reglas complejas
    int countDirection(int x, int y, int dx, int dy, int player) const;
    bool checkLineWin(int x, int y, int player) const;
    std::vector<std::pair<int, int>> getCapturesInDirection(int x, int y, int dx, int dy, int player);  
    std::vector<std::pair<int, int>> getCapturesForOpponentInDirection(int x, int y, int dx, int dy, int opponent);
    bool isFreeThree(int x, int y, int dx, int dy, int player) const;
    bool checkBrokenPattern1(int x, int y, int dx, int dy, int player) const;
    bool checkBrokenPattern2(int x, int y, int dx, int dy, int player) const;
};

#endif