#ifndef GAME_TYPES_HPP
#define GAME_TYPES_HPP

#include <utility>
#include <vector>
#include <cstdint>

// Forward declaration para evitar dependencias circulares
class ZobristHasher;

struct Move {
    int x, y;
    
    Move(int row = -1, int col = -1) : x(row), y(col) {}
    
    bool isValid() const { return x >= 0 && y >= 0 && x < 19 && y < 19; }
    bool operator==(const Move& other) const { return x == other.x && y == other.y; }
};

struct GameState {
    static constexpr int BOARD_SIZE = 19;
    static constexpr int EMPTY = 0;
    static constexpr int PLAYER1 = 1;
    static constexpr int PLAYER2 = 2;
    
    int board[BOARD_SIZE][BOARD_SIZE];
    int captures[2] = {0, 0};
    int currentPlayer = PLAYER1;
    int turnCount = 0;
    int depth = 0;
    
    // NUEVO: Hash Zobrist del estado actual
    uint64_t zobristHash = 0;
    
    // NUEVO: Última jugada del humano para generación de candidatos defensivos
    Move lastHumanMove;
    
    // NUEVO: Flag para modo de solo capturas (15 capturas para ganar, no 5 en línea)
    bool captureModeOnly = false;
    
    // NUEVO: Forced capture mechanism
    // When a player makes 5-in-a-row but opponent can break it by capture,
    // opponent MUST play one of these positions on their next turn
    std::vector<Move> forcedCaptureMoves;
    int forcedCapturePlayer = 0;  // Which player must make the forced capture (0 = none)
    int pendingWinPlayer = 0;     // Which player has the pending 5-in-a-row
    
    // NUEVO: Referencia al hasher (compartido entre todos los estados)
    static const ZobristHasher* hasher;
    
    GameState();
    GameState(const GameState& other);
    GameState& operator=(const GameState& other);
    
    bool isValid(int x, int y) const;
    bool isEmpty(int x, int y) const;
    int getPiece(int x, int y) const;
    int getDepth() const { return depth; };
    void SetDepth(int ndepth) { depth = ndepth; };
    int getOpponent(int player) const;
    
    // NEW: Methods for hash management
    /**
     * Initializes the static hasher (call once at program start)
     */
    static void initializeHasher();
    
    /**
     * Updates the hash after applying a move
     * MUST be called after modifying board state
     */
    void updateHashAfterMove(const Move& move, int player, 
                           const std::vector<Move>& capturedPieces,
                           int oldCaptures);
    
    /**
     * Recalculates the complete hash from scratch (only for verification/debug)
     */
    void recalculateHash();
    
    /**
     * Obtiene el hash actual del estado
     */
    uint64_t getZobristHash() const { return zobristHash; }
};

#endif