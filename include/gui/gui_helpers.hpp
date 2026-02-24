#ifndef GUI_HELPERS_HPP
#define GUI_HELPERS_HPP

#include <SFML/Graphics.hpp>
#include <string>

namespace GuiHelpers {
    // Layout constants
    constexpr int WINDOW_WIDTH = 1000;
    constexpr int WINDOW_HEIGHT = 800;
    constexpr int BOARD_SIZE_PX = 760;
    constexpr int BOARD_OFFSET_X = 20;
    constexpr int BOARD_OFFSET_Y = 20;
    constexpr int CELL_SIZE = 40;
    constexpr int PIECE_RADIUS = 15;
    
    // Coordinate conversion
    sf::Vector2i boardPositionToPixel(int boardX, int boardY);
    bool isPointInBoard(int pixelX, int pixelY, int& outBoardX, int& outBoardY);
    
    // Helpers de UI
    void drawButton(sf::RenderWindow& window, const sf::Font& font,
                   const std::string& text, int x, int y, int width, int height,
                   bool isHovered, const sf::Clock& animClock);
    
    void drawText(sf::RenderWindow& window, const sf::Font& font,
                 const std::string& text, int x, int y, int size, sf::Color color);
    
    // Helpers de color
    sf::Color getPieceColor(int piece, bool isColorblindMode,
                           const sf::Color& player1Color,
                           const sf::Color& player2Color);
}

#endif // GUI_HELPERS_HPP
