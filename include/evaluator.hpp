/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   evaluator.hpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/14 21:16:58 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/22 16:46:31 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef EVALUATOR_HPP
#define EVALUATOR_HPP

#include "game_types.hpp"

class Evaluator
{
public:
	static constexpr int WIN = 100000;
	static constexpr int FOUR_OPEN = 50000;
	static constexpr int FOUR_HALF = 10000;
	static constexpr int THREE_OPEN = 5000;
	static constexpr int THREE_HALF = 1500;
	static constexpr int TWO_OPEN = 100;
	static constexpr int CAPTURE_OPPORTUNITY = 2000;
	static constexpr int CAPTURE_THREAT = 500;

	static int evaluateForPlayer(const GameState &state, int player);

	// NUEVO: Evaluador principal con información de distancia al mate
	static int evaluate(const GameState &state, int maxDepth, int currentDepth);

	// LEGACY: Mantener la versión anterior para compatibilidad
	static int evaluate(const GameState &state);

	static int evaluateImmediateThreats(const GameState &state, int player);

	static bool moveBlocksThreat(const Move &move, const Move &threat);

	// NUEVO: Detección eficiente de amenazas de mate en 1 usando patrones
	static bool hasWinningThreats(const GameState &state, int player);

private:
	struct PatternInfo
	{
		int consecutiveCount;  // Piezas consecutivas máximas
		int totalPieces;       // NUEVO: Total de piezas en el patrón (incluyendo gaps)
		int freeEnds;          // Extremos libres
		bool hasGaps;          // Si tiene gaps pequeños
		int totalSpan;         // Span total del patrón
		int gapCount;          // NUEVO: Número de gaps
	};

	static int analyzePosition(const GameState &state, int player);

	static PatternInfo analyzeLine(const GameState &state, int x, int y,
								   int dx, int dy, int player);

	static int patternToScore(const PatternInfo &pattern);

	static int evaluateCaptures(const GameState &state, int player);

	// NUEVO: Evaluación de amenazas inmediatas

	// NUEVO: Función auxiliar para contar patrones específicos
	static int countPatternType(const GameState &state, int player, int consecutiveCount, int freeEnds);

	static bool isLineStart(const GameState &state, int x, int y, int dx, int dy, int player);

	static constexpr int MAIN_DIRECTIONS[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};
};

#endif