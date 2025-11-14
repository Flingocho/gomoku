#ifndef DISPLAY_HPP
#define DISPLAY_HPP

#include "game_types.hpp"
#include <utility>
#include <string>

class Display {
public:
    static void printBoard(const GameState& state);
    static void printBoardtoFile(const GameState& state, std::ofstream& file);
    static void printGameInfo(const GameState& state, int aiTimeMs = 0);
    static std::pair<int, int> getUserMove();
    static void printWelcome();
    static void printWinner(int player);
    static void clearScreen();
	static char getPieceChar(int piece);
    static std::string getPieceColor(int piece);
    static std::pair<int, int> parseCoordinate(const std::string& input);
    
private:
    
};

#endif