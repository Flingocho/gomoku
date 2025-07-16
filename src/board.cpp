#include "../include/board.hpp"
#include <iostream>
#include <iomanip>
#include <set>

Board::Board() {
    // Inicializar tablero vacío
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            board[i][j] = 0;
        }
    }
    captures[0] = captures[1] = 0;
}

bool Board::isValid(int x, int y) const {
    return x >= 0 && x < SIZE && y >= 0 && y < SIZE;
}

bool Board::isEmpty(int x, int y) const {
    if (!isValid(x, y)) return false;
    return board[x][y] == 0;
}

int Board::getPiece(int x, int y) const {
    if (!isValid(x, y)) return -1;
    return board[x][y];
}

bool Board::placePiece(int x, int y, int player) {
    if (!isEmpty(x, y)) return false;
    if (player != 1 && player != 2) return false;
    
    // Verificar double free-threes ANTES de colocar
    if (isDoubleFree(x, y, player)) {
        // std::cout << "DEBUG: Double free-three detected! Move rejected." << std::endl;
        return false;  // Movimiento ilegal
    }
    
    // std::cout << "DEBUG: Placing piece at (" << x << "," << y << ") for player " << player << std::endl;
    
    board[x][y] = player;
    
    // Verificar capturas del jugador que colocó
    auto captured = checkCaptures(x, y, player);
    if (!captured.empty()) {
        // std::cout << "DEBUG: Player " << player << " captures " << captured.size() << " pieces!" << std::endl;
        for (auto& pos : captured) {
            // std::cout << "DEBUG: Capturing piece at (" << pos.first << "," << pos.second << ")" << std::endl;
            board[pos.first][pos.second] = 0;
        }
        captures[player - 1] += captured.size();
    }
    
    // Verificar si el oponente puede capturar debido a esta pieza
    int opponent = (player == 1) ? 2 : 1;
    auto opponentCaptures = checkCapturesForOpponent(x, y, opponent);
    if (!opponentCaptures.empty()) {
        // std::cout << "DEBUG: Opponent " << opponent << " captures " << opponentCaptures.size() << " pieces!" << std::endl;
        for (auto& pos : opponentCaptures) {
            // std::cout << "DEBUG: Opponent capturing piece at (" << pos.first << "," << pos.second << ")" << std::endl;
            board[pos.first][pos.second] = 0;
        }
        captures[opponent - 1] += opponentCaptures.size();
    }
    
    // if (captured.empty() && opponentCaptures.empty()) {
    //     std::cout << "DEBUG: No captures found." << std::endl;
    // }
    
    return true;
}

void Board::printRaw() const {
    std::cout << "Raw board state:\n";
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            std::cout << board[i][j] << " ";
        }
        std::cout << "\n";
    }
    std::cout << "Captures: P1=" << captures[0] << " P2=" << captures[1] << "\n";
}

// ================== REGLAS DEL JUEGO ==================

bool Board::checkWin(int player) const {
    // Victoria por capturas: 10 piezas capturadas
    if (captures[player - 1] >= 10) return true;
    
    // Victoria por 5 en línea
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (board[i][j] == player) {
                if (checkLineWin(i, j, player)) return true;
            }
        }
    }
    return false;
}

bool Board::checkLineWin(int x, int y, int player) const {
    // 4 direcciones: horizontal, vertical, diagonal /, diagonal 
    int directions[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};
    
    for (int d = 0; d < 4; d++) {
        int dx = directions[d][0];
        int dy = directions[d][1];
        
        // Contar en ambas direcciones
        int count = 1; // La pieza actual
        count += countDirection(x, y, dx, dy, player);
        count += countDirection(x, y, -dx, -dy, player);
        
        if (count >= 5) return true;
    }
    return false;
}

int Board::countDirection(int x, int y, int dx, int dy, int player) const {
    int count = 0;
    int nx = x + dx;
    int ny = y + dy;
    
    while (isValid(nx, ny) && board[nx][ny] == player) {
        count++;
        nx += dx;
        ny += dy;
    }
    return count;
}

std::vector<std::pair<int, int>> Board::checkCaptures(int x, int y, int player) {
    std::set<std::pair<int, int>> capturedSet;  // Usar set para evitar duplicados
    
    // 8 direcciones (pero sin duplicados)
    int directions[8][2] = {
        {-1, -1}, {-1, 0}, {-1, 1},
        {0, -1},           {0, 1},
        {1, -1},  {1, 0},  {1, 1}
    };
    
    for (int d = 0; d < 8; d++) {
        int dx = directions[d][0];
        int dy = directions[d][1];
        
        auto dirCaptures = getCapturesInDirection(x, y, dx, dy, player);
        for (auto& cap : dirCaptures) {
            capturedSet.insert(cap);
        }
    }
    
    // Convertir set a vector
    std::vector<std::pair<int, int>> captured(capturedSet.begin(), capturedSet.end());
    return captured;
}

std::vector<std::pair<int, int>> Board::getCapturesInDirection(int x, int y, int dx, int dy, int player) {
    std::vector<std::pair<int, int>> captures;
    int opponent = (player == 1) ? 2 : 1;
    
    std::cout << "DEBUG: Direction (" << dx << "," << dy << ") - Forward: " << countDirection(x, y, dx, dy, player) 
              << ", Backward: " << countDirection(x, y, -dx, -dy, player) << std::endl;
    
    // Buscar patrón hacia adelante: [PIEZA_ACTUAL][ENEMIGO][ENEMIGO][MI_PIEZA]
    int pos1x = x + dx, pos1y = y + dy;
    int pos2x = x + 2*dx, pos2y = y + 2*dy;
    int pos3x = x + 3*dx, pos3y = y + 3*dy;
    
    if (isValid(pos1x, pos1y) && isValid(pos2x, pos2y) && isValid(pos3x, pos3y)) {
        int piece1 = board[pos1x][pos1y];
        int piece2 = board[pos2x][pos2y];
        int piece3 = board[pos3x][pos3y];
        
        std::cout << "DEBUG: Forward pattern: " << player << "-" << piece1 << "-" << piece2 << "-" << piece3 << std::endl;
        
        if (piece1 == opponent && piece2 == opponent && piece3 == player) {
            captures.push_back({pos1x, pos1y});
            captures.push_back({pos2x, pos2y});
        }
    }
    
    // Buscar patrón hacia atrás: [MI_PIEZA][ENEMIGO][ENEMIGO][PIEZA_ACTUAL]
    pos1x = x - dx; pos1y = y - dy;
    pos2x = x - 2*dx; pos2y = y - 2*dy;
    pos3x = x - 3*dx; pos3y = y - 3*dy;
    
    if (isValid(pos1x, pos1y) && isValid(pos2x, pos2y) && isValid(pos3x, pos3y)) {
        int piece1 = board[pos1x][pos1y];
        int piece2 = board[pos2x][pos2y];
        int piece3 = board[pos3x][pos3y];
        
        std::cout << "DEBUG: Backward pattern: " << piece3 << "-" << piece2 << "-" << piece1 << "-" << player << std::endl;
        
        if (piece1 == opponent && piece2 == opponent && piece3 == player) {
            captures.push_back({pos1x, pos1y});
            captures.push_back({pos2x, pos2y});
        }
    }
    
    return captures;
}

std::vector<std::pair<int, int>> Board::checkCapturesForOpponent(int x, int y, int opponent) {
    std::set<std::pair<int, int>> capturedSet;  // Usar set para evitar duplicados
    
    // 8 direcciones (pero sin duplicados)
    int directions[8][2] = {
        {-1, -1}, {-1, 0}, {-1, 1},
        {0, -1},           {0, 1},
        {1, -1},  {1, 0},  {1, 1}
    };
    
    for (int d = 0; d < 8; d++) {
        int dx = directions[d][0];
        int dy = directions[d][1];
        
        auto dirCaptures = getCapturesForOpponentInDirection(x, y, dx, dy, opponent);
        for (auto& cap : dirCaptures) {
            capturedSet.insert(cap);
        }
    }
    
    // Convertir set a vector
    std::vector<std::pair<int, int>> captured(capturedSet.begin(), capturedSet.end());
    return captured;
}

std::vector<std::pair<int, int>> Board::getCapturesForOpponentInDirection(int x, int y, int dx, int dy, int opponent) {
    std::vector<std::pair<int, int>> captures;
    int player = (opponent == 1) ? 2 : 1;  // El que acaba de colocar
    
    // Buscar patrón: [OPPONENT][VICTIM1][VICTIM2=RECIEN_COLOCADA][OPPONENT]
    int victim1_x = x - dx, victim1_y = y - dy;
    int opp1_x = x - 2*dx, opp1_y = y - 2*dy;
    int opp2_x = x + dx, opp2_y = y + dy;
    
    if (isValid(victim1_x, victim1_y) && isValid(opp1_x, opp1_y) && isValid(opp2_x, opp2_y)) {
        if (board[opp1_x][opp1_y] == opponent &&
            board[victim1_x][victim1_y] == player &&
            board[x][y] == player &&
            board[opp2_x][opp2_y] == opponent) {
            
            captures.push_back({victim1_x, victim1_y});
            captures.push_back({x, y});
        }
    }
    
    return captures;
}

bool Board::isDoubleFree(int x, int y, int player) const {
    // Simular colocación temporal
    const_cast<Board*>(this)->board[x][y] = player;
    
    int freeThreeCount = 0;
    
    // 4 direcciones principales para evitar contar duplicados
    int directions[4][2] = {
        {0, 1},   // Horizontal →
        {1, 0},   // Vertical ↓ 
        {1, 1},   // Diagonal \ ↘
        {1, -1}   // Diagonal / ↙
    };
    
    for (int d = 0; d < 4; d++) {
        int dx = directions[d][0];
        int dy = directions[d][1];
        
        if (isFreeThree(x, y, dx, dy, player)) {
            freeThreeCount++;
            std::cout << "DEBUG: Found free-three in direction (" << dx << "," << dy << ")" << std::endl;
        }
    }
    
    // Remover pieza temporal
    const_cast<Board*>(this)->board[x][y] = 0;
    
    std::cout << "DEBUG: Total free-threes created: " << freeThreeCount << std::endl;
    return freeThreeCount >= 2;
}

bool Board::isFreeThree(int x, int y, int dx, int dy, int player) const {
    // Contar piezas consecutivas en ambas direcciones
    int countForward = countDirection(x, y, dx, dy, player);
    int countBackward = countDirection(x, y, -dx, -dy, player);
    int totalInLine = 1 + countForward + countBackward;
    
    std::cout << "DEBUG: Direction (" << dx << "," << dy << ") - Forward: " << countForward 
              << ", Backward: " << countBackward << ", Total: " << totalInLine << std::endl;
    
    // Para ser free-three, necesitamos exactamente 3 en línea
    if (totalInLine != 3) return false;
    
    // Verificar extremos libres
    int frontX = x + (countForward + 1) * dx;
    int frontY = y + (countForward + 1) * dy;
    int backX = x - (countBackward + 1) * dx;
    int backY = y - (countBackward + 1) * dy;
    
    bool frontFree = isValid(frontX, frontY) && board[frontX][frontY] == 0;
    bool backFree = isValid(backX, backY) && board[backX][backY] == 0;
    
    std::cout << "DEBUG: Front (" << frontX << "," << frontY << ") free: " << frontFree 
              << ", Back (" << backX << "," << backY << ") free: " << backFree << std::endl;
    
    // Free-three más permisivo: al menos UN extremo libre
    // (puedes cambiar a "frontFree && backFree" para más estricto)
    return frontFree || backFree;
}

// ================== MÉTODOS PARA SIMULACIÓN DEL AI ==================

bool Board::placePieceSimple(int x, int y, int player) {
    if (!isEmpty(x, y)) return false;
    if (player != 1 && player != 2) return false;
    
    board[x][y] = player;
    return true;
}

void Board::removePiece(int x, int y) {
    if (isValid(x, y)) {
        board[x][y] = 0;
    }
}