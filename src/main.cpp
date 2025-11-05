/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/14 21:27:46 by jainavas          #+#    #+#             */
/*   Updated: 2025/10/01 22:32:22 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/game_engine.hpp"
#include "../include/gui_renderer.hpp"
#include "../include/game_types.hpp"
#include "../include/debug_analyzer.hpp"
#include "../include/suggestion_engine.hpp" // NUEVO
#include <iostream>
#include <chrono>

int main()
{
	// TERMINAL: Solo debug/info - mantiene funcionalidad actual
	std::cout << "=== GOMOKU AI WITH ZOBRIST HASHING ===" << std::endl;
	std::cout << "Initializing optimizations..." << std::endl;

	// Inicialización del sistema (igual que antes)
	GameState::initializeHasher();
	std::cout << "✓ Zobrist hasher initialized" << std::endl;

	g_debugAnalyzer = new DebugAnalyzer(DebugAnalyzer::DEBUG_HEURISTIC);
	g_debugAnalyzer->enableFileLogging("gomoku_debug.log");
	std::cout << "✓ Initializing debug system..." << std::endl;

	GameEngine game;
	std::cout << "✓ Game engine initialized" << std::endl;

	GuiRenderer renderer;
	std::cout << "✓ Graphical interface initialized" << std::endl;

	std::cout << "\n=== ACTIVE OPTIMIZATIONS ===" << std::endl;
	std::cout << "• Zobrist Hashing: ENABLED (Incremental O(1) Hash)" << std::endl;
	std::cout << "• Transposition Table: ENABLED (64MB cache)" << std::endl;
	std::cout << "• Move Ordering: ENABLED (Smart ordering)" << std::endl;
	std::cout << "• Alpha-Beta Pruning: ENABLED (Aggressive pruning)" << std::endl;
	std::cout << "Expecting ~50-100x speed improvement..." << std::endl;
	std::cout << "========================================\n"
			  << std::endl;

	// Variables para modo hotseat
	bool gameActive = true;
	Move currentSuggestion(-1, -1);
	bool suggestionCalculated = false;

	while (renderer.isWindowOpen() && gameActive)
	{
		// Procesar eventos de ventana
		renderer.processEvents();

		// State machine principal
		switch (renderer.getState())
		{
		case GuiRenderer::MENU:
		{
			// Mostrar menú y esperar selección
			GuiRenderer::MenuOption choice = renderer.showMenuAndGetChoice();

			if (choice == GuiRenderer::VS_AI)
			{
				std::cout << "Starting game vs AI" << std::endl;
				game.setGameMode(GameMode::VS_AI);
				game.newGame();
				renderer.resetAiStats();
				renderer.setState(GuiRenderer::PLAYING);
				suggestionCalculated = false;
				currentSuggestion = Move(-1, -1);
			}
			else if (choice == GuiRenderer::VS_HUMAN)
			{
				std::cout << "Starting game vs Human (hotseat with suggestions)" << std::endl;
				game.setGameMode(GameMode::VS_HUMAN_SUGGESTED);
				game.newGame();
				renderer.resetAiStats();
				renderer.setState(GuiRenderer::PLAYING);
				suggestionCalculated = false;
				currentSuggestion = Move(-1, -1);
			}
		else if (choice == GuiRenderer::COLORBLIND)
		{
			std::cout << "Starting Colorblind Mode vs AI (same color pieces)" << std::endl;
			game.setGameMode(GameMode::VS_AI); // Misma lógica que VS_AI pero con colores iguales
			game.newGame();
			renderer.resetAiStats();
			renderer.setState(GuiRenderer::PLAYING);
			suggestionCalculated = false;
			currentSuggestion = Move(-1, -1);
		}
		else if (choice == GuiRenderer::RUST_AI)
		{
			std::cout << "Starting game vs Rust AI" << std::endl;
			game.setGameMode(GameMode::VS_AI);
			game.setAiImplementation(RUST_IMPLEMENTATION); // Usar implementación Rust
			game.newGame();
			renderer.resetAiStats();
			renderer.setState(GuiRenderer::PLAYING);
			suggestionCalculated = false;
			currentSuggestion = Move(-1, -1);
		}
		else if (choice == GuiRenderer::CAPTURE_MODE)
		{
			std::cout << "Starting Capture Mode vs AI (15 captures to win)" << std::endl;
			game.setGameMode(GameMode::CAPTURE_MODE);
			game.newGame();
			renderer.resetAiStats();
			renderer.setState(GuiRenderer::PLAYING);
			suggestionCalculated = false;
			currentSuggestion = Move(-1, -1);
		}
		else if (choice == GuiRenderer::QUIT)
		{
			gameActive = false;
		}

		// Renderizar menú
		renderer.render(game.getState());
		break;
	}		case GuiRenderer::PLAYING:
		{
			const GameState &state = game.getState();

			// Verificar si el juego terminó
			if (game.isGameOver())
			{
				std::vector<Move> winningLine = game.findWinningLine();
				renderer.setWinningLine(winningLine);
				renderer.setState(GuiRenderer::GAME_OVER);
				renderer.refreshSelectedMenuOption(); // Resetear selección al entrar en GAME_OVER
				renderer.showGameResult(game.getWinner());
				break;
			}

			// DIFERENTES COMPORTAMIENTOS SEGÚN MODO
			if (game.getGameMode() == GameMode::VS_AI || game.getGameMode() == GameMode::CAPTURE_MODE)
			{
				// ============================================
				// MODO VS AI y CAPTURE_MODE (mismo comportamiento)
				// ============================================
				if (state.currentPlayer == GameState::PLAYER1)
				{
					// Turno humano
					static bool waitingForMove = true;

					if (waitingForMove)
					{
						std::cout << "Waiting for player move..." << std::endl;
						waitingForMove = false;
					}

					if (renderer.hasUserMove())
					{
						Move humanMove = renderer.getUserMove();

						if (humanMove.isValid())
						{
							if (!game.makeHumanMove(humanMove))
							{
								std::cout << "❌ Invalid move" << std::endl;
								renderer.showInvalidMoveError(humanMove); // NUEVO: Mostrar error en GUI
								renderer.clearUserMove(); // CRITICAL: Limpiar movimiento inválido del buffer
							}
							else
							{
								std::cout << "✓ Player moved: "
										  << char('A' + humanMove.y) << (humanMove.x + 1) << std::endl;
								waitingForMove = true;
								renderer.clearUserMove(); // Limpiar movimiento válido también
							}
						}
					}
				}
				else
				{
					// Turno AI
					std::cout << "AI thinking..." << std::endl;

					Move aiMove = game.makeAIMove();

					if (aiMove.isValid())
					{
						renderer.setLastAiMove(aiMove);
						renderer.addAiTime(game.getLastAIThinkingTime());
						std::cout << "AI played: " << char('A' + aiMove.y) << (aiMove.x + 1) << std::endl;
						std::cout << "Time: " << game.getLastAIThinkingTime() << "ms" << std::endl;
						std::cout << "Nodes evaluated: " << game.getLastNodesEvaluated() << std::endl;
						std::cout << "Cache hits: " << game.getLastCacheHits()
								  << " (hit rate: " << (game.getLastCacheHitRate() * 100) << "%)" << std::endl;
						std::cout << "Cache size: " << game.getCacheSize() << " entries" << std::endl;
					}
				}
			}
			else // GameMode::VS_HUMAN_SUGGESTED
			{
				// ============================================
				// MODO HOTSEAT CON SUGERENCIAS (nuevo)
				// ============================================

				// PASO 1: Calcular sugerencia si no está calculada
				if (!suggestionCalculated)
				{
					std::string playerName = (state.currentPlayer == GameState::PLAYER1) ? "Player 1 (O)" : "Player 2 (X)";
					std::cout << "💡 Calculating suggestion for " << playerName << "..." << std::endl;

					auto startTime = std::chrono::high_resolution_clock::now();
					currentSuggestion = SuggestionEngine::getSuggestion(state);
					auto endTime = std::chrono::high_resolution_clock::now();

					int suggestionTime = std::chrono::duration_cast<std::chrono::milliseconds>(
											 endTime - startTime)
											 .count();

					if (currentSuggestion.isValid())
					{
						std::cout << "💡 Suggestion: "
								  << char('A' + currentSuggestion.y)
								  << (currentSuggestion.x + 1)
								  << " (calculated in " << suggestionTime << "ms)" << std::endl;

						renderer.setSuggestion(currentSuggestion);
					}

					suggestionCalculated = true;
				}

				// PASO 2: Esperar movimiento del jugador actual
				if (renderer.hasUserMove())
				{
					Move humanMove = renderer.getUserMove();

					if (humanMove.isValid())
					{
						// Guardar el jugador ANTES de aplicar el movimiento
						int playerWhoMoved = state.currentPlayer;
						std::string playerName = (playerWhoMoved == GameState::PLAYER1) ? "Player 1 (O)" : "Player 2 (X)";

						if (!game.makeHumanMove(humanMove))
						{
							std::cout << "❌ Invalid move" << std::endl;
							renderer.showInvalidMoveError(humanMove); // NUEVO: Mostrar error en GUI
							renderer.clearUserMove(); // CRITICAL: Limpiar movimiento inválido del buffer
						}
						else
						{
							std::cout << "✓ " << playerName << " moved: "
									  << char('A' + humanMove.y) << (humanMove.x + 1);

							// Indicar si siguió la sugerencia
							if (currentSuggestion.isValid() &&
								humanMove.x == currentSuggestion.x &&
								humanMove.y == currentSuggestion.y)
							{
								std::cout << " ✨ (followed suggestion)";
							}
							std::cout << std::endl;

							// IMPORTANTE: Reset para próximo turno
							renderer.clearSuggestion();
							renderer.clearUserMove(); // Limpiar movimiento válido también
							suggestionCalculated = false;
							currentSuggestion = Move(-1, -1);
						}
					}
				}
			}

			// Renderizar (común para ambos modos)
			renderer.render(game.getState(), game.getLastAIThinkingTime());
			break;
		}

		case GuiRenderer::GAME_OVER:
		{
			// Primero procesar eventos (para capturar clicks)
			renderer.processEvents();
			renderer.render(game.getState());

            // NUEVO: Detectar si se seleccionó "New Game"
            int selectedOption = renderer.getSelectedMenuOption();
            if (selectedOption == 0)
            {
                std::cout << "\n=== REINICIANDO JUEGO ===" << std::endl;				// Reiniciar el juego
				game.newGame();
				renderer.clearSuggestion();
				renderer.clearInvalidMoveError();
				renderer.resetAiStats();
				renderer.setWinningLine(std::vector<Move>()); // Limpiar línea ganadora
				renderer.setState(GuiRenderer::PLAYING);
				renderer.refreshSelectedMenuOption(); // Reset selección DESPUÉS de procesarla

				// Resetear variables de sugerencias (si usas modo vs humano)
				suggestionCalculated = false;
				currentSuggestion = Move(-1, -1);

				std::cout << "Nuevo juego iniciado" << std::endl;

				// Volver al loop de juego
				break;
			}

			// Resetear selección solo si no se procesó nada importante
			renderer.refreshSelectedMenuOption();

			break;
		}
		}
	}

	// Cleanup
	std::cout << "\n=== CLOSING APPLICATION ===" << std::endl;
	if (game.isGameOver())
	{
		std::cout << "Final statistics:" << std::endl;
		std::cout << "Final cache size: " << game.getCacheSize() << " entries" << std::endl;
		std::cout << "Final hash: 0x" << std::hex << game.getState().getZobristHash() << std::dec << std::endl;
	}

	if (g_debugAnalyzer)
	{
		delete g_debugAnalyzer;
		g_debugAnalyzer = nullptr;
	}

	std::cout << "Thanks for playing!" << std::endl;
	return 0;
}
