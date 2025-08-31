/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/21 16:36:17 by jainavas          #+#    #+#             */
/*   Updated: 2025/08/21 18:51:46 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/board.hpp"
#include "../include/display.hpp"
#include "../include/ai.hpp"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    Board board;
    AI ai(2, 6);  // AI es player 2, profundidad 4 para empezar
    int currentPlayer = 1;  // Empezar con humano
    
    Display::printWelcome();
    
    while (true) {
        Display::printBoard(board);
        Display::printGameInfo(board);
        
        if (currentPlayer == 1) {
            // Turno del humano
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
            
            currentPlayer = 2;  // Cambiar al AI
        } else {
            // Turno del AI
            std::cout << "🤖 AI is thinking...\n";
            
            auto start = std::chrono::high_resolution_clock::now();
            
            // Obtener mejor movimiento del AI
            Move bestMove = ai.getBestMove(board);
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            double seconds = duration.count() / 1000.0;
            
            // Ejecutar movimiento
            if (board.placePiece(bestMove.x, bestMove.y, currentPlayer)) {
                std::cout << "AI plays: " << char('A' + bestMove.y) << (bestMove.x + 1) 
                          << " (Score: " << bestMove.score << ", Time: " << seconds << "s)\n";
                
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                
                // Verificar victoria del AI
                if (board.checkWin(currentPlayer)) {
                    Display::printBoard(board);
                    Display::printGameInfo(board);
                    Display::printWinner(currentPlayer);
                    return 0;
                }
            } else {
                std::cout << "AI made invalid move! This shouldn't happen.\n";
                return 1;
            }
            
            currentPlayer = 1;  // Cambiar al humano
        }
    }
    
    return 0;
}