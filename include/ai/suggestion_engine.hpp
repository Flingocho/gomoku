#ifndef SUGGESTION_ENGINE_HPP
#define SUGGESTION_ENGINE_HPP

#include "../core/game_types.hpp"
#include "../rules/rule_engine.hpp"
#include <vector>

/**
 * Motor simple de sugerencias para modo hotseat
 * Completamente aislado de la IA principal
 */
class SuggestionEngine {
public:
    /**
     * Obtiene una sugerencia rápida para el jugador actual
     * Usa heurísticas simples sin minimax profundo
     */
    static Move getSuggestion(const GameState& state);
    
private:
    // Evaluar un movimiento con heurística simple
    static int evaluateMove(const GameState& state, const Move& move, int player);
    
    // Generar movimientos candidatos (área local)
    static std::vector<Move> generateCandidates(const GameState& state);
    
    // Heurísticas específicas
    static int checkWinningMove(const GameState& state, const Move& move, int player);
    static int checkBlockingMove(const GameState& state, const Move& move, int player);
    static int checkCaptureMove(const GameState& state, const Move& move, int player);
    static int checkPatternValue(const GameState& state, const Move& move, int player);
	static bool createsFourInRow(const GameState& state, const Move& move, int player);
    static bool createsThreeOpen(const GameState& state, const Move& move, int player);
    static int calculateConnectivity(const GameState& state, const Move& move, int player);

};

#endif
