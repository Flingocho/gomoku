/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   game_node.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/09 18:23:40 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/11 20:56:04 by jainavas         ###   ########.fr       */
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

GameNode::GameNode(GameNode& copy)
	: boardState(copy.boardState), lastMove(copy.lastMove), currentPlayer(copy.currentPlayer),
	  alpha(copy.alpha), beta(copy.beta), heuristicScore(copy.heuristicScore), isEvaluated(copy.isEvaluated),
	  parent(copy.parent), aiReference(copy.aiReference) {
}

bool GameNode::isTerminal() const {
    // Terminal si alguien ganó o profundidad máxima
    return boardState.checkWin(1) || boardState.checkWin(2);
}

int GameNode::evaluatePosition() const {
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
    
    // Generar solo movimientos candidatos, NO nodos completos
    std::vector<Move> moves = generateCandidateMoves();
    
    if (isMaximizingPlayer()) {
        // AI jugando - maximizar
        int maxScore = std::numeric_limits<int>::min();
        
        for (const Move& move : moves) {
            // Verificar que el movimiento es válido
            if (!boardState.isEmpty(move.x, move.y) || 
                boardState.isDoubleFree(move.x, move.y, currentPlayer)) {
                continue;
            }
            
            // Simular movimiento SIN crear nodo permanente
            Board tempBoard = boardState;
            if (tempBoard.placePiece(move.x, move.y, currentPlayer)) {
                
                // Crear nodo temporal solo para la recursión
                int nextPlayer = (currentPlayer == 1) ? 2 : 1;
                GameNode tempChild(tempBoard, move, aiReference, nextPlayer);
                
                // Heredar valores alpha-beta actuales
                tempChild.alpha = alpha;
                tempChild.beta = beta;
                
                // Evaluar recursivamente
                int score = tempChild.minimax(currentDepth + 1, maxDepth);
                maxScore = std::max(maxScore, score);
                
                // Poda Alpha-Beta
                alpha = std::max(alpha, score);
                if (beta <= alpha) {
                    break; // PODA: corta ramas sin crear más nodos
                }
            }
        }
        
        return maxScore;
        
    } else {
        // Humano jugando - minimizar
        int minScore = std::numeric_limits<int>::max();
        
        for (const Move& move : moves) {
            // Verificar que el movimiento es válido
            if (!boardState.isEmpty(move.x, move.y) || 
                boardState.isDoubleFree(move.x, move.y, currentPlayer)) {
                continue;
            }
            
            // Simular movimiento SIN crear nodo permanente
            Board tempBoard = boardState;
            if (tempBoard.placePiece(move.x, move.y, currentPlayer)) {
                
                // Crear nodo temporal solo para la recursión
                int nextPlayer = (currentPlayer == 1) ? 2 : 1;
                GameNode tempChild(tempBoard, move, aiReference, nextPlayer);
                
                // Heredar valores alpha-beta actuales
                tempChild.alpha = alpha;
                tempChild.beta = beta;
                
                // Evaluar recursivamente
                int score = tempChild.minimax(currentDepth + 1, maxDepth);
                minScore = std::min(minScore, score);
                
                // Poda Alpha-Beta
                beta = std::min(beta, score);
                if (beta <= alpha) {
                    break; // PODA: corta ramas sin crear más nodos
                }
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
			std::cout << "La bestScore anterior:" << bestScore << " en " << bestMove.x << "-" << bestMove.y << " ha sido superada por " << score << " en " << child->lastMove.x << "-" << child->lastMove.y << "\n"; 
            bestScore = score;
            bestMove = child->lastMove;
        }
    }
    
    return bestMove;
}

GameNode* GameNode::findChild(const Move& move) {
    generateChildren(); // Asegurar que los hijos estén generados
    
    for (auto& child : children) {
        const Move& childMove = child->getLastMove();
        if (childMove.x == move.x && childMove.y == move.y) {
            return child.get();
        }
    }
    
    return nullptr; // No se encontró el hijo
}
