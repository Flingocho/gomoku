#include "../include/ai.hpp"
#include <algorithm>
#include <climits>
#include <iostream>
#include <cmath>

AI::AI(int player, int depth) : aiPlayer(player), maxDepth(depth) {
    humanPlayer = (player == 1) ? 2 : 1;
}

Move AI::getBestMove(Board& board) {
    std::cout << "AI analyzing with depth " << maxDepth << "..." << std::endl;
    
    // Mostrar estado de capturas antes del movimiento
    std::cout << "Capturas actuales - AI: " << board.getCaptures(aiPlayer) 
              << ", Humano: " << board.getCaptures(humanPlayer) << std::endl;
    
    // PASO 1: ¿Puedo ganar inmediatamente?
    Move winMove = findImmediateWin(board, aiPlayer);
    if (winMove.x != -1) {
        std::cout << "Found WINNING move!" << std::endl;
        return winMove;
    }
    
    // PASO 2: ¿Debo bloquear una derrota inmediata?
    Move blockMove = findImmediateWin(board, humanPlayer);
    if (blockMove.x != -1) {
        std::cout << "Blocking opponent's WINNING move!" << std::endl;
        return blockMove;
    }
    
    // PASO 2.5: ¿Debo bloquear un patrón de 4 del oponente?
    Move blockFour = findCriticalBlock(board, humanPlayer, 4);
    if (blockFour.x != -1) {
        std::cout << "Blocking opponent's 4-pattern!" << std::endl;
        return blockFour;
    }
    
    // PASO 2.7: ¿Debo bloquear un patrón de 3 del oponente?
    Move blockThree = findCriticalBlock(board, humanPlayer, 3);
    if (blockThree.x != -1) {
        std::cout << "Blocking opponent's 3-pattern!" << std::endl;
        return blockThree;
    }
    
    // PASO 3: Búsqueda normal con minimax
    auto candidates = generateMoves(board);
    std::cout << "Generated " << candidates.size() << " candidate moves" << std::endl;
    
    // Mostrar patrones actuales antes de evaluar movimientos
    std::cout << "\n--- ESTADO ACTUAL DEL TABLERO ---" << std::endl;
    debugPatterns(board, aiPlayer);
    debugPatterns(board, humanPlayer);
    std::cout << "--- FIN ESTADO ACTUAL ---\n" << std::endl;
    
    if (candidates.empty()) {
        return Move(9, 9, 0);
    }
    
    Move bestMove = candidates[0];
    int bestScore = INT_MIN;
    
    for (auto& move : candidates) {
        if (board.placePieceSimple(move.x, move.y, aiPlayer)) {
            int score = minimax(board, maxDepth - 1, false, INT_MIN, INT_MAX);
            board.removePiece(move.x, move.y);
            
            if (score > bestScore) {
                bestScore = score;
                bestMove = move;
                bestMove.score = score;
            }
            
            std::cout << "Move (" << move.x << "," << move.y << ") -> Score: " << score;
            
            // Información adicional de depuración
            board.placePieceSimple(move.x, move.y, aiPlayer);
            int myEval = evaluatePlayer(board, aiPlayer);
            int oppEval = evaluatePlayer(board, humanPlayer);
            int myPatterns3 = countPatterns(board, aiPlayer, 3);
            int myPatterns4 = countPatterns(board, aiPlayer, 4);
            int oppPatterns3 = countPatterns(board, humanPlayer, 3);
            int oppPatterns4 = countPatterns(board, humanPlayer, 4);
            board.removePiece(move.x, move.y);
            
            std::cout << " [My: " << myEval << ", Opp: " << oppEval << ", Diff: " << (myEval - oppEval) 
                      << ", My3: " << myPatterns3 << ", My4: " << myPatterns4 
                      << ", Opp3: " << oppPatterns3 << ", Opp4: " << oppPatterns4 << "]";
            
            // Mostrar también patrones de 2 y capturas para más información
            board.placePieceSimple(move.x, move.y, aiPlayer);
            int myPatterns2 = countPatterns(board, aiPlayer, 2);
            int oppPatterns2 = countPatterns(board, humanPlayer, 2);
            board.removePiece(move.x, move.y);
            
            std::cout << " [My2: " << myPatterns2 << ", Opp2: " << oppPatterns2 
                      << ", MyCap: " << board.getCaptures(aiPlayer) 
                      << ", OppCap: " << board.getCaptures(humanPlayer) << "]" << std::endl;
        }
    }
    
    std::cout << "Best move: (" << bestMove.x << "," << bestMove.y << ") with score " << bestScore << std::endl;
    return bestMove;
}

int AI::minimax(Board& board, int depth, bool isMaximizing, int alpha, int beta) {
    // Casos base
    if (depth == 0) {
        return evaluatePosition(board);
    }
    
    // Verificar fin de juego
    if (board.checkWin(aiPlayer)) return 10000 + depth;
    if (board.checkWin(humanPlayer)) return -9000 + depth;
    
    auto moves = generateMoves(board);
    if (moves.empty()) return evaluatePosition(board);
    
    if (isMaximizing) {
        int maxEval = INT_MIN;
        for (auto& move : moves) {
            if (board.placePieceSimple(move.x, move.y, aiPlayer)) {
                int eval = minimax(board, depth - 1, false, alpha, beta);
                board.removePiece(move.x, move.y);
                
                maxEval = std::max(maxEval, eval);
                alpha = std::max(alpha, eval);
                
                if (beta <= alpha) break; // Poda alpha-beta
            }
        }
        return maxEval;
    } else {
        int minEval = INT_MAX;
        for (auto& move : moves) {
            if (board.placePieceSimple(move.x, move.y, humanPlayer)) {
                int eval = minimax(board, depth - 1, true, alpha, beta);
                board.removePiece(move.x, move.y);
                
                minEval = std::min(minEval, eval);
                beta = std::min(beta, eval);
                
                if (beta <= alpha) break; // Poda alpha-beta
            }
        }
        return minEval;
    }
}

std::vector<Move> AI::generateMoves(Board& board) {
    std::set<std::pair<int, int>> candidateSet;
    
    // 1. Zona de influencia básica
    addInfluenceZones(candidateSet, board);
    
    // 2. Movimientos tácticos
    addTacticalMoves(candidateSet, board, aiPlayer);
    addTacticalMoves(candidateSet, board, humanPlayer);

    
    // 3. Si muy pocas opciones, añadir centro
    if (candidateSet.size() < 5) {
        candidateSet.insert({9, 9});
        candidateSet.insert({8, 8});
        candidateSet.insert({10, 10});
        candidateSet.insert({8, 10});
        candidateSet.insert({10, 8});
    }
    
    // Convertir a vector
    std::vector<Move> moves;
    for (auto& pos : candidateSet) {
        if (board.isEmpty(pos.first, pos.second)) {
            moves.push_back(Move(pos.first, pos.second));
        }
    }
    
    // Limitar para eficiencia
    if (moves.size() > 20) {
        moves.resize(20);
    }
    
    return moves;
}

void AI::addInfluenceZones(std::set<std::pair<int, int>>& moves, const Board& board) {
    int directions[8][2] = {
        {-1, -1}, {-1, 0}, {-1, 1},
        {0, -1},           {0, 1},
        {1, -1},  {1, 0},  {1, 1}
    };
    
    for (int i = 0; i < board.getSize(); i++) {
        for (int j = 0; j < board.getSize(); j++) {
            if (board.getPiece(i, j) != 0) {
                // Considerar 1 y 2 espacios en cada dirección
                for (int d = 0; d < 8; d++) {
                    for (int dist = 1; dist <= 2; dist++) {
                        int nx = i + directions[d][0] * dist;
                        int ny = j + directions[d][1] * dist;
                        
                        if (board.isValid(nx, ny) && board.isEmpty(nx, ny) && moves.find({nx, ny}) == moves.end()) {
                            moves.insert({nx, ny});
                        }
                    }
                }
            }
        }
    }
}

void AI::addTacticalMoves(std::set<std::pair<int, int>>& moves, Board& board, int player) {
    // Añadir posiciones adyacentes a piezas existentes
    for (int i = 0; i < board.getSize(); i++) {
        for (int j = 0; j < board.getSize(); j++) {
            if (board.getPiece(i, j) != 0 && player > -1) {
                int directions[8][2] = {
                    {-1, -1}, {-1, 0}, {-1, 1},
                    {0, -1},           {0, 1},
                    {1, -1},  {1, 0},  {1, 1}
                };
                
                for (int d = 0; d < 8; d++) {
                    int nx = i + directions[d][0];
                    int ny = j + directions[d][1];
                    
					if (board.isValid(nx, ny) && board.isEmpty(nx, ny) && moves.find({nx, ny}) == moves.end()) {
						moves.insert({nx, ny});
					}
                }
            }
        }
    }
}

int AI::evaluatePosition(Board& board) {
    // Verificación rápida de fin de juego
    if (board.checkWin(aiPlayer)) return 10000;
    if (board.checkWin(humanPlayer)) return -9000;
    
    int myScore = evaluatePlayer(board, aiPlayer);
    int opponentScore = evaluatePlayer(board, humanPlayer);
    
    // LÓGICA MEJORADA: Penalizar más las amenazas del oponente
    int myPatterns4 = countPatterns(board, aiPlayer, 4);
    int myPatterns3 = countPatterns(board, aiPlayer, 3);
    int oppPatterns4 = countPatterns(board, humanPlayer, 4);
    int oppPatterns3 = countPatterns(board, humanPlayer, 3);
    
    // NUEVO: Considerar capturas
    int myCaptures = board.getCaptures(aiPlayer);
    int oppCaptures = board.getCaptures(humanPlayer);
    
    // Bonificaciones/penalizaciones especiales
    int strategicBonus = 0;
    
    // Capturas: cada captura vale mucho, estar cerca de 10 es crítico
    strategicBonus += myCaptures * 100;  // Cada captura mía vale 100 puntos
    strategicBonus -= oppCaptures * 120; // Cada captura del oponente me cuesta 120
    
    // Si el oponente está cerca de ganar por capturas, es CRÍTICO
    if (oppCaptures >= 8) {
        strategicBonus -= 1000 * (oppCaptures - 7); // 8->1000, 9->2000
    }
    
    // Si yo estoy cerca de ganar por capturas, es excelente
    if (myCaptures >= 8) {
        strategicBonus += 800 * (myCaptures - 7); // 8->800, 9->1600
    }
    
    // Si el oponente tiene 4 en línea, es CRÍTICO bloquearlo
    if (oppPatterns4 > 0) {
        strategicBonus -= 2000 * oppPatterns4;
    }
    
    // Si el oponente tiene 3 en línea, es muy importante bloquearlo
    if (oppPatterns3 > 0) {
        strategicBonus -= 500 * oppPatterns3;
    }
    
    // Si yo tengo 4 en línea, es excelente
    if (myPatterns4 > 0) {
        strategicBonus += 1500 * myPatterns4;
    }
    
    // Si yo tengo 3 en línea, es bueno
    if (myPatterns3 > 0) {
        strategicBonus += 400 * myPatterns3;
    }
    
    // Añadir valor base para evitar puntuaciones muy negativas
    int baseValue = 100;
    
    return myScore - opponentScore + strategicBonus + baseValue;
}

int AI::evaluatePlayer(Board& board, int player) {
    int score = 0;
    
    // JERARQUÍA CORRECTA:
    
    // 1. Líneas críticas (alta puntuación)
    score += countPatterns(board, player, 4) * 1000;  // 4 en línea
    score += countPatterns(board, player, 3) * 300;   // 3 en línea  
    score += countPatterns(board, player, 2) * 50;    // 2 en línea
    score += countPatterns(board, player, 1) * 10;    // 1 pieza
    
    // 2. Valor posicional básico
    score += evaluatePositionalValue(board, player);
    
    // 3. Valor base por tener piezas en el tablero
    int pieceCount = 0;
    for (int i = 0; i < board.getSize(); i++) {
        for (int j = 0; j < board.getSize(); j++) {
            if (board.getPiece(i, j) == player) {
                pieceCount++;
            }
        }
    }
    score += pieceCount * 5; // Valor base por cada pieza
    
    return std::max(score, 0); // Asegurar que nunca sea negativo
}

// =================== NUEVAS FUNCIONES: DETECCIÓN DE PATRONES ===================

int AI::countPatterns(Board& board, int player, int targetLength) {
    int count = 0;
    int directions[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};
    
    // Para evitar contar la misma línea múltiples veces,
    // solo contamos desde el "inicio" de cada línea
    for (int i = 0; i < board.getSize(); i++) {
        for (int j = 0; j < board.getSize(); j++) {
            if (board.getPiece(i, j) == player) {
                
                for (int d = 0; d < 4; d++) {
                    int dx = directions[d][0];
                    int dy = directions[d][1];
                    
                    // Solo contar si somos el "inicio" de la línea
                    // (no hay una pieza del mismo jugador en la dirección opuesta)
                    int prevX = i - dx;
                    int prevY = j - dy;
                    bool isStart = !board.isValid(prevX, prevY) || board.getPiece(prevX, prevY) != player;
                    
                    if (isStart) {
                        // Contar línea hacia una dirección
                        int lineLength = countLineLength(board, i, j, dx, dy, player);
                        
                        // Si la línea coincide con el objetivo
                        if (lineLength == targetLength) {
                            count++;
                        }
                    }
                }
            }
        }
    }
    
    return count;
}

int AI::countLineLength(Board& board, int startX, int startY, int dx, int dy, int player) {
    int length = 1; // La pieza inicial
    
    // Contar hacia adelante
    int x = startX + dx, y = startY + dy;
    while (board.isValid(x, y) && board.getPiece(x, y) == player) {
        length++;
        x += dx;
        y += dy;
    }
    
    // Contar hacia atrás
    x = startX - dx;
    y = startY - dy;
    while (board.isValid(x, y) && board.getPiece(x, y) == player) {
        length++;
        x -= dx;
        y -= dy;
    }
    
    return length;
}

Move AI::findImmediateWin(Board& board, int player) {
    for (int i = 0; i < board.getSize(); i++) {
        for (int j = 0; j < board.getSize(); j++) {
            if (board.isEmpty(i, j)) {
                int capturesGained = 0;
                
                // Simular movimiento completo (incluyendo capturas)
                bool wins = simulateCompleteMove(board, i, j, player, capturesGained);
                
                if (wins) {
                    std::cout << "¡Victoria inmediata detectada en (" << i << "," << j << ")!";
                    if (capturesGained > 0) {
                        std::cout << " (con " << capturesGained << " capturas)";
                    }
                    std::cout << std::endl;
                    return Move(i, j, player == aiPlayer ? 10000 : 9000);
                }
            }
        }
    }
    
    return Move(-1, -1, 0); // No hay movimiento ganador
}

Move AI::findCriticalBlock(Board& board, int player, int targetPattern) {
    for (int i = 0; i < board.getSize(); i++) {
        for (int j = 0; j < board.getSize(); j++) {
            if (board.isEmpty(i, j)) {
                // Simular movimiento
                board.placePieceSimple(i, j, player);
                
                // ¿Este movimiento crea el patrón objetivo?
                bool createsThreat = (this->countPatterns(board, player, targetPattern) > 0);
                
                // Deshacer movimiento
                board.removePiece(i, j);
                
                if (createsThreat) {
                    return Move(i, j, targetPattern * 100);
                }
            }
        }
    }
    
    return Move(-1, -1, 0); // No hay amenaza crítica
}

// =================== FUNCIONES DE EVALUACIÓN POSICIONAL ===================

int AI::evaluatePositionalValue(Board& board, int player) {
    int positionalScore = 0;
    int centerX = board.getSize() / 2;
    int centerY = board.getSize() / 2;
    
    for (int i = 0; i < board.getSize(); i++) {
        for (int j = 0; j < board.getSize(); j++) {
            if (board.getPiece(i, j) == player) {
                
                // Valor por proximidad al centro
                int distanceFromCenter = abs(i - centerX) + abs(j - centerY);
                int centerValue = std::max(0, 20 - distanceFromCenter * 2);
                positionalScore += centerValue;
                
                // Valor por libertad de movimiento
                int freedomValue = countAdjacentEmpty(board, i, j) * 5;
                positionalScore += freedomValue;
                
                // Valor base por tener una pieza
                positionalScore += 10;
            }
        }
    }
    
    return positionalScore;
}

int AI::countAdjacentEmpty(const Board& board, int x, int y) {
    int count = 0;
    int directions[8][2] = {
        {-1, -1}, {-1, 0}, {-1, 1},
        {0, -1},           {0, 1},
        {1, -1},  {1, 0},  {1, 1}
    };
    
    for (int d = 0; d < 8; d++) {
        int nx = x + directions[d][0];
        int ny = y + directions[d][1];
        
        if (board.isValid(nx, ny) && board.isEmpty(nx, ny)) {
            count++;
        }
    }
    
    return count;
}

// =================== FUNCIONES LEGACY (por compatibilidad) ===================

int AI::countSimplePatterns(Board& board, int player, int length) {
    // Redirigir a la nueva función
    return countPatterns(board, player, length);
}

// Función de depuración para mostrar todos los patrones detectados
void AI::debugPatterns(Board& board, int player) {
    std::cout << "\n=== DEBUG PATRONES para jugador " << player << " ===" << std::endl;
    
    int directions[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};
    std::string dirNames[4] = {"Horizontal", "Vertical", "Diagonal ↘", "Diagonal ↙"};
    
    for (int length = 2; length <= 5; length++) {
        std::cout << "Patrones de " << length << ": ";
        
        for (int d = 0; d < 4; d++) {
            int count = 0;
            int dx = directions[d][0];
            int dy = directions[d][1];
            
            for (int i = 0; i < board.getSize(); i++) {
                for (int j = 0; j < board.getSize(); j++) {
                    if (board.getPiece(i, j) == player) {
                        // Solo contar desde el inicio de línea
                        int prevX = i - dx;
                        int prevY = j - dy;
                        bool isStart = !board.isValid(prevX, prevY) || board.getPiece(prevX, prevY) != player;
                        
                        if (isStart) {
                            int lineLength = this->countLineLength(board, i, j, dx, dy, player);
                            if (lineLength == length) {
                                count++;
                            }
                        }
                    }
                }
            }
            
            if (count > 0) {
                std::cout << dirNames[d] << "(" << count << ") ";
            }
        }
        std::cout << std::endl;
    }
    std::cout << "=========================" << std::endl;
}

// Función auxiliar para simular un movimiento completo incluyendo capturas
bool AI::simulateCompleteMove(Board& board, int x, int y, int player, int& capturesGained) {
    if (!board.isEmpty(x, y)) return false;
    
    // Simular colocación de la pieza
    board.placePieceSimple(x, y, player);
    
    // Simular capturas del jugador
    auto captured = board.checkCaptures(x, y, player);
    capturesGained = captured.size();
    
    // Simular las capturas (temporalmente)
    for (auto& pos : captured) {
        board.removePiece(pos.first, pos.second);
    }
    
    // Verificar si este movimiento resulta en victoria
    int currentCaptures = board.getCaptures(player) + capturesGained;
    bool winsBy5InLine = (this->countPatterns(board, player, 5) > 0);
    bool winsByCaptures = (currentCaptures >= 10);
    
    // Deshacer las capturas temporales
    for (auto& pos : captured) {
        int opponent = (player == 1) ? 2 : 1;
        board.placePieceSimple(pos.first, pos.second, opponent);
    }
    
    // Deshacer la colocación de la pieza
    board.removePiece(x, y);
    
    return winsBy5InLine || winsByCaptures;
}