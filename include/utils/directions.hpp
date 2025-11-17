#ifndef DIRECTIONS_HPP
#define DIRECTIONS_HPP

/**
 * Centralized direction constants for board traversal
 * Used across multiple modules: RuleEngine, Evaluator, TranspositionSearch
 */
namespace Directions
{
	/**
	 * Main 4 directions (horizontal, vertical, 2 diagonals)
	 * Used for: win detection, pattern analysis, line evaluation
	 */
	constexpr int MAIN[4][2] = {
		{0, 1},  // Horizontal →
		{1, 0},  // Vertical ↓
		{1, 1},  // Diagonal ↘
		{1, -1}  // Diagonal ↗
	};

	/**
	 * All 8 directions (includes reverse of main directions)
	 * Used for: capture detection, neighbor search, proximity checks
	 */
	constexpr int CAPTURE[8][2] = {
		{-1, -1}, {-1, 0}, {-1, 1},  // Up-left, Up, Up-right
		{0, -1},           {0, 1},   // Left,         Right
		{1, -1},  {1, 0},  {1, 1}    // Down-left, Down, Down-right
	};

	/**
	 * Alias for all 8 directions (same as CAPTURE)
	 * Semantic name for non-capture use cases
	 */
	constexpr int ALL[8][2] = {
		{-1, -1}, {-1, 0}, {-1, 1},
		{0, -1},           {0, 1},
		{1, -1},  {1, 0},  {1, 1}
	};

	/**
	 * Number of main directions (4)
	 */
	constexpr int MAIN_COUNT = 4;

	/**
	 * Number of all directions (8)
	 */
	constexpr int ALL_COUNT = 8;
}

#endif // DIRECTIONS_HPP
