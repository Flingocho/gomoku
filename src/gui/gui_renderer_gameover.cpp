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
    // Draw modern background
    // ============================================
    drawModernBackground();
    
    // ============================================
    // Render the board (left side)
    // ============================================
    drawBoard();
    drawPieces(state);
	int PIECE_RADIUS = (CELL_SIZE / 2) - 4;
    
    // ============================================
    // Highlight the winning line
    // ============================================
    if (!winningLine.empty()) {
        for (const Move& move : winningLine) {
            sf::Vector2i pos = boardPositionToPixel(move.x, move.y);
            
            // Bright golden halo
            sf::CircleShape highlight(PIECE_RADIUS + 8);
            highlight.setPosition(pos.x - PIECE_RADIUS - 8, pos.y - PIECE_RADIUS - 8);
            highlight.setFillColor(sf::Color::Transparent);
            highlight.setOutlineThickness(5);
            highlight.setOutlineColor(sf::Color(255, 215, 0, 255)); // Solid gold
            window.draw(highlight);
            
            // Second larger halo
            sf::CircleShape highlight2(PIECE_RADIUS + 14);
            highlight2.setPosition(pos.x - PIECE_RADIUS - 14, pos.y - PIECE_RADIUS - 14);
            highlight2.setFillColor(sf::Color::Transparent);
            highlight2.setOutlineThickness(3);
            highlight2.setOutlineColor(sf::Color(255, 255, 0, 150)); // Bright yellow
            window.draw(highlight2);
        }
        
        // Line connecting the pieces
        if (winningLine.size() >= 2) {
            sf::Vector2i start = boardPositionToPixel(winningLine.front().x, winningLine.front().y);
            sf::Vector2i end = boardPositionToPixel(winningLine.back().x, winningLine.back().y);
            
            // Calculate thickness based on angle
            float dx = end.x - start.x;
            float dy = end.y - start.y;
            float length = std::sqrt(dx*dx + dy*dy);
            float angle = std::atan2(dy, dx) * 180.0f / 3.14159f;
            
            // Rectangle as thick line
            sf::RectangleShape line(sf::Vector2f(length, 6));
            line.setPosition(start.x, start.y);
            line.setRotation(angle);
            line.setFillColor(sf::Color(255, 215, 0, 200));
            window.draw(line);
        }
    }
    
    // ============================================
    // Determine winner using storedWinner (set by showGameResult)
    // ============================================
    bool player1Wins = (storedWinner == GameState::PLAYER1);
    
    // ============================================
    // Show appropriate animation on top of the highlighter (if active)
    // ============================================
    if (showGameOverAnimation) {
        if (player1Wins && !winAnimationFrames.empty()) {
            // VICTORY - Show victory animation
            // Update animation frame (change every 50ms for 115 frames - smooth animation)
            if (winAnimationClock.getElapsedTime().asMilliseconds() > 50) {
                currentWinFrame = (currentWinFrame + 1) % winAnimationFrames.size();
                winAnimationSprite.setTexture(winAnimationFrames[currentWinFrame]);
                winAnimationClock.restart();
            }
            
            // Calculate position centered with the board
            sf::Vector2u frameSize = winAnimationFrames[currentWinFrame].getSize();
            float scale = winAnimationSprite.getScale().x;
            float frameWidth = frameSize.x * scale;
            float frameHeight = frameSize.y * scale;
            
            // Centered horizontally and vertically with the board
            float gifX = BOARD_OFFSET_X + (BOARD_SIZE_PX - frameWidth) / 2.0f;
            float gifY = BOARD_OFFSET_Y + (BOARD_SIZE_PX - frameHeight) / 2.0f;
            
            winAnimationSprite.setPosition(gifX, gifY);
            winAnimationSprite.setColor(sf::Color::White);
            
            window.draw(winAnimationSprite);
        } else if (!player1Wins && !defeatAnimationFrames.empty()) {
            // DEFEAT - Show defeat animation
            // Update animation frame (change every 50ms for smoother animation - 41 frames)
            if (defeatAnimationClock.getElapsedTime().asMilliseconds() > 50) {
                currentDefeatFrame = (currentDefeatFrame + 1) % defeatAnimationFrames.size();
                defeatAnimationSprite.setTexture(defeatAnimationFrames[currentDefeatFrame]);
                defeatAnimationClock.restart();
            }
            
            // Calculate position centered with the board
            sf::Vector2u frameSize = defeatAnimationFrames[currentDefeatFrame].getSize();
            float scale = defeatAnimationSprite.getScale().x;
            float frameWidth = frameSize.x * scale;
            float frameHeight = frameSize.y * scale;
            
            // Centered horizontally and vertically with the board
            float gifX = BOARD_OFFSET_X + (BOARD_SIZE_PX - frameWidth) / 2.0f;
            float gifY = BOARD_OFFSET_Y + (BOARD_SIZE_PX - frameHeight) / 2.0f;
            
            defeatAnimationSprite.setPosition(gifX, gifY);
            defeatAnimationSprite.setColor(sf::Color::White);
            
            window.draw(defeatAnimationSprite);
        }
        
        // ============================================
        // NEXT button to skip animation
        // ============================================
        nextButtonWidth = 120;
        nextButtonHeight = 45;
        nextButtonX = BOARD_OFFSET_X + (BOARD_SIZE_PX - nextButtonWidth) / 2;
        nextButtonY = BOARD_OFFSET_Y + BOARD_SIZE_PX - nextButtonHeight - 30;
        
        // Semi-transparent button background
        sf::RectangleShape buttonBg(sf::Vector2f(nextButtonWidth, nextButtonHeight));
        buttonBg.setPosition(nextButtonX, nextButtonY);
        buttonBg.setFillColor(sf::Color(40, 40, 40, 220));
        buttonBg.setOutlineThickness(2);
        
        // Hover effect
        bool nextHover = (hoveredMenuOption == 10);  // 10 is used for the NEXT button
        if (nextHover) {
            buttonBg.setOutlineColor(sf::Color(255, 215, 0));  // Gold on hover
            buttonBg.setFillColor(sf::Color(60, 60, 60, 240));
        } else {
            buttonBg.setOutlineColor(sf::Color(200, 200, 200));
        }
        window.draw(buttonBg);
        
        // Button text
        sf::Text nextText;
        nextText.setFont(font);
        nextText.setString("NEXT");
        nextText.setCharacterSize(20);
        nextText.setFillColor(nextHover ? sf::Color(255, 215, 0) : sf::Color::White);
        
        // Center text in the button
        sf::FloatRect textBounds = nextText.getLocalBounds();
        nextText.setPosition(
            nextButtonX + (nextButtonWidth - textBounds.width) / 2 - textBounds.left,
            nextButtonY + (nextButtonHeight - textBounds.height) / 2 - textBounds.top
        );
        window.draw(nextText);
    }
    
    // ============================================
    // Right side panel (info panel area)
    // ============================================
    int panelX = BOARD_OFFSET_X + BOARD_SIZE_PX + 30;
    int panelY = BOARD_OFFSET_Y;
    int panelWidth = 280;
    int panelHeight = BOARD_SIZE_PX; // Same height as the board
    
    // Dark overlay only on the right panel
    sf::RectangleShape overlay(sf::Vector2f(panelWidth + 20, panelHeight));
    overlay.setPosition(panelX - 10, panelY);
    overlay.setFillColor(sf::Color(0, 0, 0, 200));
    window.draw(overlay);
    
    // Panel background
    sf::RectangleShape panelBg(sf::Vector2f(panelWidth, panelHeight));
    panelBg.setPosition(panelX, panelY);
    panelBg.setFillColor(sf::Color(30, 30, 30, 250));
    panelBg.setOutlineThickness(4);
    panelBg.setOutlineColor(sf::Color(255, 215, 0)); // Gold border
    window.draw(panelBg);
    
    // ============================================
    // Panel content
    // ============================================
    int yOffset = panelY + 20;
    int centerX = panelX + panelWidth / 2;
    
    // GAME OVER title
    drawText("GAME OVER", centerX - 90, yOffset, 32, sf::Color(255, 100, 100));
    yOffset += 50;
    
    // ============================================
    // Determine win reason (player1Wins already determined above)
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
    
    // Main result
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
    
    // Win reason (centered)
    drawText(winReason, centerX - winReason.length() * 6, yOffset, 20, sf::Color::White);
    yOffset += 25;
    drawText(winDetails, centerX - winDetails.length() * 4, yOffset, 14, sf::Color(180, 180, 180));
    
    yOffset += 40;
    
    // Separator
    sf::RectangleShape separator(sf::Vector2f(panelWidth - 30, 2));
    separator.setPosition(panelX + 15, yOffset);
    separator.setFillColor(sf::Color(255, 215, 0));
    window.draw(separator);
    
    yOffset += 25;
    
    // ============================================
    // Final statistics
    // ============================================
    drawText("FINAL STATS", centerX - 60, yOffset, 18, sf::Color(255, 215, 0));
    yOffset += 30;
    
    // Compact stats
    drawText("Your Captures:", panelX + 15, yOffset, 14, sf::Color::White);
    drawText(std::to_string(state.captures[0]), panelX + panelWidth - 40, yOffset, 14, sf::Color(150, 255, 150));
    yOffset += 22;
    
    drawText("AI Captures:", panelX + 15, yOffset, 14, sf::Color::White);
    drawText(std::to_string(state.captures[1]), panelX + panelWidth - 40, yOffset, 14, sf::Color(255, 150, 150));
    yOffset += 22;
    
    drawText("Total Moves:", panelX + 15, yOffset, 14, sf::Color::White);
    drawText(std::to_string(state.turnCount), panelX + panelWidth - 40, yOffset, 14, sf::Color(200, 200, 200));
    
    yOffset += 35;
    
    // Another separator
    sf::RectangleShape separator2(sf::Vector2f(panelWidth - 30, 2));
    separator2.setPosition(panelX + 15, yOffset);
    separator2.setFillColor(sf::Color(100, 100, 100));
    window.draw(separator2);
    
    yOffset += 25;
    
    // ============================================
    // Buttons with hover effect
    // ============================================
    int buttonWidth = panelWidth - 30;
    int buttonHeight = 50;
    int buttonX = panelX + 15;
    
    // Save exact button positions for use in mouse events
    gameOverButtonsY = yOffset;
    gameOverButtonsPositionValid = true;
    
    // "New Game" button with hover
    bool newGameHover = (hoveredMenuOption == 0);
    drawButton("NEW GAME", buttonX, gameOverButtonsY, buttonWidth, buttonHeight, newGameHover);
    
    // "Main Menu" button with hover
    bool menuHover = (hoveredMenuOption == 1);
    drawButton("MAIN MENU", buttonX, gameOverButtonsY + buttonHeight + 15, buttonWidth, buttonHeight, menuHover);
    
    // Instruction (use calculated position)
    yOffset = gameOverButtonsY + (buttonHeight + 15) * 2 + 20;
    
    yOffset += buttonHeight + 20;
    
    // Instruction
    drawText("Press ESC for menu", centerX - 80, yOffset, 12, sf::Color(120, 120, 120));
}

// ===============================================
// MOUSE EVENT HANDLERS
// ===============================================

void GuiRenderer::handleMouseMove(int x, int y) {
    if (currentState == MENU) {
        int buttonWidth = 250;
        int buttonHeight = 45;
        int buttonX = WINDOW_WIDTH/2 - buttonWidth/2;
        int buttonSpacing = 55;
        
        // Reset hover state
        hoveredMenuOption = -1;
        
        // Check each button
        if (x >= buttonX && x <= buttonX + buttonWidth) {
            if (y >= 220 && y <= 220 + buttonHeight) {
                hoveredMenuOption = 0; // VS AI hover
            } else if (y >= 220 + buttonSpacing && y <= 220 + buttonSpacing + buttonHeight) {
                hoveredMenuOption = 1; // VS Human hover
            } else if (y >= 220 + buttonSpacing * 2 && y <= 220 + buttonSpacing * 2 + buttonHeight) {
                hoveredMenuOption = 2; // Colorblind hover
            } else if (y >= 220 + buttonSpacing * 3 && y <= 220 + buttonSpacing * 3 + buttonHeight) {
                hoveredMenuOption = 3; // Rust AI hover
            } else if (y >= 220 + buttonSpacing * 4 && y <= 220 + buttonSpacing * 4 + buttonHeight) {
                hoveredMenuOption = 4; // Options hover
            } else if (y >= 220 + buttonSpacing * 5 && y <= 220 + buttonSpacing * 5 + buttonHeight) {
                hoveredMenuOption = 5; // Quit hover
            }
        }
    }
    
    if (currentState == OPTIONS) {
        int buttonWidth = 300;
        int buttonHeight = 40;
        int buttonX = WINDOW_WIDTH/2 - buttonWidth/2;
        int smallBtnW = 60;
        int smallBtnH = 35;
        
        // Y positions must match renderOptions layout
        int musicToggleY   = 148;
        int musicVolBtnY   = 220;
        int soundToggleY   = 298;
        int soundVolBtnY   = 370;
        int debugToggleY   = 448;
        int backBtnY       = 520;
        
        hoveredMenuOption = -1;
        
        // Music toggle
        if (x >= buttonX && x <= buttonX + buttonWidth && 
            y >= musicToggleY && y <= musicToggleY + buttonHeight) {
            hoveredMenuOption = 0;
        }
        // Music volume down
        else if (x >= buttonX && x <= buttonX + smallBtnW && 
                 y >= musicVolBtnY && y <= musicVolBtnY + smallBtnH) {
            hoveredMenuOption = 1;
        }
        // Music volume up
        else if (x >= buttonX + buttonWidth - smallBtnW && x <= buttonX + buttonWidth && 
                 y >= musicVolBtnY && y <= musicVolBtnY + smallBtnH) {
            hoveredMenuOption = 2;
        }
        // Sound FX toggle
        else if (x >= buttonX && x <= buttonX + buttonWidth && 
                 y >= soundToggleY && y <= soundToggleY + buttonHeight) {
            hoveredMenuOption = 3;
        }
        // FX volume down
        else if (x >= buttonX && x <= buttonX + smallBtnW && 
                 y >= soundVolBtnY && y <= soundVolBtnY + smallBtnH) {
            hoveredMenuOption = 4;
        }
        // FX volume up
        else if (x >= buttonX + buttonWidth - smallBtnW && x <= buttonX + buttonWidth && 
                 y >= soundVolBtnY && y <= soundVolBtnY + smallBtnH) {
            hoveredMenuOption = 5;
        }
        // Debug toggle
        else if (x >= buttonX && x <= buttonX + buttonWidth && 
                 y >= debugToggleY && y <= debugToggleY + buttonHeight) {
            hoveredMenuOption = 6;
        }
        // Back button
        else if (x >= buttonX && x <= buttonX + buttonWidth && 
                 y >= backBtnY && y <= backBtnY + buttonHeight) {
            hoveredMenuOption = 7;
        }
    }
    
    if (currentState == PLAYING) {
        // Update hover position on the board
        if (isPointInBoard(x, y)) {
            auto [boardX, boardY] = pixelToBoardPosition(x, y);
            hoverPosition = Move(boardX, boardY);
        } else {
            hoverPosition = Move(-1, -1);
        }
    }

	if (currentState == GAME_OVER && gameOverButtonsPositionValid) {
        int panelX = BOARD_OFFSET_X + BOARD_SIZE_PX + 30;
        int panelWidth = 280;
        
        int buttonWidth = panelWidth - 30;
        int buttonHeight = 50;
        int buttonX = panelX + 15;
        
        int button1Y = gameOverButtonsY;
        int button2Y = gameOverButtonsY + buttonHeight + 15;
        
        hoveredMenuOption = -1;
        
        // Check NEXT button (only if animation is active)
        if (showGameOverAnimation && nextButtonWidth > 0) {
            if (x >= nextButtonX && x <= nextButtonX + nextButtonWidth && 
                y >= nextButtonY && y <= nextButtonY + nextButtonHeight) {
                hoveredMenuOption = 10;  // NEXT button
                return;
            }
        }
        
        // Check NEW GAME button
        if (x >= buttonX && x <= buttonX + buttonWidth && 
            y >= button1Y && y <= button1Y + buttonHeight) {
            hoveredMenuOption = 0;
        }
        // Check MAIN MENU button
        else if (x >= buttonX && x <= buttonX + buttonWidth && 
                 y >= button2Y && y <= button2Y + buttonHeight) {
            hoveredMenuOption = 1;
        }
    }
}

void GuiRenderer::handleGameOverClick(int x, int y) {
    if (!gameOverButtonsPositionValid) return;
    
    // Check click on NEXT button (skip animation)
    if (showGameOverAnimation && nextButtonWidth > 0) {
        if (x >= nextButtonX && x <= nextButtonX + nextButtonWidth && 
            y >= nextButtonY && y <= nextButtonY + nextButtonHeight) {
            audioManager.playSound("click_menu");
            showGameOverAnimation = false;  // Hide animation
            return;
        }
    }
    
    int panelX = BOARD_OFFSET_X + BOARD_SIZE_PX + 30;
    int panelWidth = 280;
    
    int buttonWidth = panelWidth - 30;
    int buttonHeight = 50;
    int buttonX = panelX + 15;
    
    int button1Y = gameOverButtonsY;
    int button2Y = gameOverButtonsY + buttonHeight + 15;
    
    // Click on "NEW GAME"
    if (x >= buttonX && x <= buttonX + buttonWidth && 
        y >= button1Y && y <= button1Y + buttonHeight) {
        audioManager.playSound("click_menu");
        selectedMenuOption = 0;
        return;
    }
    
    // Click on "MAIN MENU"
    if (x >= buttonX && x <= buttonX + buttonWidth && 
        y >= button2Y && y <= button2Y + buttonHeight) {
        audioManager.playSound("click_menu");
        clearSuggestion();
        clearInvalidMoveError();
        setWinningLine(std::vector<Move>());
        setState(MENU);
        selectedMenuOption = -1;
        return;
    }
}

// ===============================================
// UTILITY FUNCTIONS
// ===============================================

void GuiRenderer::showGameResult(int winner) {
    storedWinner = winner;
    setState(GAME_OVER);
}
