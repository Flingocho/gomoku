/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   evaluator.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/14 21:24:46 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/14 21:25:14 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/evaluator.hpp"
#include "../include/rule_engine.hpp"

int Evaluator::evaluate(const GameState& state) {
    // Verificar condiciones de fin inmediato
    if (RuleEngine::checkWin(state, GameState::PLAYER2)) return WIN;
    if (RuleEngine::checkWin(state, GameState::PLAYER1)) return -WIN;
    
    int aiScore = evaluateForPlayer(state, GameState::PLAYER2);
    int humanScore = evaluateForPlayer(state, GameState::PLAYER1);
    
    return aiScore - humanScore;
}

int Evaluator::evaluateForPlayer(const GameState& state, int player) {
    int score = 0;
    
    // 1. Evaluar patrones de línea
    score += analyzePosition(state, player);
    
    // 2. Evaluar capturas
    score += evaluateCaptures(state, player);
    
    return score;
}

int Evaluator::analyzePosition(const GameState& state, int player) {
    int totalScore = 0;
    
    // Una sola pasada por el tablero
    for (int i = 0; i < GameState::BOARD_SIZE; i++) {
        for (int j = 0; j < GameState::BOARD_SIZE; j++) {
            // Solo analizar desde piezas del jugador
            if (state.board[i][j] != player) continue;
            
            // Analizar en las 4 direcciones principales desde esta pieza
            for (int d = 0; d < 4; d++) {
                int dx = MAIN_DIRECTIONS[d][0];
                int dy = MAIN_DIRECTIONS[d][1];
                
                // Solo evaluar si es el inicio de la línea (evita duplicados)
                if (isLineStart(state, i, j, dx, dy, player)) {
                    PatternInfo pattern = analyzeLine(state, i, j, dx, dy, player);
                    totalScore += patternToScore(pattern);
                }
            }
        }
    }
    
    return totalScore;
}

bool Evaluator::isLineStart(const GameState& state, int x, int y, int dx, int dy, int player) {
    // Es inicio si la posición anterior no tiene pieza del mismo jugador
    int prevX = x - dx;
    int prevY = y - dy;
    
    return !state.isValid(prevX, prevY) || state.getPiece(prevX, prevY) != player;
}

Evaluator::PatternInfo Evaluator::analyzeLine(const GameState& state, int x, int y, 
                                             int dx, int dy, int player) {
    PatternInfo info = {0, 0, false, 0};
    
    // Contar piezas consecutivas hacia adelante
    int currentX = x, currentY = y;
    while (state.isValid(currentX, currentY) && state.getPiece(currentX, currentY) == player) {
        info.consecutiveCount++;
        currentX += dx;
        currentY += dy;
    }
    
    // Verificar extremo adelante
    if (state.isValid(currentX, currentY) && state.isEmpty(currentX, currentY)) {
        info.freeEnds++;
    }
    
    // Verificar extremo atrás
    int backX = x - dx, backY = y - dy;
    if (state.isValid(backX, backY) && state.isEmpty(backX, backY)) {
        info.freeEnds++;
    }
    
    return info;
}

int Evaluator::patternToScore(const PatternInfo& pattern) {
    int count = pattern.consecutiveCount;
    int freeEnds = pattern.freeEnds;
    
    // Patrones de victoria
    if (count >= 5) return WIN;
    
    // Patrones críticos
    if (count == 4) {
        if (freeEnds == 2) return FOUR_OPEN;    // Imparable
        if (freeEnds == 1) return FOUR_HALF;    // Amenaza forzada
    }
    
    if (count == 3) {
        if (freeEnds == 2) return THREE_OPEN;   // Muy peligroso
        if (freeEnds == 1) return THREE_HALF;   // Amenaza
    }
    
    if (count == 2 && freeEnds == 2) {
        return TWO_OPEN; // Desarrollo
    }
    
    return 0;
}

int Evaluator::evaluateCaptures(const GameState& state, int player) {
    int score = 0;
    int opponent = state.getOpponent(player);
    
    // Puntuación por capturas ya realizadas
    int myCaptures = state.captures[player - 1];
    int oppCaptures = state.captures[opponent - 1];
    
    // Bonus progresivo por acercarse a 10 capturas
    if (myCaptures >= 8) score += 15000;       // 1 par para ganar
    else if (myCaptures >= 6) score += 6000;   // 2 pares para ganar
    else if (myCaptures >= 4) score += 2000;   // 3 pares para ganar
    else score += myCaptures * 200;            // Progreso general
    
    // Malus por capturas del oponente
    if (oppCaptures >= 8) score -= 15000;
    else if (oppCaptures >= 6) score -= 6000;
    else if (oppCaptures >= 4) score -= 2000;
    else score -= oppCaptures * 200;
    
    // Contar oportunidades de captura inmediata
    int captureOpportunities = 0;
    for (int i = 0; i < GameState::BOARD_SIZE; i++) {
        for (int j = 0; j < GameState::BOARD_SIZE; j++) {
            if (state.board[i][j] == player) {
                Move pos(i, j);
                auto captures = RuleEngine::findCaptures(state, pos, player);
                if (!captures.empty()) {
                    captureOpportunities++;
                }
            }
        }
    }
    
    score += captureOpportunities * CAPTURE_OPPORTUNITY;
    
    return score;
}
