/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   transposition_search.hpp                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/14 21:38:39 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/14 22:07:50 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// include/transposition_search.hpp - VERSION ACTUALIZADA
#ifndef TRANSPOSITION_SEARCH_HPP
#define TRANSPOSITION_SEARCH_HPP

#include "game_types.hpp"
#include "rule_engine.hpp"
#include "evaluator.hpp"
#include <unordered_map>
#include <cstdint>
#include <vector>

class TranspositionSearch {
public:
    struct SearchResult {
        Move bestMove;
        int score;
        int nodesEvaluated;
        int cacheHits;
        float cacheHitRate;
        
        SearchResult() : bestMove(), score(0), nodesEvaluated(0), cacheHits(0), cacheHitRate(0.0f) {}
    };

private:
    using StateHash = uint64_t;
    
    struct CacheEntry {
        int score;
        int depth;
        Move bestMove;
        enum Type { EXACT, LOWER_BOUND, UPPER_BOUND } type;
        
        CacheEntry() : score(0), depth(0), type(EXACT) {}
        CacheEntry(int s, int d, Move m, Type t) : score(s), depth(d), bestMove(m), type(t) {}
    };
    
    std::unordered_map<StateHash, CacheEntry> transpositionTable;
    int nodesEvaluated;
    int cacheHits;
    
    StateHash hashState(const GameState& state) const;
    
    int calculateAdaptiveDepth(const GameState& state, int requestedDepth);
    
    int minimax(GameState& state, int depth, int alpha, int beta, bool maximizing, Move* bestMove = nullptr);
    
    std::vector<Move> generateOrderedMoves(const GameState& state);
    
    void orderMoves(std::vector<Move>& moves, const GameState& state);
    
    int quickEvaluateMove(const GameState& state, const Move& move);
    
    int countThreats(const GameState& state, int player);
    int countLinesFromPosition(const GameState& state, int x, int y, int player);
    int countInDirection(const GameState& state, int x, int y, int dx, int dy, int player);

public:
    TranspositionSearch() : nodesEvaluated(0), cacheHits(0) {}
    
    SearchResult findBestMove(const GameState& state, int maxDepth);
    
    void clearCache() { transpositionTable.clear(); }
    
    size_t getCacheSize() const { return transpositionTable.size(); }
};

#endif