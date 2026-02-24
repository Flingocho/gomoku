#include "../include/core/game_engine.hpp"
#include "../include/gui/gui_renderer.hpp"
#include "../include/core/game_types.hpp"
#include "../include/debug/debug_analyzer.hpp"
#include "../include/ai/suggestion_engine.hpp"
#include <iostream>
#include <chrono>
#include <stdexcept>
#include <new>

int main()
{
	try
	{
	std::cout << "=== GOMOKU AI WITH ZOBRIST HASHING ===" << std::endl;
	std::cout << "Initializing..." << std::endl;

	// Inicialización del sistema
	GameState::initializeHasher();

	g_debugAnalyzer = new DebugAnalyzer(DebugAnalyzer::DEBUG_OFF);
	g_debugAnalyzer->enableFileLogging("gomoku_debug.log");

	GameEngine game;
	GuiRenderer renderer;

	std::cout << "✓ Game ready\n" << std::endl;

	// Variables de control
	bool gameActive = true;
	bool menuStateInitialized = false;

	while (renderer.isWindowOpen() && gameActive)
	{
		renderer.processEvents();

		switch (renderer.getState())
		{
		case GuiRenderer::MENU:
		{
			if (!menuStateInitialized) {
				renderer.clearSuggestion();
				renderer.setWinningLine(std::vector<Move>());
				renderer.clearInvalidMoveError();
				menuStateInitialized = true;
			}
			
			GuiRenderer::MenuOption choice = renderer.showMenuAndGetChoice();

			if (choice == GuiRenderer::VS_AI)
			{
				std::cout << "Starting game vs AI" << std::endl;
				game.setGameMode(GameMode::VS_AI);
				game.setAiImplementation(CPP_IMPLEMENTATION);
				game.newGame();
				renderer.resetAiStats();
				renderer.setState(GuiRenderer::PLAYING);
				menuStateInitialized = false;
			}
			else if (choice == GuiRenderer::VS_HUMAN)
			{
				std::cout << "Starting game vs Human (with AI suggestions)" << std::endl;
				game.setGameMode(GameMode::VS_HUMAN_SUGGESTED);
				game.newGame();
				renderer.resetAiStats();
				renderer.setState(GuiRenderer::PLAYING);
				menuStateInitialized = false;
			}
			else if (choice == GuiRenderer::COLORBLIND)
			{
				std::cout << "Starting Colorblind Mode vs AI" << std::endl;
				game.setGameMode(GameMode::VS_AI);
				game.setAiImplementation(CPP_IMPLEMENTATION);
				game.newGame();
				renderer.resetAiStats();
				renderer.setState(GuiRenderer::PLAYING);
				menuStateInitialized = false;
			}
			else if (choice == GuiRenderer::RUST_AI)
			{
				std::cout << "Starting game vs Rust AI" << std::endl;
				game.setGameMode(GameMode::VS_AI);
				game.setAiImplementation(RUST_IMPLEMENTATION);
				game.newGame();
				renderer.resetAiStats();
				renderer.setState(GuiRenderer::PLAYING);
				menuStateInitialized = false;
			}
			else if (choice == GuiRenderer::OPTIONS_MENU)
			{
				renderer.setState(GuiRenderer::OPTIONS);
				renderer.refreshSelectedMenuOption();
			}
			else if (choice == GuiRenderer::QUIT)
			{
				gameActive = false;
			}

			renderer.render(game.getState());
			break;
		}

		case GuiRenderer::OPTIONS:
		{
			// Actualizar nivel de debug según configuración
			if (renderer.isDebugEnabled()) {
				g_debugAnalyzer->setDebugLevel(DebugAnalyzer::DEBUG_TOP_MOVES);
			} else {
				g_debugAnalyzer->setDebugLevel(DebugAnalyzer::DEBUG_OFF);
			}
			
			renderer.render(game.getState());
			break;
		}

		case GuiRenderer::PLAYING:
		{
			const GameState &state = game.getState();

			if (game.isGameOver())
			{
				std::vector<Move> winningLine = game.findWinningLine();
				renderer.setWinningLine(winningLine);
				renderer.setState(GuiRenderer::GAME_OVER);
				renderer.refreshSelectedMenuOption();
				renderer.showGameResult(game.getWinner());
				
				int winner = game.getWinner();
				if (winner == GameState::PLAYER1) {
					renderer.playVictorySound();
				} else if (winner == GameState::PLAYER2) {
					renderer.playDefeatSound();
				}
				
				break;
			}

			// Modo VS_HUMAN_SUGGESTED: Ambos jugadores son humanos con sugerencias
			if (game.getGameMode() == GameMode::VS_HUMAN_SUGGESTED)
			{
				// Generar sugerencia solo cuando no hay una activa (evitar recalcular cada frame)
				if (!renderer.hasSuggestion()) {
					Move suggestion = SuggestionEngine::getSuggestion(state, 6);
					if (suggestion.isValid()) {
						renderer.setSuggestion(suggestion);
					}
				}
				
				if (renderer.hasUserMove())
				{
					Move humanMove = renderer.getUserMove();

					if (humanMove.isValid())
					{
						if (!game.makeHumanMove(humanMove))
						{
							renderer.showInvalidMoveError(humanMove);
							renderer.clearUserMove();
						}
						else
						{
							std::string player = (state.currentPlayer == GameState::PLAYER1) ? "Player 1" : "Player 2";
							if (renderer.isDebugEnabled()) {
								std::cout << player << ": " << char('A' + humanMove.y) << (humanMove.x + 1) << std::endl;
							}
							renderer.playPlacePieceSound();
							renderer.clearUserMove();
							renderer.clearSuggestion();
						}
					}
				}
			}
			// Modo VS_AI: Jugador 1 humano, Jugador 2 IA
			else if (state.currentPlayer == GameState::PLAYER1)
			{
				if (renderer.hasUserMove())
				{
					Move humanMove = renderer.getUserMove();

					if (humanMove.isValid())
					{
						if (!game.makeHumanMove(humanMove))
						{
							renderer.showInvalidMoveError(humanMove);
							renderer.clearUserMove();
						}
						else
						{
							if (renderer.isDebugEnabled()) {
								std::cout << "Player: " << char('A' + humanMove.y) << (humanMove.x + 1) << std::endl;
							}
							renderer.playPlacePieceSound();
							renderer.clearUserMove();
						}
					}
				}
			}
			else
			{
				// Turno AI
				Move aiMove = game.makeAIMove();

				if (aiMove.isValid())
				{
					renderer.setLastAiMove(aiMove);
					renderer.addAiTime(game.getLastAIThinkingTime());
					renderer.playPlacePieceSound();
					
					if (renderer.isDebugEnabled()) {
						std::cout << "AI: " << char('A' + aiMove.y) << (aiMove.x + 1) 
								  << " (" << game.getLastAIThinkingTime() << "ms)" << std::endl;
					}
				}
			}

			renderer.render(game.getState(), game.getLastAIThinkingTime());
			break;
		}

		case GuiRenderer::GAME_OVER:
		{
			renderer.render(game.getState());

			int selectedOption = renderer.getSelectedMenuOption();
			if (selectedOption == 0)
			{
				game.newGame();
				game.clearAICache();
				renderer.clearSuggestion();
				renderer.clearInvalidMoveError();
				renderer.resetAiStats();
				renderer.setWinningLine(std::vector<Move>());
				renderer.setState(GuiRenderer::PLAYING);
				renderer.refreshSelectedMenuOption();
				menuStateInitialized = false;
				break;
			}

			renderer.refreshSelectedMenuOption();
			break;
		}
		}
	}

	// Cleanup
	if (g_debugAnalyzer)
	{
		delete g_debugAnalyzer;
		g_debugAnalyzer = nullptr;
	}
	
	// Cleanup hasher
	GameState::cleanupHasher();

	std::cout << "Thanks for playing!" << std::endl;
	return 0;

	} catch (const std::bad_alloc& e) {
		std::cerr << "Error: Out of memory - " << e.what() << std::endl;
		if (g_debugAnalyzer) {
			delete g_debugAnalyzer;
			g_debugAnalyzer = nullptr;
		}
		GameState::cleanupHasher();
		return 1;
	} catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		if (g_debugAnalyzer) {
			delete g_debugAnalyzer;
			g_debugAnalyzer = nullptr;
		}
		GameState::cleanupHasher();
		return 1;
	} catch (...) {
		std::cerr << "Error: Unknown exception occurred" << std::endl;
		if (g_debugAnalyzer) {
			delete g_debugAnalyzer;
			g_debugAnalyzer = nullptr;
		}
		GameState::cleanupHasher();
		return 1;
	}
}
