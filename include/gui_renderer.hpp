/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   gui_renderer.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/01 00:00:00 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/23 17:23:14 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef GUI_RENDERER_HPP
#define GUI_RENDERER_HPP

#include <SFML/Graphics.hpp>
#include "game_types.hpp"
#include <string>
#include <chrono>

class GuiRenderer {
public:
    enum AppState {
        MENU,
        PLAYING, 
        GAME_OVER
    };
    
    enum MenuOption {
        VS_AI,
        VS_HUMAN,
        QUIT,
        NONE
    };
    
private:
    // Core SFML
    sf::RenderWindow window;
    sf::Font font;
    sf::Event event;
    
    // App state
    AppState currentState;
    int selectedMenuOption;   // Para clicks reales
    int hoveredMenuOption;    // NUEVO: Para hover effects
    Move pendingMove; // Para capturar clicks del usuario
    bool moveReady;
    Move hoverPosition;  // NUEVO: Para hover en el tablero
    
    // Visual constants
    static constexpr int WINDOW_WIDTH = 1000;
    static constexpr int WINDOW_HEIGHT = 800;
    static constexpr int BOARD_SIZE_PX = 600;
    static constexpr int CELL_SIZE = BOARD_SIZE_PX / GameState::BOARD_SIZE;
    static constexpr int BOARD_OFFSET_X = 50;
    static constexpr int BOARD_OFFSET_Y = 100;
    
    // Colors
    sf::Color backgroundColor;
    sf::Color boardLineColor;
    sf::Color player1Color; // Humano
    sf::Color player2Color; // AI
    sf::Color hoverColor;
    
public:
    GuiRenderer();
    ~GuiRenderer();
    
    // Core functionality - reemplaza Display
    bool isWindowOpen() const;
    void processEvents(); // Procesa eventos sin bloquear
    void render(const GameState& state, int aiTimeMs = 0);
    
    // Game flow control
    MenuOption showMenuAndGetChoice();
    Move waitForUserMove(const GameState& state); // Reemplaza getUserMove()
    void showGameResult(int winner);
    
    // State management
    void setState(AppState newState) { currentState = newState; }
    AppState getState() const { return currentState; }
    bool hasUserMove() const { return moveReady; }
    Move getUserMove(); // Non-blocking, returns pending move
    void clearUserMove() { moveReady = false; }
	void refreshSelectedMenuOption() { selectedMenuOption = -1; }
    
private:
    // Internal rendering methods
    void renderMenu();
    void renderGame(const GameState& state, int aiTimeMs);
    void renderGameOver(const GameState& state);  // CORREGIDO: Recibir state para mostrar ganador
    
    // Board rendering
    void drawBoard();
    void drawPieces(const GameState& state);
    void drawHoverIndicator();
    
    // UI elements
    void drawButton(const std::string& text, int x, int y, int width, int height, 
                   bool highlighted = false);
    void drawText(const std::string& text, int x, int y, int size = 24, 
                 sf::Color color = sf::Color::White);
    void drawGameInfo(const GameState& state, int aiTimeMs);
    
    // Utility functions
    sf::Vector2i boardPositionToPixel(int boardX, int boardY) const;
    std::pair<int, int> pixelToBoardPosition(int x, int y) const;
    bool isPointInBoard(int x, int y) const;
    char getPieceSymbol(int piece) const;
    sf::Color getPieceColor(int piece) const;
    
    // Event handling helpers
    void handleMenuClick(int x, int y);
    void handleGameClick(int x, int y);
    void handleGameOverClick(int x, int y);  // NUEVO
    void handleMouseMove(int x, int y);
    
    // Member variables for hover position
};

#endif