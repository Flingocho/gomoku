/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   game_node.hpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/21 16:35:04 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/03 18:03:35 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef GAME_NODE_HPP
#define GAME_NODE_HPP

#include "board.hpp"
#include <vector>

// Estructura Move - ya definida en ai.hpp pero necesitamos aquí también
struct Move {
    int x, y;
    int score;
    
    Move(int x = -1, int y = -1, int score = 0) : x(x), y(y), score(score) {}
    
    bool isValid() const { return x >= 0 && y >= 0; }
};

// Clase GameNode simplificada - esqueleto para nuevo algoritmo
class GameNode {
public:
    // Estructura para estadísticas
    struct TreeStats {
        int totalNodes;
        int totalEvaluations;  
        int maxDepthReached;
    };

private:
    Board board;
    int currentPlayer;
    int score;

public:
    // Constructor
    GameNode(const Board& board, int player);
    
    // Destructor
    ~GameNode();
    
    // Función principal que debe implementarse
    Move getBestMove(int maxDepth);
    
    // Función para obtener estadísticas
    TreeStats getTreeStats() const;
    
    // TODO: Agregar nuevas funciones del algoritmo aquí
};

#endif

#ifndef GAME_NODE_HPP
#define GAME_NODE_HPP

#include "board.hpp"
#include "time_manager.hpp"
#include <vector>
#include <memory>
#include <limits>

class TimeManager; // Forward declaration

struct Move {
    int x, y;
    int score;
    
    Move(int x = -1, int y = -1, int score = 0) : x(x), y(y), score(score) {}
    
    bool isValid() const { return x >= 0 && y >= 0; }
};

class GameNode {
private:
    Board gameState;                               // Estado del tablero en este nodo
    Move moveToHere;                              // Movimiento que llevó a este estado
    int currentPlayer;                            // Jugador que debe mover (1 o 2)
    int depth;                                    // Profundidad en el árbol (0 = raíz)
    int evaluation;                               // Valor heurístico calculado
    bool isEvaluated;                            // ¿Ya se calculó la evaluación?
    
    std::vector<std::unique_ptr<GameNode>> children; // Nodos hijos
    GameNode* parent;                             // Nodo padre (raw pointer para evitar ciclos)
    
    // Para optimizaciones
    mutable bool childrenGenerated;               // ¿Ya se generaron los hijos?
    int alpha, beta;                             // Valores para poda alpha-beta

public:
    // === CONSTRUCTORES ===
    
    // Constructor para nodo raíz
    GameNode(const Board& initialState, int player);
    
    // Constructor para nodo hijo
    GameNode(const Board& parentState, const Move& move, int player, int nodeDepth, GameNode* parentNode);
    
    // Destructor (los unique_ptr se encargan de la limpieza automática)
    ~GameNode() = default;
    
    // === GETTERS ===
    
    const Board& getGameState() const { return gameState; }
    const Move& getMove() const { return moveToHere; }
    int getPlayer() const { return currentPlayer; }
    int getDepth() const { return depth; }
    int getEvaluation() const { return evaluation; }
    bool hasEvaluation() const { return isEvaluated; }
    GameNode* getParent() const { return parent; }
    
    size_t getChildrenCount() const { return children.size(); }
    const GameNode* getChild(size_t index) const;
    
    // === GENERACIÓN DEL ÁRBOL ===
    
    // Genera todos los movimientos válidos como nodos hijos
    void generateChildren();
    
    // Verifica si este nodo es terminal (fin de juego o profundidad máxima)
    bool isTerminal(int maxDepth) const;
    
    // Verifica si el juego ha terminado en este estado
    bool isGameOver() const;
    
    // === EVALUACIÓN ===
    
    // Evalúa este nodo usando la función heurística
    int evaluateNode();
    
    // Fuerza una evaluación específica (para nodos terminales)
    void setEvaluation(int value);
    
    // === MINIMAX ===
	int minimaxWithTime(int maxDepth, bool isMaximizingPlayer, int alpha, int beta, TimeManager& timer);
    
    // Ejecuta minimax desde este nodo
    int minimax(int maxDepth, bool isMaximizingPlayer, int alpha = std::numeric_limits<int>::min(), 
               int beta = std::numeric_limits<int>::max());
    
    // Encuentra el mejor movimiento desde este nodo
    Move getBestMove(int searchDepth);
    
    // === UTILIDADES ===
    
    // Genera lista de movimientos candidatos (sin crear nodos)
    std::vector<Move> generateCandidateMoves() const;
    
    // Aplica un movimiento al estado del tablero y devuelve el nuevo estado
    Board applyMove(const Move& move) const;
    
    // Verifica si un movimiento es válido en este estado
    bool isValidMove(const Move& move) const;
    
    // === DEBUG ===
    
    // Imprime información del nodo
    void printNodeInfo() const;
    
    // Imprime el árbol completo (cuidado con la profundidad)
    void printTree(int maxPrintDepth = 3) const;
    
    // Obtiene estadísticas del árbol
    struct TreeStats {
        int totalNodes;
        int leafNodes;
        int maxDepthReached;
        int totalEvaluations;
    };
    
    TreeStats getTreeStats() const;

private:
    // === MÉTODOS AUXILIARES ===
    
    // Función heurística específica para Gomoku
    int evaluateGomokuPosition() const;
    
    // Detecta patrones específicos (2, 3, 4 en línea)
    int countPatterns(int player, int patternLength) const;
    
    // Evalúa amenazas inmediatas
    int evaluateThreats(int player) const;
    
    // Evalúa valor posicional
    int evaluatePositional(int player) const;
    
    // Optimización: ordena movimientos por prometedores
    void orderMoves(std::vector<Move>& moves) const;
    
    // Implementación recursiva para estadísticas
    void collectStats(TreeStats& stats) const;
    
    // Implementación recursiva para imprimir árbol
    void printTreeRecursive(int currentDepth, int maxDepth, const std::string& prefix) const;
};

#endif