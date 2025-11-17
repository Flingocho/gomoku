// ============================================
// RULES_WIN.CPP
// Lógica de detección de victoria
// Victoria por 5-en-línea y por capturas
// ============================================

#include "../../include/rules/rule_engine.hpp"
#include <iostream>

using namespace Directions;

bool RuleEngine::checkWin(const GameState &state, int player)
{
    int opponent = state.getOpponent(player);
    
    // 1. Victoria por capturas (esta no cambia)
    if (state.captures[player - 1] >= 10)
    {
        return true;
    }

    // 2. Victoria por 5 en línea (AHORA CON VERIFICACIÓN)
    for (int i = 0; i < GameState::BOARD_SIZE; i++) {
        for (int j = 0; j < GameState::BOARD_SIZE; j++) {
            if (state.board[i][j] == player) {
                Move pos(i, j);
                
                // Buscar línea de 5 en cada dirección
                for (int d = 0; d < MAIN_COUNT; d++) {
                    int dx = MAIN[d][0];
                    int dy = MAIN[d][1];
                    
                    if (checkLineWinInDirection(state, pos, dx, dy, player)) {
                        // ✅ Encontramos 5 en línea
                        
                        // NUEVA VERIFICACIÓN 1: ¿Oponente puede romperla?
                        // This will be handled by the game engine setting forced captures
                        std::vector<Move> captureMoves;
                        bool canBreak = canBreakLineByCapture(state, pos, dx, dy, player, &captureMoves);
                        
                        if (canBreak) {
                            // NOTE: The game engine will handle setting forced captures
                            // For now, we treat this as "not a win yet"
                            continue;
                        }
                        
                        // NUEVA VERIFICACIÓN 2: ¿Estoy en peligro de perder por captura?
                        if (state.captures[opponent - 1] >= 8) {
                            // Tengo 4+ pares capturados en mi contra
                            // ¿El oponente puede capturar uno más?
                            if (opponentCanCaptureNextTurn(state, opponent)) {
                                return false;  // No gano, el oponente puede ganar por captura
                            }
                        }
                        
                        // Si llegamos aquí, es victoria legítima
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

bool RuleEngine::checkLineWin(const GameState &state, const Move &move, int player)
{
	// Verificar las 4 direcciones principales (sin duplicados)
	for (int d = 0; d < MAIN_COUNT; d++)
	{
		int dx = MAIN[d][0];
		int dy = MAIN[d][1];

		int count = 1; // La pieza actual
		count += countInDirection(state, move, dx, dy, player);
		count += countInDirection(state, move, -dx, -dy, player);

		if (count >= 5)
			return true;
	}

	return false;
}

bool RuleEngine::checkLineWinInDirection(const GameState &state, const Move &start, 
                                         int dx, int dy, int player)
{
    // Verificar si desde 'start' hay exactamente 5 o más en línea en dirección (dx, dy)
    
    // IMPORTANTE: Solo contar si 'start' es realmente el inicio de la línea
    // (para evitar contar la misma línea múltiples veces)
    
    // Verificar que no hay pieza del mismo jugador ANTES de start
    Move before(start.x - dx, start.y - dy);
    if (state.isValid(before.x, before.y) && state.getPiece(before.x, before.y) == player) {
        return false;  // No es el inicio real de la línea
    }
    
    // Contar cuántas fichas consecutivas hay desde start
    int count = 1;  // La ficha en 'start'
    Move current(start.x + dx, start.y + dy);
    
    while (state.isValid(current.x, current.y) && 
           state.getPiece(current.x, current.y) == player) {
        count++;
        current.x += dx;
        current.y += dy;
    }
    
    // ¿Hay 5 o más?
    return (count >= 5);
}
