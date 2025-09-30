/* ******************************************************		int value = depth * 100;  // Depth is more important
		if (type == EXACT) value += 50;  // Exact nodes are more valuable****************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   transposition_search.hpp                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/14 21:38:39 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/28 23:41:08 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef TRANSPOSITION_SEARCH_HPP
#define TRANSPOSITION_SEARCH_HPP

#include "game_types.hpp"
#include "rule_engine.hpp"
#include "evaluator.hpp"
#include "zobrist_hasher.hpp"
#include <vector>
#include <cstdint>

class TranspositionSearch
{
public:
	struct SearchResult
	{
		Move bestMove;
		int score;
		int nodesEvaluated;
		int cacheHits;
		float cacheHitRate;

		SearchResult() : bestMove(), score(0), nodesEvaluated(0), cacheHits(0), cacheHitRate(0.0f) {}
	};

private:
	struct CacheEntry
	{
		uint64_t zobristKey; // NUEVO: Clave Zobrist completa para verificar colisiones
		int score;
		int depth;
		Move bestMove;
		uint32_t generation; // NUEVO: Para aging-based replacement
		enum Type
		{
			EXACT,
			LOWER_BOUND,
			UPPER_BOUND
		} type;

		CacheEntry() : zobristKey(0), score(0), depth(0), generation(0), type(EXACT) {}
		CacheEntry(uint64_t key, int s, int d, Move m, Type t, uint32_t gen = 0)
			: zobristKey(key), score(s), depth(d), bestMove(m), generation(gen), type(t) {}

		// NUEVO: Calcula valor de importancia para replacement strategy
		int getImportanceValue() const {
			int value = depth * 100;  // Profundidad es más importante
			if (type == EXACT) value += 50;  // Nodos exactos son más valiosos
			else if (type == LOWER_BOUND) value += 25;
			return value;
		}
	};

	/**
	 * Zobrist-optimized Transposition Table
	 * - Size power of 2 to use & instead of %
	 * - Collision verification with complete key
	 * - Improved replacement strategy
	 */
	std::vector<CacheEntry> transpositionTable;
	size_t tableSizeMask; // Para index = zobristKey & tableSizeMask
	uint32_t currentGeneration; // NUEVO: Para aging-based replacement

	int nodesEvaluated;
	int cacheHits;
	Move previousBestMove;

	/**
	 * Calculates adaptive depth based on game phase
	 */
	int calculateAdaptiveDepth(const GameState &state, int requestedDepth);

	/**
	 * Minimax con alpha-beta usando Zobrist hashing
	 * MODIFICADO: Incluye originalMaxDepth para cálculo de distancia al mate
	 */
	int minimax(GameState &state, int depth, int alpha, int beta, bool maximizing,
				int originalMaxDepth, Move *bestMove = nullptr);

	/**
	 * Ordena movimientos por probabilidad de cutoff
	 */
	void orderMoves(std::vector<Move> &moves, const GameState &state);

	// Funciones auxiliares para move ordering
	int countThreats(const GameState &state, int player);
	int countLinesFromPosition(const GameState &state, int x, int y, int player);
	int countInDirection(const GameState &state, int x, int y, int dx, int dy, int player);
	bool isBlocked(const GameState &state, int x, int y, int dx, int dy, int steps, int player);

	/**
	 * Busca entrada en transposition table
	 * @param zobristKey: State Zobrist key
	 * @param entry: [out] Found entry
	 * @return true if found valid entry
	 */
	bool lookupTransposition(uint64_t zobristKey, CacheEntry &entry);

	/**
	 * Store entry in transposition table
	 * @param zobristKey: State Zobrist key
	 * @param score: State score
	 * @param depth: Search depth
	 * @param bestMove: Mejor movimiento encontrado
	 * @param type: Tipo de nodo (EXACT, LOWER_BOUND, UPPER_BOUND)
	 */
	void storeTransposition(uint64_t zobristKey, int score, int depth, Move bestMove, CacheEntry::Type type);

	/**
	 * Inicializa la transposition table con tamaño óptimo
	 */
	void initializeTranspositionTable(size_t sizeInMB = 64);

	std::vector<Move> generateCandidatesAdaptiveRadius(const GameState &state);
	int getSearchRadiusForGamePhase(int pieceCount);
	int getMaxCandidatesForGamePhase(const GameState &state);
	bool isEarlyGamePhase(int pieceCount);
	bool isCentralStrategicPosition(int x, int y);

	// Evaluación geométrica de movimientos
	void orderMovesByGeometricValue(std::vector<Move> &moves, const GameState &state);
	int calculateGeometricMoveValue(const GameState &state, const Move &move);
	int calculateCentralityBonus(const Move &move);
	int calculateAlignmentValue(int alignmentLength);
	int calculateInterruptionValue(int interruptionLength);
	int calculateConnectivityBonus(const GameState &state, const Move &move, int player);
	int countPiecesInDirection(const GameState &state, int x, int y, int dx, int dy, int player);
	
	// NUEVO: Generar candidatos alrededor del último movimiento humano
	void addCandidatesAroundLastHumanMove(std::vector<Move> &candidates, const GameState &state);

	int quickCategorizeMove(const GameState& state, const Move& move);
    
    // Funciones auxiliares de verificación rápida
    bool wouldCreateFiveInRow(const GameState& state, const Move& move, int player);
    bool createsFourInRow(const GameState& state, const Move& move, int player);
    bool createsThreeInRow(const GameState& state, const Move& move, int player);
    bool hasImmediateCapture(const GameState& state, const Move& move, int player);
    bool isNearExistingPieces(const GameState& state, const Move& move);
    
    // Funciones para verificar bloqueos
    bool blocksOpponentWin(const GameState& state, const Move& move, int opponent);
    bool blocksOpponentFour(const GameState& state, const Move& move, int opponent);
    bool blocksOpponentThree(const GameState& state, const Move& move, int opponent);
    
    // Función auxiliar para contar en dirección
    int countConsecutiveInDirection(const GameState& state, int x, int y, 
                                   int dx, int dy, int player, int maxCount = 5);

public:
	/**
	 * Constructor: Inicializa tabla de transposición
	 * @param tableSizeMB: Tamaño de la tabla en MB (debe ser potencia de 2)
	 */
	TranspositionSearch(size_t tableSizeMB = 64);

	/**
	 * Destructor: Limpia recursos
	 */
	~TranspositionSearch() = default;

	/**
	 * Encuentra el mejor movimiento usando búsqueda optimizada
	 * NUEVO: Usa Zobrist hashing para máxima eficiencia
	 */
	SearchResult findBestMove(const GameState &state, int maxDepth);

	/**
	 * Limpia la cache de transposición
	 */
	void clearCache();

	/**
	 * Obtiene estadísticas de la cache
	 */
	size_t getCacheSize() const { return transpositionTable.size(); }

	/**
	 * Estadísticas detalladas de cache
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

	CacheStats getCacheStats() const;

	/**
	 * Muestra estadísticas detalladas del cache
	 */
	void printCacheStats() const;

	SearchResult findBestMoveIterative(const GameState &state, int maxDepth);
	void orderMovesWithPreviousBest(std::vector<Move> &moves, const GameState &state);
	
	/**
	 * Genera movimientos ordenados inteligentemente
	 * OPTIMIZADO: Usa hash de movimientos previos para ordenamiento
	 */
	std::vector<Move> generateOrderedMoves(const GameState &state);

	/**
	 * Evaluación rápida para move ordering
	 */
	int quickEvaluateMove(const GameState &state, const Move &move);

private:
};

#endif