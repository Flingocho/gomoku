/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rule_engine.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/14 21:16:51 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/18 16:16:54 by jainavas         ###   ########.fr       */
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

	static constexpr int DIRECTIONS[8][2] = {
		{-1, -1}, {-1, 0}, {-1, 1}, {0, -1}, {0, 1}, {1, -1}, {1, 0}, {1, 1}};
};

#endif