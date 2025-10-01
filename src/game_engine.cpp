/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   game_engine.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/14 21:26:45 by jainavas          #+#    #+#             */
/*   Updated: 2025/10/01 21:52:11 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/game_engine.hpp"
#include "../include/display.hpp"
#include "../include/debug_analyzer.hpp"
#include <chrono>
#include <iostream>
#include <algorithm>
#include <map>
#include <utility>

void GameEngine::newGame()
{
	state = GameState(); // Reset to initial state
	lastHumanMove = Move(-1, -1); // También reiniciar el campo local
}

bool GameEngine::makeHumanMove(const Move& move) {
    // En modo VS_HUMAN_SUGGESTED, ambos jugadores son humanos
    if (currentMode == GameMode::VS_AI && state.currentPlayer != GameState::PLAYER1) {
        return false;
    }
    
    lastHumanMove = move;
    state.lastHumanMove = move;
    
    RuleEngine::MoveResult result = RuleEngine::applyMove(state, move);
    
    return result.success;
}

Move GameEngine::makeAIMove() {
    if (state.currentPlayer != GameState::PLAYER2) return Move();
    
    auto start = std::chrono::high_resolution_clock::now();
    Move bestMove = ai.getBestMove(state);
    auto end = std::chrono::high_resolution_clock::now();
    lastAITime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    DEBUG_LOG_STATS("AI Stats: " + std::to_string(ai.getLastNodesEvaluated()) + 
                   " nodes, " + std::to_string(lastAITime) + "ms");
    
    if (bestMove.isValid()) {
        RuleEngine::applyMove(state, bestMove);
    }
    
    return bestMove;
}

bool GameEngine::isGameOver() const
{
	return RuleEngine::checkWin(state, GameState::PLAYER1) ||
		   RuleEngine::checkWin(state, GameState::PLAYER2);
}

int GameEngine::getWinner() const
{
	if (RuleEngine::checkWin(state, GameState::PLAYER1))
		return GameState::PLAYER1;
	if (RuleEngine::checkWin(state, GameState::PLAYER2))
		return GameState::PLAYER2;
	return 0; // No winner
}

std::vector<Move> GameEngine::findWinningLine() const {
    std::vector<Move> line;
    
    // Determinar quién ganó
    int winner = 0;
    if (RuleEngine::checkWin(state, GameState::PLAYER1)) {
        winner = GameState::PLAYER1;
    } else if (RuleEngine::checkWin(state, GameState::PLAYER2)) {
        winner = GameState::PLAYER2;
    } else {
        return line; // No hay ganador por alineación
    }
    
    // Buscar la línea de 5
    int directions[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};
    
    for (int i = 0; i < GameState::BOARD_SIZE; i++) {
        for (int j = 0; j < GameState::BOARD_SIZE; j++) {
            if (state.board[i][j] != winner) continue;
            
            for (int d = 0; d < 4; d++) {
                int dx = directions[d][0];
                int dy = directions[d][1];
                
                // Contar consecutivas en esta dirección
                int count = 0;
                std::vector<Move> tempLine;
                
                for (int k = 0; k < 5; k++) {
                    int ni = i + k * dx;
                    int nj = j + k * dy;
                    
                    if (state.isValid(ni, nj) && state.board[ni][nj] == winner) {
                        tempLine.push_back(Move(ni, nj));
                        count++;
                    } else {
                        break;
                    }
                }
                
                if (count >= 5) {
                    return tempLine; // Encontrada
                }
            }
        }
    }
    
    return line;
}
