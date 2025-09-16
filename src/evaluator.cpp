/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   evaluator.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/14 21:24:46 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/16 19:54:37 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/evaluator.hpp"
#include "../include/rule_engine.hpp"

// NUEVO: Evaluador principal con información de distancia al mate
int Evaluator::evaluate(const GameState& state, int maxDepth, int currentDepth) {
    int mateDistance = maxDepth - currentDepth;
    
    // Verificar condiciones de fin inmediato CON distancia al mate
    if (RuleEngine::checkWin(state, GameState::PLAYER2)) {
        // Victoria más cercana = mayor puntuación
        // WIN - mateDistance hace que mate en 1 valga más que mate en 5
        return WIN - mateDistance;
    }
    if (RuleEngine::checkWin(state, GameState::PLAYER1)) {
        // Derrota más lejana = menos malo
        // -WIN + mateDistance hace que perder en 5 sea menos malo que perder en 1
        return -WIN + mateDistance;
    }
    
    int aiScore = evaluateForPlayer(state, GameState::PLAYER2);
    int humanScore = evaluateForPlayer(state, GameState::PLAYER1);
    
    return aiScore - humanScore;
}

// LEGACY: Mantener la versión anterior para compatibilidad (sin distancia al mate)
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
    
	score += evaluateImmediateThreats(state, player);
	
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

// ALTERNATIVA MÁS EFICIENTE: Solo evaluar posiciones relevantes
int Evaluator::evaluateCaptures(const GameState& state, int player) {
    int score = 0;
    int opponent = state.getOpponent(player);
    
    // Puntuación por capturas ya realizadas (igual que antes)
    int myCaptures = state.captures[player - 1];
    int oppCaptures = state.captures[opponent - 1];
    
    if (myCaptures >= 8) score += 15000;
    else if (myCaptures >= 6) score += 6000;
    else if (myCaptures >= 4) score += 2000;
    else score += myCaptures * 200;
    
    if (oppCaptures >= 8) score -= 15000;
    else if (oppCaptures >= 6) score -= 6000;
    else if (oppCaptures >= 4) score -= 2000;
    else score -= oppCaptures * 200;
    
    // OPTIMIZACIÓN: Solo evaluar posiciones adyacentes a piezas existentes
    int myCaptureOpportunities = 0;
    int oppCaptureOpportunities = 0;
    
    for (int i = 0; i < GameState::BOARD_SIZE; i++) {
        for (int j = 0; j < GameState::BOARD_SIZE; j++) {
            if (!state.isEmpty(i, j)) continue;
            
            // Solo evaluar si está cerca de alguna pieza (optimización)
            bool nearPiece = false;
            for (int di = -1; di <= 1 && !nearPiece; di++) {
                for (int dj = -1; dj <= 1 && !nearPiece; dj++) {
                    int ni = i + di, nj = j + dj;
                    if (state.isValid(ni, nj) && state.getPiece(ni, nj) != GameState::EMPTY) {
                        nearPiece = true;
                    }
                }
            }
            
            if (!nearPiece) continue;  // Skip posiciones lejanas
            
            Move pos(i, j);
            
            // Capturas ofensivas
            auto myCaptures = RuleEngine::findCaptures(state, pos, player);
            if (!myCaptures.empty()) {
                myCaptureOpportunities += myCaptures.size() / 2;
            }
            
            // Capturas defensivas (amenazas del oponente)
            auto oppCaptures = RuleEngine::findCaptures(state, pos, opponent);
            if (!oppCaptures.empty()) {
                oppCaptureOpportunities += oppCaptures.size() / 2;
            }
        }
    }
    
    score += myCaptureOpportunities * CAPTURE_OPPORTUNITY;
    score -= oppCaptureOpportunities * CAPTURE_THREAT;
    
    return score;
}

// NUEVO: Evaluación de amenazas inmediatas integrada con el sistema existente
int Evaluator::evaluateImmediateThreats(const GameState& state, int player) {
    int opponent = state.getOpponent(player);
    int threatScore = 0;
    
    // 1. Contar cuántos 4-abiertos tiene cada jugador (victoria garantizada)
    int myFourOpen = countPatternType(state, player, 4, 2);     // 4 consecutive, 2 free ends
    int oppFourOpen = countPatternType(state, opponent, 4, 2);
    
    // 2. Contar cuántos 4-semi tiene cada jugador (victoria en 1 turno si no se bloquea)
    int myFourHalf = countPatternType(state, player, 4, 1);     // 4 consecutive, 1 free end
    int oppFourHalf = countPatternType(state, opponent, 4, 1);
    
    // 3. CLAVE: Si el oponente tiene amenazas de 4, son MUY peligrosas
    // Usamos valores altos pero no tan altos como WIN para mantener el balance
    if (oppFourOpen > 0) {
        threatScore -= 80000;  // El oponente puede ganar inmediatamente
    }
    if (oppFourHalf > 0) {
        threatScore -= 60000;  // El oponente puede ganar en 1 turno
    }
    
    // 4. Nuestras amenazas son buenas, pero consideramos el contexto
    if (myFourOpen > 0) {
        if (oppFourOpen == 0 && oppFourHalf == 0) {
            threatScore += 70000;  // Solo nosotros amenazamos = muy bueno
        } else {
            // Si ambos amenazan, el que mueve primero gana
            // Como somos la IA (PLAYER2), movemos después del humano
            threatScore += 20000;  // Situación compleja, menos bonus
        }
    }
    
    if (myFourHalf > 0) {
        if (oppFourOpen == 0 && oppFourHalf == 0) {
            threatScore += 40000;  // Solo nosotros amenazamos
        } else {
            threatScore += 10000;  // Ambos amenazan = situación defensiva
        }
    }
    
    // 5. Múltiples amenazas del oponente = desastre total
    if (oppFourOpen + oppFourHalf > 1) {
        threatScore -= 95000;  // Múltiples amenazas = imposible defender
    }
    
    // 6. BONUS: Si tenemos múltiples amenazas y el oponente no tiene ninguna
    if ((myFourOpen + myFourHalf > 1) && (oppFourOpen + oppFourHalf == 0)) {
        threatScore += 85000;  // Victoria garantizada con múltiples amenazas
    }
    
    return threatScore;
}

// NUEVO: Función auxiliar para contar patrones específicos reutilizando lógica existente
int Evaluator::countPatternType(const GameState& state, int player, int consecutiveCount, int freeEnds) {
    int count = 0;
    
    // Reutilizar la lógica existente de analyzePosition pero con filtros específicos
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
                    
                    // FILTRO: Solo contar el patrón específico que buscamos
                    if (pattern.consecutiveCount == consecutiveCount && 
                        pattern.freeEnds == freeEnds) {
                        count++;
                    }
                }
            }
        }
    }
    
    return count;
}