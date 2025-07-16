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
    // Min-Max principal
    int minimax(Board& board, int depth, bool isMaximizing, int alpha, int beta);
    
    // Generación de movimientos candidatos
    std::vector<Move> generateMoves(Board& board);
    void addInfluenceZones(std::set<std::pair<int, int>>& moves, const Board& board);
    void addTacticalMoves(std::set<std::pair<int, int>>& moves, Board& board, int player);
    
    // Heurística: evaluar qué tan buena es una posición
    int evaluatePosition(Board& board);
    int evaluatePlayer(Board& board, int player);
    int evaluatePlayerAdvanced(Board& board, int player);  // Nueva heurística inteligente
    
    // Funciones auxiliares para heurística
    int countPatterns(Board& board, int player, int patternLength, bool needsFreeEnds = false);
    int countSimplePatterns(Board& board, int player, int length);  
    int countThreatPatterns(Board& board, int player, int length);  // Cuenta amenazas
    int countFreeThrees(Board& board, int player);                 // Cuenta 3-libres
    bool hasPattern(Board& board, int x, int y, int player, int length);
    bool has4InLineWithFreeEnd(Board& board, int x, int y, int player);  
    bool isNearOtherPieces(const Board& board, int x, int y, int radius = 2);
};

#endif