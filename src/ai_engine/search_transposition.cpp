// ============================================
// SEARCH_TRANSPOSITION.CPP
// Transposition table management (cache with Zobrist hashing)
// ============================================

#include "../../include/ai/transposition_search.hpp"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cstring>
#include <cstring>

TranspositionSearch::TranspositionSearch(size_t tableSizeMB)
	: currentGeneration(1), nodesEvaluated(0), cacheHits(0)
{
	initializeTranspositionTable(tableSizeMB);
	std::memset(historyTable, 0, sizeof(historyTable));
	for (int i = 0; i < MAX_SEARCH_DEPTH; i++)
		killerMoves[i][0] = killerMoves[i][1] = Move();
}

void TranspositionSearch::initializeTranspositionTable(size_t sizeInMB)
{
	// Calculate number of entries: each CacheEntry ~40 bytes
	size_t bytesPerEntry = sizeof(CacheEntry);
	size_t totalBytes = sizeInMB * 1024 * 1024;
	size_t numEntries = totalBytes / bytesPerEntry;

	// Round to nearest power of 2 (to use & instead of %)
	size_t powerOf2 = 1;
	while (powerOf2 < numEntries)
	{
		powerOf2 <<= 1;
	}
	if (powerOf2 > numEntries)
	{
		powerOf2 >>= 1; // Use lower power if we overshoot
	}

	try {
		transpositionTable.resize(powerOf2);
	} catch (const std::bad_alloc&) {
		// If not enough memory for desired size, try with half
		while (powerOf2 > 1024) {
			powerOf2 >>= 1;
			try {
				transpositionTable.resize(powerOf2);
				std::cerr << "Warning: Transposition table reduced to "
				          << (powerOf2 * bytesPerEntry / (1024 * 1024)) << "MB" << std::endl;
				break;
			} catch (const std::bad_alloc&) {
				continue;
			}
		}
		if (transpositionTable.empty()) {
			transpositionTable.resize(1024); // Absolute minimum
			std::cerr << "Warning: Transposition table at minimum size" << std::endl;
		}
	}
	tableSizeMask = transpositionTable.size() - 1; // For index = hash & tableSizeMask

	// TranspositionTable initialization will be logged from main
}

bool TranspositionSearch::lookupTransposition(uint64_t zobristKey, CacheEntry &entry)
{
	size_t index = zobristKey & tableSizeMask;
	const CacheEntry &candidate = transpositionTable[index];

	// Early return if empty (most common case)
	if (candidate.zobristKey == 0)
	{
		return false;
	}

	// Exact key verification
	if (candidate.zobristKey == zobristKey)
	{
		entry = candidate;

		// Only update generation if different
		if (candidate.generation != currentGeneration)
		{
			transpositionTable[index].generation = currentGeneration;
		}
		return true;
	}

	return false; // Hash collision
}

void TranspositionSearch::storeTransposition(uint64_t zobristKey, int score, int depth,
											 Move bestMove, CacheEntry::Type type)
{
	size_t index = zobristKey & tableSizeMask;
	CacheEntry &existing = transpositionTable[index];

	// Smart replacement based on importance
	bool shouldReplace = false;

	if (existing.zobristKey == 0)
	{
		// Empty entry - always replace
		shouldReplace = true;
	}
	else if (existing.zobristKey == zobristKey)
	{
		// Same position - update if depth is greater or equal
		shouldReplace = (depth >= existing.depth);
	}
	else
	{
		// Hash collision - use sophisticated replacement strategy
		CacheEntry newEntry(zobristKey, score, depth, bestMove, type, currentGeneration);

		// Calculate importance values
		int existingImportance = existing.getImportanceValue();
		int newImportance = newEntry.getImportanceValue();

		// Aging factor: older entries have lower priority
		uint32_t ageDiff = currentGeneration - existing.generation;
		if (ageDiff > 0)
		{
			existingImportance -= (ageDiff * 10); // Penalize old entries
		}

		// Replace if new entry is more important
		shouldReplace = (newImportance > existingImportance);

		// Bias towards EXACT entries on tie
		if (newImportance == existingImportance && type == CacheEntry::EXACT)
		{
			shouldReplace = true;
		}
	}

	if (shouldReplace)
	{
		transpositionTable[index] = CacheEntry(zobristKey, score, depth, bestMove, type, currentGeneration);
	}
}

void TranspositionSearch::clearCache()
{
	std::fill(transpositionTable.begin(), transpositionTable.end(), CacheEntry());
	currentGeneration = 1; // Reset generation
	std::memset(historyTable, 0, sizeof(historyTable));
	for (int i = 0; i < MAX_SEARCH_DEPTH; i++)
		killerMoves[i][0] = killerMoves[i][1] = Move();
	std::cout << "TranspositionTable: Cache cleared (" << transpositionTable.size() << " entries)" << std::endl;
}

TranspositionSearch::CacheStats TranspositionSearch::getCacheStats() const
{
	CacheStats stats;
	stats.totalEntries = transpositionTable.size();
	stats.usedEntries = 0;
	stats.collisions = 0;
	stats.currentGeneration = currentGeneration;
	stats.exactEntries = 0;
	stats.boundEntries = 0;
	double totalDepth = 0;

	for (const auto &entry : transpositionTable)
	{
		if (entry.zobristKey != 0)
		{
			stats.usedEntries++;
			totalDepth += entry.depth;

			if (entry.type == CacheEntry::EXACT)
			{
				stats.exactEntries++;
			}
			else
			{
				stats.boundEntries++;
			}
		}
	}

	stats.fillRate = static_cast<double>(stats.usedEntries) / stats.totalEntries;
	stats.avgDepth = stats.usedEntries > 0 ? totalDepth / stats.usedEntries : 0.0;

	return stats;
}

void TranspositionSearch::printCacheStats() const
{
	CacheStats stats = getCacheStats();

	std::cout << "=== TRANSPOSITION TABLE STATS ===" << std::endl;
	std::cout << "Total entries: " << stats.totalEntries << std::endl;
	std::cout << "Used entries: " << stats.usedEntries << std::endl;
	std::cout << "Fill rate: " << std::fixed << std::setprecision(2) << (stats.fillRate * 100) << "%" << std::endl;
	std::cout << "Current generation: " << stats.currentGeneration << std::endl;
	std::cout << "Exact entries: " << stats.exactEntries << " ("
			  << std::fixed << std::setprecision(1) << (stats.usedEntries > 0 ? (double)stats.exactEntries / stats.usedEntries * 100 : 0) << "%)" << std::endl;
	std::cout << "Bound entries: " << stats.boundEntries << " ("
			  << std::fixed << std::setprecision(1) << (stats.usedEntries > 0 ? (double)stats.boundEntries / stats.usedEntries * 100 : 0) << "%)" << std::endl;
	std::cout << "Average depth: " << std::fixed << std::setprecision(1) << stats.avgDepth << std::endl;
	std::cout << "Memory usage: " << (stats.totalEntries * sizeof(CacheEntry) / 1024 / 1024) << " MB" << std::endl;
	std::cout << "================================" << std::endl;
}
