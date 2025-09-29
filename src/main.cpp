/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/14 21:27:46 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/29 19:58:44 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/game_engine.hpp"
#include "../include/gui_renderer.hpp"
#include "../include/game_types.hpp"
#include "../include/debug_analyzer.hpp"
#include "../include/suggestion_engine.hpp"  // NUEVO
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

    GuiRenderer renderer;
    std::cout << "✓ Interfaz gráfica inicializada" << std::endl;

    std::cout << "\n=== OPTIMIZACIONES ACTIVAS ===" << std::endl;
    std::cout << "• Zobrist Hashing: ACTIVADO (Hash incremental O(1))" << std::endl;
    std::cout << "• Transposition Table: ACTIVADO (64MB cache)" << std::endl;
    std::cout << "• Move Ordering: ACTIVADO (Ordenamiento inteligente)" << std::endl;
    std::cout << "• Alpha-Beta Pruning: ACTIVADO (Poda agresiva)" << std::endl;
    std::cout << "Esperando mejora de ~50-100x en velocidad..." << std::endl;
    std::cout << "========================================\n" << std::endl;

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
                std::cout << "Iniciando juego vs AI" << std::endl;
                game.setGameMode(GameMode::VS_AI);
                game.newGame();
                renderer.resetAiStats();
                renderer.setState(GuiRenderer::PLAYING);
                suggestionCalculated = false;
                currentSuggestion = Move(-1, -1);
            }
            else if (choice == GuiRenderer::VS_HUMAN)
            {
                std::cout << "Iniciando juego vs Humano (hotseat con sugerencias)" << std::endl;
                game.setGameMode(GameMode::VS_HUMAN_SUGGESTED);
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
        }

        case GuiRenderer::PLAYING:
        {
            const GameState &state = game.getState();

            // Verificar si el juego terminó
            if (game.isGameOver())
            {
                renderer.setState(GuiRenderer::GAME_OVER);
                break;
            }

            // DIFERENTES COMPORTAMIENTOS SEGÚN MODO
            if (game.getGameMode() == GameMode::VS_AI)
            {
                // ============================================
                // MODO VS AI (código original sin cambios)
                // ============================================
                if (state.currentPlayer == GameState::PLAYER1)
                {
                    // Turno humano
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
                    // Turno AI
                    std::cout << "AI pensando..." << std::endl;
                    
                    Move aiMove = game.makeAIMove();

                    if (aiMove.isValid())
                    {
                        renderer.setLastAiMove(aiMove);
                        renderer.addAiTime(game.getLastAIThinkingTime());
                        std::cout << "AI jugó: " << char('A' + aiMove.y) << (aiMove.x + 1) << std::endl;
                        std::cout << "Tiempo: " << game.getLastAIThinkingTime() << "ms" << std::endl;
                        std::cout << "Nodos evaluados: " << game.getLastNodesEvaluated() << std::endl;
                        std::cout << "Cache hits: " << game.getLastCacheHits()
                                  << " (hit rate: " << (game.getLastCacheHitRate() * 100) << "%)" << std::endl;
                        std::cout << "Cache size: " << game.getCacheSize() << " entradas" << std::endl;
                    }
                }
            }
            else  // GameMode::VS_HUMAN_SUGGESTED
            {
                // ============================================
                // MODO HOTSEAT CON SUGERENCIAS (nuevo)
                // ============================================
                
                // PASO 1: Calcular sugerencia si no está calculada
                if (!suggestionCalculated)
                {
                    std::string playerName = (state.currentPlayer == GameState::PLAYER1) ? 
                                            "Jugador 1 (O)" : "Jugador 2 (X)";
                    std::cout << "💡 Calculando sugerencia para " << playerName << "..." << std::endl;
                    
                    auto startTime = std::chrono::high_resolution_clock::now();
                    currentSuggestion = SuggestionEngine::getSuggestion(state);
                    auto endTime = std::chrono::high_resolution_clock::now();
                    
                    int suggestionTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                        endTime - startTime).count();
                    
                    if (currentSuggestion.isValid())
                    {
                        std::cout << "💡 Sugerencia: " 
                                  << char('A' + currentSuggestion.y) 
                                  << (currentSuggestion.x + 1)
                                  << " (calculado en " << suggestionTime << "ms)" << std::endl;
                        
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
                        std::string playerName = (playerWhoMoved == GameState::PLAYER1) ? 
                                                "Jugador 1 (O)" : "Jugador 2 (X)";
                        
                        if (!game.makeHumanMove(humanMove))
                        {
                            std::cout << "❌ Movimiento inválido" << std::endl;
                        }
                        else
                        {
                            std::cout << "✓ " << playerName << " movió: "
                                      << char('A' + humanMove.y) << (humanMove.x + 1);
                            
                            // Indicar si siguió la sugerencia
                            if (currentSuggestion.isValid() && 
                                humanMove.x == currentSuggestion.x && 
                                humanMove.y == currentSuggestion.y)
                            {
                                std::cout << " ✨ (siguió la sugerencia)";
                            }
                            std::cout << std::endl;
                            
                            // IMPORTANTE: Reset para próximo turno
                            renderer.clearSuggestion();
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
            renderer.refreshSelectedMenuOption();
            renderer.processEvents();
            renderer.render(game.getState());
            break;
        }
        }
    }

    // Cleanup
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
