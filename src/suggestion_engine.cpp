/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   suggestion_engine.cpp                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/29 20:19:51 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/29 20:20:27 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/suggestion_engine.hpp"
#include <algorithm>
#include <limits>

Move SuggestionEngine::getSuggestion(const GameState& state) {
    std::vector<Move> candidates = generateCandidates(state);
    
    if (candidates.empty()) {
        // Fallback: centro del tablero
        return Move(9, 9);
    }
    
    Move bestMove = candidates[0];
    int bestScore = std::numeric_limits<int>::min();
    int currentPlayer = state.currentPlayer;
    
    // Evaluar cada candidato
    for (const Move& move : candidates) {
        int score = evaluateMove(state, move, currentPlayer);
        
        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
        }
    }
    
    return bestMove;
}

int SuggestionEngine::evaluateMove(const GameState& state, const Move& move, int player) {
    int score = 0;
    int opponent = state.getOpponent(player);
    
    // PRIORIDAD 1: ¿Gana inmediatamente? (MÁXIMA PRIORIDAD)
    int winScore = checkWinningMove(state, move, player);
    if (winScore > 0) {
        return 10000000;  // Victoria inmediata
    }
    
    // PRIORIDAD 2: ¿Bloquea victoria del oponente? (CRÍTICO)
    int blockScore = checkBlockingMove(state, move, player);
    if (blockScore > 0) {
        return 5000000;  // Debe bloquear
    }
    
    // PRIORIDAD 3: ¿Crea amenaza de 4 en línea? (MUY ALTO)
    GameState testState = state;
    testState.board[move.x][move.y] = player;
    
    if (createsFourInRow(testState, move, player)) {
        score += 500000;  // Amenaza muy fuerte
    }
    
    // PRIORIDAD 4: ¿Bloquea amenaza de 4 del oponente?
    testState = state;
    testState.board[move.x][move.y] = opponent;
    if (createsFourInRow(testState, move, opponent)) {
        score += 300000;  // Bloquear amenaza de 4
    }
    
    // Volver al estado con nuestra pieza
    testState = state;
    testState.board[move.x][move.y] = player;
    
    // PRIORIDAD 5: ¿Crea amenaza de 3 abierto?
    if (createsThreeOpen(testState, move, player)) {
        score += 100000;
    }
    
    // PRIORIDAD 6: ¿Bloquea 3 abierto del oponente?
    testState.board[move.x][move.y] = opponent;
    if (createsThreeOpen(testState, move, opponent)) {
        score += 50000;
    }
    
    // PRIORIDAD 7: Capturas (menos importante que patrones)
    int captureScore = checkCaptureMove(state, move, player);
    score += captureScore * 10000;
    
    // PRIORIDAD 8: Valor de patrones generales
    int patternScore = checkPatternValue(state, move, player);
    score += patternScore;
    
    // PRIORIDAD 9: Conectividad (estar cerca de otras piezas)
    score += calculateConnectivity(state, move, player);
    
    // PRIORIDAD 10: Centralidad (bonus leve por posiciones centrales)
    int centerDist = std::max(std::abs(move.x - 9), std::abs(move.y - 9));
    score += (9 - centerDist) * 10;
    
    return score;
}

std::vector<Move> SuggestionEngine::generateCandidates(const GameState& state) {
    std::vector<Move> candidates;
    
    // Estrategia: generar movimientos en radio 2 de piezas existentes
    bool hasPieces = false;
    
    for (int i = 0; i < GameState::BOARD_SIZE; i++) {
        for (int j = 0; j < GameState::BOARD_SIZE; j++) {
            if (!state.isEmpty(i, j)) {
                hasPieces = true;
                
                // Generar candidatos alrededor
                for (int di = -2; di <= 2; di++) {
                    for (int dj = -2; dj <= 2; dj++) {
                        int ni = i + di;
                        int nj = j + dj;
                        
                        if (state.isValid(ni, nj) && state.isEmpty(ni, nj)) {
                            Move candidate(ni, nj);
                            
                            // Evitar duplicados
                            bool exists = false;
                            for (const Move& m : candidates) {
                                if (m.x == ni && m.y == nj) {
                                    exists = true;
                                    break;
                                }
                            }
                            
                            if (!exists && RuleEngine::isLegalMove(state, candidate)) {
                                candidates.push_back(candidate);
                            }
                        }
                    }
                }
            }
        }
    }
    
    // Si no hay piezas (primer movimiento), sugerir centro
    if (!hasPieces) {
        candidates.push_back(Move(9, 9));
    }
    
    return candidates;
}

int SuggestionEngine::checkWinningMove(const GameState& state, const Move& move, int player) {
    GameState testState = state;
    testState.board[move.x][move.y] = player;
    
    if (RuleEngine::checkWin(testState, player)) {
        return 100;  // Victoria inmediata
    }
    
    return 0;
}

int SuggestionEngine::checkBlockingMove(const GameState& state, const Move& move, int player) {
    int opponent = state.getOpponent(player);
    
    // Simular que el oponente juega aquí
    GameState testState = state;
    testState.board[move.x][move.y] = opponent;
    
    if (RuleEngine::checkWin(testState, opponent)) {
        return 100;  // Bloquea victoria del oponente
    }
    
    return 0;
}

int SuggestionEngine::checkCaptureMove(const GameState& state, const Move& move, int player) {
    auto captures = RuleEngine::findCaptures(state, move, player);
    return captures.size() / 2;  // Número de pares capturados
}

int SuggestionEngine::checkPatternValue(const GameState& state, const Move& move, int player) {
    GameState testState = state;
    testState.board[move.x][move.y] = player;
    
    int score = 0;
    int directions[4][2] = {{1,0}, {0,1}, {1,1}, {1,-1}};
    
    // Contar patrones en las 4 direcciones
    for (int d = 0; d < 4; d++) {
        int dx = directions[d][0];
        int dy = directions[d][1];
        
        int count = 1;  // La pieza que acabamos de colocar
        
        // Contar hacia adelante
        int x = move.x + dx, y = move.y + dy;
        while (testState.isValid(x, y) && testState.getPiece(x, y) == player) {
            count++;
            x += dx;
            y += dy;
        }
        
        // Contar hacia atrás
        x = move.x - dx;
        y = move.y - dy;
        while (testState.isValid(x, y) && testState.getPiece(x, y) == player) {
            count++;
            x -= dx;
            y -= dy;
        }
        
        // Scoring según el patrón (reducido porque ya se evalúa arriba)
        if (count == 2) score += 100;   // Dos en línea
        else if (count == 1) score += 10;  // Pieza individual
    }
    
    return score;
}

// NUEVAS FUNCIONES AUXILIARES

bool SuggestionEngine::createsFourInRow(const GameState& state, const Move& move, int player) {
    int directions[4][2] = {{1,0}, {0,1}, {1,1}, {1,-1}};
    
    for (int d = 0; d < 4; d++) {
        int dx = directions[d][0];
        int dy = directions[d][1];
        
        int count = 1;  // La pieza en move
        
        // Contar hacia adelante
        int x = move.x + dx, y = move.y + dy;
        int forward = 0;
        while (state.isValid(x, y) && state.getPiece(x, y) == player && forward < 4) {
            count++;
            forward++;
            x += dx;
            y += dy;
        }
        
        // Contar hacia atrás
        x = move.x - dx;
        y = move.y - dy;
        int backward = 0;
        while (state.isValid(x, y) && state.getPiece(x, y) == player && backward < 4) {
            count++;
            backward++;
            x -= dx;
            y -= dy;
        }
        
        // ¿Tiene exactamente 4 y al menos un extremo libre?
        if (count == 4) {
            // Verificar extremos libres
            int frontX = move.x + (forward + 1) * dx;
            int frontY = move.y + (forward + 1) * dy;
            int backX = move.x - (backward + 1) * dx;
            int backY = move.y - (backward + 1) * dy;
            
            bool frontFree = state.isValid(frontX, frontY) && state.isEmpty(frontX, frontY);
            bool backFree = state.isValid(backX, backY) && state.isEmpty(backX, backY);
            
            if (frontFree || backFree) {
                return true;  // 4 en línea con al menos un extremo libre
            }
        }
    }
    
    return false;
}

bool SuggestionEngine::createsThreeOpen(const GameState& state, const Move& move, int player) {
    int directions[4][2] = {{1,0}, {0,1}, {1,1}, {1,-1}};
    
    for (int d = 0; d < 4; d++) {
        int dx = directions[d][0];
        int dy = directions[d][1];
        
        int count = 1;
        
        // Contar hacia adelante
        int x = move.x + dx, y = move.y + dy;
        int forward = 0;
        while (state.isValid(x, y) && state.getPiece(x, y) == player && forward < 3) {
            count++;
            forward++;
            x += dx;
            y += dy;
        }
        
        // Contar hacia atrás
        x = move.x - dx;
        y = move.y - dy;
        int backward = 0;
        while (state.isValid(x, y) && state.getPiece(x, y) == player && backward < 3) {
            count++;
            backward++;
            x -= dx;
            y -= dy;
        }
        
        // ¿Tiene exactamente 3 y AMBOS extremos libres?
        if (count == 3) {
            int frontX = move.x + (forward + 1) * dx;
            int frontY = move.y + (forward + 1) * dy;
            int backX = move.x - (backward + 1) * dx;
            int backY = move.y - (backward + 1) * dy;
            
            bool frontFree = state.isValid(frontX, frontY) && state.isEmpty(frontX, frontY);
            bool backFree = state.isValid(backX, backY) && state.isEmpty(backX, backY);
            
            if (frontFree && backFree) {
                return true;  // 3 abierto (ambos extremos libres)
            }
        }
    }
    
    return false;
}

int SuggestionEngine::calculateConnectivity(const GameState& state, const Move& move, int player) {
    int connectivity = 0;
    
    // Verificar las 8 direcciones adyacentes
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            if (dx == 0 && dy == 0) continue;
            
            int adjX = move.x + dx;
            int adjY = move.y + dy;
            
            if (state.isValid(adjX, adjY)) {
                if (state.getPiece(adjX, adjY) == player) {
                    connectivity += 50;  // Bonus por estar cerca de nuestras piezas
                } else if (state.getPiece(adjX, adjY) == state.getOpponent(player)) {
                    connectivity += 20;  // Bonus menor por estar cerca del oponente
                }
            }
        }
    }
    
    return connectivity;
}
