#ifndef AI_HPP
#define AI_HPP

#include "board.hpp"
#include <vector>
#include <utility>
#include <set>

struct Move {
    int x, y;
    int score;
    
    Move(int x = -1, int y = -1, int score = 0) : x(x), y(y), score(score) {}
};

class AI {
private:
    int aiPlayer;     // 1 o 2
    int humanPlayer;  // El oponente
    int maxDepth;     // Profundidad de búsqueda
    
public:
    AI(int player, int depth = 6);
    
    // Método principal: encontrar el mejor movimiento
    Move getBestMove(Board& board);
    
private:
    // Min-Max con poda alpha-beta
    int minimax(Board& board, int depth, bool isMaximizing, int alpha, int beta);
    
    // Generación de movimientos candidatos
    std::vector<Move> generateMoves(Board& board);
    void addInfluenceZones(std::set<std::pair<int, int>>& moves, const Board& board);
    void addTacticalMoves(std::set<std::pair<int, int>>& moves, Board& board, int player);
    
    // Heurística: evaluar qué tan buena es una posición
    int evaluatePosition(Board& board);
    int evaluatePlayer(Board& board, int player);
    
    // Detección de patrones básicos (NUEVO)
    int countPatterns(Board& board, int player, int targetLength);
    int countLineLength(Board& board, int startX, int startY, int dx, int dy, int player);
    
    // Detección inmediata de victoria/derrota (NUEVO)
    Move findImmediateWin(Board& board, int player);
    Move findCriticalBlock(Board& board, int player, int targetPattern);
    
    // Evaluación posicional
    int evaluatePositionalValue(Board& board, int player);
    int countAdjacentEmpty(const Board& board, int x, int y);
    
    // Funciones legacy (mantener por compatibilidad)
    int countSimplePatterns(Board& board, int player, int length);
    
    // Función de depuración
    void debugPatterns(Board& board, int player);
    
    // Función auxiliar para simulación completa
    bool simulateCompleteMove(Board& board, int x, int y, int player, int& capturesGained);
};

#endif