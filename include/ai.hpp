/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ai.hpp                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/21 16:35:45 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/30 17:10:57 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef AI_HPP
#define AI_HPP

#include "game_types.hpp"
#include "transposition_search.hpp"

enum AIImplementation {
    CPP_IMPLEMENTATION,
    RUST_IMPLEMENTATION
};

class AI {
public:
    AI(int searchDepth = 10, AIImplementation impl = CPP_IMPLEMENTATION)
        : depth(searchDepth), implementation(impl) {}
    
    // Obtener mejor movimiento para el estado actual
    Move getBestMove(const GameState& state);

	int getDepthForGamePhase(const GameState &state);

	// Configurar profundidad de búsqueda
    void setDepth(int newDepth) { depth = newDepth; }
    int getDepth() const { return depth; }
    
    // Set AI implementation
    void setImplementation(AIImplementation impl) { implementation = impl; }
    AIImplementation getImplementation() const { return implementation; }
    
    // Get statistics from last search
    int getLastNodesEvaluated() const { return lastResult.nodesEvaluated; }
    int getLastScore() const { return lastResult.score; }
    int getLastCacheHits() const { return lastResult.cacheHits; }
    float getLastCacheHitRate() const { return lastResult.cacheHitRate; }
    size_t getCacheSize() const { return searchEngine.getCacheSize(); }
    
    // Cache management
    void clearCache() { searchEngine.clearCache(); }
    
    // Funciones adicionales para game_engine
    TranspositionSearch::SearchResult findBestMoveIterative(const GameState& state, int maxDepth);
    std::vector<Move> generateOrderedMoves(const GameState& state);
    int quickEvaluateMove(const GameState& state, const Move& move);
    
private:
    int depth;
    AIImplementation implementation;
    TranspositionSearch searchEngine;
    TranspositionSearch::SearchResult lastResult;
};

#endif