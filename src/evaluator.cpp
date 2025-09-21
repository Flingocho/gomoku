/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   evaluator.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/14 21:24:46 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/21 18:19:36 by jainavas         ###   ########.fr       */
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

// En src/evaluator.cpp, reemplazar evaluateImmediateThreats:

int Evaluator::evaluateImmediateThreats(const GameState& state, int player) {
    int opponent = state.getOpponent(player);
    int threatScore = 0;
    
    // NUEVA LÓGICA: Detectar amenazas de mate en 1 real
    std::vector<Move> myMateThreats = findMateInOneThreats(state, player);
    std::vector<Move> oppMateThreats = findMateInOneThreats(state, opponent);
    
    // 1. Si tengo amenazas de mate en 1
    if (!myMateThreats.empty()) {
        threatScore += 90000; // Muy bueno - puedo ganar
    }
    
    // 2. CRÍTICO: Si el oponente tiene amenazas de mate en 1
    if (!oppMateThreats.empty()) {
        threatScore -= 95000; // Muy malo - debo defender
        
        // Si hay múltiples amenazas del oponente = desastre
        if (oppMateThreats.size() > 1) {
            threatScore -= 100000; // Imposible defender múltiples amenazas
        }
    }
    
    // 3. LÓGICA ORIGINAL: Contar patrones de 4 (como respaldo)
    int myFourOpen = countPatternType(state, player, 4, 2);
    int oppFourOpen = countPatternType(state, opponent, 4, 2);
    int myFourHalf = countPatternType(state, player, 4, 1);
    int oppFourHalf = countPatternType(state, opponent, 4, 1);
    
    if (oppFourOpen > 0) {
        threatScore -= 80000;
    }
    if (oppFourHalf > 0) {
        threatScore -= 60000;
    }
    if (myFourOpen > 0) {
        threatScore += 70000;
    }
    if (myFourHalf > 0) {
        threatScore += 40000;
    }
    
    return threatScore;
}

// NUEVA: Función que detecta amenazas reales de mate en 1
std::vector<Move> Evaluator::findMateInOneThreats(const GameState& state, int player) {
    std::vector<Move> threats;
    
    // Buscar todas las posiciones vacías
    for (int i = 0; i < GameState::BOARD_SIZE; i++) {
        for (int j = 0; j < GameState::BOARD_SIZE; j++) {
            if (state.isEmpty(i, j)) {
                Move candidate(i, j);
                
                // Simular el movimiento
                GameState tempState = state;
                tempState.board[i][j] = player;
                
                // ¿Este movimiento resulta en victoria?
                if (RuleEngine::checkWin(tempState, player)) {
                    threats.push_back(candidate);
                }
            }
        }
    }
    
    return threats;
}

// NUEVA: Verificar si un movimiento bloquea una amenaza específica
bool Evaluator::moveBlocksThreat(const Move& move, const Move& threat) {
    // Bloquea si ocupa la misma posición que la amenaza
    return (move.x == threat.x && move.y == threat.y);
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