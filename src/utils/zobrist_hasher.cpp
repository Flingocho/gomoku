#include "../../include/utils/zobrist_hasher.hpp"
#include <random>
#include <iostream>
#include <array>
#include <algorithm>
#include <functional>

// Static initialization
std::mt19937_64 ZobristHasher::rng;
bool ZobristHasher::rngInitialized = false;

ZobristHasher::ZobristHasher() {
    initializeZobristTable();
}

ZobristHasher::~ZobristHasher() {}

void ZobristHasher::initializeZobristTable() {
    // Initialize random generator once
    if (!rngInitialized) {
        std::random_device rd;
        // Seed with multiple values for better entropy
        std::array<uint32_t, std::mt19937_64::state_size> seed_data;
        std::generate(seed_data.begin(), seed_data.end(), std::ref(rd));
        std::seed_seq seq(seed_data.begin(), seed_data.end());
        rng.seed(seq);
        rngInitialized = true;
        
        // Successful initialization - will be logged from main
    }
    
    // Initialize main table
    for (int i = 0; i < GameState::BOARD_SIZE; i++) {
        for (int j = 0; j < GameState::BOARD_SIZE; j++) {
            // EMPTY is always 0 (by Zobrist convention)
            zobristTable[i][j][GameState::EMPTY] = 0;
            
            // Generate unique keys for each player
            zobristTable[i][j][GameState::PLAYER1] = generateRandomKey();
            zobristTable[i][j][GameState::PLAYER2] = generateRandomKey();
        }
    }
    
    // Hash for player turn
    turnHash = generateRandomKey();
    
    // Hash for captures
    for (int player = 0; player < 2; player++) {
        for (int captures = 0; captures <= 10; captures++) {
            captureHashes[player][captures] = generateRandomKey();
        }
    }
    
    // Zobrist statistics - will be logged from main if needed
}

ZobristHasher::ZobristKey ZobristHasher::generateRandomKey() {
    std::uniform_int_distribution<uint64_t> dis;
    return dis(rng);
}

ZobristHasher::ZobristKey ZobristHasher::computeFullHash(const GameState& state) const {
    ZobristKey hash = 0;
    
    // Hash all pieces on the board
    for (int i = 0; i < GameState::BOARD_SIZE; i++) {
        for (int j = 0; j < GameState::BOARD_SIZE; j++) {
            int piece = state.getPiece(i, j);
            if (piece != GameState::EMPTY) {
                hash ^= zobristTable[i][j][piece];
            }
        }
    }
    
    // Hash current turn (only if PLAYER2)
    // Convention: PLAYER1 = base hash, PLAYER2 = base hash ^ turnHash
    if (state.currentPlayer == GameState::PLAYER2) {
        hash ^= turnHash;
    }
    
    // Hash captures for both players
    hash ^= captureHashes[0][std::min(state.captures[0], 10)]; // PLAYER1 captures
    hash ^= captureHashes[1][std::min(state.captures[1], 10)]; // PLAYER2 captures
    
    return hash;
}

ZobristHasher::ZobristKey ZobristHasher::updateHashAfterMove(
    ZobristKey currentHash,
    const Move& move,
    int player,
    const std::vector<Move>& capturedPieces,
    int oldCaptures,
    int newCaptures) const {
    
    ZobristKey newHash = currentHash;
    
    // 1. Place new piece
    newHash ^= zobristTable[move.x][move.y][player];
    
    // 2. Remove captured pieces
    int opponent = (player == GameState::PLAYER1) ? GameState::PLAYER2 : GameState::PLAYER1;
    for (const Move& captured : capturedPieces) {
        newHash ^= zobristTable[captured.x][captured.y][opponent];
    }
    
    // 3. Switch turn (always alternate)
    newHash ^= turnHash;
    
    // 4. Update capture hash
    int playerIndex = player - 1; // PLAYER1=0, PLAYER2=1
    
    // Remove previous capture hash (clamped for safety)
    newHash ^= captureHashes[playerIndex][std::min(oldCaptures, 10)];
    
    // Add new capture hash (clamped for safety)
    newHash ^= captureHashes[playerIndex][std::min(newCaptures, 10)];
    
    return newHash;
}

ZobristHasher::ZobristKey ZobristHasher::revertMove(
    ZobristKey currentHash,
    const Move& move,
    int player,
    const std::vector<Move>& capturedPieces,
    int oldCaptures,
    int newCaptures) const {
    
    // Due to XOR properties: A ^ B ^ B = A
    // Reverting is exactly the same process as applying
    return updateHashAfterMove(currentHash, move, player, capturedPieces, 
                              newCaptures, oldCaptures); // Note: we swap oldCaptures and newCaptures
}

ZobristHasher::ZobristKey ZobristHasher::getPieceHash(int x, int y, int piece) const {
    if (x < 0 || x >= GameState::BOARD_SIZE || 
        y < 0 || y >= GameState::BOARD_SIZE ||
        piece < 0 || piece > 2) {
        return 0;
    }
    
    return zobristTable[x][y][piece];
}

// Overloaded updateHashAfterMove with separate capture tracking
ZobristHasher::ZobristKey ZobristHasher::updateHashAfterMove(
    ZobristKey currentHash,
    const Move& move,
    int player,
    const std::vector<Move>& myCapturedPieces,
    const std::vector<Move>& opponentCapturedPieces,
    int oldMyCaptures,
    int newMyCaptures,
    int oldOppCaptures,
    int newOppCaptures) const {
    
    ZobristKey newHash = currentHash;
    int opponent = (player == GameState::PLAYER1) ? GameState::PLAYER2 : GameState::PLAYER1;
    
    // 1. Place new piece
    newHash ^= zobristTable[move.x][move.y][player];
    
    // 2. Remove own captures (opponent's pieces)
    for (const Move& captured : myCapturedPieces) {
        newHash ^= zobristTable[captured.x][captured.y][opponent];
    }
    
    // 3. Remove opponent's captures (own pieces)
    for (const Move& captured : opponentCapturedPieces) {
        newHash ^= zobristTable[captured.x][captured.y][player];
    }
    
    // 4. Switch turn
    newHash ^= turnHash;
    
    // 5. Update own captures (clamped for safety)
    int playerIndex = player - 1;
    newHash ^= captureHashes[playerIndex][std::min(oldMyCaptures, 10)];
    newHash ^= captureHashes[playerIndex][std::min(newMyCaptures, 10)];
    
    // 6. Update opponent's captures (clamped for safety)
    int opponentIndex = opponent - 1;
    if (oldOppCaptures != newOppCaptures) {
        newHash ^= captureHashes[opponentIndex][std::min(oldOppCaptures, 10)];
        newHash ^= captureHashes[opponentIndex][std::min(newOppCaptures, 10)];
    }
    
    return newHash;
}
