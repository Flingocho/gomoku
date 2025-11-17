// ============================================
// DEBUG_CORE.CPP
// Sistema de logging y gestión del debug analyzer
// ============================================

#include "../../include/debug/debug_analyzer.hpp"
#include "../../include/ai/evaluator.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

// NUEVO: Acceso al sistema de debug del evaluador
extern EvaluationDebugCapture g_evalDebug;

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

bool DebugAnalyzer::shouldDebug(int depth, int score, bool isRootLevel) const {
    if (currentLevel == DEBUG_OFF) return false;
    
    return isRootLevel ||                           // Siempre nivel raíz
           (currentLevel >= DEBUG_CRITICAL && std::abs(score) > 10000) ||  // Crítico
           (currentLevel >= DEBUG_HEURISTIC && depth <= 2);               // Primeros niveles
}
