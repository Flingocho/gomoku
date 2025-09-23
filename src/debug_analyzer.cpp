/* ************************************************************************** */
/*                                                                            */
/*   debug_analyzer.cpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/18 17:00:00 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/18 17:00:00 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/debug_analyzer.hpp"
#include "../include/evaluator.hpp"
#include "../include/rule_engine.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>

// Instancia global
DebugAnalyzer* g_debugAnalyzer = nullptr;

DebugAnalyzer::DebugAnalyzer(DebugLevel level) : 
    currentLevel(level), fileLoggingEnabled(false) {
    clear();
}

DebugAnalyzer::~DebugAnalyzer() {
    if (debugFile.is_open()) {
        debugFile.close();
    }
}

void DebugAnalyzer::enableFileLogging(const std::string& filename) {
    if (debugFile.is_open()) {
        debugFile.close();
    }
    
    debugFile.open(filename, std::ios::out | std::ios::app);
    if (debugFile.is_open()) {
        fileLoggingEnabled = true;
        logToFile("=== GOMOKU DEBUG SESSION START ===");
    } else {
        std::cerr << "Warning: Could not open debug file " << filename << std::endl;
        fileLoggingEnabled = false;
    }
}

void DebugAnalyzer::disableFileLogging() {
    if (debugFile.is_open()) {
        logToFile("=== GOMOKU DEBUG SESSION END ===");
        debugFile.close();
    }
    fileLoggingEnabled = false;
}

void DebugAnalyzer::analyzeRootMove(const Move& move, int score, const EvaluationBreakdown& breakdown) {
    if (currentLevel == DEBUG_OFF) return;
    
    MoveAnalysis analysis(move);
    analysis.score = score;
    analysis.breakdown = breakdown;
    analysis.reasoning = breakdown.explanation;
    
    rootMoveAnalyses.push_back(analysis);
}

void DebugAnalyzer::setChosenMove(const Move& move, int finalScore) {
    if (currentLevel == DEBUG_OFF && finalScore != 0) return;
    
    // Marcar el movimiento elegido
    for (auto& analysis : rootMoveAnalyses) {
        if (analysis.move.x == move.x && analysis.move.y == move.y) {
            analysis.wasChosenAsRoot = true;
            break;
        }
    }
}

void DebugAnalyzer::createSnapshot(const GameState& state, int totalTime, int totalNodes) {
    if (currentLevel == DEBUG_OFF) return;
    
    lastSnapshot.state = state;
    lastSnapshot.totalTime = totalTime;
    lastSnapshot.totalNodes = totalNodes;
    lastSnapshot.gamePhase = analyzeGamePhase(state);
    lastSnapshot.criticalThreats = findCriticalThreats(state);
    
    // Copiar análisis de movimientos y ordenar por score
    lastSnapshot.topMoves = rootMoveAnalyses;
    std::sort(lastSnapshot.topMoves.begin(), lastSnapshot.topMoves.end(),
              [](const MoveAnalysis& a, const MoveAnalysis& b) {
                  return a.score > b.score;
              });
    
    // Encontrar movimiento elegido
    for (const auto& analysis : rootMoveAnalyses) {
        if (analysis.wasChosenAsRoot) {
            lastSnapshot.chosenMove = analysis.move;
            break;
        }
    }
    
    // Imprimir análisis si el nivel lo permite
    if (currentLevel >= DEBUG_TOP_MOVES) {
        printCurrentAnalysis();
    }
    
    // Limpiar para próximo turno
    clear();
}

DebugAnalyzer::EvaluationBreakdown DebugAnalyzer::evaluateWithBreakdown(
    const GameState& state, const Move& move, int player) {
    
    EvaluationBreakdown breakdown(move);
    
    // Crear estado temporal para evaluación
    GameState tempState = state;
    tempState.board[move.x][move.y] = player;
    
    // ¡CRÍTICO! Verificar condiciones de victoria PRIMERO
    if (RuleEngine::checkWin(tempState, player)) {
        breakdown.totalScore = 500000; // Score similar al WIN del evaluador
        breakdown.isWinning = true;
        breakdown.explanation = "WINNING MOVE! ";
        return breakdown;
    }
    if (RuleEngine::checkWin(tempState, state.getOpponent(player))) {
        breakdown.totalScore = -500000;
        breakdown.isLosing = true;
        breakdown.explanation = "LOSING MOVE! ";
        return breakdown;
    }
    
    // 1. Evaluar patrones
    breakdown.patternScore = Evaluator::evaluateForPlayer(tempState, player) - 
                           Evaluator::evaluateForPlayer(tempState, state.getOpponent(player));
    
    // 2. Evaluar capturas
    auto captures = RuleEngine::findCaptures(tempState, move, player);
    breakdown.captureScore = captures.size() * 1000;
    
    // 3. Evaluar amenazas inmediatas
    breakdown.threatScore = Evaluator::evaluateImmediateThreats(tempState, player);
    
    // 4. Evaluar posición
    int centerDist = std::max(std::abs(move.x - 9), std::abs(move.y - 9));
    breakdown.positionScore = (9 - centerDist) * 10;
    
    // 5. Total
    breakdown.totalScore = breakdown.patternScore + breakdown.captureScore + 
                          breakdown.threatScore + breakdown.positionScore;
    
    // 6. Análisis de criticidad
    breakdown.isWinning = breakdown.totalScore > 50000;
    breakdown.isLosing = breakdown.totalScore < -50000;
    breakdown.isCriticalThreat = std::abs(breakdown.totalScore) > 10000;
    
    // 7. Generar explicación
    std::ostringstream explanation;
    
    if (breakdown.isWinning) {
        explanation << "WINNING MOVE! ";
    } else if (breakdown.isLosing) {
        explanation << "LOSING MOVE! ";
    }
    
    if (breakdown.captureScore > 0) {
        explanation << "Captures:" << (breakdown.captureScore/1000) << " ";
    }
    
    if (breakdown.threatScore > 20000) {
        explanation << "CRITICAL_THREAT ";
    } else if (breakdown.threatScore > 5000) {
        explanation << "threat ";
    }
    
    if (breakdown.patternScore > 1000) {
        explanation << "good_pattern ";
    } else if (breakdown.patternScore < -1000) {
        explanation << "bad_pattern ";
    }
    
    explanation << "[P:" << breakdown.patternScore 
                << " C:" << breakdown.captureScore 
                << " T:" << breakdown.threatScore 
                << " Pos:" << breakdown.positionScore << "]";
    
    breakdown.explanation = explanation.str();
    
    return breakdown;
}

bool DebugAnalyzer::shouldDebug(int depth, int score, bool isRootLevel) const {
    if (currentLevel == DEBUG_OFF) return false;
    
    return isRootLevel ||                           // Siempre nivel raíz
           (currentLevel >= DEBUG_CRITICAL && std::abs(score) > 10000) ||  // Crítico
           (currentLevel >= DEBUG_HEURISTIC && depth <= 2);               // Primeros niveles
}

void DebugAnalyzer::logCriticalPosition(const GameState& state, const std::string& reason) {
    if (currentLevel < DEBUG_CRITICAL) return;
    
    std::ostringstream msg;
    msg << "\n🚨 CRITICAL POSITION: " << reason << "\n";
    msg << "Turn: " << state.turnCount << ", Player: " << state.currentPlayer << "\n";
    msg << "Captures: P1=" << state.captures[0] << " P2=" << state.captures[1] << "\n";
    
    std::cout << msg.str();
    logToFile(msg.str());
}

void DebugAnalyzer::clear() {
    rootMoveAnalyses.clear();
}

void DebugAnalyzer::printCurrentAnalysis() const {
    if (rootMoveAnalyses.empty() || currentLevel == DEBUG_OFF) return;
    
    std::ostringstream analysisLog;
    
    analysisLog << "\n🤖 AI DECISION ANALYSIS\n";
    analysisLog << "========================\n";
    analysisLog << "Turn: " << lastSnapshot.state.turnCount << "\n";
    analysisLog << "Current Player: " << (lastSnapshot.state.currentPlayer == GameState::PLAYER1 ? "HUMAN (O)" : "AI (X)") << "\n";
    analysisLog << "Game Phase: " << lastSnapshot.gamePhase << "\n";
    analysisLog << "Captures: HUMAN=" << lastSnapshot.state.captures[0] << " AI=" << lastSnapshot.state.captures[1] << "\n";
    
    if (!lastSnapshot.criticalThreats.empty()) {
        analysisLog << "⚠️  Critical Threats: " << lastSnapshot.criticalThreats << "\n";
    }
    
    // Mostrar el tablero actual
    analysisLog << "\nCURRENT BOARD STATE:\n";
    analysisLog << "   ";
    for (int j = 0; j < GameState::BOARD_SIZE; j++) {
        analysisLog << std::setw(2) << char('A' + j) << " ";
    }
    analysisLog << "\n";
    
    for (int i = 0; i < GameState::BOARD_SIZE; i++) {
        analysisLog << std::setw(2) << (i + 1) << " ";
        for (int j = 0; j < GameState::BOARD_SIZE; j++) {
            char piece = '.';
            if (lastSnapshot.state.board[i][j] == GameState::PLAYER1) piece = 'O';
            else if (lastSnapshot.state.board[i][j] == GameState::PLAYER2) piece = 'X';
            analysisLog << piece << "  ";
        }
        analysisLog << "\n";
    }
    
    analysisLog << "\nTOP 10 MOVE CANDIDATES:\n";
    analysisLog << std::left << std::setw(8) << "Rank" 
                << std::setw(6) << "Move" 
                << std::setw(12) << "Score" 
                << std::setw(50) << "Reason" << "\n";
    analysisLog << std::string(80, '-') << "\n";
    
    // Mostrar top 10 movimientos
    int count = 0;
    for (const auto& analysis : lastSnapshot.topMoves) {
        if (count >= 10) break;
        
        std::string marker = analysis.wasChosenAsRoot ? "👑CHOSEN" : std::to_string(count + 1);
        
        analysisLog << std::left << std::setw(8) << marker
                    << std::setw(6) << formatMove(analysis.move)
                    << std::setw(12) << formatScore(analysis.score)
                    << analysis.breakdown.explanation << "\n";
        
        // Desglose detallado de todos los movimientos si el nivel lo permite
        if (currentLevel >= DEBUG_HEURISTIC) {
            const auto& b = analysis.breakdown;
            analysisLog << "        └─ Pattern:" << b.patternScore 
                        << " Capture:" << b.captureScore
                        << " Threat:" << b.threatScore 
                        << " Position:" << b.positionScore << "\n";
        }
        
        // Información extra para el movimiento elegido
        if (analysis.wasChosenAsRoot) {
            analysisLog << "        ★ FINAL DECISION: " << formatMove(analysis.move) 
                        << " with score " << analysis.score << "\n";
            analysisLog << "        ★ REASONING: " << analysis.breakdown.explanation << "\n";
            if (analysis.breakdown.isWinning) {
                analysisLog << "        ★ THIS IS A WINNING MOVE!\n";
            } else if (analysis.breakdown.isLosing) {
                analysisLog << "        ★ WARNING: This might be a losing move!\n";
            } else if (analysis.breakdown.isCriticalThreat) {
                analysisLog << "        ★ Critical threat situation!\n";
            }
        }
        
        count++;
    }
    
    analysisLog << "\nPERFORMANCE STATS:\n";
    analysisLog << "Time: " << lastSnapshot.totalTime << "ms\n";
    analysisLog << "Nodes evaluated: " << lastSnapshot.totalNodes << "\n";
    analysisLog << "Nodes per second: " << (lastSnapshot.totalTime > 0 ? (lastSnapshot.totalNodes * 1000 / lastSnapshot.totalTime) : 0) << "\n";
    analysisLog << std::string(80, '=') << "\n\n";
    
    // Escribir al archivo de log
    logToFile(analysisLog.str());
    
    // Solo mostrar un resumen breve en consola
    std::cout << "AI Analysis logged to file. Chosen move: " << formatMove(lastSnapshot.chosenMove) << std::endl;
}

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

void DebugAnalyzer::logToFile(const std::string& message) const {
    if (fileLoggingEnabled && debugFile.is_open()) {
        const_cast<std::ofstream&>(debugFile) << message << std::endl;
        const_cast<std::ofstream&>(debugFile).flush();
    }
}

void DebugAnalyzer::logInfo(const std::string& message) const {
    if (currentLevel == DEBUG_OFF) return;
    
    std::ostringstream logMsg;
    logMsg << "[INFO] " << message;
    logToFile(logMsg.str());
}

void DebugAnalyzer::logStats(const std::string& message) const {
    if (currentLevel == DEBUG_OFF) return;
    
    std::ostringstream logMsg;
    logMsg << "[STATS] " << message;
    logToFile(logMsg.str());
}

void DebugAnalyzer::logInit(const std::string& message) const {
    if (currentLevel == DEBUG_OFF) return;
    
    std::ostringstream logMsg;
    logMsg << "[INIT] " << message;
    logToFile(logMsg.str());
    
    // También mostrar mensajes de inicialización en consola para feedback inmediato
    std::cout << "✓ " << message << std::endl;
}

void DebugAnalyzer::logAI(const std::string& message) const {
    if (currentLevel == DEBUG_OFF) return;
    
    std::ostringstream logMsg;
    logMsg << "[AI] " << message;
    logToFile(logMsg.str());
}

void DebugAnalyzer::GameSnapshot::saveToFile(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Could not save snapshot to " << filename << std::endl;
        return;
    }
    
    file << "GOMOKU AI DETAILED SNAPSHOT\n";
    file << "===========================\n";
    file << "Turn: " << state.turnCount << "\n";
    file << "Current Player: " << (state.currentPlayer == GameState::PLAYER1 ? "HUMAN (O)" : "AI (X)") << "\n";
    file << "Game Phase: " << gamePhase << "\n";
    file << "Captures: HUMAN=" << state.captures[0] << " AI=" << state.captures[1] << "\n";
    file << "Time: " << totalTime << "ms\n";
    file << "Nodes: " << totalNodes << "\n";
    file << "Performance: " << (totalTime > 0 ? (totalNodes * 1000 / totalTime) : 0) << " nodes/second\n";
    
    if (!criticalThreats.empty()) {
        file << "⚠️  Critical Threats: " << criticalThreats << "\n";
    }
    
    file << "\nBOARD STATE:\n";
    file << "   ";
    for (int j = 0; j < GameState::BOARD_SIZE; j++) {
        file << std::setw(2) << char('A' + j) << " ";
    }
    file << "\n";
    
    for (int i = 0; i < GameState::BOARD_SIZE; i++) {
        file << std::setw(2) << (i + 1) << " ";
        for (int j = 0; j < GameState::BOARD_SIZE; j++) {
            char piece = '.';
            if (state.board[i][j] == GameState::PLAYER1) piece = 'O';
            else if (state.board[i][j] == GameState::PLAYER2) piece = 'X';
            file << piece << "  ";
        }
        file << "\n";
    }
    
    file << "\nTOP 10 MOVE ANALYSIS:\n";
    file << std::left << std::setw(8) << "Rank" 
         << std::setw(6) << "Move" 
         << std::setw(12) << "Score" 
         << std::setw(50) << "Reasoning" << "\n";
    file << std::string(80, '-') << "\n";
    
    int count = 0;
    for (const auto& moveAnalysis : topMoves) {
        if (count >= 10) break;
        
        std::string marker = moveAnalysis.wasChosenAsRoot ? "👑CHOSEN" : std::to_string(count + 1);
        
        file << std::left << std::setw(8) << marker
             << std::setw(6) << (char('A' + moveAnalysis.move.y)) << (moveAnalysis.move.x + 1)
             << std::setw(12) << moveAnalysis.score
             << moveAnalysis.reasoning << "\n";
        
        if (moveAnalysis.wasChosenAsRoot) {
            file << "        ★ FINAL DECISION with detailed breakdown\n";
            const auto& b = moveAnalysis.breakdown;
            file << "        └─ Pattern:" << b.patternScore 
                 << " Capture:" << b.captureScore
                 << " Threat:" << b.threatScore 
                 << " Position:" << b.positionScore << "\n";
        }
        
        count++;
    }
    
    file.close();
}

void DebugAnalyzer::GameSnapshot::printToConsole() const {
    // Implementación similar pero para consola con colores si es necesario
    std::cout << "\n📸 GAME SNAPSHOT\n";
    std::cout << "Turn " << state.turnCount << " - " << gamePhase << "\n";
    if (!criticalThreats.empty()) {
        std::cout << "⚠️ " << criticalThreats << "\n";
    }
    std::cout << "Performance: " << totalTime << "ms, " << totalNodes << " nodes\n";
}