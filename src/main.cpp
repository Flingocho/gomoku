/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/21 16:36:17 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/10 19:18:53 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/ai.hpp"
#include "../include/display.hpp"
#include <iostream>
#include <chrono>

int main() {
    AI ai(2, 1, 3); // AI=player2, Human=player1, depth=3 (empezar bajo para probar)
    Board board;
    
    int currentPlayer = 1;  // Empezar con jugador 1 (humano)
    
    Display::printWelcome();
    
    while (true) {
        Display::printBoard(board);
        Display::printGameInfo(board);
        
        if (currentPlayer == 1) {
            // Turno del humano
            std::cout << "Your turn (Player " << currentPlayer << ")\n";
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
        } else {
            // Turno de la IA
            std::cout << "AI thinking..." << std::endl;
            
            auto start = std::chrono::high_resolution_clock::now();
            
            // NUEVO: Usar el árbol de búsqueda minimax
            Move aiMove = ai.getBestMoveWithTree(board);
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration<double>(end - start).count();
            
            if (aiMove.isValid()) {
                if (board.placePiece(aiMove.x, aiMove.y, currentPlayer)) {
                    std::cout << "AI played: " << char('A' + aiMove.y) << (aiMove.x + 1) << std::endl;
                    Display::printGameInfo(board, duration);
                } else {
                    std::cout << "AI generated invalid move!" << std::endl;
                    break;
                }
            } else {
                std::cout << "AI couldn't find a valid move!" << std::endl;
                break;
            }
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
    }
    
    return 0;
}