#include "../../include/core/display.hpp"
#include <iostream>
#include <iomanip>
#include <cctype>
#include <fstream>

// Códigos de color ANSI
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define BLUE    "\033[34m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"

void Display::clearScreen() {
    #ifdef _WIN32
        system("cls");
    #else
        if (!system("clear"))
			return;
    #endif
}

char Display::getPieceChar(int piece) {
    switch (piece) {
        case GameState::EMPTY: return '.';
        case GameState::PLAYER1: return 'O';  // Humano
        case GameState::PLAYER2: return 'X';  // AI
        default: return '?';
    }
}

std::string Display::getPieceColor(int piece) {
    switch (piece) {
        case GameState::PLAYER1: return BLUE;   // Humano en azul
        case GameState::PLAYER2: return RED;    // AI en rojo
        default: return RESET;
    }
}

void Display::printBoard(const GameState& state) {
    std::cout << "\n  ";
    
    // Imprimir coordenadas superiores (A-S)
    for (int j = 0; j < GameState::BOARD_SIZE; j++) {
        std::cout << " " << char('A' + j);
    }
    std::cout << "\n";
    
    // Imprimir filas
    for (int i = 0; i < GameState::BOARD_SIZE; i++) {
        // Número de fila (1-19)
        std::cout << std::setw(2) << (i + 1) << " ";
        
        for (int j = 0; j < GameState::BOARD_SIZE; j++) {
            int piece = state.getPiece(i, j);
            std::cout << getPieceColor(piece) << getPieceChar(piece) << RESET << " ";
        }
        
        std::cout << std::setw(2) << (i + 1) << "\n";
    }
    
    // Coordenadas inferiores
    std::cout << "  ";
    for (int j = 0; j < GameState::BOARD_SIZE; j++) {
        std::cout << " " << char('A' + j);
    }
    std::cout << "\n";
}

void Display::printGameInfo(const GameState& state, int aiTimeMs) {
    std::cout << "\n" << GREEN << "=== GOMOKU ===" << RESET << "\n";
    std::cout << "Captures: " << BLUE << "You: " << state.captures[0] << RESET;
    std::cout << "  " << RED << "AI: " << state.captures[1] << RESET;
    
    // Mostrar si alguien está cerca de ganar por capturas
    if (state.captures[0] >= 8) {
        std::cout << " " << BLUE << "(2 more to win!)" << RESET;
    }
    if (state.captures[1] >= 8) {
        std::cout << " " << RED << "(2 more to win!)" << RESET;
    }
    std::cout << "\n";
    
    if (aiTimeMs > 0) {
        std::cout << "AI thinking time: " << YELLOW 
                  << (aiTimeMs / 1000.0) << "s" << RESET << "\n";
    }
    std::cout << "\n";
}

std::pair<int, int> Display::getUserMove() {
    std::string input;
    std::cout << "Your move (e.g., 'J10' or 'quit'): ";
    std::cin >> input;
    
    if (input == "quit" || input == "q") {
        return {-1, -1};  // Señal de quit
    }
    
    return parseCoordinate(input);
}

std::pair<int, int> Display::parseCoordinate(const std::string& input) {
    if (input.length() < 2) return {-2, -2};  // Error
    
    // Convertir letra a columna (A=0, B=1, etc.)
    char colChar = std::toupper(input[0]);
    if (colChar < 'A' || colChar > 'S') return {-2, -2};
    int col = colChar - 'A';
    
    // Convertir número a fila
    std::string rowStr = input.substr(1);
    try {
        int row = std::stoi(rowStr) - 1;  // 1-based to 0-based
        if (row < 0 || row >= GameState::BOARD_SIZE) return {-2, -2};
        return {row, col};
    } catch (...) {
        return {-2, -2};
    }
}

void Display::printWelcome() {
    //clearScreen();
    std::cout << GREEN << "=================================\n";
    std::cout << "         GOMOKU AI GAME\n";
    std::cout << "=================================" << RESET << "\n\n";
    std::cout << "Rules:\n";
    std::cout << "- Get 5 in a row to win\n";
    std::cout << "- Capture 10 enemy pieces to win\n";
    std::cout << "- No double free-threes allowed\n\n";
    std::cout << "You are " << BLUE << "O" << RESET << ", AI is " << RED << "X" << RESET << "\n";
    std::cout << "Enter moves like: J10, A1, S19\n\n";
}

void Display::printWinner(int player) {
    std::cout << "\n" << GREEN << "=== GAME OVER ===" << RESET << "\n";
    if (player == GameState::PLAYER1) {
        std::cout << BLUE << "🎉 YOU WIN! 🎉" << RESET << "\n";
    } else if (player == GameState::PLAYER2) {
        std::cout << RED << "🤖 AI WINS! 🤖" << RESET << "\n";
    } else {
        std::cout << YELLOW << "DRAW!" << RESET << "\n";
    }
    std::cout << "\n";
}

void Display::printBoardtoFile(const GameState& state, std::ofstream& file) {
    file << "\n  ";
    
    // Imprimir coordenadas superiores (A-S)
    for (int j = 0; j < GameState::BOARD_SIZE; j++) {
        file << " " << char('A' + j);
    }
    file << "\n";
    
    // Imprimir filas
    for (int i = 0; i < GameState::BOARD_SIZE; i++) {
        // Número de fila (1-19)
        file << std::setw(2) << (i + 1) << " ";
        
        for (int j = 0; j < GameState::BOARD_SIZE; j++) {
            int piece = state.getPiece(i, j);
            file << getPieceChar(piece) << " ";
        }
        
        file << std::setw(2) << (i + 1) << "\n";
    }
    
    // Coordenadas inferiores
    file << "  ";
    for (int j = 0; j < GameState::BOARD_SIZE; j++) {
        file << " " << char('A' + j);
    }
    file << "\n";
	file.flush();
}
