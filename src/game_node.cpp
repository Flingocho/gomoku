/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   game_node.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/09 18:23:40 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/09 19:02:38 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/game_node.hpp"
#include "../include/ai.hpp"  // Include AI here to avoid circular dependency

GameNode::GameNode(const Board& board, const Move& move, const AI* ai, int player) 
    : boardState(board), lastMove(move), currentPlayer(player),
      alpha(std::numeric_limits<int>::min()), 
      beta(std::numeric_limits<int>::max()),
      heuristicScore(0), isEvaluated(false), parent(nullptr), aiReference(ai) {
}

GameNode::GameNode(const Board& board, int player, const AI* ai) 
        : boardState(board), lastMove(Move()), currentPlayer(player),
          alpha(std::numeric_limits<int>::min()), 
          beta(std::numeric_limits<int>::max()),
          heuristicScore(0), isEvaluated(false), parent(nullptr), aiReference(ai) {
}

bool GameNode::isTerminal() const {
    // Terminal si alguien ganó o profundidad máxima
    return boardState.checkWin(1) || boardState.checkWin(2);
}

int GameNode::evaluatePosition() const {
    // Necesitaremos acceso a la clase AI para la evaluación
    // Por ahora placeholder
    if (boardState.checkWin(2)) return 100000;  // AI gana
    if (boardState.checkWin(1)) return -100000; // Humano gana
    return aiReference->evaluatePosition(boardState); // Temporal
}

bool GameNode::isMaximizingPlayer() const {
    return currentPlayer == 2; // AI es maximizing player
}

void GameNode::generateChildren() {
    if (!children.empty()) return; // Ya generados
    
    std::vector<Move> moves = generateCandidateMoves();
    
    for (const Move& move : moves) {
        if (boardState.isEmpty(move.x, move.y) && 
            !boardState.isDoubleFree(move.x, move.y, currentPlayer)) {
            
            Board newBoard = boardState;
            if (newBoard.placePiece(move.x, move.y, currentPlayer)) {
                int nextPlayer = (currentPlayer == 1) ? 2 : 1;
                
                auto child = std::make_unique<GameNode>(newBoard, move, aiReference, nextPlayer);
                
                child->parent = this;
                children.push_back(std::move(child));
            }
        }
    }
}

std::vector<Move> GameNode::generateCandidateMoves() const {
    std::vector<Move> moves;
    
    // Por ahora: buscar posiciones cerca de piezas existentes
    for (int i = 0; i < boardState.getSize(); i++) {
        for (int j = 0; j < boardState.getSize(); j++) {
            if (boardState.isEmpty(i, j) && isNearPieces(i, j)) {
                moves.push_back(Move(i, j));
            }
        }
    }
    
    return moves;
}

bool GameNode::isNearPieces(int x, int y) const {
    // Verificar radio de 2 casillas
    for (int dx = -2; dx <= 2; dx++) {
        for (int dy = -2; dy <= 2; dy++) {
            int nx = x + dx, ny = y + dy;
            if (boardState.isValid(nx, ny) && boardState.getPiece(nx, ny) != 0) {
                return true;
            }
        }
    }
    return false;
}

int GameNode::minimax(int currentDepth, int maxDepth) {
    // Caso base: nodo terminal o profundidad máxima
    if (currentDepth >= maxDepth || isTerminal()) {
        return evaluatePosition();
    }
    
    // Generar hijos solo cuando los necesitamos
    generateChildren();
    
    if (isMaximizingPlayer()) {
        // AI jugando - maximizar
        int maxScore = std::numeric_limits<int>::min();
        
        for (auto& child : children) {
            int score = child->minimax(currentDepth + 1, maxDepth);
            maxScore = std::max(maxScore, score);
            
            // Poda Alpha-Beta
            alpha = std::max(alpha, score);
            if (beta <= alpha) {
                break; // Poda beta
            }
        }
        
        return maxScore;
    } else {
        // Humano jugando - minimizar
        int minScore = std::numeric_limits<int>::max();
        
        for (auto& child : children) {
            int score = child->minimax(currentDepth + 1, maxDepth);
            minScore = std::min(minScore, score);
            
            // Poda Alpha-Beta
            beta = std::min(beta, score);
            if (beta <= alpha) {
                break; // Poda alfa
            }
        }
        
        return minScore;
    }
}

Move GameNode::getBestMove(int searchDepth) {
    generateChildren();
    
    if (children.empty()) {
        return Move(); // No hay movimientos válidos
    }
    
    Move bestMove;
    int bestScore = std::numeric_limits<int>::min();
    
    for (auto& child : children) {
        // Cada hijo hereda los valores alpha-beta del padre
        child->alpha = this->alpha;
        child->beta = this->beta;
        
        int score = child->minimax(1, searchDepth); // Empezar en profundidad 1
        
        if (score > bestScore) {
            bestScore = score;
            bestMove = child->lastMove;
        }
    }
    
    return bestMove;
}
