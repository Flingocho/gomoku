#ifndef ZOBRIST_HASHER_HPP
#define ZOBRIST_HASHER_HPP

#include "../core/game_types.hpp"
#include <cstdint>
#include <vector>
#include <random>

/**
 * ZobristHasher: Efficient hashing for Gomoku board states
 *
 * Features:
 * - Incremental O(1) hash updates vs O(n²) full recalculation
 * - Practically impossible collisions (2^64 key space)
 * - Direct integration with transposition table
 */
class ZobristHasher
{
public:
	using ZobristKey = uint64_t;

	/**
	 * Constructor: Initializes random number table
	 * The table must remain consistent throughout execution
	 */
	ZobristHasher();
	~ZobristHasher();

	/**
	 * Compute full hash for a state (use only for initialization)
	 * Complexity: O(n²) where n = BOARD_SIZE
	 */
	ZobristKey computeFullHash(const GameState &state) const;

	/**
	 * Incrementally update hash after a move
	 * Complexity: O(1) - at most 10 XOR operations
	 *
	 * @param currentHash: Hash of state before the move
	 * @param move: Move made
	 * @param player: Player who made the move
	 * @param capturedPieces: Pieces captured by the move
	 * @param oldCaptures: Player's captures before the move
	 * @param newCaptures: Player's captures after the move
	 * @return Updated hash of the new state
	 */
	ZobristKey updateHashAfterMove(ZobristKey currentHash,
								   const Move &move,
								   int player,
								   const std::vector<Move> &capturedPieces,
								   int oldCaptures,
								   int newCaptures) const;

	// Overload with separate player/opponent capture tracking
	ZobristKey updateHashAfterMove(ZobristKey currentHash,
								   const Move &move,
								   int player,
								   const std::vector<Move> &myCapturedPieces,
								   const std::vector<Move> &opponentCapturedPieces,
								   int oldMyCaptures,
								   int newMyCaptures,
								   int oldOppCaptures,
								   int newOppCaptures) const;
	/**
	 * Revert a move in the hash (for backtracking)
	 * Due to XOR properties: reverting = applying the same change
	 */
	ZobristKey revertMove(ZobristKey currentHash,
						  const Move &move,
						  int player,
						  const std::vector<Move> &capturedPieces,
						  int oldCaptures,
						  int newCaptures) const;

	/**
	 * Get the hash for a specific piece at a given position
	 * Useful for debugging and verification
	 */
	ZobristKey getPieceHash(int x, int y, int piece) const;

private:
	// Main table: [row][col][piece_type]
	// zobristTable[x][y][0] = 0 (EMPTY by convention)
	// zobristTable[x][y][1] = hash for PLAYER1
	// zobristTable[x][y][2] = hash for PLAYER2
	ZobristKey zobristTable[GameState::BOARD_SIZE][GameState::BOARD_SIZE][3];

	// Hash for player turn
	ZobristKey turnHash;

	// Hash for captures: [player_index][capture_count]
	// captureHashes[0][5] = hash when PLAYER1 has 5 captures
	ZobristKey captureHashes[2][11]; // 0-10 possible captures

	/**
	 * Initialize all tables with high-quality random numbers
	 * Uses std::random_device for real system entropy
	 */
	void initializeZobristTable();

	/**
	 * Generate a 64-bit random number
	 */
	ZobristKey generateRandomKey();

	// Static random generator for initialization
	static std::mt19937_64 rng;
	static bool rngInitialized;
};

#endif