#include "../include/game_node.hpp"
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <set>

// === CONSTRUCTORES ===

GameNode::GameNode(const Board& initialState, int player) 
    : gameState(initialState), moveToHere(-1, -1), currentPlayer(player), 
      depth(0), evaluation(0), isEvaluated(false), parent(nullptr), 
      childrenGenerated(false), alpha(std::numeric_limits<int>::min()), 
      beta(std::numeric_limits<int>::max()) {
}

GameNode::GameNode(const Board& parentState, const Move& move, int player, int nodeDepth, GameNode* parentNode)
    : moveToHere(move), currentPlayer(player), depth(nodeDepth), 
      evaluation(0), isEvaluated(false), parent(parentNode), 
      childrenGenerated(false), alpha(std::numeric_limits<int>::min()), 
      beta(std::numeric_limits<int>::max()) {
    
    // Crear el estado aplicando el movimiento al estado del padre
    gameState = parentState;  // Copiar estado padre
    gameState.placePiece(move.x, move.y, player);  // Aplicar movimiento
}

// === GETTERS ===

const GameNode* GameNode::getChild(size_t index) const {
    if (index < children.size()) {
        return children[index].get();
    }
    return nullptr;
}

// === GENERACIÓN DEL ÁRBOL ===

void GameNode::generateChildren() {
    if (childrenGenerated) return;
    
    // Verificar si el juego ya terminó
    if (isGameOver()) {
        childrenGenerated = true;
        return;
    }
    
    // Generar movimientos candidatos
    std::vector<Move> candidateMoves = generateCandidateMoves();
    
    // Ordenar movimientos por prometedores (optimización)
    orderMoves(candidateMoves);
    
    // Crear nodos hijos para cada movimiento válido
    for (const Move& move : candidateMoves) {
        if (isValidMove(move)) {
            int nextPlayer = (currentPlayer == 1) ? 2 : 1;
            
            auto child = std::make_unique<GameNode>(
                gameState, move, nextPlayer, depth + 1, this
            );
            
            children.push_back(std::move(child));
        }
    }
    
    childrenGenerated = true;
    
    std::cout << "DEBUG: Generated " << children.size() 
              << " children at depth " << depth << std::endl;
}

bool GameNode::isTerminal(int maxDepth) const {
    return depth >= maxDepth || isGameOver();
}

bool GameNode::isGameOver() const {
    // Verificar victoria por 5 en línea o por capturas
    return gameState.checkWin(1) || gameState.checkWin(2);
}

// === EVALUACIÓN ===

int GameNode::evaluateNode() {
    if (isEvaluated) return evaluation;
    
    evaluation = evaluateGomokuPosition();
    isEvaluated = true;
    
    return evaluation;
}

void GameNode::setEvaluation(int value) {
    evaluation = value;
    isEvaluated = true;
}

// === MINIMAX ===

int GameNode::minimax(int maxDepth, bool isMaximizingPlayer, int alpha, int beta) {
    // Caso base: nodo terminal
    if (isTerminal(maxDepth)) {
        // Si el juego terminó, dar puntuación extrema
        if (isGameOver()) {
            if (gameState.checkWin(2)) {  // AI ganó (asumiendo AI = player 2)
                return 10000 + (maxDepth - depth);  // Bonus por ganar rápido
            } else if (gameState.checkWin(1)) {  // Humano ganó
                return -10000 - (maxDepth - depth);  // Penalización por perder rápido
            }
        }
        // Si llegamos a profundidad máxima, evaluar posición
        return evaluateNode();
    }
    
    // Generar hijos si no existen
    generateChildren();
    
    if (children.empty()) {
        return evaluateNode();  // No hay movimientos válidos
    }
    
    if (isMaximizingPlayer) {
        int maxEval = std::numeric_limits<int>::min();
        
        for (auto& child : children) {
            int eval = child->minimax(maxDepth, false, alpha, beta);
            maxEval = std::max(maxEval, eval);
            alpha = std::max(alpha, eval);
            
            // Poda alpha-beta
            if (beta <= alpha) {
                break;
            }
        }
        
        setEvaluation(maxEval);
        return maxEval;
        
    } else {
        int minEval = std::numeric_limits<int>::max();
        
        for (auto& child : children) {
            int eval = child->minimax(maxDepth, true, alpha, beta);
            minEval = std::min(minEval, eval);
            beta = std::min(beta, eval);
            
            // Poda alpha-beta
            if (beta <= alpha) {
                break;
            }
        }
        
        setEvaluation(minEval);
        return minEval;
    }
}

Move GameNode::getBestMove(int searchDepth) {
    std::cout << "=== INICIANDO BÚSQUEDA ===\n";
    std::cout << "Profundidad: " << searchDepth << std::endl;
    std::cout << "Jugador actual: " << currentPlayer << std::endl;
    
    // Generar hijos
    generateChildren();
    
    if (children.empty()) {
        std::cout << "ERROR: No hay movimientos válidos!\n";
        return Move(-1, -1, 0);
    }
    
    Move bestMove(-1, -1, std::numeric_limits<int>::min());
    bool isAI = (currentPlayer == 2);  // Asumiendo AI = player 2
    
    std::cout << "\n--- EVALUANDO MOVIMIENTOS ---\n";
    
    for (size_t i = 0; i < children.size(); ++i) {
        auto& child = children[i];
        
        // Ejecutar minimax en el hijo
        int score = child->minimax(searchDepth - 1, !isAI, 
                                  std::numeric_limits<int>::min(), 
                                  std::numeric_limits<int>::max());
        
        // Mostrar información del movimiento
        Move childMove = child->getMove();
        std::cout << "Movimiento (" << childMove.x << "," << childMove.y 
                  << ") -> Score: " << score << std::endl;
        
        // Actualizar mejor movimiento
        if (score > bestMove.score) {
            bestMove = Move(childMove.x, childMove.y, score);
        }
    }
    
    std::cout << "\n--- MEJOR MOVIMIENTO ---\n";
    std::cout << "Posición: (" << bestMove.x << "," << bestMove.y 
              << ") con score: " << bestMove.score << std::endl;
    
    // Mostrar estadísticas del árbol
    TreeStats stats = getTreeStats();
    std::cout << "\n--- ESTADÍSTICAS DEL ÁRBOL ---\n";
    std::cout << "Nodos totales: " << stats.totalNodes << std::endl;
    std::cout << "Nodos hoja: " << stats.leafNodes << std::endl;
    std::cout << "Profundidad máxima: " << stats.maxDepthReached << std::endl;
    std::cout << "Evaluaciones: " << stats.totalEvaluations << std::endl;
    
    return bestMove;
}

// === UTILIDADES ===

std::vector<Move> GameNode::generateCandidateMoves() const {
    std::vector<Move> moves;
    
    // Estrategia simple: considerar posiciones cerca de piezas existentes
    std::set<std::pair<int, int>> candidates;
    
    // Añadir posiciones adyacentes a piezas existentes
    for (int i = 0; i < gameState.getSize(); ++i) {
        for (int j = 0; j < gameState.getSize(); ++j) {
            if (gameState.getPiece(i, j) != 0) {
                // Añadir posiciones en un radio de 2 casillas
                for (int di = -2; di <= 2; ++di) {
                    for (int dj = -2; dj <= 2; ++dj) {
                        int ni = i + di;
                        int nj = j + dj;
                        
                        if (gameState.isValid(ni, nj) && gameState.isEmpty(ni, nj)) {
                            candidates.insert({ni, nj});
                        }
                    }
                }
            }
        }
    }
    
    // Si no hay piezas, empezar por el centro
    if (candidates.empty()) {
        int center = gameState.getSize() / 2;
        candidates.insert({center, center});
        candidates.insert({center-1, center});
        candidates.insert({center+1, center});
        candidates.insert({center, center-1});
        candidates.insert({center, center+1});
    }
    
    // Convertir a vector de Move
    for (const auto& pos : candidates) {
        moves.emplace_back(pos.first, pos.second);
    }
    
    // Limitar número de candidatos para eficiencia
    if (moves.size() > 25) {
        moves.resize(25);
    }
    
    return moves;
}

Board GameNode::applyMove(const Move& move) const {
    Board newState = gameState;  // Copia
    
    if (newState.placePiece(move.x, move.y, currentPlayer)) {
        return newState;
    } else {
        std::cerr << "ERROR: No se pudo aplicar movimiento (" 
                  << move.x << "," << move.y << ")" << std::endl;
        return gameState;  // Devolver estado original si falla
    }
}

bool GameNode::isValidMove(const Move& move) const {
    return gameState.isValid(move.x, move.y) && gameState.isEmpty(move.x, move.y);
}

// === EVALUACIÓN HEURÍSTICA ===

int GameNode::evaluateGomokuPosition() const {
    if (gameState.checkWin(2)) return 10000;  // AI gana
    if (gameState.checkWin(1)) return -10000; // Humano gana
    
    int aiScore = 0;
    int humanScore = 0;
    
    // Evaluar patrones para ambos jugadores
    aiScore += countPatterns(2, 4) * 1000;    // 4 en línea
    aiScore += countPatterns(2, 3) * 100;     // 3 en línea
    aiScore += countPatterns(2, 2) * 10;      // 2 en línea
    
    humanScore += countPatterns(1, 4) * 1000;
    humanScore += countPatterns(1, 3) * 100;
    humanScore += countPatterns(1, 2) * 10;
    
    // Evaluar capturas
    aiScore += gameState.getCaptures(2) * 50;
    humanScore += gameState.getCaptures(1) * 50;
    
    // Evaluar amenazas
    aiScore += evaluateThreats(2);
    humanScore += evaluateThreats(1);
    
    return aiScore - humanScore;
}

int GameNode::countPatterns(int player, int patternLength) const {
    int count = 0;
    int directions[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};
    
    for (int i = 0; i < gameState.getSize(); ++i) {
        for (int j = 0; j < gameState.getSize(); ++j) {
            if (gameState.getPiece(i, j) == player) {
                for (int d = 0; d < 4; ++d) {
                    int dx = directions[d][0];
                    int dy = directions[d][1];
                    
                    // Solo contar desde el inicio de cada línea
                    int prevX = i - dx;
                    int prevY = j - dy;
                    bool isStart = !gameState.isValid(prevX, prevY) || 
                                  gameState.getPiece(prevX, prevY) != player;
                    
                    if (isStart) {
                        int lineLength = 1;
                        int x = i + dx, y = j + dy;
                        
                        while (gameState.isValid(x, y) && gameState.getPiece(x, y) == player) {
                            lineLength++;
                            x += dx;
                            y += dy;
                        }
                        
                        if (lineLength == patternLength) {
                            count++;
                        }
                    }
                }
            }
        }
    }
    
    return count;
}

int GameNode::evaluateThreats(int player) const {
    // Evaluar amenazas inmediatas (simplificado por ahora)
    int threats = 0;
    
    // Contar patrones de 3 con extremos libres (free-threes)
    threats += countPatterns(player, 3) * 50;
    
    // Penalizar si el oponente está cerca de ganar por capturas
    int opponent = (player == 1) ? 2 : 1;
    int opponentCaptures = gameState.getCaptures(opponent);
    if (opponentCaptures >= 8) {
        threats -= 500 * (opponentCaptures - 7);
    }
    
    return threats;
}

// === MÉTODOS AUXILIARES ===

void GameNode::orderMoves(std::vector<Move>& moves) const {
    // Ordenamiento simple: movimientos cerca del centro primero
    int center = gameState.getSize() / 2;
    
    std::sort(moves.begin(), moves.end(), [center](const Move& a, const Move& b) {
        int distA = abs(a.x - center) + abs(a.y - center);
        int distB = abs(b.x - center) + abs(b.y - center);
        return distA < distB;
    });
}

// === DEBUG ===

void GameNode::printNodeInfo() const {
    std::cout << "=== INFO DEL NODO ===\n";
    std::cout << "Profundidad: " << depth << std::endl;
    std::cout << "Jugador: " << currentPlayer << std::endl;
    std::cout << "Movimiento: (" << moveToHere.x << "," << moveToHere.y << ")" << std::endl;
    std::cout << "Evaluación: " << (isEvaluated ? std::to_string(evaluation) : "No evaluado") << std::endl;
    std::cout << "Hijos: " << children.size() << std::endl;
    std::cout << "Terminal: " << (isGameOver() ? "Sí" : "No") << std::endl;
}

GameNode::TreeStats GameNode::getTreeStats() const {
    TreeStats stats = {0, 0, 0, 0};
    collectStats(stats);
    return stats;
}

void GameNode::collectStats(TreeStats& stats) const {
    stats.totalNodes++;
    stats.maxDepthReached = std::max(stats.maxDepthReached, depth);
    
    if (isEvaluated) {
        stats.totalEvaluations++;
    }
    
    if (children.empty()) {
        stats.leafNodes++;
    } else {
        for (const auto& child : children) {
            child->collectStats(stats);
        }
    }
}