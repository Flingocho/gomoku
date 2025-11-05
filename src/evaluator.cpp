/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   evaluator.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/14 21:24:46 by jainavas          #+#    #+#             */
/*   Updated: 2025/10/01 23:06:33 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/evaluator.hpp"
#include "../include/rule_engine.hpp"
#include <iostream>

EvaluationDebugCapture g_evalDebug;

// NUEVO: Evaluador principal con información de distancia al mate
int Evaluator::evaluate(const GameState &state, int maxDepth, int currentDepth)
{
	int mateDistance = maxDepth - currentDepth;

	// Verificar condiciones de fin inmediato CON distancia al mate
	// En modo de solo capturas, verificar 15 capturas en lugar de 5 en línea
	if (state.captureModeOnly) {
		// CAPTURE MODE: Victoria por 15 capturas
		if (state.captures[1] >= 15) { // AI (PLAYER2 = índice 1)
			return WIN - mateDistance;
		}
		if (state.captures[0] >= 15) { // Human (PLAYER1 = índice 0)
			return -WIN + mateDistance;
		}
	} else {
		// MODO NORMAL: Victoria por 5 en línea
		if (RuleEngine::checkWin(state, GameState::PLAYER2))
		{
			// Victoria más cercana = mayor puntuación
			// WIN - mateDistance hace que mate en 1 valga más que mate en 5
			return WIN - mateDistance;
		}
		if (RuleEngine::checkWin(state, GameState::PLAYER1))
		{
			// Derrota más lejana = menos malo
			// -WIN + mateDistance hace que perder en 5 sea menos malo que perder en 1
			return -WIN + mateDistance;
		}
	}

	int aiScore = evaluateForPlayer(state, GameState::PLAYER2);
	int humanScore = evaluateForPlayer(state, GameState::PLAYER1);

	return aiScore - humanScore;
} // NUEVO: Evaluación con captura de debug real del algoritmo

// LEGACY: Mantener la versión anterior para compatibilidad (sin distancia al mate)
int Evaluator::evaluate(const GameState &state)
{
	// Verificar condiciones de fin inmediato
	// En modo de solo capturas, verificar 15 capturas en lugar de 5 en línea
	if (state.captureModeOnly) {
		if (state.captures[1] >= 15) // AI wins
			return WIN;
		if (state.captures[0] >= 15) // Human wins
			return -WIN;
	} else {
		if (RuleEngine::checkWin(state, GameState::PLAYER2))
			return WIN;
		if (RuleEngine::checkWin(state, GameState::PLAYER1))
			return -WIN;
	}

	// NUEVO: Evaluar AI con captura de debug si está activo
	if (g_evalDebug.active)
		g_evalDebug.currentPlayer = GameState::PLAYER2;
	int aiScore = evaluateForPlayer(state, GameState::PLAYER2);

	// NUEVO: Evaluar HUMAN con captura de debug si está activo
	if (g_evalDebug.active)
		g_evalDebug.currentPlayer = GameState::PLAYER1;
	int humanScore = evaluateForPlayer(state, GameState::PLAYER1);

	// NUEVO: Completar información de debug
	if (g_evalDebug.active)
	{
		g_evalDebug.totalScore = aiScore - humanScore;
		g_evalDebug.aiScore = aiScore;
		g_evalDebug.humanScore = humanScore;
	}

	return aiScore - humanScore;
}

int Evaluator::evaluateForPlayer(const GameState &state, int player)
{
	int score = 0;

	// NUEVO: Si el debug está activo y es para el jugador correcto, capturar información
	bool captureForThisPlayer = g_evalDebug.active && player == g_evalDebug.currentPlayer;

	// En modo de solo capturas, NO evaluar amenazas de alineación
	if (!state.captureModeOnly) {
		score += evaluateImmediateThreats(state, player);
	}

	// 1. EVALUACIÓN UNIFICADA: patrones + capturas en una sola pasada
	score += analyzePosition(state, player);

	// NUEVO: Capturar información si está activado
	if (captureForThisPlayer)
	{
		if (player == GameState::PLAYER2)
		{ // AI
			g_evalDebug.aiScore = score;
		}
		else
		{ // HUMAN
			g_evalDebug.humanScore = score;
		}
	}

	return score;
}

int Evaluator::analyzePosition(const GameState& state, int player) {
    int totalScore = 0;
    int opponent = state.getOpponent(player);
    
    // Variables para capturas
    int captureOpportunities = 0;
    int captureThreats = 0;
    
    // OPTIMIZACIÓN: Marcar líneas ya evaluadas
    bool evaluated[GameState::BOARD_SIZE][GameState::BOARD_SIZE][4] = {{{false}}};
    
    // ============================================
    // PARTE 1: EVALUACIÓN DE PATRONES (SOLO EN MODO NORMAL)
    // ============================================
    // En modo de solo capturas, NO evaluar patrones de alineación
    if (!state.captureModeOnly) {
        for (int i = 0; i < GameState::BOARD_SIZE; i++) {
            for (int j = 0; j < GameState::BOARD_SIZE; j++) {
                if (state.board[i][j] == player) {
                    for (int d = 0; d < 4; d++) {
                        if (evaluated[i][j][d]) continue;
                        
                        int dx = MAIN_DIRECTIONS[d][0];
                        int dy = MAIN_DIRECTIONS[d][1];
                        
                        if (isLineStart(state, i, j, dx, dy, player)) {
                            PatternInfo pattern = analyzeLine(state, i, j, dx, dy, player);
                            totalScore += patternToScore(pattern);
                            
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
            }
        }
    }
    
    // ============================================
    // PARTE 2: EVALUACIÓN DE CAPTURAS (OPTIMIZADA)
    // ============================================
    
    // ✅ CAPTURA OFENSIVA: Buscar pares del oponente que PUEDO capturar
    auto myOpportunities = findAllCaptureOpportunities(state, player);
    for (const auto& opp : myOpportunities) {
        int captureCount = opp.captured.size() / 2;
        int currentCaptures = state.captures[player - 1];
        int newTotal = currentCaptures + captureCount;
        
        captureOpportunities += evaluateCaptureContext(
            state, player, opp.captured, newTotal
        );
    }
    
    // ✅ CAPTURA DEFENSIVA: Buscar pares MÍOS que el oponente puede capturar
    auto oppThreats = findAllCaptureOpportunities(state, opponent);
    for (const auto& threat : oppThreats) {
        int captureCount = threat.captured.size() / 2;
        int oppCurrentCaptures = state.captures[opponent - 1];
        int oppNewTotal = oppCurrentCaptures + captureCount;
        
        captureThreats += evaluateCaptureContext(
            state, opponent, threat.captured, oppNewTotal
        );
    }
    
    // ============================================
    // PARTE 3: SCORING DE CAPTURAS EXISTENTES
    // ============================================
    int myCaptures = state.captures[player - 1];
    int oppCaptures = state.captures[opponent - 1];
    
    if (myCaptures >= 9) totalScore += 300000;
    else if (myCaptures >= 8) totalScore += 200000;
    else if (myCaptures >= 6) totalScore += 15000;
    else if (myCaptures >= 4) totalScore += 6000;
    else totalScore += myCaptures * 500;
    
    if (oppCaptures >= 9) totalScore -= 400000;
    else if (oppCaptures >= 8) totalScore -= 300000;
    else if (oppCaptures >= 6) totalScore -= 20000;
    else if (oppCaptures >= 4) totalScore -= 8000;
    else totalScore -= oppCaptures * 800;
    
    totalScore += captureOpportunities;
    totalScore -= captureThreats;
    
    return totalScore;
}

bool Evaluator::isLineStart(const GameState &state, int x, int y, int dx, int dy, int player)
{
	// Es inicio si la posición anterior no tiene pieza del mismo jugador
	int prevX = x - dx;
	int prevY = y - dy;

	return !state.isValid(prevX, prevY) || state.getPiece(prevX, prevY) != player;
}

Evaluator::PatternInfo Evaluator::analyzeLine(const GameState &state, int x, int y,
											  int dx, int dy, int player)
{
	PatternInfo info = {0, 0, 0, false, 0, 0};

	// PASO 1: Análisis extendido - escanear hasta 6 posiciones para detectar gaps
	const int MAX_SCAN = 6;
	int sequence[MAX_SCAN];
	int actualPositions = 0;

	// Llenar el array con el contenido de las 6 posiciones
	for (int i = 0; i < MAX_SCAN; i++)
	{
		int checkX = x + i * dx;
		int checkY = y + i * dy;

		if (!state.isValid(checkX, checkY))
		{
			break;
		}

		sequence[i] = state.getPiece(checkX, checkY);
		actualPositions = i + 1;
	}

	// PASO 2: Analizar patrones consecutivos desde el inicio
	int consecutiveFromStart = 0;
	while (consecutiveFromStart < actualPositions &&
		   sequence[consecutiveFromStart] == player)
	{
		consecutiveFromStart++;
	}

	info.consecutiveCount = consecutiveFromStart;

	// PASO 3: Si tenemos 5+ consecutivas, es victoria inmediata
	if (info.consecutiveCount >= 5)
	{
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
	for (int i = 0; i < actualPositions && i < 6; i++)
	{
		if (sequence[i] == player)
		{
			totalPieces++;
			lastPiecePos = i;
		}
		else if (sequence[i] != GameState::EMPTY)
		{
			// Si hay una pieza del oponente, cortar el análisis aquí
			break;
		}
		else if (totalPieces > 0)
		{
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
	if (state.isValid(backX, backY) && state.isEmpty(backX, backY))
	{
		info.freeEnds++;
	}

	// Verificar extremo delantero (después del último elemento analizado)
	int frontX = x + totalSpan * dx;
	int frontY = y + totalSpan * dy;
	if (state.isValid(frontX, frontY) && state.isEmpty(frontX, frontY))
	{
		info.freeEnds++;
	}

	// PASO 8: Asignar valores finales
	info.totalPieces = totalPieces;
	info.totalSpan = totalSpan;
	info.hasGaps = hasGaps;
	info.gapCount = gapCount;

	return info;
}

int Evaluator::patternToScore(const PatternInfo &pattern)
{
	int consecutiveCount = pattern.consecutiveCount;
	int totalPieces = pattern.totalPieces;
	int freeEnds = pattern.freeEnds;
	bool hasGaps = pattern.hasGaps;

	// PASO 1: Patrones de victoria (5+ piezas consecutivas o con gaps válidos)
	if (consecutiveCount >= 5)
		return WIN;

	// NUEVO: Victoria con gaps - X-XXXX, XX-XXX, etc.
	if (totalPieces >= 5 && hasGaps && freeEnds >= 1)
	{
		return WIN; // También es victoria
	}

	// PASO 2: Patrones críticos de 4 piezas
	if (totalPieces == 4)
	{
		// Caso 1: 4 consecutivas (XXXX)
		if (consecutiveCount == 4)
		{
			if (freeEnds == 2)
			{
				if (g_evalDebug.active)
				{
					if (g_evalDebug.currentPlayer == GameState::PLAYER2)
					{
						g_evalDebug.aiFourOpen++;
					}
					else
					{
						g_evalDebug.humanFourOpen++;
					}
				}
				return FOUR_OPEN; // Imparable
			}
			if (freeEnds == 1)
			{
				if (g_evalDebug.active)
				{
					if (g_evalDebug.currentPlayer == GameState::PLAYER2)
					{
						g_evalDebug.aiFourHalf++;
					}
					else
					{
						g_evalDebug.humanFourHalf++;
					}
				}
				return FOUR_HALF; // Amenaza forzada
			}
		}
		// Caso 2: 4 con gaps (X-XXX, XX-XX, XXX-X)
		else if (hasGaps)
		{
			if (freeEnds == 2)
			{
				if (g_evalDebug.active)
				{
					if (g_evalDebug.currentPlayer == GameState::PLAYER2)
					{
						g_evalDebug.aiFourOpen++;
					}
					else
					{
						g_evalDebug.humanFourOpen++;
					}
				}
				return FOUR_OPEN; // ¡CRÍTICO! X-XXX es imparable
			}
			if (freeEnds == 1)
			{
				if (g_evalDebug.active)
				{
					if (g_evalDebug.currentPlayer == GameState::PLAYER2)
					{
						g_evalDebug.aiFourHalf++;
					}
					else
					{
						g_evalDebug.humanFourHalf++;
					}
				}
				return FOUR_HALF; // Amenaza fuerte
			}
		}
	}

	// PASO 3: Patrones de 3 piezas
	if (totalPieces == 3)
	{
		// Caso 1: 3 consecutivas (XXX)
		if (consecutiveCount == 3)
		{
			if (freeEnds == 2)
			{
				if (g_evalDebug.active)
				{
					if (g_evalDebug.currentPlayer == GameState::PLAYER2)
					{
						g_evalDebug.aiThreeOpen++;
					}
					else
					{
						g_evalDebug.humanThreeOpen++;
					}
				}
				return THREE_OPEN; // Muy peligroso
			}
			if (freeEnds == 1)
				return THREE_HALF; // Amenaza
		}
		// Caso 2: 3 con gaps (X-XX, XX-X)
		else if (hasGaps)
		{
			if (freeEnds == 2)
			{
				if (g_evalDebug.active)
				{
					if (g_evalDebug.currentPlayer == GameState::PLAYER2)
					{
						g_evalDebug.aiThreeOpen++;
					}
					else
					{
						g_evalDebug.humanThreeOpen++;
					}
				}
				return THREE_OPEN; // También peligroso
			}
			if (freeEnds == 1)
				return THREE_HALF; // Amenaza partida
		}
	}

	// PASO 4: Patrones de 2 piezas (desarrollo)
	if (totalPieces == 2 && freeEnds == 2)
	{
		if (g_evalDebug.active)
		{
			if (g_evalDebug.currentPlayer == GameState::PLAYER2)
			{
				g_evalDebug.aiTwoOpen++;
			}
			else
			{
				g_evalDebug.humanTwoOpen++;
			}
		}
		return TWO_OPEN; // Desarrollo (XX o X-X)
	}

	return 0;
}

// ALTERNATIVA MÁS EFICIENTE: Solo evaluar posiciones relevantes
int Evaluator::evaluateCaptures(const GameState &state, int player)
{
	int score = 0;
	int opponent = state.getOpponent(player);

	// Puntuación por capturas ya realizadas (igual que antes)
	int myCaptures = state.captures[player - 1];
	int oppCaptures = state.captures[opponent - 1];

	if (myCaptures >= 8)
		score += 15000;
	else if (myCaptures >= 6)
		score += 6000;
	else if (myCaptures >= 4)
		score += 2000;
	else
		score += myCaptures * 1000;

	if (oppCaptures >= 8)
		score -= 15000;
	else if (oppCaptures >= 6)
		score -= 6000;
	else if (oppCaptures >= 4)
		score -= 2000;
	else
		score -= oppCaptures * 1000;

	// OPTIMIZACIÓN: Solo evaluar posiciones adyacentes a piezas existentes
	int myCaptureOpportunities = 0;
	int oppCaptureOpportunities = 0;

	for (int i = 0; i < GameState::BOARD_SIZE; i++)
	{
		for (int j = 0; j < GameState::BOARD_SIZE; j++)
		{
			if (!state.isEmpty(i, j))
				continue;

			// Solo evaluar si está cerca de alguna pieza (optimización)
			bool nearPiece = false;
			for (int di = -1; di <= 1 && !nearPiece; di++)
			{
				for (int dj = -1; dj <= 1 && !nearPiece; dj++)
				{
					int ni = i + di, nj = j + dj;
					if (state.isValid(ni, nj) && state.getPiece(ni, nj) != GameState::EMPTY)
					{
						nearPiece = true;
					}
				}
			}

			if (!nearPiece)
				continue; // Skip posiciones lejanas

			Move pos(i, j);

			// Capturas ofensivas
			auto myCaptures = RuleEngine::findCaptures(state, pos, player);
			if (!myCaptures.empty())
			{
				myCaptureOpportunities += myCaptures.size() / 2;
			}

			// Capturas defensivas (amenazas del oponente)
			auto oppCaptures = RuleEngine::findCaptures(state, pos, opponent);
			if (!oppCaptures.empty())
			{
				oppCaptureOpportunities += oppCaptures.size() / 2;
			}
		}
	}

	score += myCaptureOpportunities * CAPTURE_OPPORTUNITY;
	score -= oppCaptureOpportunities * CAPTURE_THREAT;

	return score;
}

// En src/evaluator.cpp, reemplazar evaluateImmediateThreats:

int Evaluator::evaluateImmediateThreats(const GameState &state, int player)
{
	int opponent = state.getOpponent(player);
	int threatScore = 0;

	// NUEVA LÓGICA EFICIENTE: Detectar amenazas usando patrones existentes
	bool myHasWinThreats = hasWinningThreats(state, player);
	bool oppHasWinThreats = hasWinningThreats(state, opponent);

	// 1. Si tengo amenazas de mate en 1
	if (myHasWinThreats)
	{
		threatScore += 90000; // Muy bueno - puedo ganar
	}

	// 2. CRÍTICO: Si el oponente tiene amenazas de mate en 1
	if (oppHasWinThreats)
	{
		threatScore -= 105000; // Muy malo - debo defender
	}

	// 3. LÓGICA ORIGINAL: Contar patrones de 4 (como respaldo)
	int myFourOpen = countPatternType(state, player, 4, 2);
	int oppFourOpen = countPatternType(state, opponent, 4, 2);
	int myFourHalf = countPatternType(state, player, 4, 1);
	int oppFourHalf = countPatternType(state, opponent, 4, 1);

	if (oppFourOpen > 0)
	{
		threatScore -= 80000;
	}
	if (oppFourHalf > 0)
	{
		threatScore -= 60000;
	}
	if (myFourOpen > 0)
	{
		threatScore += 70000;
	}
	if (myFourHalf > 0)
	{
		threatScore += 40000;
	}

	return threatScore;
}

// NUEVA: Función EFICIENTE que detecta amenazas de mate usando patrones existentes
bool Evaluator::hasWinningThreats(const GameState &state, int player)
{
	// Buscar patrones de 4 consecutivas con al menos un extremo libre
	// Esto indica una amenaza de mate en 1 movimiento

	// FOUR_OPEN (4 con ambos extremos libres) = amenaza imparable
	int fourOpen = countPatternType(state, player, 4, 2);
	if (fourOpen > 0)
	{
		return true; // Amenaza imparable
	}

	// FOUR_HALF (4 con un extremo libre) = amenaza forzada
	int fourHalf = countPatternType(state, player, 4, 1);
	if (fourHalf > 0)
	{
		return true; // Amenaza que requiere defensa
	}

	// Verificar múltiples THREE_OPEN que crean amenazas duales
	int threeOpen = countPatternType(state, player, 3, 2);
	if (threeOpen >= 2)
	{
		return true; // Múltiples amenazas de 3 abiertas = mate probable
	}

	return false;
}

// NUEVA: Verificar si un movimiento reduce las amenazas del oponente
bool Evaluator::moveBlocksThreat(const Move &move, const Move & /* threat */)
{
	// Simplificación: cualquier movimiento táctico puede potencialmente bloquear amenazas
	// Esta función se mantiene por compatibilidad pero se simplifica
	return (move.x >= 0 && move.y >= 0); // Movimiento válido
}

// NUEVO: Función auxiliar para contar patrones específicos reutilizando lógica existente
int Evaluator::countPatternType(const GameState &state, int player, int consecutiveCount, int freeEnds)
{
	int count = 0;

	// Reutilizar la lógica existente de analyzePosition pero con filtros específicos
	for (int i = 0; i < GameState::BOARD_SIZE; i++)
	{
		for (int j = 0; j < GameState::BOARD_SIZE; j++)
		{
			// Solo analizar desde piezas del jugador
			if (state.board[i][j] != player)
				continue;

			// Analizar en las 4 direcciones principales desde esta pieza
			for (int d = 0; d < 4; d++)
			{
				int dx = MAIN_DIRECTIONS[d][0];
				int dy = MAIN_DIRECTIONS[d][1];

				// Solo evaluar si es el inicio de la línea (evita duplicados)
				if (isLineStart(state, i, j, dx, dy, player))
				{
					PatternInfo pattern = analyzeLine(state, i, j, dx, dy, player);

					// CORRECCIÓN: Considerar también patrones con gaps
					// Para patrones de 4: pueden ser 4 consecutivos O 4 con gaps
					bool matches = false;
					
					if (consecutiveCount == 4) {
						// Para 4 en línea: aceptar 4 consecutivos O 4 con gaps
						matches = (pattern.consecutiveCount == 4 && pattern.freeEnds == freeEnds) ||
								 (pattern.totalPieces == 4 && pattern.hasGaps && pattern.freeEnds == freeEnds);
					} else {
						// Para otros patrones, usar la lógica original
						matches = (pattern.consecutiveCount == consecutiveCount && pattern.freeEnds == freeEnds);
					}
					
					if (matches) {
						count++;
					}
				}
			}
		}
	}

	return count;
}

bool Evaluator::isValidCapturePattern(const GameState &state, int x, int y,
									  int dx, int dy, int attacker, int victim)
{
	// Verificar patrón: NUEVA(x,y) + VICTIM + VICTIM + ATTACKER
	int pos1X = x + dx, pos1Y = y + dy;
	int pos2X = x + 2 * dx, pos2Y = y + 2 * dy;
	int pos3X = x + 3 * dx, pos3Y = y + 3 * dy;

	return state.isValid(pos1X, pos1Y) && state.isValid(pos2X, pos2Y) && state.isValid(pos3X, pos3Y) &&
		   state.getPiece(pos1X, pos1Y) == victim &&
		   state.getPiece(pos2X, pos2Y) == victim &&
		   state.getPiece(pos3X, pos3Y) == attacker;
}

bool Evaluator::captureBreaksOpponentPattern(const GameState &state,
								  const std::vector<Move> &capturedPieces,
								  int opponent)
{
	// Para cada ficha capturada
	for (const Move &captured : capturedPieces)
	{
		// Verificar las 4 direcciones principales desde esa posición
		for (int d = 0; d < 4; d++)
		{
			int dx = MAIN_DIRECTIONS[d][0];
			int dy = MAIN_DIRECTIONS[d][1];

			// ¿Había un patrón peligroso del oponente aquí?
			int countBefore = countPatternThroughPosition(
				state, captured, dx, dy, opponent);

			// Si había 3+ en línea pasando por aquí → lo rompemos
			if (countBefore >= 3)
			{
				return true; // ¡Captura defensiva valiosa!
			}
		}
	}
	return false;
}

int Evaluator::countPatternThroughPosition(const GameState &state,
								const Move &pos,
								int dx, int dy,
								int player)
{
	int count = 0;

	// Contar hacia atrás
	int x = pos.x - dx, y = pos.y - dy;
	while (state.isValid(x, y) && state.getPiece(x, y) == player)
	{
		count++;
		x -= dx;
		y -= dy;
	}

	// Contar hacia adelante
	x = pos.x + dx;
	y = pos.y + dy;
	while (state.isValid(x, y) && state.getPiece(x, y) == player)
	{
		count++;
		x += dx;
		y += dy;
	}

	return count;
}

int Evaluator::evaluateCaptureContext(const GameState &state, int player, const std::vector<Move, std::allocator<Move>> &capturedPieces, int newCaptureCount)
{
	int value = 0;
	int opponent = state.getOpponent(player);

	// 1. VALOR BASE: Proximidad a victoria por captura
	if (newCaptureCount >= 10)
	{
		return 500000; // ¡VICTORIA INMEDIATA!
	}
	else if (newCaptureCount == 9)
	{
		value += 100000; // Una captura más y ganamos
	}
	else if (newCaptureCount >= 8)
	{
		value += 50000; // Muy cerca
	}
	else if (newCaptureCount >= 6)
	{
		value += 15000; // Presión considerable
	}
	else
	{
		value += newCaptureCount * 2000; // Desarrollo normal
	}

	// 2. VALOR DEFENSIVO: ¿Rompe patrones del oponente?
	for (const Move &captured : capturedPieces)
	{
		// Revisar las 4 direcciones principales
		for (int d = 0; d < 4; d++)
		{
			int dx = MAIN_DIRECTIONS[d][0];
			int dy = MAIN_DIRECTIONS[d][1];

			// Contar patrón que existía a través de esta posición
			int patternSize = countPatternThroughPosition(
				state, captured, dx, dy, opponent);

			// Scoring según qué tan peligroso era el patrón
			if (patternSize >= 4)
			{
				value += 30000; // ¡Rompimos un 4 en línea!
			}
			else if (patternSize == 3)
			{
				value += 12000; // Rompimos un 3 en línea
			}
			else if (patternSize == 2)
			{
				value += 3000; // Rompimos un 2 en línea
			}
		}
	}

	// 3. VALOR OFENSIVO: ¿Crea espacio para nuestros patrones?
	// Las posiciones capturadas ahora están vacías
	// ¿Mejoran nuestras líneas adyacentes?
	for (const Move &captured : capturedPieces)
	{
		// Verificar si ahora podemos extender nuestras líneas
		for (int d = 0; d < 4; d++)
		{
			int dx = MAIN_DIRECTIONS[d][0];
			int dy = MAIN_DIRECTIONS[d][1];

			// ¿Hay fichas nuestras adyacentes?
			int x1 = captured.x + dx, y1 = captured.y + dy;
			int x2 = captured.x - dx, y2 = captured.y - dy;

			bool hasMyPieceAdjacent =
				(state.isValid(x1, y1) && state.getPiece(x1, y1) == player) ||
				(state.isValid(x2, y2) && state.getPiece(x2, y2) == player);

			if (hasMyPieceAdjacent)
			{
				value += 1500; // Crea oportunidades tácticas
			}
		}
	}

	// 4. PELIGRO: ¿El oponente está cerca de ganar por captura?
	int opponentCaptures = state.captures[opponent - 1];
	if (opponentCaptures >= 8)
	{
		// Captura defensiva crítica si evita que capturen más
		value += 25000;
	}

	return value;
}

std::vector<Evaluator::CaptureOpportunity> Evaluator::findAllCaptureOpportunities(
    const GameState& state, 
    int player) {
    
    std::vector<CaptureOpportunity> opportunities;
    int opponent = state.getOpponent(player);
    
    // Recorrer todas las posiciones buscando PARES del oponente
    for (int i = 0; i < GameState::BOARD_SIZE; i++) {
        for (int j = 0; j < GameState::BOARD_SIZE; j++) {
            
            // ¿Hay una pieza del oponente aquí?
            if (state.board[i][j] != opponent) continue;
            
            // Verificar las 4 direcciones principales
            for (int d = 0; d < 4; d++) {
                int dx = MAIN_DIRECTIONS[d][0];
                int dy = MAIN_DIRECTIONS[d][1];
                
                // ¿Hay OTRA pieza del oponente en esta dirección?
                int x2 = i + dx, y2 = j + dy;
                if (!state.isValid(x2, y2) || state.board[x2][y2] != opponent) 
                    continue;
                
                // ✅ Encontramos un PAR: (i,j) - (x2,y2)
                
                // OPCIÓN 1: Flanquear por DELANTE
                int xFront = x2 + dx, yFront = y2 + dy;
                if (state.isValid(xFront, yFront) && state.isEmpty(xFront, yFront)) {
                    // ¿Tengo una pieza MÍA detrás del par?
                    int xBack = i - dx, yBack = j - dy;
                    if (state.isValid(xBack, yBack) && state.board[xBack][yBack] == player) {
                        // ✅ Patrón: X-OO-[VACÍO] → Puedo jugar en VACÍO
                        CaptureOpportunity opp;
                        opp.position = Move(xFront, yFront);
                        opp.captured = {Move(i, j), Move(x2, y2)};
                        opportunities.push_back(opp);
                    }
                }
                
                // OPCIÓN 2: Flanquear por DETRÁS
                int xBack = i - dx, yBack = j - dy;
                if (state.isValid(xBack, yBack) && state.isEmpty(xBack, yBack)) {
                    // ¿Tengo una pieza MÍA delante del par?
                    int xFront2 = x2 + dx, yFront2 = y2 + dy;
                    if (state.isValid(xFront2, yFront2) && state.board[xFront2][yFront2] == player) {
                        // ✅ Patrón: [VACÍO]-OO-X → Puedo jugar en VACÍO
                        CaptureOpportunity opp;
                        opp.position = Move(xBack, yBack);
                        opp.captured = {Move(i, j), Move(x2, y2)};
                        opportunities.push_back(opp);
                    }
                }
            }
        }
    }
    
    return opportunities;
}
