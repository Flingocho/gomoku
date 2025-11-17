// ============================================================================
// GUI Renderer - Game Module
// Contains: Game rendering, game click handling, user move methods
// ============================================================================

#include "../../include/gui/gui_renderer.hpp"
#include <iostream>
#include <chrono>

// ============================================================================
// USER MOVE METHODS
// ============================================================================

Move GuiRenderer::waitForUserMove(const GameState& state) {
    moveReady = false;
    setState(PLAYING);
    
    auto timeout = std::chrono::milliseconds(100); // 100ms timeout
    auto startTime = std::chrono::steady_clock::now();
    
    // Este será el método "blocking" que espera hasta que el usuario haga click
    while (isWindowOpen() && !moveReady) {
        processEvents();
        render(state);
        
        auto elapsed = std::chrono::steady_clock::now() - startTime;
        if (elapsed > timeout) {
            // No bloquear más - permitir que el main loop continue
            return Move(-1, -1); // Señal de "aún esperando"
        }
    }
    
    return moveReady ? pendingMove : Move(-1, -1);
}

Move GuiRenderer::getUserMove() {
    // Versión non-blocking
    if (moveReady) {
        Move move = pendingMove;
        clearUserMove();
        return move;
    }
    return Move(-1, -1);
}

// ============================================================================
// GAME RENDERING
// ============================================================================

void GuiRenderer::renderGame(const GameState& state, int aiTimeMs) {
    // Fondo moderno igual que el menú
    drawModernBackground();
    
    drawBoard();
    drawPieces(state);
    drawSuggestionIndicator();
    drawInvalidMoveIndicator();
    drawHoverIndicator();
    drawGameInfo(state, aiTimeMs);
}

// ============================================================================
// GAME CLICK HANDLING
// ============================================================================

void GuiRenderer::handleGameClick(int x, int y) {
    if (!isPointInBoard(x, y)) return;
    
    auto [boardX, boardY] = pixelToBoardPosition(x, y);
    pendingMove = Move(boardX, boardY);
    moveReady = true;
    
    std::cout << "Movimiento capturado: " << char('A' + boardY) << (boardX + 1) << std::endl;
}
