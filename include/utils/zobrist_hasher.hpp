#ifndef ZOBRIST_HASHER_HPP
#define ZOBRIST_HASHER_HPP

#include "../core/game_types.hpp"
#include <cstdint>
#include <vector>
#include <random>

/**
 * ZobristHasher: Implementación eficiente de hashing para estados de Gomoku
 *
 * Características:
 * - Hash incremental O(1) vs O(n²) recalculo completo
 * - Colisiones prácticamente imposibles (2^64 espacio)
 * - Integración directa con transposition table
 */
class ZobristHasher
{
public:
	using ZobristKey = uint64_t;

	/**
	 * Constructor: Inicializa tabla de números aleatorios
	 * CRÍTICO: Esta tabla debe ser consistente durante toda la ejecución
	 */
	ZobristHasher();

	/**
	 * Computa hash completo para un estado (usar solo para inicialización)
	 * Complejidad: O(n²) donde n = BOARD_SIZE
	 */
	ZobristKey computeFullHash(const GameState &state) const;

	/**
	 * Actualiza hash incrementalmente después de un movimiento
	 * Complejidad: O(1) - máximo 10 operaciones XOR
	 *
	 * @param currentHash: Hash del estado antes del movimiento
	 * @param move: Movimiento realizado
	 * @param player: Jugador que hizo el movimiento
	 * @param capturedPieces: Piezas capturadas por el movimiento
	 * @param oldCaptures: Capturas del jugador antes del movimiento
	 * @param newCaptures: Capturas del jugador después del movimiento
	 * @return Nuevo hash del estado
	 */
	ZobristKey updateHashAfterMove(ZobristKey currentHash,
								   const Move &move,
								   int player,
								   const std::vector<Move> &capturedPieces,
								   int oldCaptures,
								   int newCaptures) const;

	// En zobrist_hasher.hpp:
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
	 * Revierte un movimiento en el hash (para backtracking)
	 * Debido a las propiedades del XOR: revertir = aplicar el mismo cambio
	 */
	ZobristKey revertMove(ZobristKey currentHash,
						  const Move &move,
						  int player,
						  const std::vector<Move> &capturedPieces,
						  int oldCaptures,
						  int newCaptures) const;

	/**
	 * Obtiene el hash para una pieza específica en una posición
	 * Útil para debugging y verificaciones
	 */
	ZobristKey getPieceHash(int x, int y, int piece) const;

private:
	// Tabla principal: [fila][columna][tipo_pieza]
	// zobristTable[x][y][0] = 0 (EMPTY por convención)
	// zobristTable[x][y][1] = hash para PLAYER1
	// zobristTable[x][y][2] = hash para PLAYER2
	ZobristKey zobristTable[GameState::BOARD_SIZE][GameState::BOARD_SIZE][3];

	// Hash para indicar turno del jugador
	ZobristKey turnHash;

	// Hash para capturas: [jugador][numero_capturas]
	// captureHashes[0][5] = hash cuando PLAYER1 tiene 5 capturas
	ZobristKey captureHashes[2][11]; // 0-10 capturas posibles

	/**
	 * Inicializa todas las tablas con números aleatorios de alta calidad
	 * Usa std::random_device para entropía real del sistema
	 */
	void initializeZobristTable();

	/**
	 * Genera un número aleatorio de 64 bits usando generador criptográfico
	 */
	ZobristKey generateRandomKey();

	// Generador aleatorio estático para inicialización
	static std::mt19937_64 rng;
	static bool rngInitialized;
};

#endif