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
        
        // Ordenar hijos por evaluación heurística para mejor poda
        std::vector<std::pair<int, int>> childEvals;
        for (size_t i = 0; i < children.size(); ++i) {
            int heuristicEval = children[i]->evaluateNode();
            childEvals.push_back({heuristicEval, i});
        }
        std::sort(childEvals.rbegin(), childEvals.rend()); // Orden descendente
        
        for (const auto& childPair : childEvals) {
            int childIndex = childPair.second;
            int eval = children[childIndex]->minimax(maxDepth - 1, false, alpha, beta);
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
        
        // Ordenar hijos por evaluación heurística para mejor poda
        std::vector<std::pair<int, int>> childEvals;
        for (size_t i = 0; i < children.size(); ++i) {
            int heuristicEval = children[i]->evaluateNode();
            childEvals.push_back({heuristicEval, i});
        }
        std::sort(childEvals.begin(), childEvals.end()); // Orden ascendente
        
        for (const auto& childPair : childEvals) {
            int childIndex = childPair.second;
            int eval = children[childIndex]->minimax(maxDepth - 1, true, alpha, beta);
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
    // Limitar la profundidad para evitar explosión combinatoria
    searchDepth = std::min(searchDepth, 10); // Máximo 6 niveles de profundidad
    
    std::cout << "=== INICIANDO BÚSQUEDA ===\n";
    std::cout << "Profundidad limitada: " << searchDepth << std::endl;
    std::cout << "Jugador actual: " << currentPlayer << std::endl;
    
    // Generar hijos
    generateChildren();
    
    if (children.empty()) {
        std::cout << "ERROR: No hay movimientos válidos!\n";
        return Move(-1, -1, 0);
    }
    
    std::cout << "Movimientos candidatos: " << children.size() << std::endl;
    
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
    std::set<std::pair<int, int>> candidates;
    
    // Si no hay piezas, empezar por el centro
    bool boardEmpty = true;
    for (int i = 0; i < gameState.getSize() && boardEmpty; ++i) {
        for (int j = 0; j < gameState.getSize() && boardEmpty; ++j) {
            if (gameState.getPiece(i, j) != 0) {
                boardEmpty = false;
            }
        }
    }
    
    if (boardEmpty) {
        int center = gameState.getSize() / 2;
        moves.emplace_back(center, center);
        return moves;
    }
    
    // 1. Buscar movimientos ganadores inmediatos (máxima prioridad)
    for (int i = 0; i < gameState.getSize(); ++i) {
        for (int j = 0; j < gameState.getSize(); ++j) {
            if (gameState.isEmpty(i, j)) {
                // Simular el movimiento
                Board testState = gameState;
                if (testState.placePiece(i, j, currentPlayer)) {
                    if (testState.checkWin(currentPlayer)) {
                        moves.clear();
                        moves.emplace_back(i, j);
                        return moves; // Si hay movimiento ganador, solo considerar ese
                    }
                }
            }
        }
    }
    
    // 1.5. Buscar movimientos de captura inmediata (alta prioridad)
    std::set<std::pair<int, int>> captureOptions;
    int opponent = (currentPlayer == 1) ? 2 : 1;
    
    for (int i = 0; i < gameState.getSize(); ++i) {
        for (int j = 0; j < gameState.getSize(); ++j) {
            if (gameState.isEmpty(i, j)) {
                // Verificar si colocar aquí permite capturar piezas enemigas
                bool canCapture = false;
                int directions[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};
                
                for (int d = 0; d < 4 && !canCapture; ++d) {
                    int dx = directions[d][0];
                    int dy = directions[d][1];
                    
                    // Buscar patrón X_YYZ donde X=currentPlayer, YY=oponente, Z=currentPlayer
                    int x1 = i + dx, y1 = j + dy;        // Primera pieza oponente
                    int x2 = i + 2*dx, y2 = j + 2*dy;    // Segunda pieza oponente  
                    int x3 = i + 3*dx, y3 = j + 3*dy;    // Pieza aliada cerrando
                    
                    if (gameState.isValid(x1, y1) && gameState.isValid(x2, y2) && gameState.isValid(x3, y3)) {
                        if (gameState.getPiece(x1, y1) == opponent && 
                            gameState.getPiece(x2, y2) == opponent && 
                            gameState.getPiece(x3, y3) == currentPlayer) {
                            canCapture = true;
                        }
                    }
                    
                    // También buscar el patrón inverso Z_YYX
                    int xb1 = i - dx, yb1 = j - dy;      // Primera pieza oponente
                    int xb2 = i - 2*dx, yb2 = j - 2*dy;  // Segunda pieza oponente
                    int xb3 = i - 3*dx, yb3 = j - 3*dy;  // Pieza aliada cerrando
                    
                    if (gameState.isValid(xb1, yb1) && gameState.isValid(xb2, yb2) && gameState.isValid(xb3, yb3)) {
                        if (gameState.getPiece(xb1, yb1) == opponent && 
                            gameState.getPiece(xb2, yb2) == opponent && 
                            gameState.getPiece(xb3, yb3) == currentPlayer) {
                            canCapture = true;
                        }
                    }
                }
                
                if (canCapture) {
                    captureOptions.insert({i, j});
                }
            }
        }
    }
    
    // Si hay capturas disponibles, priorizarlas
    if (!captureOptions.empty()) {
        for (const auto& pos : captureOptions) {
            candidates.insert(pos);
        }
    }
    
    // 2. Buscar movimientos que bloqueen amenazas críticas del oponente (3+ en línea)
    std::set<std::pair<int, int>> defenseOptions;
    
    // 2.1 Primero bloquear amenazas de 4 (victoria inmediata)
    for (int i = 0; i < gameState.getSize(); ++i) {
        for (int j = 0; j < gameState.getSize(); ++j) {
            if (gameState.isEmpty(i, j)) {
                Board testState = gameState;
                if (testState.placePiece(i, j, opponent)) {
                    if (testState.checkWin(opponent)) {
                        defenseOptions.insert({i, j});
                    }
                }
            }
        }
    }
    
    // 2.2 Si no hay amenazas inmediatas, buscar amenazas de 3 en línea con extremos libres
    if (defenseOptions.empty()) {
        for (int i = 0; i < gameState.getSize(); ++i) {
            for (int j = 0; j < gameState.getSize(); ++j) {
                if (gameState.isEmpty(i, j)) {
                    // Verificar si colocar aquí bloquea una amenaza de 3 en línea
                    bool blocksThree = false;
                    int directions[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};
                    
                    for (int d = 0; d < 4 && !blocksThree; ++d) {
                        int dx = directions[d][0];
                        int dy = directions[d][1];
                        
                        // Buscar patrones como _OOO_ donde colocar pieza bloquea la amenaza
                        // Verificar hacia una dirección
                        int consecutiveOpponent = 0;
                        int x = i + dx, y = j + dy;
                        
                        // Contar piezas consecutivas del oponente hacia adelante
                        while (gameState.isValid(x, y) && gameState.getPiece(x, y) == opponent) {
                            consecutiveOpponent++;
                            x += dx;
                            y += dy;
                        }
                        
                        // Si hay al menos 3 consecutivas y el extremo está libre
                        if (consecutiveOpponent >= 3 && gameState.isValid(x, y) && gameState.isEmpty(x, y)) {
                            blocksThree = true;
                        }
                        
                        // También verificar hacia la dirección opuesta
                        if (!blocksThree) {
                            consecutiveOpponent = 0;
                            x = i - dx, y = j - dy;
                            
                            // Contar piezas consecutivas del oponente hacia atrás
                            while (gameState.isValid(x, y) && gameState.getPiece(x, y) == opponent) {
                                consecutiveOpponent++;
                                x -= dx;
                                y -= dy;
                            }
                            
                            // Si hay al menos 3 consecutivas y el extremo está libre
                            if (consecutiveOpponent >= 3 && gameState.isValid(x, y) && gameState.isEmpty(x, y)) {
                                blocksThree = true;
                            }
                        }
                        
                        // Verificar patrones más complejos: O_OO, OO_O (3 con un hueco)
                        if (!blocksThree) {
                            // Patrón hacia adelante: verificar si colocar aquí completa/bloquea una secuencia peligrosa
                            int x1 = i + dx, y1 = j + dy;
                            int x2 = i + 2*dx, y2 = j + 2*dy; 
                            int x3 = i + 3*dx, y3 = j + 3*dy;
                            int x4 = i + 4*dx, y4 = j + 4*dy;
                            
                            if (gameState.isValid(x1, y1) && gameState.isValid(x2, y2) && 
                                gameState.isValid(x3, y3) && gameState.isValid(x4, y4)) {
                                
                                // Patrón O_OO_ o OO_O_ 
                                if ((gameState.getPiece(x1, y1) == opponent && gameState.isEmpty(x2, y2) && 
                                     gameState.getPiece(x3, y3) == opponent && gameState.getPiece(x4, y4) == opponent) ||
                                    (gameState.getPiece(x1, y1) == opponent && gameState.getPiece(x2, y2) == opponent && 
                                     gameState.isEmpty(x3, y3) && gameState.getPiece(x4, y4) == opponent)) {
                                    blocksThree = true;
                                }
                            }
                        }
                    }
                    
                    if (blocksThree) {
                        defenseOptions.insert({i, j});
                    }
                }
            }
        }
    }
    
    // Añadir opciones de defensa a candidatos
    for (const auto& pos : defenseOptions) {
        candidates.insert(pos);
    }
    // 3. Si no hay movimientos críticos, buscar posiciones estratégicas
    if (candidates.empty()) {
        // Solo considerar posiciones adyacentes a piezas existentes (radio 1)
        for (int i = 0; i < gameState.getSize(); ++i) {
            for (int j = 0; j < gameState.getSize(); ++j) {
                if (gameState.getPiece(i, j) != 0) {
                    // Añadir posiciones adyacentes
                    for (int di = -1; di <= 1; ++di) {
                        for (int dj = -1; dj <= 1; ++dj) {
                            if (di == 0 && dj == 0) continue;
                            
                            int ni = i + di;
                            int nj = j + dj;
                            
                            if (gameState.isValid(ni, nj) && gameState.isEmpty(ni, nj)) {
                                // Verificar si este movimiento NO crea patrones vulnerables
                                bool safeMove = true;
                                int opponent = (currentPlayer == 1) ? 2 : 1;
                                int directions[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};
                                
                                // Verificar en todas las direcciones si crear este movimiento es peligroso
                                for (int d = 0; d < 4 && safeMove; ++d) {
                                    int dx = directions[d][0];
                                    int dy = directions[d][1];
                                    
                                    // Verificar si crearía el patrón _XX_ (vulnerable)
                                    int left = ni - dx, left_y = nj - dy;
                                    int right = ni + dx, right_y = nj + dy;
                                    int far_left = ni - 2*dx, far_left_y = nj - 2*dy;
                                    int far_right = ni + 2*dx, far_right_y = nj + 2*dy;
                                    
                                    // Patrón _XX_: colocar pieza crearía dos piezas seguidas con extremos libres
                                    if (gameState.isValid(left, left_y) && gameState.isValid(far_left, far_left_y) &&
                                        gameState.isValid(right, right_y) && gameState.isValid(far_right, far_right_y)) {
                                        
                                        // Si hay una pieza aliada a la derecha y extremos libres
                                        if (gameState.getPiece(right, right_y) == currentPlayer && 
                                            gameState.isEmpty(far_left, far_left_y) && gameState.isEmpty(far_right, far_right_y)) {
                                            
                                            // Verificar si hay oponente que pueda amenazar
                                            int threat_left = ni - 3*dx, threat_left_y = nj - 3*dy;
                                            int threat_right = ni + 3*dx, threat_right_y = nj + 3*dy;
                                            
                                            bool hasLeftThreat = gameState.isValid(threat_left, threat_left_y) && 
                                                               gameState.getPiece(threat_left, threat_left_y) == opponent;
                                            bool hasRightThreat = gameState.isValid(threat_right, threat_right_y) && 
                                                                gameState.getPiece(threat_right, threat_right_y) == opponent;
                                            
                                            if (hasLeftThreat || hasRightThreat) {
                                                safeMove = false; // Muy peligroso
                                            }
                                        }
                                        
                                        // También verificar el patrón inverso (pieza aliada a la izquierda)
                                        if (gameState.getPiece(left, left_y) == currentPlayer && 
                                            gameState.isEmpty(far_left, far_left_y) && gameState.isEmpty(far_right, far_right_y)) {
                                            
                                            int threat_left = ni - 3*dx, threat_left_y = nj - 3*dy;
                                            int threat_right = ni + 3*dx, threat_right_y = nj + 3*dy;
                                            
                                            bool hasLeftThreat = gameState.isValid(threat_left, threat_left_y) && 
                                                               gameState.getPiece(threat_left, threat_left_y) == opponent;
                                            bool hasRightThreat = gameState.isValid(threat_right, threat_right_y) && 
                                                                gameState.getPiece(threat_right, threat_right_y) == opponent;
                                            
                                            if (hasLeftThreat || hasRightThreat) {
                                                safeMove = false;
                                            }
                                        }
                                    }
                                }
                                
                                if (safeMove) {
                                    candidates.insert({ni, nj});
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    // Convertir a vector y limitar severamente el número de candidatos
    for (const auto& pos : candidates) {
        moves.emplace_back(pos.first, pos.second);
        if (moves.size() >= 8) break; // LÍMITE MUY RESTRICTIVO
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
	if (countPatterns(2, 2) != 0)
	{
		aiScore += countPatterns(2, 2) * 10;      // 2 en línea
		if (countPatterns(2, 3) != 0)
		{
			aiScore += countPatterns(2, 4) * 1000;    // 4 en línea
			aiScore += countPatterns(2, 3) * 100;     // 3 en línea
		}
	}
	if (countPatterns(1, 2) != 0)
	{
		humanScore += countPatterns(1, 2) * 10;
		if (countPatterns(1, 3) != 0)
		{
			humanScore += countPatterns(1, 4) * 1000;
			humanScore += countPatterns(1, 3) * 100;
		}
		
	}
    // Evaluar capturas
    aiScore += gameState.getCaptures(2) * 50;
    humanScore += gameState.getCaptures(1) * 50;
    
    // Evaluar amenazas (ya incluye exposición a capturas)
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
    // Evaluar amenazas inmediatas (mejorado)
    int threats = 0;
    int opponent = (player == 1) ? 2 : 1;
    
    // 1. AMENAZAS DE VICTORIA: Contar patrones de 4 en línea
    int fourInRow = countPatterns(player, 4);
    threats += fourInRow * 5000; // Muy alto para amenazas de victoria
    
    // 2. AMENAZAS SERIAS: Contar patrones de 3 en línea con extremos libres
    int threeInRowFree = 0;
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
                        
                        // Contar longitud de la línea
                        while (gameState.isValid(x, y) && gameState.getPiece(x, y) == player) {
                            lineLength++;
                            x += dx;
                            y += dy;
                        }
                        
                        // Si tenemos exactamente 3 en línea
                        if (lineLength == 3) {
                            // Verificar si ambos extremos están libres
                            int startX = prevX, startY = prevY; // Un espacio antes del inicio
                            int endX = x, endY = y; // Un espacio después del final
                            
                            bool startFree = gameState.isValid(startX, startY) && 
                                           gameState.isEmpty(startX, startY);
                            bool endFree = gameState.isValid(endX, endY) && 
                                         gameState.isEmpty(endX, endY);
                            
                            if (startFree && endFree) {
                                threeInRowFree++; // Es un "free three" (3 en línea con ambos extremos libres)
                            }
                        }
                    }
                }
            }
        }
    }
    
    threats += threeInRowFree * 200; // Alto para amenazas serias
    
    // 3. AMENAZAS DEL OPONENTE: Penalizar si el oponente tiene amenazas
    int opponentFours = countPatterns(opponent, 4);
    threats -= opponentFours * 6000; // Penalización muy alta
    
    int opponentThreesFree = 0;
    for (int i = 0; i < gameState.getSize(); ++i) {
        for (int j = 0; j < gameState.getSize(); ++j) {
            if (gameState.getPiece(i, j) == opponent) {
                for (int d = 0; d < 4; ++d) {
                    int dx = directions[d][0];
                    int dy = directions[d][1];
                    
                    // Solo contar desde el inicio de cada línea
                    int prevX = i - dx;
                    int prevY = j - dy;
                    bool isStart = !gameState.isValid(prevX, prevY) || 
                                  gameState.getPiece(prevX, prevY) != opponent;
                    
                    if (isStart) {
                        int lineLength = 1;
                        int x = i + dx, y = j + dy;
                        
                        // Contar longitud de la línea
                        while (gameState.isValid(x, y) && gameState.getPiece(x, y) == opponent) {
                            lineLength++;
                            x += dx;
                            y += dy;
                        }
                        
                        // Si tenemos exactamente 3 en línea
                        if (lineLength == 3) {
                            // Verificar si ambos extremos están libres
                            int startX = prevX, startY = prevY; // Un espacio antes del inicio
                            int endX = x, endY = y; // Un espacio después del final
                            
                            bool startFree = gameState.isValid(startX, startY) && 
                                           gameState.isEmpty(startX, startY);
                            bool endFree = gameState.isValid(endX, endY) && 
                                         gameState.isEmpty(endX, endY);
                            
                            if (startFree && endFree) {
                                opponentThreesFree++; // Es un "free three" del oponente
                            }
                        }
                    }
                }
            }
        }
    }
    
    threats -= opponentThreesFree * 250; // Penalización alta
    
    // 4. Contar patrones básicos de 3 y 2 con bonificaciones menores
    threats += countPatterns(player, 3) * 50;
    threats += countPatterns(player, 2) * 10;
    
    // 5. Penalizar si el oponente está cerca de ganar por capturas
    int opponentCaptures = gameState.getCaptures(opponent);
    if (opponentCaptures >= 8) {
        threats -= 500 * (opponentCaptures - 7);
    }
    
    // NUEVA: Bonificar oportunidades de captura disponibles
    int captureOpportunities = 0;
    for (int i = 0; i < gameState.getSize(); ++i) {
        for (int j = 0; j < gameState.getSize(); ++j) {
            if (gameState.isEmpty(i, j)) {
                // Verificar si desde aquí se puede capturar
                int directions[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};
                
                for (int d = 0; d < 4; ++d) {
                    int dx = directions[d][0];
                    int dy = directions[d][1];
                    
                    // Patrón X_OOP donde X=player, OO=opponent, P=player
                    int x1 = i + dx, y1 = j + dy;        // Primera pieza oponente
                    int x2 = i + 2*dx, y2 = j + 2*dy;    // Segunda pieza oponente  
                    int x3 = i + 3*dx, y3 = j + 3*dy;    // Pieza aliada cerrando
                    
                    if (gameState.isValid(x1, y1) && gameState.isValid(x2, y2) && gameState.isValid(x3, y3)) {
                        if (gameState.getPiece(x1, y1) == opponent && 
                            gameState.getPiece(x2, y2) == opponent && 
                            gameState.getPiece(x3, y3) == player) {
                            captureOpportunities++;
                        }
                    }
                    
                    // Patrón P_OOX (inverso)
                    int xb1 = i - dx, yb1 = j - dy;      // Primera pieza oponente
                    int xb2 = i - 2*dx, yb2 = j - 2*dy;  // Segunda pieza oponente
                    int xb3 = i - 3*dx, yb3 = j - 3*dy;  // Pieza aliada cerrando
                    
                    if (gameState.isValid(xb1, yb1) && gameState.isValid(xb2, yb2) && gameState.isValid(xb3, yb3)) {
                        if (gameState.getPiece(xb1, yb1) == opponent && 
                            gameState.getPiece(xb2, yb2) == opponent && 
                            gameState.getPiece(xb3, yb3) == player) {
                            captureOpportunities++;
                        }
                    }
                }
            }
        }
    }
    
    threats += captureOpportunities * 40; // Bonificar oportunidades de captura
    
    // NUEVA: Penalizar si el oponente tiene oportunidades de captura contra nosotros
    int opponentCaptureThreats = 0;
    for (int i = 0; i < gameState.getSize(); ++i) {
        for (int j = 0; j < gameState.getSize(); ++j) {
            if (gameState.isEmpty(i, j)) {
                // Verificar si el oponente podría capturar desde aquí
                int directions2[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};
                
                for (int d = 0; d < 4; ++d) {
                    int dx = directions2[d][0];
                    int dy = directions2[d][1];
                    
                    // Patrón O_PPX donde O=opponent, PP=player, X=opponent
                    int x1 = i + dx, y1 = j + dy;        // Primera pieza nuestra
                    int x2 = i + 2*dx, y2 = j + 2*dy;    // Segunda pieza nuestra 
                    int x3 = i + 3*dx, y3 = j + 3*dy;    // Pieza oponente cerrando
                    
                    if (gameState.isValid(x1, y1) && gameState.isValid(x2, y2) && gameState.isValid(x3, y3)) {
                        if (gameState.getPiece(x1, y1) == player && 
                            gameState.getPiece(x2, y2) == player && 
                            gameState.getPiece(x3, y3) == opponent) {
                            opponentCaptureThreats++;
                        }
                    }
                    
                    // Patrón X_PPO (inverso)
                    int xb1 = i - dx, yb1 = j - dy;      // Primera pieza nuestra
                    int xb2 = i - 2*dx, yb2 = j - 2*dy;  // Segunda pieza nuestra
                    int xb3 = i - 3*dx, yb3 = j - 3*dy;  // Pieza oponente cerrando
                    
                    if (gameState.isValid(xb1, yb1) && gameState.isValid(xb2, yb2) && gameState.isValid(xb3, yb3)) {
                        if (gameState.getPiece(xb1, yb1) == player && 
                            gameState.getPiece(xb2, yb2) == player && 
                            gameState.getPiece(xb3, yb3) == opponent) {
                            opponentCaptureThreats++;
                        }
                    }
                }
            }
        }
    }
    
    threats -= opponentCaptureThreats * 60; // Penalización por amenazas de captura del oponente
    
    // Evaluar exposición a capturas (integrado aquí)
    int exposurePenalty = 0;
        
    for (int i = 0; i < gameState.getSize(); ++i) {
        for (int j = 0; j < gameState.getSize(); ++j) {
            if (gameState.getPiece(i, j) == player) {
                // Verificar si esta pieza está expuesta a captura
                for (int d = 0; d < 4; ++d) {
                    int dx = directions[d][0];
                    int dy = directions[d][1];
                    
                    // Verificar patrón hacia adelante: XY_O (pieza actual, pieza aliada, vacío, oponente)
                    int x1 = i + dx, y1 = j + dy;          // Posición de pieza aliada
                    int x2 = i + 2*dx, y2 = j + 2*dy;      // Posición vacía
                    int x3 = i + 3*dx, y3 = j + 3*dy;      // Posición del oponente
                    
                    if (gameState.isValid(x1, y1) && gameState.isValid(x2, y2) && gameState.isValid(x3, y3)) {
                        if (gameState.getPiece(x1, y1) == player && 
                            gameState.isEmpty(x2, y2) && 
                            gameState.getPiece(x3, y3) == opponent) {
                            exposurePenalty++;
                        }
                    }
                    
                    // Verificar patrón hacia atrás: O_YX (oponente, vacío, pieza aliada, pieza actual)
                    int xb1 = i - dx, yb1 = j - dy;        // Posición de pieza aliada
                    int xb2 = i - 2*dx, yb2 = j - 2*dy;    // Posición vacía
                    int xb3 = i - 3*dx, yb3 = j - 3*dy;    // Posición del oponente
                    
                    if (gameState.isValid(xb1, yb1) && gameState.isValid(xb2, yb2) && gameState.isValid(xb3, yb3)) {
                        if (gameState.getPiece(xb1, yb1) == player && 
                            gameState.isEmpty(xb2, yb2) && 
                            gameState.getPiece(xb3, yb3) == opponent) {
                            exposurePenalty++;
                        }
                    }
                    
                    // Verificar si la pieza está en medio de un patrón O_X_O (muy peligroso)
                    int xl = i - dx, yl = j - dy;          // Izquierda
                    int xr = i + dx, yr = j + dy;          // Derecha
                    int xll = i - 2*dx, yll = j - 2*dy;    // Más a la izquierda
                    int xrr = i + 2*dx, yrr = j + 2*dy;    // Más a la derecha
                    
                    if (gameState.isValid(xl, yl) && gameState.isValid(xr, yr) && 
                        gameState.isValid(xll, yll) && gameState.isValid(xrr, yrr)) {
                        // Patrón O_X_O (pieza completamente rodeada)
                        if (gameState.isEmpty(xl, yl) && gameState.isEmpty(xr, yr) &&
                            gameState.getPiece(xll, yll) == opponent && gameState.getPiece(xrr, yrr) == opponent) {
                            exposurePenalty += 3; // Muy peligroso, se puede capturar desde ambos lados
                        }
                        
                        // Patrón _XX_ (dos piezas juntas con extremos libres) - vulnerable a OXX_ o _XXO
                        if (gameState.getPiece(xr, yr) == player &&  // Pieza aliada a la derecha
                            gameState.isEmpty(xl, yl) && gameState.isEmpty(xrr, yrr)) {  // Extremos libres
                            // Verificar si hay oponente que pueda amenazar desde cualquier lado
                            int x_threat1 = i - 2*dx, y_threat1 = j - 2*dy;  // Amenaza desde la izquierda
                            int x_threat2 = i + 3*dx, y_threat2 = j + 3*dy;   // Amenaza desde la derecha
                            
                            bool leftThreat = gameState.isValid(x_threat1, y_threat1) && 
                                            gameState.getPiece(x_threat1, y_threat1) == opponent;
                            bool rightThreat = gameState.isValid(x_threat2, y_threat2) && 
                                             gameState.getPiece(x_threat2, y_threat2) == opponent;
                            
                            if (leftThreat || rightThreat) {
                                exposurePenalty += 2; // Peligroso: pareja vulnerable con oponente cerca
                            } else {
                                exposurePenalty += 1; // Menos peligroso pero aún vulnerable
                            }
                        }
                    }
                    
                    // Verificar patrón _X_ (pieza solitaria con extremos libres) - puede ser vulnerable
                    if (gameState.isValid(xl, yl) && gameState.isValid(xr, yr)) {
                        if (gameState.isEmpty(xl, yl) && gameState.isEmpty(xr, yr)) {
                            // Verificar si hay oponentes que puedan amenazar
                            int x_threat_l = i - 2*dx, y_threat_l = j - 2*dy;
                            int x_threat_r = i + 2*dx, y_threat_r = j + 2*dy;
                            
                            bool leftThreat = gameState.isValid(x_threat_l, y_threat_l) && 
                                            gameState.getPiece(x_threat_l, y_threat_l) == opponent;
                            bool rightThreat = gameState.isValid(x_threat_r, y_threat_r) && 
                                             gameState.getPiece(x_threat_r, y_threat_r) == opponent;
                            
                            if (leftThreat && rightThreat) {
                                exposurePenalty += 2; // Muy vulnerable: oponentes en ambos lados
                            } else if (leftThreat || rightThreat) {
                                exposurePenalty += 1; // Vulnerable: oponente en un lado
                            }
                        }
                    }
                }
            }
        }
    }
    
    threats -= exposurePenalty * 25; // Penalización por exposición
    
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