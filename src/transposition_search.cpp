/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   transposition_search.cpp                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/14 21:38:31 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/14 22:03:21 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/transposition_search.hpp"
#include <algorithm>
#include <limits>
#include <iostream>

TranspositionSearch::SearchResult TranspositionSearch::findBestMove(const GameState& state, int maxDepth) {
    SearchResult result;
    nodesEvaluated = 0;
    cacheHits = 0;
    
    // NUEVO: Profundidad adaptativa basada en fase del juego
    int adaptiveDepth = calculateAdaptiveDepth(state, maxDepth);
    
    std::cout << "AI using depth: " << adaptiveDepth << " (requested: " << maxDepth << ")" << std::endl;
    
    Move bestMove;
    int score = minimax(const_cast<GameState&>(state), adaptiveDepth, 
                       std::numeric_limits<int>::min(), 
                       std::numeric_limits<int>::max(), 
                       state.currentPlayer == GameState::PLAYER2, // AI maximiza
                       &bestMove);
    
    result.bestMove = bestMove;
    result.score = score;
    result.nodesEvaluated = nodesEvaluated;
    result.cacheHits = cacheHits;
    result.cacheHitRate = nodesEvaluated > 0 ? (float)cacheHits / nodesEvaluated : 0.0f;
    
    return result;
}

int TranspositionSearch::calculateAdaptiveDepth(const GameState& state, int requestedDepth) {
    // Profundidad basada en el número de piezas en el tablero
    int pieceCount = state.turnCount;
    
    if (pieceCount <= 2) {
        return 4;  // Primeros movimientos: muy rápido
    } else if (pieceCount <= 6) {
        return 6;  // Early game: rápido
    } else if (pieceCount <= 15) {
        return 8;  // Mid game: moderado
    } else {
        return std::min(requestedDepth, 10);  // Late game: profundidad completa
    }
}

int TranspositionSearch::minimax(GameState& state, int depth, int alpha, int beta, 
                                bool maximizing, Move* bestMove) {
    nodesEvaluated++;
    
    // Debug cada 10000 nodos
    if (nodesEvaluated % 10000 == 0) {
        std::cout << "Nodes evaluated: " << nodesEvaluated << ", Cache hits: " << cacheHits << std::endl;
    }
    
    // Verificar transposition table
    StateHash hash = hashState(state);
    auto it = transpositionTable.find(hash);
    
    if (it != transpositionTable.end() && it->second.depth >= depth) {
        cacheHits++;
        const CacheEntry& entry = it->second;
        
        if (entry.type == CacheEntry::EXACT) {
            if (bestMove) *bestMove = entry.bestMove;
            return entry.score;
        } else if (entry.type == CacheEntry::LOWER_BOUND && entry.score >= beta) {
            return entry.score;
        } else if (entry.type == CacheEntry::UPPER_BOUND && entry.score <= alpha) {
            return entry.score;
        }
    }
    
    // Casos base
    if (depth == 0 || RuleEngine::checkWin(state, GameState::PLAYER1) || 
        RuleEngine::checkWin(state, GameState::PLAYER2)) {
        
        int score = Evaluator::evaluate(state);
        transpositionTable[hash] = CacheEntry(score, depth, Move(), CacheEntry::EXACT);
        return score;
    }
    
    // Generar movimientos
    std::vector<Move> moves = generateOrderedMoves(state);
    if (moves.empty()) {
        int score = Evaluator::evaluate(state);
        transpositionTable[hash] = CacheEntry(score, depth, Move(), CacheEntry::EXACT);
        return score;
    }
    
    Move currentBestMove;
    int originalAlpha = alpha;
    
    if (maximizing) {
        int maxEval = std::numeric_limits<int>::min();
        
        for (const Move& move : moves) {
            GameState newState = state;
            RuleEngine::MoveResult result = RuleEngine::applyMove(newState, move);
            
            if (!result.success) continue;
            
            int eval = minimax(newState, depth - 1, alpha, beta, false);
            
            if (eval > maxEval) {
                maxEval = eval;
                currentBestMove = move;
            }
            
            alpha = std::max(alpha, eval);
            if (beta <= alpha) break; // Poda alpha-beta
        }
        
        // Guardar en transposition table
        CacheEntry::Type entryType;
        if (maxEval <= originalAlpha) {
            entryType = CacheEntry::UPPER_BOUND;
        } else if (maxEval >= beta) {
            entryType = CacheEntry::LOWER_BOUND;
        } else {
            entryType = CacheEntry::EXACT;
        }
        
        transpositionTable[hash] = CacheEntry(maxEval, depth, currentBestMove, entryType);
        
        if (bestMove) *bestMove = currentBestMove;
        return maxEval;
        
    } else {
        int minEval = std::numeric_limits<int>::max();
        
        for (const Move& move : moves) {
            GameState newState = state;
            RuleEngine::MoveResult result = RuleEngine::applyMove(newState, move);
            
            if (!result.success) continue;
            
            int eval = minimax(newState, depth - 1, alpha, beta, true);
            
            if (eval < minEval) {
                minEval = eval;
                currentBestMove = move;
            }
            
            beta = std::min(beta, eval);
            if (beta <= alpha) break; // Poda alpha-beta
        }
        
        // Guardar en transposition table
        CacheEntry::Type entryType;
        if (minEval <= originalAlpha) {
            entryType = CacheEntry::UPPER_BOUND;
        } else if (minEval >= beta) {
            entryType = CacheEntry::LOWER_BOUND;
        } else {
            entryType = CacheEntry::EXACT;
        }
        
        transpositionTable[hash] = CacheEntry(minEval, depth, currentBestMove, entryType);
        
        if (bestMove) *bestMove = currentBestMove;
        return minEval;
    }
}

TranspositionSearch::StateHash TranspositionSearch::hashState(const GameState& state) const {
    // Hash simple pero efectivo
    StateHash hash = 0;
    StateHash multiplier = 1;
    
    // Solo hash de piezas no vacías para eficiencia
    for (int i = 0; i < GameState::BOARD_SIZE; i++) {
        for (int j = 0; j < GameState::BOARD_SIZE; j++) {
            int piece = state.getPiece(i, j);
            if (piece != GameState::EMPTY) {
                hash ^= (piece * multiplier) + (i * 19 + j);
                multiplier = multiplier * 3 + 1;
            }
        }
    }
    
    // Hash de capturas y turno
    hash ^= (state.captures[0] << 16) | state.captures[1];
    hash ^= (state.currentPlayer << 20);
    
    return hash;
}

std::vector<Move> TranspositionSearch::generateOrderedMoves(const GameState& state) {
    std::vector<Move> moves;
    
    // NUEVO: Estrategia más inteligente basada en fase del juego
    if (state.turnCount == 0) {
        // Primer movimiento: solo centro
        moves.push_back(Move(9, 9));
        return moves;
    } else if (state.turnCount == 1) {
        // Segundo movimiento: cerca del centro
        for (int di = -2; di <= 2; di++) {
            for (int dj = -2; dj <= 2; dj++) {
                int i = 9 + di, j = 9 + dj;
                if (state.isValid(i, j) && state.isEmpty(i, j)) {
                    moves.push_back(Move(i, j));
                }
            }
        }
    } else {
        // Movimientos normales: cerca de piezas existentes
        for (int i = 0; i < GameState::BOARD_SIZE; i++) {
            for (int j = 0; j < GameState::BOARD_SIZE; j++) {
                if (!state.isEmpty(i, j)) continue;
                
                // Solo considerar si está cerca de alguna pieza
                bool nearPiece = false;
                for (int di = -1; di <= 1 && !nearPiece; di++) {
                    for (int dj = -1; dj <= 1 && !nearPiece; dj++) {
                        int ni = i + di, nj = j + dj;
                        if (state.isValid(ni, nj) && state.getPiece(ni, nj) != GameState::EMPTY) {
                            nearPiece = true;
                        }
                    }
                }
                
                if (nearPiece && RuleEngine::isLegalMove(state, Move(i, j))) {
                    moves.push_back(Move(i, j));
                }
            }
        }
    }
    
    // Ordenar movimientos
    orderMoves(moves, state);
    
    // NUEVO: Límite más agresivo basado en fase del juego
    size_t limit = state.turnCount < 10 ? 8 : 15;  // Menos movimientos en early game
    if (moves.size() > limit) {
        moves.resize(limit);
    }
    
    return moves;
}

void TranspositionSearch::orderMoves(std::vector<Move>& moves, const GameState& state) {
    std::sort(moves.begin(), moves.end(), [&](const Move& a, const Move& b) {
        return quickEvaluateMove(state, a) > quickEvaluateMove(state, b);
    });
}

int TranspositionSearch::quickEvaluateMove(const GameState& state, const Move& move) {
    GameState tempState = state;
    RuleEngine::MoveResult result = RuleEngine::applyMove(tempState, move);
    
    if (!result.success) return std::numeric_limits<int>::min();
    
    int score = 0;
    
    // Bonus por capturas (muy alto)
    score += result.capturedPieces.size() * 2000;
    
    // Bonus por ganar inmediatamente (máximo)
    if (result.createsWin) score += 100000;
    
    // Bonus por proximidad al centro (menor)
    int centerDist = std::max(std::abs(move.x - 9), std::abs(move.y - 9));
    score += (10 - centerDist) * 5;
    
    // NUEVO: Bonus por crear amenazas
    int threats = countThreats(tempState, state.currentPlayer);
    score += threats * 500;
    
    return score;
}

int TranspositionSearch::countThreats(const GameState& state, int player) {
    // Contar rápidamente amenazas obvias
    int threats = 0;
    
    // Solo verificar algunas posiciones clave para no hacer esto muy costoso
    for (int i = 0; i < GameState::BOARD_SIZE; i += 2) {  // Saltar posiciones para rapidez
        for (int j = 0; j < GameState::BOARD_SIZE; j += 2) {
            if (state.getPiece(i, j) == player) {
                // Contar líneas de 3 o 4 que podrían ser amenazas
                // (implementación simplificada para rapidez)
                threats += countLinesFromPosition(state, i, j, player);
            }
        }
    }
    
    return threats;
}

int TranspositionSearch::countLinesFromPosition(const GameState& state, int x, int y, int player) {
    int lines = 0;
    int directions[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};
    
    for (int d = 0; d < 4; d++) {
        int dx = directions[d][0], dy = directions[d][1];
        
        // Contar en ambas direcciones
        int count = 1;  // La pieza actual
        count += countInDirection(state, x, y, dx, dy, player);
        count += countInDirection(state, x, y, -dx, -dy, player);
        
        if (count >= 3) lines++;  // Línea de 3+ es una amenaza potencial
    }
    
    return lines;
}

int TranspositionSearch::countInDirection(const GameState& state, int x, int y, int dx, int dy, int player) {
    int count = 0;
    x += dx; y += dy;
    
    while (state.isValid(x, y) && state.getPiece(x, y) == player) {
        count++;
        x += dx; y += dy;
    }
    
    return count;
}