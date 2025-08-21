/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   display.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/21 16:35:26 by jainavas          #+#    #+#             */
/*   Updated: 2025/08/21 16:35:29 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef DISPLAY_HPP
#define DISPLAY_HPP

#include "./board.hpp"
#include <utility>
#include <string>

class Display {
public:
    static void printBoard(const Board& board);
    static void printGameInfo(const Board& board, double aiTime = 0.0);
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