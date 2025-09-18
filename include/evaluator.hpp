/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   evaluator.hpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/14 21:16:58 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/18 17:13:44 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef EVALUATOR_HPP
#define EVALUATOR_HPP

#include "game_types.hpp"

class Evaluator {
public:
    static constexpr int WIN = 100000;
    static constexpr int FOUR_OPEN = 50000;
    static constexpr int FOUR_HALF = 10000;
    static constexpr int THREE_OPEN = 5000;
    static constexpr int THREE_HALF = 1500;
    static constexpr int TWO_OPEN = 100;
    static constexpr int CAPTURE_OPPORTUNITY = 2000;
    static constexpr int CAPTURE_THREAT = 500;
    
    static int evaluateForPlayer(const GameState& state, int player);
    
    // NUEVO: Evaluador principal con información de distancia al mate
    static int evaluate(const GameState& state, int maxDepth, int currentDepth);
    
    // LEGACY: Mantener la versión anterior para compatibilidad
    static int evaluate(const GameState& state);
	
    static int evaluateImmediateThreats(const GameState& state, int player);

    
private:
    struct PatternInfo {
        int consecutiveCount;
        int freeEnds;
        bool hasGaps;
        int totalSpan;
    };
    
    static int analyzePosition(const GameState& state, int player);
    
    static PatternInfo analyzeLine(const GameState& state, int x, int y, 
                                  int dx, int dy, int player);
    
    static int patternToScore(const PatternInfo& pattern);
    
    static int evaluateCaptures(const GameState& state, int player);
    
    // NUEVO: Evaluación de amenazas inmediatas
    
    // NUEVO: Función auxiliar para contar patrones específicos
    static int countPatternType(const GameState& state, int player, int consecutiveCount, int freeEnds);
    
    static bool isLineStart(const GameState& state, int x, int y, int dx, int dy, int player);
    
    static constexpr int MAIN_DIRECTIONS[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};
};

#endif