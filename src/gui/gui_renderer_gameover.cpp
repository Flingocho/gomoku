// ===============================================
// GUI Renderer - Game Over Module
// ===============================================
// Handles: Game over screen rendering and interaction
// Dependencies: Core module, board module, UI module
// ===============================================

#include "../../include/gui/gui_renderer.hpp"
#include <sstream>
#include <cmath>
#include <iostream>

// ===============================================
// GAME OVER RENDERING
// ===============================================

void GuiRenderer::renderGameOver(const GameState& state) {
    // ============================================
    // PASO 0: Fondo moderno espectacular
    // ============================================
    drawModernBackground();
    
    // ============================================
    // PASO 1: Renderizar el tablero normal (izquierda)
    // ============================================
    drawBoard();
    drawPieces(state);
	int PIECE_RADIUS = (CELL_SIZE / 2) - 4;
    
    // ============================================
    // PASO 2: Resaltar la línea ganadora
    // ============================================
    if (!winningLine.empty()) {
        for (const Move& move : winningLine) {
            sf::Vector2i pos = boardPositionToPixel(move.x, move.y);
            
            // Halo brillante dorado
            sf::CircleShape highlight(PIECE_RADIUS + 8);
            highlight.setPosition(pos.x - PIECE_RADIUS - 8, pos.y - PIECE_RADIUS - 8);
            highlight.setFillColor(sf::Color::Transparent);
            highlight.setOutlineThickness(5);
            highlight.setOutlineColor(sf::Color(255, 215, 0, 255)); // Dorado sólido
            window.draw(highlight);
            
            // Segundo halo más grande
            sf::CircleShape highlight2(PIECE_RADIUS + 14);
            highlight2.setPosition(pos.x - PIECE_RADIUS - 14, pos.y - PIECE_RADIUS - 14);
            highlight2.setFillColor(sf::Color::Transparent);
            highlight2.setOutlineThickness(3);
            highlight2.setOutlineColor(sf::Color(255, 255, 0, 150)); // Amarillo brillante
            window.draw(highlight2);
        }
        
        // Línea conectando las fichas
        if (winningLine.size() >= 2) {
            sf::Vector2i start = boardPositionToPixel(winningLine.front().x, winningLine.front().y);
            sf::Vector2i end = boardPositionToPixel(winningLine.back().x, winningLine.back().y);
            
            // Calcular grosor basado en ángulo
            float dx = end.x - start.x;
            float dy = end.y - start.y;
            float length = std::sqrt(dx*dx + dy*dy);
            float angle = std::atan2(dy, dx) * 180.0f / 3.14159f;
            
            // Rectángulo como línea gruesa
            sf::RectangleShape line(sf::Vector2f(length, 6));
            line.setPosition(start.x, start.y);
            line.setRotation(angle);
            line.setFillColor(sf::Color(255, 215, 0, 200));
            window.draw(line);
        }
    }
    
    // ============================================
    // PASO 2.5: Determinar ganador ANTES de mostrar animación
    // ============================================
    bool player1Wins = false;
    
    if (state.captures[0] >= 10) {
        player1Wins = true;
    } else if (state.captures[1] >= 10) {
        player1Wins = false;
    } else {
        if (state.currentPlayer == GameState::PLAYER1) {
            player1Wins = false; // AI ganó
        } else {
            player1Wins = true; // Jugador ganó
        }
    }
    
    // ============================================
    // PASO 2.6: Mostrar animación apropiada ENCIMA del highlighter
    // ============================================
    if (player1Wins && !winAnimationFrames.empty()) {
        // VICTORIA - Mostrar animación de victoria
        // Actualizar frame de animación (cambiar cada 50ms para 115 frames - animación fluida)
        if (winAnimationClock.getElapsedTime().asMilliseconds() > 50) {
            currentWinFrame = (currentWinFrame + 1) % winAnimationFrames.size();
            winAnimationSprite.setTexture(winAnimationFrames[currentWinFrame]);
            winAnimationClock.restart();
        }
        
        // Calcular posición centrada con el tablero
        sf::Vector2u frameSize = winAnimationFrames[currentWinFrame].getSize();
        float scale = winAnimationSprite.getScale().x;
        float frameWidth = frameSize.x * scale;
        float frameHeight = frameSize.y * scale;
        
        // Posición: centrado horizontalmente y verticalmente con el tablero
        float gifX = BOARD_OFFSET_X + (BOARD_SIZE_PX - frameWidth) / 2.0f;
        float gifY = BOARD_OFFSET_Y + (BOARD_SIZE_PX - frameHeight) / 2.0f;
        
        winAnimationSprite.setPosition(gifX, gifY);
        winAnimationSprite.setColor(sf::Color::White);
        
        window.draw(winAnimationSprite);
    } else if (!player1Wins && !defeatAnimationFrames.empty()) {
        // DERROTA - Mostrar animación de derrota
        // Actualizar frame de animación (cambiar cada 50ms para animación más fluida - 41 frames)
        if (defeatAnimationClock.getElapsedTime().asMilliseconds() > 50) {
            currentDefeatFrame = (currentDefeatFrame + 1) % defeatAnimationFrames.size();
            defeatAnimationSprite.setTexture(defeatAnimationFrames[currentDefeatFrame]);
            defeatAnimationClock.restart();
        }
        
        // Calcular posición centrada con el tablero
        sf::Vector2u frameSize = defeatAnimationFrames[currentDefeatFrame].getSize();
        float scale = defeatAnimationSprite.getScale().x;
        float frameWidth = frameSize.x * scale;
        float frameHeight = frameSize.y * scale;
        
        // Posición: centrado horizontalmente y verticalmente con el tablero
        float gifX = BOARD_OFFSET_X + (BOARD_SIZE_PX - frameWidth) / 2.0f;
        float gifY = BOARD_OFFSET_Y + (BOARD_SIZE_PX - frameHeight) / 2.0f;
        
        defeatAnimationSprite.setPosition(gifX, gifY);
        defeatAnimationSprite.setColor(sf::Color::White);
        
        window.draw(defeatAnimationSprite);
    }
    
    // ============================================
    // PASO 3: Panel lateral derecho (donde va el info panel)
    // ============================================
    int panelX = BOARD_OFFSET_X + BOARD_SIZE_PX + 30;
    int panelY = BOARD_OFFSET_Y;
    int panelWidth = 280;
    int panelHeight = BOARD_SIZE_PX; // Mismo alto que el tablero
    
    // Overlay oscuro solo en el panel derecho
    sf::RectangleShape overlay(sf::Vector2f(panelWidth + 20, panelHeight));
    overlay.setPosition(panelX - 10, panelY);
    overlay.setFillColor(sf::Color(0, 0, 0, 200));
    window.draw(overlay);
    
    // Fondo del panel
    sf::RectangleShape panelBg(sf::Vector2f(panelWidth, panelHeight));
    panelBg.setPosition(panelX, panelY);
    panelBg.setFillColor(sf::Color(30, 30, 30, 250));
    panelBg.setOutlineThickness(4);
    panelBg.setOutlineColor(sf::Color(255, 215, 0)); // Borde dorado
    window.draw(panelBg);
    
    // ============================================
    // PASO 4: Contenido del panel
    // ============================================
    int yOffset = panelY + 20;
    int centerX = panelX + panelWidth / 2;
    
    // Título GAME OVER
    drawText("GAME OVER", centerX - 90, yOffset, 32, sf::Color(255, 100, 100));
    yOffset += 50;
    
    // ============================================
    // PASO 5: Determinar razón de victoria (player1Wins ya determinado arriba)
    // ============================================
    std::string winReason = "";
    std::string winDetails = "";
    
    if (state.captures[0] >= 10) {
        winReason = "BY CAPTURES";
        winDetails = "10 pairs captured";
    } else if (state.captures[1] >= 10) {
        winReason = "BY CAPTURES";
        winDetails = "AI got 10 pairs";
    } else {
        if (state.currentPlayer == GameState::PLAYER1) {
            winReason = "BY ALIGNMENT";
            if (!winningLine.empty()) {
                char col = 'A' + winningLine[0].y;
                int row = winningLine[0].x + 1;
                winDetails = "Line at " + std::string(1, col) + std::to_string(row);
            } else {
                winDetails = "5 in a row";
            }
        } else {
            winReason = "BY ALIGNMENT";
            if (!winningLine.empty()) {
                char col = 'A' + winningLine[0].y;
                int row = winningLine[0].x + 1;
                winDetails = "Line at " + std::string(1, col) + std::to_string(row);
            } else {
                winDetails = "5 in a row";
            }
        }
    }
    
    // Resultado principal
    if (player1Wins) {
        drawText("YOU WIN!", centerX - 70, yOffset, 28, sf::Color(100, 255, 100));
        yOffset += 35;
        drawText("Victory!", centerX - 40, yOffset, 18, sf::Color(200, 255, 200));
    } else {
        drawText("AI WINS", centerX - 55, yOffset, 28, sf::Color(255, 80, 80));
        yOffset += 35;
        drawText("Defeat", centerX - 35, yOffset, 18, sf::Color(255, 150, 150));
    }
    
    yOffset += 35;
    
    // Razón de victoria (centrada)
    drawText(winReason, centerX - winReason.length() * 6, yOffset, 20, sf::Color::White);
    yOffset += 25;
    drawText(winDetails, centerX - winDetails.length() * 4, yOffset, 14, sf::Color(180, 180, 180));
    
    yOffset += 40;
    
    // Separador
    sf::RectangleShape separator(sf::Vector2f(panelWidth - 30, 2));
    separator.setPosition(panelX + 15, yOffset);
    separator.setFillColor(sf::Color(255, 215, 0));
    window.draw(separator);
    
    yOffset += 25;
    
    // ============================================
    // PASO 6: Estadísticas finales
    // ============================================
    drawText("FINAL STATS", centerX - 60, yOffset, 18, sf::Color(255, 215, 0));
    yOffset += 30;
    
    // Stats compactas
    drawText("Your Captures:", panelX + 15, yOffset, 14, sf::Color::White);
    drawText(std::to_string(state.captures[0]), panelX + panelWidth - 40, yOffset, 14, sf::Color(150, 255, 150));
    yOffset += 22;
    
    drawText("AI Captures:", panelX + 15, yOffset, 14, sf::Color::White);
    drawText(std::to_string(state.captures[1]), panelX + panelWidth - 40, yOffset, 14, sf::Color(255, 150, 150));
    yOffset += 22;
    
    drawText("Total Moves:", panelX + 15, yOffset, 14, sf::Color::White);
    drawText(std::to_string(state.turnCount), panelX + panelWidth - 40, yOffset, 14, sf::Color(200, 200, 200));
    
    yOffset += 35;
    
    // Otro separador
    sf::RectangleShape separator2(sf::Vector2f(panelWidth - 30, 2));
    separator2.setPosition(panelX + 15, yOffset);
    separator2.setFillColor(sf::Color(100, 100, 100));
    window.draw(separator2);
    
    yOffset += 25;
    
    // ============================================
    // PASO 7: Botones con hover effect
    // ============================================
    int buttonWidth = panelWidth - 30;
    int buttonHeight = 50;
    int buttonX = panelX + 15;
    
    // GUARDAR posición exacta de botones para usar en mouse events
    gameOverButtonsY = yOffset;
    gameOverButtonsPositionValid = true;
    
    // Botón "New Game" con hover
    bool newGameHover = (hoveredMenuOption == 0);
    drawButton("NEW GAME", buttonX, gameOverButtonsY, buttonWidth, buttonHeight, newGameHover);
    
    // Botón "Main Menu" con hover
    bool menuHover = (hoveredMenuOption == 1);
    drawButton("MAIN MENU", buttonX, gameOverButtonsY + buttonHeight + 15, buttonWidth, buttonHeight, menuHover);
    
    // Instrucción (usar posición calculada)
    yOffset = gameOverButtonsY + (buttonHeight + 15) * 2 + 20;
    
    yOffset += buttonHeight + 20;
    
    // Instrucción
    drawText("Press ESC for menu", centerX - 80, yOffset, 12, sf::Color(120, 120, 120));
}

// ===============================================
// MOUSE EVENT HANDLERS
// ===============================================

void GuiRenderer::handleMouseMove(int x, int y) {
    if (currentState == MENU) {
        // CORREGIDO: Usar hoveredMenuOption para efectos visuales
        int buttonWidth = 250;
        int buttonHeight = 50;  // ACTUALIZADO
        int buttonX = WINDOW_WIDTH/2 - buttonWidth/2;
        int buttonSpacing = 65;  // ACTUALIZADO
        
        // Reset hover state
        int previousHover = hoveredMenuOption;
        hoveredMenuOption = -1;
        
        // Check each button
        if (x >= buttonX && x <= buttonX + buttonWidth) {
            if (y >= 230 && y <= 230 + buttonHeight) {
                hoveredMenuOption = 0; // VS AI hover
            } else if (y >= 230 + buttonSpacing && y <= 230 + buttonSpacing + buttonHeight) {
                hoveredMenuOption = 1; // VS Human hover
            } else if (y >= 230 + buttonSpacing * 2 && y <= 230 + buttonSpacing * 2 + buttonHeight) {
                hoveredMenuOption = 2; // Colorblind hover
            } else if (y >= 230 + buttonSpacing * 3 && y <= 230 + buttonSpacing * 3 + buttonHeight) {
                hoveredMenuOption = 3; // Rust AI hover
            } else if (y >= 230 + buttonSpacing * 4 && y <= 230 + buttonSpacing * 4 + buttonHeight) {
                hoveredMenuOption = 4; // Quit hover
            }
        }
        
        // Solo logear si cambió el hover (evitar spam)
        if (previousHover != hoveredMenuOption && hoveredMenuOption != -1) {
            std::string options[] = {"VS AI", "VS Human", "Colorblind Mode", "Rust AI", "Quit"};
            std::cout << "Hover: " << options[hoveredMenuOption] << std::endl;
        }
    }
    
    if (currentState == PLAYING) {
        // NUEVO: Actualizar hover position en el tablero
        if (isPointInBoard(x, y)) {
            auto [boardX, boardY] = pixelToBoardPosition(x, y);
            hoverPosition = Move(boardX, boardY);
        } else {
            hoverPosition = Move(-1, -1); // Fuera del tablero
        }
    }

	if (currentState == GAME_OVER && gameOverButtonsPositionValid) {
        int panelX = BOARD_OFFSET_X + BOARD_SIZE_PX + 30;
        int panelWidth = 280;
        
        int buttonWidth = panelWidth - 30;
        int buttonHeight = 50;
        int buttonX = panelX + 15;
        
        // Usar posición exacta guardada desde renderGameOver
        int button1Y = gameOverButtonsY;
        int button2Y = gameOverButtonsY + buttonHeight + 15;
        
        // Reset hover
        int previousHover = hoveredMenuOption;
        hoveredMenuOption = -1;
        
        // Check botón NEW GAME
        if (x >= buttonX && x <= buttonX + buttonWidth && 
            y >= button1Y && y <= button1Y + buttonHeight) {
            hoveredMenuOption = 0;
        }
        // Check botón MAIN MENU
        else if (x >= buttonX && x <= buttonX + buttonWidth && 
                 y >= button2Y && y <= button2Y + buttonHeight) {
            hoveredMenuOption = 1;
        }
        
        // Debug hover (solo si cambió)
        if (previousHover != hoveredMenuOption && hoveredMenuOption != -1) {
            std::cout << "Game Over Hover: " << (hoveredMenuOption == 0 ? "New Game" : "Main Menu") << std::endl;
        }
    }
}

void GuiRenderer::handleGameOverClick(int x, int y) {
    if (!gameOverButtonsPositionValid) return; // No hay posiciones válidas aún
    
    int panelX = BOARD_OFFSET_X + BOARD_SIZE_PX + 30;
    int panelWidth = 280;
    
    int buttonWidth = panelWidth - 30;
    int buttonHeight = 50;
    int buttonX = panelX + 15;
    
    // Usar posición exacta guardada desde renderGameOver
    int button1Y = gameOverButtonsY;
    int button2Y = gameOverButtonsY + buttonHeight + 15;
    
    // Click en "NEW GAME"
    if (x >= buttonX && x <= buttonX + buttonWidth && 
        y >= button1Y && y <= button1Y + buttonHeight) {
        audioManager.playSound("click_menu"); // Play click sound
        selectedMenuOption = 0;
        std::cout << "✓ Seleccionado: Nuevo Juego" << std::endl;
        return;
    }
    
    // Click en "MAIN MENU"
    if (x >= buttonX && x <= buttonX + buttonWidth && 
        y >= button2Y && y <= button2Y + buttonHeight) {
        audioManager.playSound("click_menu");
        // Limpiar todo el estado visual al volver al menú
        clearSuggestion();
        clearInvalidMoveError();
        setWinningLine(std::vector<Move>());
        setState(MENU);
        selectedMenuOption = -1;
        std::cout << "✓ Returned to menu from game over - all state cleaned" << std::endl;
        return;
    }
    
    std::cout << "✗ Click fuera de los botones" << std::endl;
}

// ===============================================
// UTILITY FUNCTIONS
// ===============================================

void GuiRenderer::showGameResult(int) {
    setState(GAME_OVER);
}
