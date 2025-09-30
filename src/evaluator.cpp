/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   evaluator.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/14 21:24:46 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/30 20:30:05 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/evaluator.hpp"
#include "../include/rule_engine.hpp"
#include <iostream>

EvaluationDebugCapture g_evalDebug;

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
}// NUEVO: Evaluación con captura de debug real del algoritmo

// LEGACY: Mantener la versión anterior para compatibilidad (sin distancia al mate)
int Evaluator::evaluate(const GameState& state) {
    // Verificar condiciones de fin inmediato
    if (RuleEngine::checkWin(state, GameState::PLAYER2)) return WIN;
    if (RuleEngine::checkWin(state, GameState::PLAYER1)) return -WIN;
    
    // NUEVO: Evaluar AI con captura de debug si está activo
    if (g_evalDebug.active) g_evalDebug.currentPlayer = GameState::PLAYER2;
    int aiScore = evaluateForPlayer(state, GameState::PLAYER2);
    
    // NUEVO: Evaluar HUMAN con captura de debug si está activo  
    if (g_evalDebug.active) g_evalDebug.currentPlayer = GameState::PLAYER1;
    int humanScore = evaluateForPlayer(state, GameState::PLAYER1);
    
    // NUEVO: Completar información de debug
    if (g_evalDebug.active) {
        g_evalDebug.totalScore = aiScore - humanScore;
        g_evalDebug.aiScore = aiScore;
        g_evalDebug.humanScore = humanScore;
    }
    
    return aiScore - humanScore;
}

int Evaluator::evaluateForPlayer(const GameState& state, int player) {
    int score = 0;
    
    // NUEVO: Si el debug está activo y es para el jugador correcto, capturar información
    bool captureForThisPlayer = g_evalDebug.active && player == g_evalDebug.currentPlayer;
    
    score += evaluateImmediateThreats(state, player);
    
    // 1. EVALUACIÓN UNIFICADA: patrones + capturas en una sola pasada
    score += analyzePosition(state, player);
    
    // NUEVO: Capturar información si está activado
    if (captureForThisPlayer) {
        if (player == GameState::PLAYER2) { // AI
            g_evalDebug.aiScore = score;
        } else { // HUMAN
            g_evalDebug.humanScore = score;
        }
    }
    
    return score;
}

int Evaluator::analyzePosition(const GameState& state, int player) {
    int totalScore = 0;
    int opponent = state.getOpponent(player);
    
    // NUEVA: Variables para capturas acumuladas
    int captureOpportunities = 0;
    int captureThreats = 0;
    
    // OPTIMIZACIÓN: Marcar líneas ya evaluadas para evitar duplicados
    bool evaluated[GameState::BOARD_SIZE][GameState::BOARD_SIZE][4] = {{{false}}};
    
    // ============================================
    // PARTE 1: EVALUACIÓN DE PATRONES (sin cambios)
    // ============================================
    for (int i = 0; i < GameState::BOARD_SIZE; i++) {
        for (int j = 0; j < GameState::BOARD_SIZE; j++) {
            if (state.board[i][j] == player) {
                // Analizar patrones en las 4 direcciones principales
                for (int d = 0; d < 4; d++) {
                    if (evaluated[i][j][d]) continue;
                    
                    int dx = MAIN_DIRECTIONS[d][0];
                    int dy = MAIN_DIRECTIONS[d][1];
                    
                    if (isLineStart(state, i, j, dx, dy, player)) {
                        PatternInfo pattern = analyzeLine(state, i, j, dx, dy, player);
                        totalScore += patternToScore(pattern);
                        
                        // Marcar toda la línea como evaluada
                        int markX = i, markY = j;
                        for (int k = 0; k < pattern.consecutiveCount && 
                                       state.isValid(markX, markY); k++) {
                            if (markX >= 0 && markX < GameState::BOARD_SIZE && 
                                markY >= 0 && markY < GameState::BOARD_SIZE) {
                                evaluated[markX][markY][d] = true;
                            }
                            markX += dx;
                            markY += dy;
                        }
                    }
                }
            }
            
            // ============================================
            // PARTE 2: EVALUACIÓN DE CAPTURAS (MEJORADA)
            // ============================================
            else if (state.board[i][j] == GameState::EMPTY) {
                // Solo evaluar capturas en posiciones vacías cercanas a piezas
                bool nearPiece = false;
                
                // Verificación rápida de proximidad (radio 1)
                for (int di = -1; di <= 1 && !nearPiece; di++) {
                    for (int dj = -1; dj <= 1 && !nearPiece; dj++) {
                        if (di == 0 && dj == 0) continue;
                        int ni = i + di, nj = j + dj;
                        if (state.isValid(ni, nj) && state.getPiece(ni, nj) != GameState::EMPTY) {
                            nearPiece = true;
                        }
                    }
                }
                
                // Solo evaluar capturas si está cerca de alguna pieza
                if (nearPiece) {
                    Move testMove(i, j);
                    
                    // CAPTURA OFENSIVA MEJORADA
                    auto myCaptures = RuleEngine::findCaptures(state, testMove, player);
                    if (!myCaptures.empty()) {
                        int captureCount = myCaptures.size() / 2;
                        int currentCaptures = state.captures[player - 1];
                        int newTotal = currentCaptures + captureCount;
                        
                        // SCORING INTELIGENTE basado en resultado final
                        if (newTotal >= 10) {
                            captureOpportunities += 70000; // ¡GANA EL JUEGO!
                        } else if (newTotal >= 8) {
                            captureOpportunities += 15000; // Muy cerca de ganar
                        } else if (newTotal >= 6) {
                            captureOpportunities += 5000;  // Progreso importante
                        } else {
                            captureOpportunities += captureCount * CAPTURE_OPPORTUNITY;
                        }
                    }
                    
                    // CAPTURA DEFENSIVA MEJORADA
                    auto oppCaptures = RuleEngine::findCaptures(state, testMove, opponent);
                    if (!oppCaptures.empty()) {
                        int oppCaptureCount = oppCaptures.size() / 2;
                        int oppCurrentCaptures = state.captures[opponent - 1];
                        int oppNewTotal = oppCurrentCaptures + oppCaptureCount;
                        
                        // SCORING DEFENSIVO basado en peligro real
                        if (oppNewTotal >= 10) {
                            captureThreats += 80000; // ¡EVITAR A TODA COSTA!
                        } else if (oppNewTotal >= 8) {
                            captureThreats += 20000; // Muy peligroso
                        } else if (oppNewTotal >= 6) {
                            captureThreats += 8000;  // Peligroso
                        } else {
                            captureThreats += oppCaptureCount * CAPTURE_THREAT;
                        }
                    }
                }
            }
        }
    }
    
    // ============================================
    // PARTE 3: SCORING DE CAPTURAS EXISTENTES (MEJORADO)
    // ============================================
    int myCaptures = state.captures[player - 1];
    int oppCaptures = state.captures[opponent - 1];
    
    // MEJORADO: Bonus/malus más agresivos por capturas ya realizadas
    if (myCaptures >= 8 && captureOpportunities > 0) totalScore += 200000;      // Era 15000 - mucho más agresivo
    else if (myCaptures >= 6 && captureOpportunities > 0) totalScore += 15000; // Era 6000
    else if (myCaptures >= 4 && captureOpportunities > 0) totalScore += 6000;  // Era 2000
    else totalScore += myCaptures * 500;           // Era 200
    
    // CRÍTICO: Penalización más severa por capturas del oponente
    if (oppCaptures >= 8 && captureThreats > 0) totalScore -= 300000;     // Era -15000 - ¡más defensivo!
    else if (oppCaptures >= 6 && captureThreats > 0) totalScore -= 20000; // Era -6000
    else if (oppCaptures >= 4 && captureThreats > 0) totalScore -= 8000;  // Era -2000
    else totalScore -= oppCaptures * 800;           // Era -200
    
    // Bonus por oportunidades de captura (usa los valores mejorados)
    totalScore += captureOpportunities;
    totalScore -= captureThreats;
    
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
    PatternInfo info = {0, 0, 0, false, 0, 0};
    
    // PASO 1: Análisis extendido - escanear hasta 6 posiciones para detectar gaps
    const int MAX_SCAN = 6;
    int sequence[MAX_SCAN];
    int actualPositions = 0;
    
    // Llenar el array con el contenido de las 6 posiciones
    for (int i = 0; i < MAX_SCAN; i++) {
        int checkX = x + i * dx;
        int checkY = y + i * dy;
        
        if (!state.isValid(checkX, checkY)) {
            break;
        }
        
        sequence[i] = state.getPiece(checkX, checkY);
        actualPositions = i + 1;
    }
    
    // PASO 2: Analizar patrones consecutivos desde el inicio
    int consecutiveFromStart = 0;
    while (consecutiveFromStart < actualPositions && 
           sequence[consecutiveFromStart] == player) {
        consecutiveFromStart++;
    }
    
    info.consecutiveCount = consecutiveFromStart;
    
    // PASO 3: Si tenemos 5+ consecutivas, es victoria inmediata
    if (info.consecutiveCount >= 5) {
        info.totalPieces = info.consecutiveCount;
        info.totalSpan = info.consecutiveCount;
        info.freeEnds = 2;
        return info;
    }
    
    // PASO 4: Análisis de patrones con gaps (X-XXX, XX-XX, etc.)
    int totalPieces = 0;
    int gapCount = 0;
    int lastPiecePos = -1;
    
    // Contar piezas totales y gaps en los primeros 5-6 espacios
    for (int i = 0; i < actualPositions && i < 6; i++) {
        if (sequence[i] == player) {
            totalPieces++;
            lastPiecePos = i;
        } else if (sequence[i] != GameState::EMPTY) {
            // Si hay una pieza del oponente, cortar el análisis aquí
            break;
        } else if (totalPieces > 0) {
            // Es un gap (espacio vacío después de encontrar piezas)
            gapCount++;
        }
    }
    
    // PASO 5: Determinar el span total (desde primera hasta última pieza)
    int totalSpan = lastPiecePos + 1;
    
    // PASO 6: Detectar si tiene gaps significativos
    bool hasGaps = (gapCount > 0 && totalPieces > info.consecutiveCount);
    
    // PASO 7: Calcular extremos libres
    info.freeEnds = 0;
    
    // Verificar extremo trasero (antes del inicio)
    int backX = x - dx, backY = y - dy;
    if (state.isValid(backX, backY) && state.isEmpty(backX, backY)) {
        info.freeEnds++;
    }
    
    // Verificar extremo delantero (después del último elemento analizado)
    int frontX = x + totalSpan * dx;
    int frontY = y + totalSpan * dy;
    if (state.isValid(frontX, frontY) && state.isEmpty(frontX, frontY)) {
        info.freeEnds++;
    }
    
    // PASO 8: Asignar valores finales
    info.totalPieces = totalPieces;
    info.totalSpan = totalSpan;
    info.hasGaps = hasGaps;
    info.gapCount = gapCount;
    
    return info;
}

int Evaluator::patternToScore(const PatternInfo& pattern) {
    int consecutiveCount = pattern.consecutiveCount;
    int totalPieces = pattern.totalPieces;
    int freeEnds = pattern.freeEnds;
    bool hasGaps = pattern.hasGaps;
    
    // PASO 1: Patrones de victoria (5+ piezas consecutivas o con gaps válidos)
    if (consecutiveCount >= 5) return WIN;
    
    // NUEVO: Victoria con gaps - X-XXXX, XX-XXX, etc.
    if (totalPieces >= 5 && hasGaps && freeEnds >= 1) {
        return WIN; // También es victoria
    }
    
    // PASO 2: Patrones críticos de 4 piezas
    if (totalPieces == 4) {
        // Caso 1: 4 consecutivas (XXXX)
        if (consecutiveCount == 4) {
            if (freeEnds == 2) {
                if (g_evalDebug.active) {
                    if (g_evalDebug.currentPlayer == GameState::PLAYER2) {
                        g_evalDebug.aiFourOpen++;
                    } else {
                        g_evalDebug.humanFourOpen++;
                    }
                }
                return FOUR_OPEN;    // Imparable
            }
            if (freeEnds == 1) {
                if (g_evalDebug.active) {
                    if (g_evalDebug.currentPlayer == GameState::PLAYER2) {
                        g_evalDebug.aiFourHalf++;
                    } else {
                        g_evalDebug.humanFourHalf++;
                    }
                }
                return FOUR_HALF;    // Amenaza forzada
            }
        }
        // Caso 2: 4 con gaps (X-XXX, XX-XX, XXX-X)
        else if (hasGaps) {
            if (freeEnds == 2) {
                if (g_evalDebug.active) {
                    if (g_evalDebug.currentPlayer == GameState::PLAYER2) {
                        g_evalDebug.aiFourOpen++;
                    } else {
                        g_evalDebug.humanFourOpen++;
                    }
                }
                return FOUR_OPEN;    // ¡CRÍTICO! X-XXX es imparable
            }
            if (freeEnds == 1) {
                if (g_evalDebug.active) {
                    if (g_evalDebug.currentPlayer == GameState::PLAYER2) {
                        g_evalDebug.aiFourHalf++;
                    } else {
                        g_evalDebug.humanFourHalf++;
                    }
                }
                return FOUR_HALF;    // Amenaza fuerte
            }
        }
    }
    
    // PASO 3: Patrones de 3 piezas
    if (totalPieces == 3) {
        // Caso 1: 3 consecutivas (XXX)
        if (consecutiveCount == 3) {
            if (freeEnds == 2) {
                if (g_evalDebug.active) {
                    if (g_evalDebug.currentPlayer == GameState::PLAYER2) {
                        g_evalDebug.aiThreeOpen++;
                    } else {
                        g_evalDebug.humanThreeOpen++;
                    }
                }
                return THREE_OPEN;   // Muy peligroso
            }
            if (freeEnds == 1) return THREE_HALF;   // Amenaza
        }
        // Caso 2: 3 con gaps (X-XX, XX-X)
        else if (hasGaps) {
            if (freeEnds == 2) {
                if (g_evalDebug.active) {
                    if (g_evalDebug.currentPlayer == GameState::PLAYER2) {
                        g_evalDebug.aiThreeOpen++;
                    } else {
                        g_evalDebug.humanThreeOpen++;
                    }
                }
                return THREE_OPEN;   // También peligroso
            }
            if (freeEnds == 1) return THREE_HALF;   // Amenaza partida
        }
    }
    
    // PASO 4: Patrones de 2 piezas (desarrollo)
    if (totalPieces == 2 && freeEnds == 2) {
        if (g_evalDebug.active) {
            if (g_evalDebug.currentPlayer == GameState::PLAYER2) {
                g_evalDebug.aiTwoOpen++;
            } else {
                g_evalDebug.humanTwoOpen++;
            }
        }
        return TWO_OPEN; // Desarrollo (XX o X-X)
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
    else score += myCaptures * 1000;
    
    if (oppCaptures >= 8) score -= 15000;
    else if (oppCaptures >= 6) score -= 6000;
    else if (oppCaptures >= 4) score -= 2000;
    else score -= oppCaptures * 1000;
    
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
    
    // NUEVA LÓGICA EFICIENTE: Detectar amenazas usando patrones existentes
    bool myHasWinThreats = hasWinningThreats(state, player);
    bool oppHasWinThreats = hasWinningThreats(state, opponent);
    
    // 1. Si tengo amenazas de mate en 1
    if (myHasWinThreats) {
        threatScore += 90000; // Muy bueno - puedo ganar
    }
    
    // 2. CRÍTICO: Si el oponente tiene amenazas de mate en 1
    if (oppHasWinThreats) {
        threatScore -= 105000; // Muy malo - debo defender
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

// NUEVA: Función EFICIENTE que detecta amenazas de mate usando patrones existentes
bool Evaluator::hasWinningThreats(const GameState& state, int player) {
    // Buscar patrones de 4 consecutivas con al menos un extremo libre
    // Esto indica una amenaza de mate en 1 movimiento
    
    // FOUR_OPEN (4 con ambos extremos libres) = amenaza imparable
    int fourOpen = countPatternType(state, player, 4, 2);
    if (fourOpen > 0) {
        return true; // Amenaza imparable
    }
    
    // FOUR_HALF (4 con un extremo libre) = amenaza forzada
    int fourHalf = countPatternType(state, player, 4, 1);
    if (fourHalf > 0) {
        return true; // Amenaza que requiere defensa
    }
    
    // Verificar múltiples THREE_OPEN que crean amenazas duales
    int threeOpen = countPatternType(state, player, 3, 2);
    if (threeOpen >= 2) {
        return true; // Múltiples amenazas de 3 abiertas = mate probable
    }
    
    return false;
}

// NUEVA: Verificar si un movimiento reduce las amenazas del oponente
bool Evaluator::moveBlocksThreat(const Move& move, const Move& /* threat */) {
    // Simplificación: cualquier movimiento táctico puede potencialmente bloquear amenazas
    // Esta función se mantiene por compatibilidad pero se simplifica
    return (move.x >= 0 && move.y >= 0); // Movimiento válido
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

bool Evaluator::isValidCapturePattern(const GameState& state, int x, int y, 
                                     int dx, int dy, int attacker, int victim) {
    // Verificar patrón: NUEVA(x,y) + VICTIM + VICTIM + ATTACKER
    int pos1X = x + dx, pos1Y = y + dy;
    int pos2X = x + 2*dx, pos2Y = y + 2*dy;
    int pos3X = x + 3*dx, pos3Y = y + 3*dy;
    
    return state.isValid(pos1X, pos1Y) && state.isValid(pos2X, pos2Y) && state.isValid(pos3X, pos3Y) &&
           state.getPiece(pos1X, pos1Y) == victim &&
           state.getPiece(pos2X, pos2Y) == victim &&
           state.getPiece(pos3X, pos3Y) == attacker;
}
