/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/14 21:27:46 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/23 15:28:03 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/game_engine.hpp"
#include "../include/display.hpp"
#include "../include/game_types.hpp"
#include "../include/debug_analyzer.hpp"
#include <iostream>
#include <chrono>

int main()
{
	std::cout << "=== GOMOKU AI CON ZOBRIST HASHING ===" << std::endl;
	std::cout << "Inicializando optimizaciones..." << std::endl;

	// CRÍTICO: Inicializar sistema de debug PRIMERO
	g_debugAnalyzer = new DebugAnalyzer(DebugAnalyzer::DEBUG_HEURISTIC);
	g_debugAnalyzer->enableFileLogging("gomoku_debug.log");

	// CRÍTICO: Inicializar Zobrist hasher ANTES de crear cualquier GameState
	GameState::initializeHasher();
	DEBUG_LOG_INIT("Zobrist hasher inicializado con " + std::to_string(GameState::BOARD_SIZE * GameState::BOARD_SIZE * 2) + " claves de pieza + 22 claves de captura + 1 clave de turno");

	// Crear motor de juego (ahora con Zobrist optimizado)
	GameEngine game;
	game.newGame();
	DEBUG_LOG_INIT("Motor de juego inicializado");

	Display::printWelcome();

	std::cout << "\n=== OPTIMIZACIONES ACTIVAS ===" << std::endl;
	std::cout << "• Zobrist Hashing: ACTIVADO (Hash incremental O(1))" << std::endl;
	std::cout << "• Transposition Table: ACTIVADO (64MB cache)" << std::endl;
	std::cout << "• Move Ordering: ACTIVADO (Ordenamiento inteligente)" << std::endl;
	std::cout << "• Alpha-Beta Pruning: ACTIVADO (Poda agresiva)" << std::endl;
	std::cout << "• Debug avanzado: ACTIVADO (salida a gomoku_debug.log)" << std::endl;
	std::cout << "Esperando mejora de ~50-100x en velocidad..." << std::endl;
	std::cout << "========================================\n"
		  << std::endl;	while (!game.isGameOver())
	{
		const GameState &state = game.getState();

		// Mostrar tablero actual
		Display::printBoard(state);
		Display::printGameInfo(state, game.getLastAIThinkingTime());

		if (state.currentPlayer == GameState::PLAYER1)
		{
			// Turno del humano
			std::cout << "Tu turno (Player 1)\n";
			auto userInput = Display::getUserMove();

			if (userInput.first == -1)
			{ // Quit
				std::cout << "¡Gracias por jugar!" << std::endl;
				break;
			}

			if (userInput.first == -2)
			{ // Error de input
				std::cout << "Input inválido. Intenta de nuevo (ej: 'J10')" << std::endl;
				continue;
			}

			Move humanMove(userInput.first, userInput.second);
			if (!game.makeHumanMove(humanMove))
			{
				std::cout << "❌ Movimiento inválido. Razones:" << std::endl;
				std::cout << "- Posición ocupada o fuera de límites" << std::endl;
				std::cout << "- Violación de regla double free-three" << std::endl;
				std::cout << "Intenta de nuevo..." << std::endl;
				continue;
			}
		}
		else
		{
			// Turno de la IA
			std::cout << "AI pensando..." << std::endl;

			auto startTime = std::chrono::high_resolution_clock::now();
			Move aiMove = game.makeAIMove();
			auto endTime = std::chrono::high_resolution_clock::now();

			if (aiMove.isValid())
			{
				auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

				std::cout << "AI jugó: " << char('A' + aiMove.y) << (aiMove.x + 1) 
						  << " (" << duration.count() << "ms)" << std::endl;
				
				// Estadísticas detalladas van al archivo de debug
				DEBUG_LOG_STATS("Movimiento: " + std::string(1, char('A' + aiMove.y)) + std::to_string(aiMove.x + 1) +
							   ", Tiempo: " + std::to_string(duration.count()) + "ms" +
							   ", Nodos: " + std::to_string(game.getLastNodesEvaluated()) +
							   ", Cache hits: " + std::to_string(game.getLastCacheHits()) + 
							   " (" + std::to_string(game.getLastCacheHitRate() * 100) + "% hit rate)" +
							   ", Cache size: " + std::to_string(game.getCacheSize()) + " entradas");
			}
			else
			{
				std::cout << "¡AI no pudo encontrar un movimiento válido!" << std::endl;
				break;
			}
		}
	}

	// Mostrar resultado final
	if (game.isGameOver())
	{
		Display::printBoard(game.getState());
		Display::printGameInfo(game.getState(), game.getLastAIThinkingTime());
		Display::printWinner(game.getWinner());

		// Estadísticas finales van al debug
		DEBUG_LOG_STATS("PARTIDA FINALIZADA - Cache size final: " + std::to_string(game.getCacheSize()) + " entradas");
	}

	if (g_debugAnalyzer)
	{
		delete g_debugAnalyzer;
		g_debugAnalyzer = nullptr;
	}

	return 0;
}