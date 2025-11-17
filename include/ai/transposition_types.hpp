#ifndef TRANSPOSITION_TYPES_HPP
#define TRANSPOSITION_TYPES_HPP

#include "../core/game_types.hpp"
#include <cstdint>

/**
 * Data structures for transposition table and search results
 * Used by: TranspositionSearch
 */

// ============================================
// SEARCH RESULT
// ============================================

/**
 * Result of a minimax search with statistics
 */
struct SearchResult
{
	Move bestMove;
	int score;
	int nodesEvaluated;
	int cacheHits;
	float cacheHitRate;

	SearchResult() : bestMove(), score(0), nodesEvaluated(0), cacheHits(0), cacheHitRate(0.0f) {}
};

// ============================================
// CACHE ENTRY
// ============================================

/**
 * Transposition table cache entry
 * Stores position evaluation with Zobrist key for collision detection
 */
struct CacheEntry
{
	uint64_t zobristKey; // Full Zobrist key for collision verification
	int score;
	int depth;
	Move bestMove;
	uint32_t generation; // For aging-based replacement strategy

	enum Type
	{
		EXACT,        // Exact score
		LOWER_BOUND,  // Alpha cutoff (score >= this value)
		UPPER_BOUND   // Beta cutoff (score <= this value)
	} type;

	CacheEntry() : zobristKey(0), score(0), depth(0), generation(0), type(EXACT) {}
	
	CacheEntry(uint64_t key, int s, int d, Move m, Type t, uint32_t gen = 0)
		: zobristKey(key), score(s), depth(d), bestMove(m), generation(gen), type(t) {}

	/**
	 * Calculate importance value for replacement strategy
	 * Higher values = more important to keep in cache
	 */
	int getImportanceValue() const
	{
		int value = depth * 100; // Depth is most important
		if (type == EXACT)
			value += 50; // Exact nodes are most valuable
		else if (type == LOWER_BOUND)
			value += 25;
		return value;
	}
};

// ============================================
// CACHE STATISTICS
// ============================================

/**
 * Detailed transposition table statistics
 */
struct CacheStats
{
	size_t totalEntries;
	size_t usedEntries;
	double fillRate;
	size_t collisions;
	uint32_t currentGeneration;
	size_t exactEntries;
	size_t boundEntries;
	double avgDepth;
};

#endif // TRANSPOSITION_TYPES_HPP
