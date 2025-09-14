/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   display.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/21 16:35:26 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/14 21:22:44 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef DISPLAY_HPP
#define DISPLAY_HPP

#include "game_types.hpp"
#include <utility>
#include <string>

class Display {
public:
    static void printBoard(const GameState& state);
    static void printGameInfo(const GameState& state, int aiTimeMs = 0);
    static std::pair<int, int> getUserMove();
    static void printWelcome();
    static void printWinner(int player);
    static void clearScreen();
    
private:
    static char getPieceChar(int piece);
    static std::string getPieceColor(int piece);
    static std::pair<int, int> parseCoordinate(const std::string& input);
};

#endif