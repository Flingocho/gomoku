/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/14 21:27:46 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/24 20:01:18 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/game_engine.hpp"
#include "../include/gui_renderer.hpp"
#include "../include/game_types.hpp"
#include "../include/debug_analyzer.hpp"
#include <iostream>
#include <chrono>

int main()
{
	// TERMINAL: Solo debug/info - mantiene funcionalidad actual
	std::cout << "=== GOMOKU AI CON ZOBRIST HASHING ===" << std::endl;
	std::cout << "Inicializando optimizaciones..." << std::endl;

	// Inicialización del sistema (igual que antes)
	GameState::initializeHasher();
	std::cout << "✓ Zobrist hasher inicializado" << std::endl;

	g_debugAnalyzer = new DebugAnalyzer(DebugAnalyzer::DEBUG_HEURISTIC);
	g_debugAnalyzer->enableFileLogging("gomoku_debug.log");
	std::cout << "✓ Inicializando sistema de debug..." << std::endl;

	GameEngine game;
	std::cout << "✓ Motor de juego inicializado" << std::endl;

	// NUEVO: GUI Renderer reemplaza Display
	GuiRenderer renderer;
	std::cout << "✓ Interfaz gráfica inicializada" << std::endl;

	std::cout << "\n=== OPTIMIZACIONES ACTIVAS ===" << std::endl;
	std::cout << "• Zobrist Hashing: ACTIVADO (Hash incremental O(1))" << std::endl;
	std::cout << "• Transposition Table: ACTIVADO (64MB cache)" << std::endl;
	std::cout << "• Move Ordering: ACTIVADO (Ordenamiento inteligente)" << std::endl;
	std::cout << "• Alpha-Beta Pruning: ACTIVADO (Poda agresiva)" << std::endl;
	std::cout << "Esperando mejora de ~50-100x en velocidad..." << std::endl;
	std::cout << "========================================\n"
			  << std::endl;

	// NUEVO: Main game loop con GUI
	bool gameActive = true;

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
				std::cout << "Iniciando juego vs AI" << std::endl;
				game.newGame();
				renderer.setState(GuiRenderer::PLAYING);
			}
			else if (choice == GuiRenderer::VS_HUMAN)
			{
				std::cout << "Iniciando juego vs Humano (hotseat)" << std::endl;
				game.newGame();
				renderer.setState(GuiRenderer::PLAYING);
			}
			else if (choice == GuiRenderer::QUIT)
			{
				gameActive = false;
			}

			// Renderizar menú
			renderer.render(game.getState());
			break;
		}

			// En main.cpp, dentro del case GuiRenderer::PLAYING:
		case GuiRenderer::PLAYING:
		{
			const GameState &state = game.getState();

			// Verificar si el juego terminó
			if (game.isGameOver())
			{
				renderer.setState(GuiRenderer::GAME_OVER);
				break;
			}

			if (state.currentPlayer == GameState::PLAYER1)
			{
				// TURNO HUMANO - igual que antes
				static bool waitingForMove = true;

				if (waitingForMove)
				{
					std::cout << "Esperando movimiento del jugador..." << std::endl;
					waitingForMove = false;
				}

				if (renderer.hasUserMove())
				{
					Move humanMove = renderer.getUserMove();

					if (humanMove.isValid())
					{
						if (!game.makeHumanMove(humanMove))
						{
							std::cout << "❌ Movimiento inválido" << std::endl;
						}
						else
						{
							std::cout << "✓ Jugador movió: "
									  << char('A' + humanMove.y) << (humanMove.x + 1) << std::endl;
							waitingForMove = true;
						}
					}
				}
			}
			else
			{
				// TURNO AI - Simplificado sin threading
				std::cout << "AI pensando..." << std::endl;
				
				Move aiMove = game.makeAIMove();

				if (aiMove.isValid())
				{
					std::cout << "AI jugó: " << char('A' + aiMove.y) << (aiMove.x + 1) << std::endl;
					std::cout << "Tiempo: " << game.getLastAIThinkingTime() << "ms" << std::endl;
					std::cout << "Nodos evaluados: " << game.getLastNodesEvaluated() << std::endl;
					std::cout << "Cache hits: " << game.getLastCacheHits()
							  << " (hit rate: " << (game.getLastCacheHitRate() * 100) << "%)" << std::endl;
					std::cout << "Cache size: " << game.getCacheSize() << " entradas" << std::endl;
				}
			}

			// Renderizar SIEMPRE (no bloquear por AI)
			renderer.render(game.getState(), game.getLastAIThinkingTime());
			break;
		}

		case GuiRenderer::GAME_OVER:
		{
			// Procesar eventos para detectar clicks en botones
			renderer.refreshSelectedMenuOption();
			renderer.processEvents();
			renderer.render(game.getState());

			// // Verificar si se seleccionó "Nuevo Juego"
			// if (renderer.showMenuAndGetChoice() == GuiRenderer::VS_AI) {
			//     std::cout << "Reiniciando juego..." << std::endl;
			//     game.newGame();
			//     renderer.setState(GuiRenderer::PLAYING);
			//     // Reset selectedMenuOption para evitar bucle
			//     // (esto se hace internamente en handleGameOverClick)
			// }

			// // Renderizar pantalla de game over
			// renderer.render(game.getState());
			break;
		}
		}
	}

	// Cleanup - igual que antes
	std::cout << "\n=== CERRANDO APLICACIÓN ===" << std::endl;
	if (game.isGameOver())
	{
		std::cout << "Estadísticas finales:" << std::endl;
		std::cout << "Cache size final: " << game.getCacheSize() << " entradas" << std::endl;
		std::cout << "Hash final: 0x" << std::hex << game.getState().getZobristHash() << std::dec << std::endl;
	}

	if (g_debugAnalyzer)
	{
		delete g_debugAnalyzer;
		g_debugAnalyzer = nullptr;
	}

	std::cout << "¡Gracias por jugar!" << std::endl;
	return 0;
}
