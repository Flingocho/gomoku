#include "../include/ai.hpp"
#include <algorithm>
#include <climits>
#include <iostream>

AI::AI(int player, int depth) : aiPlayer(player), maxDepth(depth) {
    humanPlayer = (player == 1) ? 2 : 1;
}

Move AI::getBestMove(Board& board) {
    std::cout << "AI analyzing with depth " << maxDepth << "..." << std::endl;
    
    // PASO 1: Verificar si podemos ganar inmediatamente
    Move winMove = findWinningMove(board, aiPlayer);
    if (winMove.x != -1) {
        std::cout << "Found winning move!" << std::endl;
        return winMove;
    }
    
    // PASO 2: Verificar si debemos bloquear una victoria del oponente
    Move blockMove = findWinningMove(board, humanPlayer);
    if (blockMove.x != -1) {
        std::cout << "Blocking opponent's winning move!" << std::endl;
        return blockMove;
    }
    
    // PASO 3: Crear amenaza de 4 en línea (mi 3→4)
    Move threatMove = findThreatMove(board, aiPlayer);
    if (threatMove.x != -1) {
        std::cout << "Creating 4-in-a-row threat!" << std::endl;
        return threatMove;
    }
    
    // PASO 4: Bloquear amenaza de 4 del oponente (su 3→4) 
    Move blockThreatMove = findThreatMove(board, humanPlayer);
    if (blockThreatMove.x != -1) {
        std::cout << "Blocking opponent's 4-in-a-row threat!" << std::endl;
        return blockThreatMove;
    }
    
    // PASO 5: Búsqueda normal con Min-Max
    auto candidates = generateMoves(board);
    std::cout << "Generated " << candidates.size() << " candidate moves" << std::endl;
    
    if (candidates.empty()) {
        return Move(9, 9, 0);  // Fallback: centro
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
            
            std::cout << "Move (" << move.x << "," << move.y << ") -> Score: " << score << std::endl;
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
    if (board.checkWin(aiPlayer)) return 10000 + depth;  // Ganar rápido es mejor
    if (board.checkWin(humanPlayer)) return -10000 - depth;  // Perder rápido es peor
    
    auto moves = generateMoves(board);
    if (moves.empty()) return evaluatePosition(board);
    
    if (isMaximizing) {
        int maxEval = INT_MIN;
        for (auto& move : moves) {
            if (board.placePieceSimple(move.x, move.y, aiPlayer)) {
                int eval = minimax(board, depth - 1, false, alpha, beta);
                board.removePiece(move.x, move.y);  // Deshacer movimiento
                
                maxEval = std::max(maxEval, eval);
                alpha = std::max(alpha, eval);
                
                if (beta <= alpha) break;  // Poda alpha-beta
            }
        }
        return maxEval;
    } else {
        int minEval = INT_MAX;
        for (auto& move : moves) {
            if (board.placePieceSimple(move.x, move.y, humanPlayer)) {
                int eval = minimax(board, depth - 1, true, alpha, beta);
                board.removePiece(move.x, move.y);  // Deshacer movimiento
                
                minEval = std::min(minEval, eval);
                beta = std::min(beta, eval);
                
                if (beta <= alpha) break;  // Poda alpha-beta
            }
        }
        return minEval;
    }
}

std::vector<Move> AI::generateMoves(Board& board) {
    std::set<std::pair<int, int>> candidateSet;
    
    // 1. Zona de influencia básica
    addInfluenceZones(candidateSet, board);
    
    // 2. Movimientos tácticos (por ahora básico)
    addTacticalMoves(candidateSet, board, aiPlayer);
    addTacticalMoves(candidateSet, board, humanPlayer);  // Defender también
    
    // 3. Si muy pocas opciones, añadir centro
    if (candidateSet.size() < 5) {
        candidateSet.insert({9, 9});   // Centro
        candidateSet.insert({8, 8});   // Cerca del centro
        candidateSet.insert({10, 10}); 
        candidateSet.insert({8, 10});
        candidateSet.insert({10, 8});
    }
    
    // Convertir a vector de Move
    std::vector<Move> moves;
    for (auto& pos : candidateSet) {
        if (board.isEmpty(pos.first, pos.second)) {
            moves.push_back(Move(pos.first, pos.second));
        }
    }
    
    // Limitar a máximo 20 movimientos para eficiencia
    if (moves.size() > 20) {
        moves.resize(20);
    }
    
    return moves;
}

void AI::addInfluenceZones(std::set<std::pair<int, int>>& moves, const Board& board) {
    // Tu idea original: desde cada pieza, considerar 2 espacios en 8 direcciones
    int directions[8][2] = {
        {-1, -1}, {-1, 0}, {-1, 1},
        {0, -1},           {0, 1},
        {1, -1},  {1, 0},  {1, 1}
    };
    
    for (int i = 0; i < board.getSize(); i++) {
        for (int j = 0; j < board.getSize(); j++) {
            if (board.getPiece(i, j) != 0) {  // Si hay una pieza aquí
                
                // Considerar 1 y 2 espacios en cada dirección
                for (int d = 0; d < 8; d++) {
                    for (int dist = 1; dist <= 2; dist++) {
                        int nx = i + directions[d][0] * dist;
                        int ny = j + directions[d][1] * dist;
                        
                        if (board.isValid(nx, ny)) {
                            moves.insert({nx, ny});
                        }
                    }
                }
            }
        }
    }
}

void AI::addTacticalMoves(std::set<std::pair<int, int>>& moves, Board& board, int player) {
    // Añadir posiciones cerca de piezas existentes
    for (int i = 0; i < board.getSize(); i++) {
        for (int j = 0; j < board.getSize(); j++) {
            if (board.getPiece(i, j) != 0) {  // Si hay una pieza
                
                // Añadir posiciones adyacentes
                int directions[8][2] = {
                    {-1, -1}, {-1, 0}, {-1, 1},
                    {0, -1},           {0, 1},
                    {1, -1},  {1, 0},  {1, 1}
                };
                
                for (int d = 0; d < 8; d++) {
                    int nx = i + directions[d][0];
                    int ny = j + directions[d][1];
                    
                    if (board.isValid(nx, ny) && board.isEmpty(nx, ny)) {
                        moves.insert({nx, ny});
                    }
                }
            }
        }
    }
}

bool AI::hasPattern(Board& board, int x, int y, int player, int length) {
    // 4 direcciones principales
    int directions[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};
    
    for (int d = 0; d < 4; d++) {
        int dx = directions[d][0];
        int dy = directions[d][1];
        
        int count = 1; // La pieza en (x,y)
        
        // Contar hacia adelante
        int nx = x + dx, ny = y + dy;
        while (board.isValid(nx, ny) && board.getPiece(nx, ny) == player) {
            count++;
            nx += dx;
            ny += dy;
        }
        
        // Contar hacia atrás
        nx = x - dx; ny = y - dy;
        while (board.isValid(nx, ny) && board.getPiece(nx, ny) == player) {
            count++;
            nx -= dx;  // ¡ARREGLADO!
            ny -= dy;  // ¡ARREGLADO!
        }
        
        if (count >= length) return true;
    }
    return false;
}

int AI::evaluatePosition(Board& board) {
    // Verificación rápida de fin de juego
    if (board.checkWin(aiPlayer)) return 1000000;    // Victoria absoluta
    if (board.checkWin(humanPlayer)) return -1000000; // Derrota absoluta
    
    int myScore = evaluatePlayerAdvanced(board, aiPlayer);
    int opponentScore = evaluatePlayerAdvanced(board, humanPlayer);
    
    return myScore - opponentScore;
}

int AI::evaluatePlayerAdvanced(Board& board, int player) {
    int score = 0;
    
    // Capturas (importante pero no crítico)
    score += board.getCaptures(player) * 1000;
    
    // PATRONES CRÍTICOS con valores realistas
    score += countThreatPatterns(board, player, 5) * 500000;  // 5 en línea (victoria)
    score += countThreatPatterns(board, player, 4) * 100000;  // 4 en línea (amenaza crítica)
    score += countFreeThrees(board, player) * 10000;          // 3 libre (amenaza seria)
    score += countThreatPatterns(board, player, 3) * 1000;    // 3 en línea normal
    score += countThreatPatterns(board, player, 2) * 100;     // 2 en línea
    
    return score;
}

int AI::countThreatPatterns(Board& board, int player, int length) {
    int count = 0;
    
    // 4 direcciones principales
    int directions[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};
    
    for (int i = 0; i < board.getSize(); i++) {
        for (int j = 0; j < board.getSize(); j++) {
            if (board.getPiece(i, j) == player) {
                
                for (int d = 0; d < 4; d++) {
                    int dx = directions[d][0];
                    int dy = directions[d][1];
                    
                    // Contar solo hacia adelante para evitar duplicados
                    int lineLength = 1;  // La pieza actual
                    int nx = i + dx, ny = j + dy;
                    
                    while (board.isValid(nx, ny) && board.getPiece(nx, ny) == player) {
                        lineLength++;
                        nx += dx;
                        ny += dy;
                    }
                    
                    if (lineLength >= length) {
                        count++;
                    }
                }
            }
        }
    }
    
    return count;
}

int AI::countFreeThrees(Board& board, int player) {
    int count = 0;
    
    // Buscar específicamente 3 en línea con ambos extremos libres
    for (int i = 0; i < board.getSize(); i++) {
        for (int j = 0; j < board.getSize(); j++) {
            if (board.getPiece(i, j) == player) {
                
                // 4 direcciones
                int directions[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};
                
                for (int d = 0; d < 4; d++) {
                    int dx = directions[d][0];
                    int dy = directions[d][1];
                    
                    // Contar línea hacia adelante
                    int lineLength = 1;
                    int nx = i + dx, ny = j + dy;
                    while (board.isValid(nx, ny) && board.getPiece(nx, ny) == player) {
                        lineLength++;
                        nx += dx;
                        ny += dy;
                    }
                    
                    // Si tenemos exactamente 3, verificar extremos libres
                    if (lineLength == 3) {
                        // Extremo frontal
                        bool frontFree = board.isValid(nx, ny) && board.getPiece(nx, ny) == 0;
                        
                        // Extremo trasero
                        int backX = i - dx, backY = j - dy;
                        bool backFree = board.isValid(backX, backY) && board.getPiece(backX, backY) == 0;
                        
                        if (frontFree && backFree) {
                            count++;  // ¡Free-three detectado!
                        }
                    }
                }
            }
        }
    }
    
    return count;
}

int AI::countSimplePatterns(Board& board, int player, int length) {
    int count = 0;
    
    // 4 direcciones principales
    int directions[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};
    
    for (int i = 0; i < board.getSize(); i++) {
        for (int j = 0; j < board.getSize(); j++) {
            if (board.getPiece(i, j) == player) {
                
                for (int d = 0; d < 4; d++) {
                    int dx = directions[d][0];
                    int dy = directions[d][1];
                    
                    // Contar solo hacia adelante para evitar duplicados
                    int lineLength = 1;  // La pieza actual
                    int nx = i + dx, ny = j + dy;
                    
                    while (board.isValid(nx, ny) && board.getPiece(nx, ny) == player) {
                        lineLength++;
                        nx += dx;
                        ny += dy;
                    }
                    
                    if (lineLength >= length) {
                        count++;
                    }
                }
            }
        }
    }
    
    return count;
}

int AI::countPatterns(Board& board, int player, int patternLength, bool needsFreeEnds) {
    int count = 0;
    
    // 4 direcciones principales
    int directions[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};
    
    for (int i = 0; i < board.getSize(); i++) {
        for (int j = 0; j < board.getSize(); j++) {
            if (board.getPiece(i, j) == player) {
                
                for (int d = 0; d < 4; d++) {
                    int dx = directions[d][0];
                    int dy = directions[d][1];
                    
                    int lineLength = 1;  // La pieza actual
                    
                    // Contar hacia adelante
                    int nx = i + dx, ny = j + dy;
                    while (board.isValid(nx, ny) && board.getPiece(nx, ny) == player) {
                        lineLength++;
                        nx += dx;
                        ny += dy;
                    }
                    
                    if (lineLength >= patternLength) {
                        if (!needsFreeEnds) {
                            count++;
                        } else {
                            // Verificar extremos libres
                            bool frontFree = board.isValid(nx, ny) && board.getPiece(nx, ny) == 0;
                            bool backFree = board.isValid(i - dx, j - dy) && board.getPiece(i - dx, j - dy) == 0;
                            
                            if (frontFree && backFree) {
                                count++;
                            }
                        }
                    }
                }
            }
        }
    }
    
    return count;
}

bool AI::isNearOtherPieces(const Board& board, int x, int y, int radius) {
    for (int i = x - radius; i <= x + radius; i++) {
        for (int j = y - radius; j <= y + radius; j++) {
            if (board.isValid(i, j) && board.getPiece(i, j) != 0) {
                return true;
            }
        }
    }
    return false;
}

Move AI::findWinningMove(Board& board, int player) {
    // Probar cada posición vacía para ver si crea una victoria
    for (int i = 0; i < board.getSize(); i++) {
        for (int j = 0; j < board.getSize(); j++) {
            if (board.isEmpty(i, j)) {
                // Simular colocación
                board.placePieceSimple(i, j, player);
                
                // ¿Este movimiento gana el juego?
                bool wins = board.checkWin(player) || hasPattern(board, i, j, player, 5);
                
                // Deshacer movimiento
                board.removePiece(i, j);
                
                if (wins) {
                    return Move(i, j, player == aiPlayer ? 50000 : -50000);
                }
            }
        }
    }
    
    return Move(-1, -1, 0);  // No se encontró movimiento ganador
}

Move AI::findThreatMove(Board& board, int player) {
    // Buscar movimientos que crean amenaza de 4 en línea
    for (int i = 0; i < board.getSize(); i++) {
        for (int j = 0; j < board.getSize(); j++) {
            if (board.isEmpty(i, j)) {
                // Simular colocación
                board.placePieceSimple(i, j, player);
                
                // ¿Este movimiento crea 4 en línea con al menos un extremo libre?
                bool createsThreat = has4InLineWithFreeEnd(board, i, j, player);
                
                // Deshacer movimiento
                board.removePiece(i, j);
                
                if (createsThreat) {
                    return Move(i, j, player == aiPlayer ? 10000 : -10000);
                }
            }
        }
    }
    
    return Move(-1, -1, 0);  // No se encontró amenaza
}

bool AI::has4InLineWithFreeEnd(Board& board, int x, int y, int player) {
    // 4 direcciones principales
    int directions[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};
    
    for (int d = 0; d < 4; d++) {
        int dx = directions[d][0];
        int dy = directions[d][1];
        
        // Contar línea total
        int count = 1;  // La pieza en (x,y)
        
        // Contar hacia adelante
        int forwardCount = 0;
        int nx = x + dx, ny = y + dy;
        while (board.isValid(nx, ny) && board.getPiece(nx, ny) == player) {
            forwardCount++;
            nx += dx;
            ny += dy;
        }
        count += forwardCount;
        
        // Contar hacia atrás
        int backwardCount = 0;
        nx = x - dx; ny = y - dy;
        while (board.isValid(nx, ny) && board.getPiece(nx, ny) == player) {
            backwardCount++;
            nx -= dx;
            ny -= dy;
        }
        count += backwardCount;
        
        // ¿Tenemos 4 en línea?
        if (count >= 4) {
            // ¿Al menos un extremo está libre?
            int frontX = x + (forwardCount + 1) * dx;
            int frontY = y + (forwardCount + 1) * dy;
            int backX = x - (backwardCount + 1) * dx;
            int backY = y - (backwardCount + 1) * dy;
            
            bool frontFree = board.isValid(frontX, frontY) && board.getPiece(frontX, frontY) == 0;
            bool backFree = board.isValid(backX, backY) && board.getPiece(backX, backY) == 0;
            
            if (frontFree || backFree) {
                return true;  // ¡Amenaza crítica!
            }
        }
    }
    
    return false;
}