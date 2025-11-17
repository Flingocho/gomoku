#ifndef EVALUATOR_HPP
#define EVALUATOR_HPP

#include "../core/game_types.hpp"
#include "../utils/directions.hpp"
#include "../debug/debug_types.hpp"

class Evaluator
{
public:
	static constexpr int WIN = 600000;
	static constexpr int FOUR_OPEN = 50000;
	static constexpr int FOUR_HALF = 25000;
	static constexpr int THREE_OPEN = 10000;
	static constexpr int THREE_HALF = 1500;
	static constexpr int TWO_OPEN = 100;
	static constexpr int CAPTURE_OPPORTUNITY = 5000;   // Era 2000
	static constexpr int CAPTURE_THREAT = 6000;		   // Era 500
	static constexpr int CAPTURE_WIN = 500000;		   // NUEVO: Captura que gana
	static constexpr int CAPTURE_PREVENT_LOSS = 400000; // NUEVO: Prevenir derrota

	static int evaluateForPlayer(const GameState &state, int player);

	// NUEVO: Evaluador principal con información de distancia al mate
	static int evaluate(const GameState &state, int maxDepth, int currentDepth);

	// LEGACY: Mantener la versión anterior para compatibilidad
	static int evaluate(const GameState &state);

	static int evaluateImmediateThreats(const GameState &state, int player);

	// NUEVO: Detección eficiente de amenazas de mate en 1 usando patrones
	static bool hasWinningThreats(const GameState &state, int player);

	// NUEVO: Función auxiliar para contar patrones específicos (pública para debug)
	static int countPatternType(const GameState &state, int player, int consecutiveCount, int freeEnds);

private:
	struct PatternInfo
	{
		int consecutiveCount; // Piezas consecutivas máximas
		int totalPieces;	  // NUEVO: Total de piezas en el patrón (incluyendo gaps)
		int freeEnds;		  // Extremos libres
		bool hasGaps;		  // Si tiene gaps pequeños
		int totalSpan;		  // Span total del patrón
		int gapCount;		  // NUEVO: Número de gaps
	};

	static int analyzePosition(const GameState &state, int player);

	static PatternInfo analyzeLine(const GameState &state, int x, int y,
								   int dx, int dy, int player);

	static int patternToScore(const PatternInfo &pattern);

	// NUEVO: Evaluación de amenazas inmediatas

	static bool isLineStart(const GameState &state, int x, int y, int dx, int dy, int player);

	static bool isValidCapturePattern(const GameState &state, int x, int y,
									  int dx, int dy, int attacker, int victim);
	static bool captureBreaksOpponentPattern(const GameState &state, const std::vector<Move> &capturedPieces, int opponent);
	struct CaptureOpportunity {
        Move position;
        std::vector<Move> captured;
        int value;
        
        CaptureOpportunity() : value(0) {}
    };
    
    static std::vector<CaptureOpportunity> findAllCaptureOpportunities(
        const GameState& state, int player);
    
    static int evaluateCaptureContext(
        const GameState& state,
        int player,
        const std::vector<Move>& capturedPieces,
        int newCaptureCount);
    
    static int countPatternThroughPosition(
        const GameState& state,
        const Move& pos,
        int dx, int dy,
        int player);
};

#endif