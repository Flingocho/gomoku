/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   game_node.hpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/21 16:35:04 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/09 19:00:08 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef GAME_NODE_HPP
#define GAME_NODE_HPP

#include "board.hpp"
#include <vector>
#include <memory>
#include <limits>

// Forward declaration to avoid circular dependency
class AI;

// Move structure (needed here to avoid circular dependency)
struct Move {
    int x;
    int y;
    int score;
    
    Move(int row = -1, int col = -1, int s = 0) : x(row), y(col), score(s) {}
    
    bool isValid() const {
        return x >= 0 && y >= 0;
    }
};

class GameNode {
private:
    Board boardState;                          // Estado del tablero
    Move lastMove;                             // Movimiento que llevó aquí  
    int currentPlayer;                         // Turno actual (1 o 2)
    int alpha, beta;                          // Para poda alpha-beta
    int heuristicScore;                       // Evaluación heurística
    bool isEvaluated;                         // ¿Ya evaluado?
    
    std::vector<std::unique_ptr<GameNode>> children;
    GameNode* parent;                         // Raw pointer para evitar ciclos

	const AI* aiReference;
	
public:
    // Constructores
    GameNode(const Board& board, const Move& move, const AI* ai, int player);
	GameNode(const Board& board, int player, const AI* ai);
    // Métodos principales
    void generateChildren();
    int minimax(int currentDepth, int maxDepth);
    Move getBestMove(int searchDepth);
    std::vector<Move> generateCandidateMoves() const;
    bool isNearPieces(int x, int y) const;
    // Utilidades
    bool isTerminal() const;
    int evaluatePosition() const;
    bool isMaximizingPlayer() const;
    
    // Getters
    const Board& getBoard() const { return boardState; }
    int getPlayer() const { return currentPlayer; }
};

#endif