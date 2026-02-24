// ============================================================================
// GUI Renderer - Board Module
// Contains: Board drawing, pieces drawing, coordinate conversion
// ============================================================================

#include "../../include/gui/gui_renderer.hpp"
#include <iostream>
#include <sstream>
#include <cmath>

// ============================================================================
// BOARD DRAWING
// ============================================================================

void GuiRenderer::drawBoard() {
    // 1. Board shadow (3D effect)
    sf::RectangleShape boardShadow(sf::Vector2f(BOARD_SIZE_PX + 8, BOARD_SIZE_PX + 8));
    boardShadow.setPosition(BOARD_OFFSET_X + 4, BOARD_OFFSET_Y + 4);
    boardShadow.setFillColor(sf::Color(0, 0, 0, 80));
    window.draw(boardShadow);
    
    // 2. Main board background
    sf::RectangleShape boardBg(sf::Vector2f(BOARD_SIZE_PX, BOARD_SIZE_PX));
    boardBg.setPosition(BOARD_OFFSET_X, BOARD_OFFSET_Y);
    boardBg.setFillColor(sf::Color(245, 222, 179)); // Wheat
    window.draw(boardBg);
    
    // 3. Beveled border (3D effect)
    sf::RectangleShape topBevel(sf::Vector2f(BOARD_SIZE_PX, 3));
    topBevel.setPosition(BOARD_OFFSET_X, BOARD_OFFSET_Y);
    topBevel.setFillColor(sf::Color(255, 248, 220));
    window.draw(topBevel);
    
    sf::RectangleShape leftBevel(sf::Vector2f(3, BOARD_SIZE_PX));
    leftBevel.setPosition(BOARD_OFFSET_X, BOARD_OFFSET_Y);
    leftBevel.setFillColor(sf::Color(255, 248, 220));
    window.draw(leftBevel);
    
    sf::RectangleShape bottomBevel(sf::Vector2f(BOARD_SIZE_PX, 3));
    bottomBevel.setPosition(BOARD_OFFSET_X, BOARD_OFFSET_Y + BOARD_SIZE_PX - 3);
    bottomBevel.setFillColor(sf::Color(160, 130, 98));
    window.draw(bottomBevel);
    
    sf::RectangleShape rightBevel(sf::Vector2f(3, BOARD_SIZE_PX));
    rightBevel.setPosition(BOARD_OFFSET_X + BOARD_SIZE_PX - 3, BOARD_OFFSET_Y);
    rightBevel.setFillColor(sf::Color(160, 130, 98));
    window.draw(rightBevel);
    
    // 4. Individual cells with inset effect
    for (int i = 0; i < GameState::BOARD_SIZE; i++) {
        for (int j = 0; j < GameState::BOARD_SIZE; j++) {
            int cellX = BOARD_OFFSET_X + j * CELL_SIZE + 2;
            int cellY = BOARD_OFFSET_Y + i * CELL_SIZE + 2;
            int cellSize = CELL_SIZE - 4;
            
            sf::RectangleShape cell(sf::Vector2f(cellSize, cellSize));
            cell.setPosition(cellX, cellY);
            cell.setFillColor(sf::Color(210, 180, 140));
            window.draw(cell);
            
            // Inset effect - inner shadow
            sf::RectangleShape innerShadowTop(sf::Vector2f(cellSize, 1));
            innerShadowTop.setPosition(cellX, cellY);
            innerShadowTop.setFillColor(sf::Color(160, 130, 98));
            window.draw(innerShadowTop);
            
            sf::RectangleShape innerShadowLeft(sf::Vector2f(1, cellSize));
            innerShadowLeft.setPosition(cellX, cellY);
            innerShadowLeft.setFillColor(sf::Color(160, 130, 98));
            window.draw(innerShadowLeft);
            
            // Bottom-right highlight
            sf::RectangleShape innerHighlightBottom(sf::Vector2f(cellSize, 1));
            innerHighlightBottom.setPosition(cellX, cellY + cellSize - 1);
            innerHighlightBottom.setFillColor(sf::Color(235, 210, 175));
            window.draw(innerHighlightBottom);
            
            sf::RectangleShape innerHighlightRight(sf::Vector2f(1, cellSize));
            innerHighlightRight.setPosition(cellX + cellSize - 1, cellY);
            innerHighlightRight.setFillColor(sf::Color(235, 210, 175));
            window.draw(innerHighlightRight);
        }
    }
    
    // 5. Board coordinates (A-S horizontal, 1-19 vertical)
    for (int j = 0; j < GameState::BOARD_SIZE; j++) {
        char letter = 'A' + j;
        std::string coordText(1, letter);
        
        // Top coordinate
        drawText(coordText, 
                BOARD_OFFSET_X + j * CELL_SIZE + CELL_SIZE/2 - 6, 
                BOARD_OFFSET_Y - 25, 
                16, sf::Color(220, 220, 220));
        
        // Bottom coordinate
        drawText(coordText, 
                BOARD_OFFSET_X + j * CELL_SIZE + CELL_SIZE/2 - 6, 
                BOARD_OFFSET_Y + BOARD_SIZE_PX + 8, 
                16, sf::Color(220, 220, 220));
    }
    
    // Vertical numbers (1-19)
    for (int i = 0; i < GameState::BOARD_SIZE; i++) {
        std::string coordText = std::to_string(i + 1);
        
        // Left coordinate
        drawText(coordText, 
                BOARD_OFFSET_X - 25, 
                BOARD_OFFSET_Y + i * CELL_SIZE + CELL_SIZE/2 - 8, 
                16, sf::Color(220, 220, 220));
        
        // Right coordinate
        drawText(coordText, 
                BOARD_OFFSET_X + BOARD_SIZE_PX + 8, 
                BOARD_OFFSET_Y + i * CELL_SIZE + CELL_SIZE/2 - 8, 
                16, sf::Color(220, 220, 220));
    }
}

// ============================================================================
// PIECES DRAWING
// ============================================================================

void GuiRenderer::drawPieces(const GameState& state) {
    for (int i = 0; i < GameState::BOARD_SIZE; i++) {
        for (int j = 0; j < GameState::BOARD_SIZE; j++) {
            int piece = state.getPiece(i, j);
            if (piece != GameState::EMPTY) {
                sf::Vector2i pos = boardPositionToPixel(i, j);
                
                int pieceRadius = (CELL_SIZE / 2) - 4;
                sf::CircleShape pieceMain(pieceRadius);
                pieceMain.setPosition(pos.x - pieceRadius, pos.y - pieceRadius);
                
                // In colorblind mode, all pieces share the same color, EXCEPT when the game is over
                if (isColorblindMode && currentState != GAME_OVER) {
                    pieceMain.setFillColor(sf::Color(128, 128, 128));
                } else {
                    pieceMain.setFillColor(getPieceColor(piece));
                }
                
                // Piece shadow
                sf::CircleShape pieceShadow(pieceRadius);
                pieceShadow.setPosition(pos.x - pieceRadius + 2, pos.y - pieceRadius + 2);
                pieceShadow.setFillColor(sf::Color(0, 0, 0, 100));
                window.draw(pieceShadow);
                
                // Main piece
                window.draw(pieceMain);
                
                // Top highlight for 3D effect
                sf::CircleShape pieceHighlight(pieceRadius - 3);
                pieceHighlight.setPosition(pos.x - pieceRadius + 3, pos.y - pieceRadius + 3);
                
                if (isColorblindMode && currentState != GAME_OVER) {
                    pieceHighlight.setFillColor(sf::Color(160, 160, 160));
                } else if (piece == GameState::PLAYER1) {
                    pieceHighlight.setFillColor(sf::Color(135, 206, 250));
                } else {
                    pieceHighlight.setFillColor(sf::Color(255, 99, 71));
                }
                
                window.draw(pieceHighlight);
                
                // Small shine
                sf::CircleShape pieceShine(3);
                pieceShine.setPosition(pos.x - pieceRadius + 5, pos.y - pieceRadius + 5);
                pieceShine.setFillColor(sf::Color(255, 255, 255, 180));
                window.draw(pieceShine);
                
                // Highlight ring for AI's last move
                if (piece == GameState::PLAYER2 && lastAiMove.x == i && lastAiMove.y == j) {
                    sf::CircleShape lastMoveRing(pieceRadius + 4);
                    lastMoveRing.setPosition(pos.x - pieceRadius - 4, pos.y - pieceRadius - 4);
                    lastMoveRing.setFillColor(sf::Color::Transparent);
                    lastMoveRing.setOutlineThickness(2);
                    
                    static sf::Clock pulseClock;
                    float pulseTime = pulseClock.getElapsedTime().asSeconds();
                    float alpha = (sin(pulseTime * 3.0f) + 1.0f) * 0.3f + 0.4f;
                    lastMoveRing.setOutlineColor(sf::Color(255, 255, 0, (sf::Uint8)(alpha * 255)));
                    
                    window.draw(lastMoveRing);
                }
            }
        }
    }
}

// ============================================================================
// COORDINATE CONVERSION
// ============================================================================

sf::Vector2i GuiRenderer::boardPositionToPixel(int boardX, int boardY) const {
    int pixelX = BOARD_OFFSET_X + boardY * CELL_SIZE + CELL_SIZE / 2;
    int pixelY = BOARD_OFFSET_Y + boardX * CELL_SIZE + CELL_SIZE / 2;
    return sf::Vector2i(pixelX, pixelY);
}

std::pair<int, int> GuiRenderer::pixelToBoardPosition(int x, int y) const {
    int boardX = (y - BOARD_OFFSET_Y) / CELL_SIZE;
    int boardY = (x - BOARD_OFFSET_X) / CELL_SIZE;
    return {boardX, boardY};
}

bool GuiRenderer::isPointInBoard(int x, int y) const {
    return x >= BOARD_OFFSET_X && x < BOARD_OFFSET_X + BOARD_SIZE_PX &&
           y >= BOARD_OFFSET_Y && y < BOARD_OFFSET_Y + BOARD_SIZE_PX;
}

char GuiRenderer::getPieceSymbol(int piece) const {
    switch (piece) {
        case GameState::PLAYER1: return 'O';
        case GameState::PLAYER2: return 'X';
        default: return '.';
    }
}

sf::Color GuiRenderer::getPieceColor(int piece) const {
    return piece == GameState::PLAYER1 ? player1Color : player2Color;
}
