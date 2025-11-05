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
	
	// Configurar el modo de captura si estamos en CAPTURE_MODE
	if (currentMode == GameMode::CAPTURE_MODE)
	{
		state.captureModeOnly = true;
	}
}

bool GameEngine::makeHumanMove(const Move& move) {
    // En modo VS_HUMAN_SUGGESTED, ambos jugadores son humanos
    if (currentMode == GameMode::VS_AI && state.currentPlayer != GameState::PLAYER1) {
        return false;
    }
    
    // CHECK FOR FORCED CAPTURE: If there's a pending forced capture, 
    // the current player MUST play one of the forced positions
    if (!state.forcedCaptureMoves.empty() && state.forcedCapturePlayer == state.currentPlayer) {
        bool isForcedMove = false;
        for (const Move& forced : state.forcedCaptureMoves) {
            if (forced.x == move.x && forced.y == move.y) {
                isForcedMove = true;
                break;
            }
        }
        
        if (!isForcedMove) {
            // Player tried to play elsewhere when there's a forced capture
            // The player with the 5-in-a-row wins automatically!
            std::cout << "FORCED CAPTURE VIOLATION: Player " << state.currentPlayer 
                      << " must capture at one of " << state.forcedCaptureMoves.size() 
                      << " positions!" << std::endl;
            std::cout << "Player " << state.pendingWinPlayer << " wins!" << std::endl;
            // We return false to indicate the move was invalid
            // The GUI/main loop should check if pendingWinPlayer != 0 to declare winner
            return false;
        }
        
        std::cout << "FORCED CAPTURE MADE: Player " << state.currentPlayer 
                  << " captured at forced position (" << move.x << "," << move.y << ")" << std::endl;
    }
    
    lastHumanMove = move;
    state.lastHumanMove = move;
    
    RuleEngine::MoveResult result = RuleEngine::applyMove(state, move);
    
    if (result.success) {
        // After the move, check if the opponent just created a 5-in-a-row
        // that we can break with a capture
        checkAndSetForcedCaptures();
    }
    
    return result.success;
}

Move GameEngine::makeAIMove() {
    if (state.currentPlayer != GameState::PLAYER2) return Move();
    
    // CHECK FOR FORCED CAPTURE: If AI must make a forced capture
    if (!state.forcedCaptureMoves.empty() && state.forcedCapturePlayer == state.currentPlayer) {
        std::cout << "AI FORCED CAPTURE: Must play at one of " << state.forcedCaptureMoves.size() 
                  << " positions" << std::endl;
        // For now, just pick the first forced capture move
        // TODO: AI could evaluate which capture is best if there are multiple options
        Move forcedMove = state.forcedCaptureMoves[0];
        RuleEngine::applyMove(state, forcedMove);
        std::cout << "AI played forced capture at (" << forcedMove.x << "," << forcedMove.y << ")" << std::endl;
        return forcedMove;
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    Move bestMove = ai.getBestMove(state);
    auto end = std::chrono::high_resolution_clock::now();
    lastAITime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    DEBUG_LOG_STATS("AI Stats: " + std::to_string(ai.getLastNodesEvaluated()) + 
                   " nodes, " + std::to_string(lastAITime) + "ms");
    
    if (bestMove.isValid()) {
        RuleEngine::applyMove(state, bestMove);
        // After AI moves, check if human now has forced captures
        checkAndSetForcedCaptures();
    }
    
    return bestMove;
}

bool GameEngine::isGameOver() const
{
	// En modo CAPTURE_MODE, el juego termina con 15 capturas (no con 5 en línea)
	if (currentMode == GameMode::CAPTURE_MODE)
	{
		return state.captures[0] >= 15 || state.captures[1] >= 15;
	}
	
	// En modos normales, verificar 5 en línea
	return RuleEngine::checkWin(state, GameState::PLAYER1) ||
		   RuleEngine::checkWin(state, GameState::PLAYER2);
}

int GameEngine::getWinner() const
{
	// En modo CAPTURE_MODE, gana quien llegue a 15 capturas primero
	if (currentMode == GameMode::CAPTURE_MODE)
	{
		if (state.captures[0] >= 15)
			return GameState::PLAYER1;
		if (state.captures[1] >= 15)
			return GameState::PLAYER2;
		return 0; // No hay ganador todavía
	}
	
	// En modos normales, verificar 5 en línea
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

void GameEngine::checkAndSetForcedCaptures() {
    // Clear any previous forced captures
    state.forcedCaptureMoves.clear();
    state.forcedCapturePlayer = 0;
    state.pendingWinPlayer = 0;
    
    // Check if the PREVIOUS player (who just moved) created a 5-in-a-row
    int previousPlayer = state.getOpponent(state.currentPlayer);
    
    // Search for 5-in-a-row for the previous player
    int directions[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};
    
    for (int i = 0; i < GameState::BOARD_SIZE; i++) {
        for (int j = 0; j < GameState::BOARD_SIZE; j++) {
            if (state.board[i][j] != previousPlayer) continue;
            
            for (int d = 0; d < 4; d++) {
                int dx = directions[d][0];
                int dy = directions[d][1];
                
                // Check if there's a line of 5 starting here
                int count = 0;
                for (int k = 0; k < 5; k++) {
                    int ni = i + k * dx;
                    int nj = j + k * dy;
                    if (state.isValid(ni, nj) && state.board[ni][nj] == previousPlayer) {
                        count++;
                    } else {
                        break;
                    }
                }
                
                if (count >= 5) {
                    // Found 5-in-a-row! Check if current player can break it
                    Move lineStart(i, j);
                    std::vector<Move> captureMoves;
                    
                    if (RuleEngine::canBreakLineByCapture(state, lineStart, dx, dy, 
                                                          previousPlayer, &captureMoves)) {
                        // Current player CAN break it - set forced captures
                        state.forcedCaptureMoves = captureMoves;
                        state.forcedCapturePlayer = state.currentPlayer;
                        state.pendingWinPlayer = previousPlayer;
                        
                        std::cout << "FORCED CAPTURE SET: Player " << state.currentPlayer 
                                  << " MUST capture one of " << captureMoves.size() 
                                  << " positions to prevent Player " << previousPlayer 
                                  << " from winning!" << std::endl;
                        for (const Move& m : captureMoves) {
                            std::cout << "  - Forced capture position: (" << m.x << "," << m.y << ")" << std::endl;
                        }
                        return; // Only need to find one
                    }
                }
            }
        }
    }
}
