/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/14 21:27:46 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/14 21:27:48 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/game_engine.hpp"
#include "../include/display.hpp"
#include <iostream>

int main() {
    GameEngine game;
    game.newGame();
    
    Display::printWelcome();
    
    while (!game.isGameOver()) {
        const GameState& state = game.getState();
        
        // Mostrar tablero actual
        Display::printBoard(state);
        Display::printGameInfo(state, game.getLastAIThinkingTime());
        
        if (state.currentPlayer == GameState::PLAYER1) {
            // Turno del humano
            std::cout << "Your turn (Player 1)\n";
            auto userInput = Display::getUserMove();
            
            if (userInput.first == -1) {  // Quit
                std::cout << "Thanks for playing!\n";
                break;
            }
            
            if (userInput.first == -2) {  // Error de input
                std::cout << "Invalid input! Try again (e.g., 'J10')\n";
                continue;
            }
            
            Move humanMove(userInput.first, userInput.second);
            if (!game.makeHumanMove(humanMove)) {
                std::cout << "❌ Invalid move! Reasons:\n";
                std::cout << "- Position occupied or out of bounds\n";  
                std::cout << "- Double free-three rule violation\n";
                std::cout << "Try again...\n";
                continue;
            }
            
        } else {
            // Turno de la IA
            std::cout << "AI thinking..." << std::endl;
            
            Move aiMove = game.makeAIMove();
            
            if (aiMove.isValid()) {
                std::cout << "AI played: " << char('A' + aiMove.y) << (aiMove.x + 1) << std::endl;
                std::cout << "Nodes evaluated: " << game.getLastNodesEvaluated() << std::endl;
            } else {
                std::cout << "AI couldn't find a valid move!" << std::endl;
                break;
            }
        }
    }
    
    // Mostrar resultado final
    if (game.isGameOver()) {
        Display::printBoard(game.getState());
        Display::printGameInfo(game.getState(), game.getLastAIThinkingTime());
        Display::printWinner(game.getWinner());
    }
    
    return 0;
}