/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rule_engine.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/14 21:16:51 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/30 19:12:57 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef RULE_ENGINE_HPP
#define RULE_ENGINE_HPP

#include "game_types.hpp"
#include <vector>

class RuleEngine
{
public:
	struct MoveResult
	{
		bool success;
		std::vector<Move> myCapturedPieces;		  // Piezas que yo capturo
		std::vector<Move> opponentCapturedPieces; // Piezas que el oponente captura por mi movimiento
		bool createsWin;

		MoveResult(bool s = false) : success(s), createsWin(false) {}
	};

	struct CaptureInfo
	{
		std::vector<Move> myCapturedPieces;
		std::vector<Move> opponentCapturedPieces;
	};

	static CaptureInfo findAllCaptures(const GameState &state, const Move &move, int player);

	static MoveResult applyMove(GameState &state, const Move &move);

	static bool isLegalMove(const GameState &state, const Move &move);

	static bool checkWin(const GameState &state, int player);

	static std::vector<Move> findCaptures(const GameState &state, const Move &move, int player);

	static bool createsDoubleFreeThree(const GameState &state, const Move &move, int player);

	static bool canBreakLineByCapture(const GameState &state,
									  const Move &lineStart,
									  int dx, int dy,
									  int winningPlayer,
									  std::vector<Move>* outCaptureMoves = nullptr);

private:
	static std::vector<Move> findCapturesInDirection(const GameState &state,
													 const Move &move, int player,
													 int dx, int dy);

	static bool checkLineWin(const GameState &state, const Move &move, int player);
	static int countInDirection(const GameState &state, const Move &start,
								int dx, int dy, int player);

	static std::vector<Move> findFreeThrees(const GameState &state, const Move &move, int player);
	static bool isFreeThree(const GameState &state, const Move &start,
							int dx, int dy, int player);
	static bool isValidFreeThreePattern(const int windowState[5], int player);
	static bool canFormThreat(const int pattern[5], int player);
	static bool hasFourConsecutive(const int pattern[5], int player);

	static constexpr int DIRECTIONS[8][2] = {
		{-1, -1}, {-1, 0}, {-1, 1}, {0, -1}, {0, 1}, {1, -1}, {1, 0}, {1, 1}};
	static constexpr int MAIN_DIRECTIONS[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};

	static bool opponentCanCaptureNextTurn(const GameState &state, int opponent);

	// En rule_engine.hpp, en la sección private:
	static bool checkLineWinInDirection(const GameState &state,
										const Move &start,
										int dx, int dy,
										int player);
};

#endif