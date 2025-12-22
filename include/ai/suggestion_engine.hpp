#ifndef SUGGESTION_ENGINE_HPP
#define SUGGESTION_ENGINE_HPP

#include "../core/game_types.hpp"
#include "../rules/rule_engine.hpp"
#include "ai.hpp"
#include <vector>

/**
 * Motor de sugerencias para modo hotseat
 * Usa la IA principal con profundidad reducida para sugerencias rápidas
 */
class SuggestionEngine {
public:
    /**
     * Obtiene la mejor sugerencia usando la IA principal
     * @param state Estado actual del juego
     * @param depth Profundidad de búsqueda (por defecto 6 para balance velocidad/calidad)
     * @return El mejor movimiento sugerido
     */
    static Move getSuggestion(const GameState& state, int depth = 6);
    
    /**
     * Obtiene sugerencia rápida usando heurísticas simples
     * Útil cuando se necesita velocidad sobre calidad
     */
    static Move getQuickSuggestion(const GameState& state);
    
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
