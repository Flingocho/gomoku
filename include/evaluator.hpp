/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   evaluator.hpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/14 21:16:58 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/28 19:54:31 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef EVALUATOR_HPP
#define EVALUATOR_HPP

#include "game_types.hpp"

// NUEVO: Estructura simple para capturar debug durante evaluaciones reales
struct EvaluationDebugCapture
{
	bool active;
	int currentPlayer;
	Move currentMove;

	// Información capturada durante la evaluación real
	int aiScore;
	int humanScore;
	int totalScore;
	int aiThreeOpen;
	int aiFourHalf;
	int aiFourOpen;
	int aiTwoOpen;
	int humanThreeOpen;
	int humanFourHalf;
	int humanFourOpen;
	int humanTwoOpen;

	EvaluationDebugCapture() : active(false), currentPlayer(0), aiScore(0), humanScore(0), totalScore(0),
							   aiThreeOpen(0), aiFourHalf(0), aiFourOpen(0), aiTwoOpen(0),
							   humanThreeOpen(0), humanFourHalf(0), humanFourOpen(0), humanTwoOpen(0) {}

	void reset()
	{
		active = false;
		aiScore = humanScore = totalScore = 0;
		aiThreeOpen = aiFourHalf = aiFourOpen = aiTwoOpen = 0;
		humanThreeOpen = humanFourHalf = humanFourOpen = humanTwoOpen = 0;
	}
};

extern EvaluationDebugCapture g_evalDebug;

class Evaluator
{
public:
	static constexpr int WIN = 500000;
	static constexpr int FOUR_OPEN = 50000;
	static constexpr int FOUR_HALF = 25000;
	static constexpr int THREE_OPEN = 10000;
	static constexpr int THREE_HALF = 1500;
	static constexpr int TWO_OPEN = 100;
	static constexpr int CAPTURE_OPPORTUNITY = 5000;   // Era 2000
	static constexpr int CAPTURE_THREAT = 2000;		   // Era 500
	static constexpr int CAPTURE_WIN = 500000;		   // NUEVO: Captura que gana
	static constexpr int CAPTURE_PREVENT_LOSS = 400000; // NUEVO: Prevenir derrota

	static int evaluateForPlayer(const GameState &state, int player);

	// NUEVO: Evaluador principal con información de distancia al mate
	static int evaluate(const GameState &state, int maxDepth, int currentDepth);

	// LEGACY: Mantener la versión anterior para compatibilidad
	static int evaluate(const GameState &state);

	static int evaluateImmediateThreats(const GameState &state, int player);

	static bool moveBlocksThreat(const Move &move, const Move &threat);

	// NUEVO: Detección eficiente de amenazas de mate en 1 usando patrones
	static bool hasWinningThreats(const GameState &state, int player);

	// NUEVO: Función auxiliar para contar patrones específicos (pública para debug)
	static int countPatternType(const GameState &state, int player, int consecutiveCount, int freeEnds);

private:
	struct PatternInfo
	{
		int consecutiveCount; // Piezas consecutivas máximas
		int totalPieces;	  // NUEVO: Total de piezas en el patrón (incluyendo gaps)
		int freeEnds;		  // Extremos libres
		bool hasGaps;		  // Si tiene gaps pequeños
		int totalSpan;		  // Span total del patrón
		int gapCount;		  // NUEVO: Número de gaps
	};

	static int analyzePosition(const GameState &state, int player);

	static PatternInfo analyzeLine(const GameState &state, int x, int y,
								   int dx, int dy, int player);

	static int patternToScore(const PatternInfo &pattern);

	static int evaluateCaptures(const GameState &state, int player);

	// NUEVO: Evaluación de amenazas inmediatas

	static bool isLineStart(const GameState &state, int x, int y, int dx, int dy, int player);

	static constexpr int MAIN_DIRECTIONS[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};
	static constexpr int CAPTURE_DIRECTIONS[8][2] = {
		{-1, -1}, {-1, 0}, {-1, 1}, {0, -1}, {0, 1}, {1, -1}, {1, 0}, {1, 1}};

	static bool isValidCapturePattern(const GameState &state, int x, int y,
									  int dx, int dy, int attacker, int victim);
};

#endif