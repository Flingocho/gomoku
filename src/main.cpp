/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/21 16:36:17 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/03 20:25:04 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/ai.hpp"
#include "../include/display.hpp"
#include <iostream>

int main() {
	AI ai(2, 1, 10);
    Board board;
	
    int currentPlayer = 1;  // Empezar con jugador 1
    
    Display::printWelcome();
    
    while (true) {
        Display::printBoard(board);
        Display::printGameInfo(board);
        
        // Turno del jugador actual
        std::cout << "Player " << currentPlayer << "'s turn\n";
        auto move = Display::getUserMove();
        
        if (move.first == -1) {  // Quit
            std::cout << "Thanks for playing!\n";
            break;
        }
        
        if (move.first == -2) {  // Error de input
            std::cout << "Invalid input! Try again (e.g., 'J10')\n";
            continue;
        }
        
        if (!board.placePiece(move.first, move.second, currentPlayer)) {
            std::cout << "❌ Invalid move! Reasons:\n";
            std::cout << "- Position occupied or out of bounds\n";  
            std::cout << "- Double free-three rule violation\n";
            std::cout << "Try again...\n";
            continue;
        }
        
        // Verificar victoria después del movimiento
        if (board.checkWin(currentPlayer)) {
            Display::printBoard(board);
            Display::printGameInfo(board);
            Display::printWinner(currentPlayer);
            return 0;
        }
        
        // Cambiar al siguiente jugador
		board.newturn();
        currentPlayer = (currentPlayer == 1) ? 2 : 1;
		if (board.getTurns() > 6 && currentPlayer == 1)
			std::cout << "El score de la ia es " << ai.evaluatePosition(board);
    }
    
    return 0;
}