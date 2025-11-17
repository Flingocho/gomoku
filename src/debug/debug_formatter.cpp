// ============================================
// DEBUG_FORMATTER.CPP
// Formateo de movimientos, scores, tableros y análisis
// ============================================

#include "../../include/debug/debug_analyzer.hpp"
#include "../../include/rules/rule_engine.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>

std::string DebugAnalyzer::formatMove(const Move& move) const {
    if (!move.isValid()) return "??";
    
    char col = 'A' + move.y;
    int row = move.x + 1;
    return std::string(1, col) + std::to_string(row);
}

std::string DebugAnalyzer::formatScore(int score) const {
    if (score > 50000) return "WIN";
    if (score < -50000) return "LOSE";
    if (score > 10000) return "++";
    if (score < -10000) return "--";
    if (score > 1000) return "+";
    if (score < -1000) return "-";
    return std::to_string(score);
}

std::string DebugAnalyzer::analyzeGamePhase(const GameState& state) const {
    int pieceCount = 0;
    for (int i = 0; i < GameState::BOARD_SIZE; i++) {
        for (int j = 0; j < GameState::BOARD_SIZE; j++) {
            if (state.board[i][j] != GameState::EMPTY) {
                pieceCount++;
            }
        }
    }
    
    if (pieceCount <= 4) return "Opening";
    if (pieceCount <= 15) return "Early Game";
    if (pieceCount <= 30) return "Mid Game";
    return "End Game";
}

std::string DebugAnalyzer::findCriticalThreats(const GameState& state) const {
    std::ostringstream threats;
    
    // Verificar amenazas de victoria
    if (RuleEngine::checkWin(state, GameState::PLAYER1)) {
        threats << "HUMAN_WINS ";
    }
    if (RuleEngine::checkWin(state, GameState::PLAYER2)) {
        threats << "AI_WINS ";
    }
    
    // Verificar capturas cercanas a victoria
    if (state.captures[0] >= 8) {
        threats << "HUMAN_NEAR_CAPTURE_WIN(" << (10-state.captures[0]) << " more) ";
    }
    if (state.captures[1] >= 8) {
        threats << "AI_NEAR_CAPTURE_WIN(" << (10-state.captures[1]) << " more) ";
    }
    
    return threats.str();
}

std::string DebugAnalyzer::formatBoard(const GameState& state) const {
    std::ostringstream boardStr;
    
    // Encabezado con letras de columnas
    boardStr << "\n   ";
    for (int j = 0; j < GameState::BOARD_SIZE; j++) {
        boardStr << std::setw(2) << char('A' + j) << " ";
    }
    boardStr << "\n";
    
    // Filas del tablero
    for (int i = 0; i < GameState::BOARD_SIZE; i++) {
        boardStr << std::setw(2) << (i + 1) << " ";
        for (int j = 0; j < GameState::BOARD_SIZE; j++) {
            char piece = '.';
            if (state.board[i][j] == GameState::PLAYER1) piece = 'O';
            else if (state.board[i][j] == GameState::PLAYER2) piece = 'X';
            boardStr << piece << "  ";
        }
        boardStr << "\n";
    }
    
    return boardStr.str();
}

void GameSnapshot::printToConsole() const {
    // Implementación similar pero para consola con colores si es necesario
    std::cout << "\n📸 GAME SNAPSHOT\n";
    std::cout << "Turn " << state.turnCount << " - " << gamePhase << "\n";
    if (!criticalThreats.empty()) {
        std::cout << "⚠️ " << criticalThreats << "\n";
    }
    std::cout << "Performance: " << totalTime << "ms, " << totalNodes << " nodes\n";
}
