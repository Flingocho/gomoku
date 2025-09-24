/* ************************************************************************** */
/*                                                                            */
/*   debug_analyzer.hpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/18 17:00:00 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/18 17:00:00 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef DEBUG_ANALYZER_HPP
#define DEBUG_ANALYZER_HPP

#include "game_types.hpp"
#include <vector>
#include <string>
#include <fstream>
#include <chrono>

/**
 * Sistema de debug inteligente para analizar decisiones de la IA
 * - Solo trackea decisiones importantes (nivel raíz + críticas)
 * - Proporciona breakdown detallado de heurísticas
 * - Guarda snapshots para análisis posterior
 */
class DebugAnalyzer {
public:
    enum DebugLevel {
        DEBUG_OFF = 0,
        DEBUG_TOP_MOVES = 1,    // Solo mejores movimientos del nivel raíz
        DEBUG_CRITICAL = 2,     // Amenazas críticas y capturas
        DEBUG_HEURISTIC = 3,    // Desglose detallado de evaluación
        DEBUG_POSITIONS = 4     // Estados de tablero importantes
    };
    
    struct EvaluationBreakdown {
        Move move;
        int totalScore;
        int patternScore;
        int captureScore;
        int threatScore;
        int positionScore;
        int mateDistance;
        bool isWinning;
        bool isLosing;
        bool isCriticalThreat;
        std::string explanation;
        
        EvaluationBreakdown(const Move& m = Move()) : 
            move(m), totalScore(0), patternScore(0), captureScore(0),
            threatScore(0), positionScore(0), mateDistance(0),
            isWinning(false), isLosing(false), isCriticalThreat(false) {}
    };
    
    struct MoveAnalysis {
        Move move;
        int score;
        int depth;
        int nodesEvaluated;
        EvaluationBreakdown breakdown;
        std::string reasoning;
        bool wasChosenAsRoot;
        
        MoveAnalysis(const Move& m = Move()) : 
            move(m), score(0), depth(0), nodesEvaluated(0), 
            breakdown(m), wasChosenAsRoot(false) {}
    };
    
    struct GameSnapshot {
        GameState state;
        std::vector<MoveAnalysis> topMoves;
        Move chosenMove;
        int totalTime;
        int totalNodes;
        std::string gamePhase;
        std::string criticalThreats;
        
        void saveToFile(const std::string& filename) const;
        void printToConsole() const;
    };

private:
    DebugLevel currentLevel;
    std::vector<MoveAnalysis> rootMoveAnalyses;
    GameSnapshot lastSnapshot;
    std::ofstream debugFile;
    bool fileLoggingEnabled;
    
public:
    DebugAnalyzer(DebugLevel level = DEBUG_TOP_MOVES);
    ~DebugAnalyzer();
    
    // Control del sistema
    void setDebugLevel(DebugLevel level) { currentLevel = level; }
    void enableFileLogging(const std::string& filename = "gomoku_debug.log");
    void disableFileLogging();
    
    // Funciones principales de análisis
    void analyzeRootMove(const Move& move, int score, const EvaluationBreakdown& breakdown);
    void setChosenMove(const Move& move, int finalScore);
    void createSnapshot(const GameState& state, int totalTime, int totalNodes);
    
    // Evaluación con breakdown
    static EvaluationBreakdown evaluateWithBreakdown(const GameState& state, const Move& move, int player);
    
    // Utilidades
    bool shouldDebug(int depth, int score, bool isRootLevel) const;
    void logCriticalPosition(const GameState& state, const std::string& reason);
    void clear();
    
    // Output functions
    void printCurrentAnalysis() const;
    void printLastSnapshot() const { lastSnapshot.printToConsole(); }
    void saveSnapshotToFile(const std::string& filename) const { lastSnapshot.saveToFile(filename); }
    void logToFile(const std::string& message) const;
    std::string formatBoard(const GameState& state) const;
    
    // Centralized logging functions
    void logInfo(const std::string& message) const;
    void logStats(const std::string& message) const;
    void logInit(const std::string& message) const;
    void logAI(const std::string& message) const;

    std::ofstream& getDebugFile() { return debugFile; }
    
private:
    std::string formatMove(const Move& move) const;
    std::string formatScore(int score) const;
    std::string analyzeGamePhase(const GameState& state) const;
    std::string findCriticalThreats(const GameState& state) const;
};

// Instancia global para fácil acceso
extern DebugAnalyzer* g_debugAnalyzer;

// Macros para facilitar el uso
#define DEBUG_ROOT_MOVE(move, score, breakdown) \
    if (g_debugAnalyzer) g_debugAnalyzer->analyzeRootMove(move, score, breakdown)

#define DEBUG_CHOSEN_MOVE(move, score) \
    if (g_debugAnalyzer) g_debugAnalyzer->setChosenMove(move, score)

#define DEBUG_SNAPSHOT(state, time, nodes) \
    if (g_debugAnalyzer) g_debugAnalyzer->createSnapshot(state, time, nodes)

#define DEBUG_CRITICAL(state, reason) \
    if (g_debugAnalyzer && g_debugAnalyzer->shouldDebug(0, 50000, false)) \
        g_debugAnalyzer->logCriticalPosition(state, reason)

// Macros for centralized logging
#define DEBUG_LOG_INFO(msg) \
    if (g_debugAnalyzer) g_debugAnalyzer->logInfo(msg)

#define DEBUG_LOG_STATS(msg) \
    if (g_debugAnalyzer) g_debugAnalyzer->logStats(msg)

#define DEBUG_LOG_INIT(msg) \
    if (g_debugAnalyzer) g_debugAnalyzer->logInit(msg)

#define DEBUG_LOG_AI(msg) \
    if (g_debugAnalyzer) g_debugAnalyzer->logAI(msg)

#endif