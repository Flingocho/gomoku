/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/14 21:27:46 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/23 17:34:32 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/game_engine.hpp"
#include "../include/gui_renderer.hpp"  // Nuevo - reemplaza display.hpp
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
    std::cout << "========================================\n" << std::endl;

    // NUEVO: Main game loop con GUI
    bool gameActive = true;
    
    while (renderer.isWindowOpen() && gameActive) {
        // Procesar eventos de ventana
        renderer.processEvents();
        
        // State machine principal
        switch (renderer.getState()) {
            case GuiRenderer::MENU: {
                // Mostrar menú y esperar selección
                GuiRenderer::MenuOption choice = renderer.showMenuAndGetChoice();
                
                if (choice == GuiRenderer::VS_AI) {
                    std::cout << "Iniciando juego vs AI" << std::endl;
                    game.newGame();
                    renderer.setState(GuiRenderer::PLAYING);
                } else if (choice == GuiRenderer::VS_HUMAN) {
                    std::cout << "Iniciando juego vs Humano (hotseat)" << std::endl;
                    game.newGame();
                    renderer.setState(GuiRenderer::PLAYING);
                } else if (choice == GuiRenderer::QUIT) {
                    gameActive = false;
                }
                
                // Renderizar menú
                renderer.render(game.getState());
                break;
            }
            
            case GuiRenderer::PLAYING: {
                const GameState& state = game.getState();
                
                // Verificar si el juego terminó
                if (game.isGameOver()) {
                    std::cout << "Juego terminado. Ganador: " << game.getWinner() << std::endl;
                    renderer.setState(GuiRenderer::GAME_OVER); // CORREGIDO: Cambiar estado directamente
                    // NO llamar a render aquí, dejar que el siguiente frame lo maneje
                    break;
                }
                
                if (state.currentPlayer == GameState::PLAYER1) {
                    // TURNO HUMANO - Approach non-blocking
                    static bool waitingForMove = true;
                    
                    if (waitingForMove) {
                        std::cout << "Esperando movimiento del jugador..." << std::endl;
                        waitingForMove = false;
                    }
                    
                    if (renderer.hasUserMove()) {
                        Move humanMove = renderer.getUserMove();
                        
                        if (humanMove.isValid()) {
                            // Aplicar movimiento
                            if (!game.makeHumanMove(humanMove)) {
                                std::cout << "❌ Movimiento inválido: " 
                                          << char('A' + humanMove.y) << (humanMove.x + 1) << std::endl;
                                std::cout << "Razones posibles: posición ocupada, double free-three" << std::endl;
                            } else {
                                std::cout << "✓ Jugador movió: " 
                                          << char('A' + humanMove.y) << (humanMove.x + 1) << std::endl;
                                waitingForMove = true; // Reset para próximo turno
                            }
                        }
                    }
                } else {
                    // TURNO AI - Approach non-blocking
                    static bool aiThinking = false;
                    static std::chrono::time_point<std::chrono::high_resolution_clock> aiStartTime;
                    
                    if (!aiThinking) {
                        std::cout << "AI pensando..." << std::endl;
                        aiThinking = true;
                        aiStartTime = std::chrono::high_resolution_clock::now();
                    }
                    
                    // Ejecutar IA (esto sigue siendo blocking, pero solo afecta al cálculo)
                    Move aiMove = game.makeAIMove();
                    
                    if (aiMove.isValid()) {
                        auto endTime = std::chrono::high_resolution_clock::now();
                        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - aiStartTime);
                        
                        // TERMINAL: Stats y debug (como antes)
                        std::cout << "AI jugó: " << char('A' + aiMove.y) << (aiMove.x + 1) << std::endl;
                        std::cout << "Tiempo: " << duration.count() << "ms" << std::endl;
                        std::cout << "Nodos evaluados: " << game.getLastNodesEvaluated() << std::endl;
                        std::cout << "Cache hits: " << game.getLastCacheHits()
                                  << " (hit rate: " << (game.getLastCacheHitRate() * 100) << "%)" << std::endl;
                        std::cout << "Cache size: " << game.getCacheSize() << " entradas" << std::endl;
                        
                        aiThinking = false; // Reset para próximo turno
                    } else {
                        std::cout << "¡AI no pudo encontrar un movimiento válido!" << std::endl;
                        renderer.setState(GuiRenderer::GAME_OVER);
                        aiThinking = false;
                    }
                }
                
                // Renderizar estado actual del juego SIEMPRE
                renderer.render(game.getState(), game.getLastAIThinkingTime());
                break;
            }
            
            case GuiRenderer::GAME_OVER: {
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
    if (game.isGameOver()) {
        std::cout << "Estadísticas finales:" << std::endl;
        std::cout << "Cache size final: " << game.getCacheSize() << " entradas" << std::endl;
        std::cout << "Hash final: 0x" << std::hex << game.getState().getZobristHash() << std::dec << std::endl;
    }
    
    if (g_debugAnalyzer) {
        delete g_debugAnalyzer;
        g_debugAnalyzer = nullptr;
    }
    
    std::cout << "¡Gracias por jugar!" << std::endl;
    return 0;
}
