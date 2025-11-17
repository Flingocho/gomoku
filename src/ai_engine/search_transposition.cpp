// ============================================
// SEARCH_TRANSPOSITION.CPP
// Gestión de la tabla de transposición (cache con Zobrist hashing)
// ============================================

#include "../../include/ai/transposition_search.hpp"
#include <iostream>
#include <iomanip>
#include <algorithm>

TranspositionSearch::TranspositionSearch(size_t tableSizeMB)
	: currentGeneration(1), nodesEvaluated(0), cacheHits(0)
{
	initializeTranspositionTable(tableSizeMB);
}

void TranspositionSearch::initializeTranspositionTable(size_t sizeInMB)
{
	// Calcular número de entradas: cada CacheEntry ~40 bytes
	size_t bytesPerEntry = sizeof(CacheEntry);
	size_t totalBytes = sizeInMB * 1024 * 1024;
	size_t numEntries = totalBytes / bytesPerEntry;

	// Redondear a la potencia de 2 más cercana (para usar & en lugar de %)
	size_t powerOf2 = 1;
	while (powerOf2 < numEntries)
	{
		powerOf2 <<= 1;
	}
	if (powerOf2 > numEntries)
	{
		powerOf2 >>= 1; // Usar la potencia menor si nos pasamos
	}

	transpositionTable.resize(powerOf2);
	tableSizeMask = powerOf2 - 1; // Para hacer index = hash & tableSizeMask

	// Inicialización de TranspositionTable se loggeará desde main
}

bool TranspositionSearch::lookupTransposition(uint64_t zobristKey, CacheEntry &entry)
{
	size_t index = zobristKey & tableSizeMask;
	const CacheEntry &candidate = transpositionTable[index];

	// OPTIMIZACIÓN: Early return si está vacía (caso más común)
	if (candidate.zobristKey == 0)
	{
		return false;
	}

	// OPTIMIZACIÓN: Verificación exacta
	if (candidate.zobristKey == zobristKey)
	{
		entry = candidate;

		// OPTIMIZACIÓN: Solo actualizar generación si es diferente
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

	// NUEVA ESTRATEGIA: Reemplazo inteligente basado en importancia
	bool shouldReplace = false;

	if (existing.zobristKey == 0)
	{
		// Entrada vacía - siempre reemplazar
		shouldReplace = true;
	}
	else if (existing.zobristKey == zobristKey)
	{
		// Misma posición - actualizar si profundidad es mayor o igual
		shouldReplace = (depth >= existing.depth);
	}
	else
	{
		// Colisión de hash - usar estrategia de reemplazo sofisticada
		CacheEntry newEntry(zobristKey, score, depth, bestMove, type, currentGeneration);

		// Calcular valores de importancia
		int existingImportance = existing.getImportanceValue();
		int newImportance = newEntry.getImportanceValue();

		// Factor de aging: entradas más viejas tienen menor prioridad
		uint32_t ageDiff = currentGeneration - existing.generation;
		if (ageDiff > 0)
		{
			existingImportance -= (ageDiff * 10); // Penalizar entradas viejas
		}

		// Reemplazar si la nueva entrada es más importante
		shouldReplace = (newImportance > existingImportance);

		// Bias hacia entradas EXACT si hay empate
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
	currentGeneration = 1; // NUEVO: Reset generación
	std::cout << "TranspositionTable: Cache limpiada (" << transpositionTable.size() << " entradas)" << std::endl;
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
