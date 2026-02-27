#ifndef TRANSPOSITION_SEARCH_HPP
#define TRANSPOSITION_SEARCH_HPP

#include "../core/game_types.hpp"
#include "../rules/rule_engine.hpp"
#include "evaluator.hpp"
#include "../utils/zobrist_hasher.hpp"
#include "../utils/directions.hpp"
#include "transposition_types.hpp"
#include <vector>
#include <cstdint>

class TranspositionSearch
{
public:
	// Using types from transposition_types.hpp
	using SearchResult = ::SearchResult;
	using CacheEntry = ::CacheEntry;
	using CacheStats = ::CacheStats;

private:
	std::vector<CacheEntry> transpositionTable;
	size_t tableSizeMask;
	uint32_t currentGeneration;

	int nodesEvaluated;
	int cacheHits;
	Move previousBestMove;

	// History heuristic: tracks moves that caused cutoffs across the search tree
	// Higher values = move was historically good, used for move ordering
	int historyTable[GameState::BOARD_SIZE][GameState::BOARD_SIZE];

	// Killer moves: non-capture moves that caused beta cutoffs at each depth
	// Two slots per depth for better coverage
	static constexpr int MAX_SEARCH_DEPTH = 20;
	Move killerMoves[MAX_SEARCH_DEPTH][2];

	int minimax(GameState &state, int depth, int alpha, int beta, bool maximizing,
				int originalMaxDepth, Move *bestMove = nullptr);

	void orderMoves(std::vector<Move> &moves, const GameState &state);

	int countThreats(const GameState &state, int player);
	int countLinesFromPosition(const GameState &state, int x, int y, int player);
	int countInDirection(const GameState &state, int x, int y, int dx, int dy, int player);
	bool isBlocked(const GameState &state, int x, int y, int dx, int dy, int steps, int player);

	bool lookupTransposition(uint64_t zobristKey, CacheEntry &entry);
	void storeTransposition(uint64_t zobristKey, int score, int depth, Move bestMove, CacheEntry::Type type);
	void initializeTranspositionTable(size_t sizeInMB = 64);

	std::vector<Move> generateCandidatesAdaptiveRadius(const GameState &state);
	int getSearchRadiusForGamePhase(int pieceCount);
	int getMaxCandidatesForGamePhase(const GameState &state);

	void orderMovesByGeometricValue(std::vector<Move> &moves, const GameState &state);
	int calculateGeometricMoveValue(const GameState &state, const Move &move);
	int calculateCentralityBonus(const Move &move);
	int calculateAlignmentValue(int alignmentLength);
	int calculateInterruptionValue(int interruptionLength);
	int calculateConnectivityBonus(const GameState &state, const Move &move, int player);
	int countPiecesInDirection(const GameState &state, int x, int y, int dx, int dy, int player);

	void addCandidatesAroundLastHumanMove(std::vector<Move> &candidates, const GameState &state);

	int quickCategorizeMove(const GameState &state, const Move &move);

	bool wouldCreateFiveInRow(const GameState &state, const Move &move, int player);
	bool createsFourInRow(const GameState &state, const Move &move, int player);
	bool createsThreeInRow(const GameState &state, const Move &move, int player);
	bool hasImmediateCapture(const GameState &state, const Move &move, int player);
	bool isNearExistingPieces(const GameState &state, const Move &move);

	bool blocksOpponentWin(const GameState &state, const Move &move, int opponent);
	bool blocksOpponentFour(const GameState &state, const Move &move, int opponent);
	bool blocksOpponentThree(const GameState &state, const Move &move, int opponent);

	int countConsecutiveInDirection(const GameState &state, int x, int y,
									int dx, int dy, int player, int maxCount = 5);

public:
	TranspositionSearch(size_t tableSizeMB = 64);
	~TranspositionSearch() = default;

	void clearCache();
	size_t getCacheSize() const { return transpositionTable.size(); }
	CacheStats getCacheStats() const;
	void printCacheStats() const;

	SearchResult findBestMoveIterative(const GameState &state, int maxDepth);
	void orderMovesWithPreviousBest(std::vector<Move> &moves, const GameState &state);
	std::vector<Move> generateOrderedMoves(const GameState &state);
	int quickEvaluateMove(const GameState &state, const Move &move);
};

#endif // TRANSPOSITION_SEARCH_HPP
