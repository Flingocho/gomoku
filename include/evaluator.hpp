/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   evaluator.hpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/14 21:16:58 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/14 22:06:48 by jainavas         ###   ########.fr       */
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
    
    static int evaluate(const GameState& state);
    
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
    
    static bool isLineStart(const GameState& state, int x, int y, int dx, int dy, int player);
    
    static constexpr int MAIN_DIRECTIONS[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};
};

#endif