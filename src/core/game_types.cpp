#include "../../include/core/game_types.hpp"
#include "../../include/utils/zobrist_hasher.hpp"
#include <iostream>
#include <cstring>

// Static hasher initialization
const ZobristHasher* GameState::hasher = nullptr;

GameState::GameState() {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            board[i][j] = EMPTY;
        }
    }
    
    // Initialize last human move as invalid
    lastHumanMove = Move(-1, -1);
    
    // Reset captures and current player
    captures[0] = 0;
    captures[1] = 0;
    currentPlayer = PLAYER1;
    turnCount = 0;
    depth = 0;
    
    // Reset forced capture system
    forcedCaptureMoves.clear();
    forcedCapturePlayer = 0;
    pendingWinPlayer = 0;
    
    // Calculate initial hash if hasher is available
    if (hasher) {
        zobristHash = hasher->computeFullHash(*this);
    }
}

GameState::GameState(const GameState& other) {
    *this = other;
}

GameState& GameState::operator=(const GameState& other) {
    if (this != &other) {
        std::memcpy(board, other.board, sizeof(board));
        captures[0] = other.captures[0];
        captures[1] = other.captures[1];
        currentPlayer = other.currentPlayer;
        turnCount = other.turnCount;
        depth = other.depth;
        zobristHash = other.zobristHash;
        lastHumanMove = other.lastHumanMove;
        
        // Copy forced capture system
        forcedCaptureMoves = other.forcedCaptureMoves;
        forcedCapturePlayer = other.forcedCapturePlayer;
        pendingWinPlayer = other.pendingWinPlayer;
    }
    return *this;
}

bool GameState::isValid(int x, int y) const {
    return x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE;
}

bool GameState::isEmpty(int x, int y) const {
    return isValid(x, y) && board[x][y] == EMPTY;
}

int GameState::getPiece(int x, int y) const {
    return isValid(x, y) ? board[x][y] : -1;
}

int GameState::getOpponent(int player) const {
    return (player == PLAYER1) ? PLAYER2 : PLAYER1;
}

void GameState::initializeHasher() {
    if (!hasher) {
        hasher = new ZobristHasher();
        // Zobrist hasher initialized - logged from main
    }
}

void GameState::cleanupHasher() {
    if (hasher) {
        delete hasher;
        hasher = nullptr;
    }
}

void GameState::updateHashAfterMove(const Move& move, int player, 
                                   const std::vector<Move>& capturedPieces,
                                   int oldCaptures) {
    if (!hasher) {
        std::cerr << "ERROR: Hasher no inicializado. Llama GameState::initializeHasher() primero." << std::endl;
        return;
    }
    
    int playerIndex = player - 1;
    int newCaptures = captures[playerIndex];
    
    zobristHash = hasher->updateHashAfterMove(zobristHash, move, player, 
                                            capturedPieces, oldCaptures, newCaptures);
}

void GameState::recalculateHash() {
    if (!hasher) {
        std::cerr << "ERROR: Hasher no inicializado. Llama GameState::initializeHasher() primero." << std::endl;
        return;
    }
    
    zobristHash = hasher->computeFullHash(*this);
    
    // Hash debug removed to avoid console spam
}
