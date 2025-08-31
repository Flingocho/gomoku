/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   display.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/21 16:36:12 by jainavas          #+#    #+#             */
/*   Updated: 2025/08/31 20:10:31 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/display.hpp"
#include <iostream>
#include <iomanip>
#include <cctype>

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
        system("clear");
    #endif
}

char Display::getPieceChar(int piece) {
    switch (piece) {
        case 0: return '.';
        case 1: return 'O';  // Humano
        case 2: return 'X';  // AI
        default: return '?';
    }
}

std::string Display::getPieceColor(int piece) {
    switch (piece) {
        case 1: return BLUE;   // Humano en azul
        case 2: return RED;    // AI en rojo
        default: return RESET;
    }
}

void Display::printBoard(const Board& board) {
    std::cout << "\n  ";
    
    // Imprimir coordenadas superiores (A-S)
    for (int j = 0; j < 19; j++) {
        std::cout << " " << char('A' + j);
    }
    std::cout << "\n";
    
    // Imprimir filas
    for (int i = 0; i < 19; i++) {
        // Número de fila (1-19)
        std::cout << std::setw(2) << (i + 1) << " ";
        
        for (int j = 0; j < 19; j++) {
            int piece = board.getPiece(i, j);
            std::cout << getPieceColor(piece) << getPieceChar(piece) << RESET << " ";
        }
        
        std::cout << std::setw(2) << (i + 1) << "\n";
    }
    
    // Coordenadas inferiores
    std::cout << "  ";
    for (int j = 0; j < 19; j++) {
        std::cout << " " << char('A' + j);
    }
    std::cout << "\n";
}

void Display::printGameInfo(const Board& board, double aiTime) {
    std::cout << "\n" << GREEN << "=== GOMOKU ===" << RESET << "\n";
    std::cout << "Captures: " << BLUE << "You: " << board.getCaptures(1) << RESET;
    std::cout << "  " << RED << "AI: " << board.getCaptures(2) << RESET;
    
    // Mostrar si alguien está cerca de ganar por capturas
    if (board.getCaptures(1) >= 8) {
        std::cout << " " << BLUE << "(2 more to win!)" << RESET;
    }
    if (board.getCaptures(2) >= 8) {
        std::cout << " " << RED << "(2 more to win!)" << RESET;
    }
    std::cout << "\n";
    
    if (aiTime > 0) {
        std::cout << "AI thinking time: " << YELLOW << std::fixed 
                  << std::setprecision(3) << aiTime << "s" << RESET << "\n";
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
        if (row < 0 || row >= 19) return {-2, -2};
        return {row, col};
    } catch (...) {
        return {-2, -2};
    }
}

void Display::printWelcome() {
    clearScreen();
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
    if (player == 1) {
        std::cout << BLUE << "🎉 YOU WIN! 🎉" << RESET << "\n";
    } else if (player == 2) {
        std::cout << RED << "🤖 AI WINS! 🤖" << RESET << "\n";
    } else {
        std::cout << YELLOW << "DRAW!" << RESET << "\n";
    }
    std::cout << "\n";
}