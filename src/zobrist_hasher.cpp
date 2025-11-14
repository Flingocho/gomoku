#include "../include/zobrist_hasher.hpp"
#include <random>
#include <iostream>
#include <array>
#include <algorithm>
#include <functional>

// Inicialización estática
std::mt19937_64 ZobristHasher::rng;
bool ZobristHasher::rngInitialized = false;

ZobristHasher::ZobristHasher() {
    initializeZobristTable();
}

void ZobristHasher::initializeZobristTable() {
    // Inicializar generador aleatorio una sola vez
    if (!rngInitialized) {
        std::random_device rd;
        // Sembrar con múltiples valores para mejor entropía
        std::array<uint32_t, std::mt19937_64::state_size> seed_data;
        std::generate(seed_data.begin(), seed_data.end(), std::ref(rd));
        std::seed_seq seq(seed_data.begin(), seed_data.end());
        rng.seed(seq);
        rngInitialized = true;
        
        // Inicialización exitosa - se loggeará desde main
    }
    
    // Inicializar tabla principal
    for (int i = 0; i < GameState::BOARD_SIZE; i++) {
        for (int j = 0; j < GameState::BOARD_SIZE; j++) {
            // EMPTY siempre es 0 (por convención Zobrist)
            zobristTable[i][j][GameState::EMPTY] = 0;
            
            // Generar claves únicas para cada jugador
            zobristTable[i][j][GameState::PLAYER1] = generateRandomKey();
            zobristTable[i][j][GameState::PLAYER2] = generateRandomKey();
        }
    }
    
    // Hash para turno del jugador
    turnHash = generateRandomKey();
    
    // Hash para capturas
    for (int player = 0; player < 2; player++) {
        for (int captures = 0; captures <= 10; captures++) {
            captureHashes[player][captures] = generateRandomKey();
        }
    }
    
    // Estadísticas de Zobrist - se loggeará desde main si es necesario
}

ZobristHasher::ZobristKey ZobristHasher::generateRandomKey() {
    std::uniform_int_distribution<uint64_t> dis;
    return dis(rng);
}

ZobristHasher::ZobristKey ZobristHasher::computeFullHash(const GameState& state) const {
    ZobristKey hash = 0;
    
    // Hash de todas las piezas en el tablero
    for (int i = 0; i < GameState::BOARD_SIZE; i++) {
        for (int j = 0; j < GameState::BOARD_SIZE; j++) {
            int piece = state.getPiece(i, j);
            if (piece != GameState::EMPTY) {
                hash ^= zobristTable[i][j][piece];
            }
        }
    }
    
    // Hash del turno actual (solo si es PLAYER2)
    // Convención: PLAYER1 = hash base, PLAYER2 = hash base ^ turnHash
    if (state.currentPlayer == GameState::PLAYER2) {
        hash ^= turnHash;
    }
    
    // Hash de capturas para ambos jugadores
    hash ^= captureHashes[0][state.captures[0]]; // PLAYER1 captures
    hash ^= captureHashes[1][state.captures[1]]; // PLAYER2 captures
    
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
    
    // 1. Colocar nueva pieza
    newHash ^= zobristTable[move.x][move.y][player];
    
    // 2. Remover piezas capturadas
    int opponent = (player == GameState::PLAYER1) ? GameState::PLAYER2 : GameState::PLAYER1;
    for (const Move& captured : capturedPieces) {
        newHash ^= zobristTable[captured.x][captured.y][opponent];
    }
    
    // 3. Cambiar turno (siempre alternamos)
    newHash ^= turnHash;
    
    // 4. Actualizar hash de capturas
    int playerIndex = player - 1; // PLAYER1=0, PLAYER2=1
    
    // Quitar hash de capturas anteriores
    newHash ^= captureHashes[playerIndex][oldCaptures];
    
    // Agregar hash de capturas nuevas
    newHash ^= captureHashes[playerIndex][newCaptures];
    
    return newHash;
}

ZobristHasher::ZobristKey ZobristHasher::revertMove(
    ZobristKey currentHash,
    const Move& move,
    int player,
    const std::vector<Move>& capturedPieces,
    int oldCaptures,
    int newCaptures) const {
    
    // Debido a las propiedades del XOR: A ^ B ^ B = A
    // Revertir es exactamente el mismo proceso que aplicar
    return updateHashAfterMove(currentHash, move, player, capturedPieces, 
                              newCaptures, oldCaptures); // Nota: intercambiamos oldCaptures y newCaptures
}

ZobristHasher::ZobristKey ZobristHasher::getPieceHash(int x, int y, int piece) const {
    if (x < 0 || x >= GameState::BOARD_SIZE || 
        y < 0 || y >= GameState::BOARD_SIZE ||
        piece < 0 || piece > 2) {
        return 0;
    }
    
    return zobristTable[x][y][piece];
}

// En zobrist_hasher.cpp:
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
    
    // 1. Colocar nueva pieza
    newHash ^= zobristTable[move.x][move.y][player];
    
    // 2. Remover capturas propias (piezas del oponente)
    for (const Move& captured : myCapturedPieces) {
        newHash ^= zobristTable[captured.x][captured.y][opponent];
    }
    
    // 3. Remover capturas del oponente (mis piezas)
    for (const Move& captured : opponentCapturedPieces) {
        newHash ^= zobristTable[captured.x][captured.y][player];
    }
    
    // 4. Cambiar turno
    newHash ^= turnHash;
    
    // 5. Actualizar capturas propias
    int playerIndex = player - 1;
    newHash ^= captureHashes[playerIndex][oldMyCaptures];
    newHash ^= captureHashes[playerIndex][newMyCaptures];
    
    // 6. Actualizar capturas del oponente
    int opponentIndex = opponent - 1;
    if (oldOppCaptures != newOppCaptures) {
        newHash ^= captureHashes[opponentIndex][oldOppCaptures];
        newHash ^= captureHashes[opponentIndex][newOppCaptures];
    }
    
    return newHash;
}
