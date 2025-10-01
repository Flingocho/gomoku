/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   gui_renderer.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/01 00:00:00 by jainavas          #+#    #+#             */
/*   Updated: 2025/10/01 22:29:52 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/gui_renderer.hpp"
#include <iostream>
#include <sstream>  // NUEVO: Para stringstream en drawGameInfo
#include <cmath>    // NUEVO: Para sin() en el efecto pulsante
#include <vector>   // NUEVO: Para vector de tiempos de IA

GuiRenderer::GuiRenderer() 
    : window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Gomoku AI"),
      currentState(MENU),
      selectedMenuOption(-1),
      hoveredMenuOption(-1),
      moveReady(false),
      hoverPosition(-1, -1),    // NUEVO: Inicializar hover position
      lastAiMove(-1, -1),       // NUEVO: Última ficha de la IA
      backgroundColor(sf::Color(40, 44, 52)),     // Dark blue-gray
      boardLineColor(sf::Color(139, 69, 19)),     // Saddle brown
      player1Color(sf::Color(30, 144, 255)),     // Human - Dodger Blue (más legible)
      player2Color(sf::Color::Red),               // AI - Red  
      hoverColor(sf::Color(255, 255, 255, 100)),   // Transparent white
      totalAiTime(0),                             // NUEVO: Inicializar estadísticas
      aiMoveCount(0),
	  currentSuggestion(-1, -1),     // NUEVO
      showSuggestion(false),
	  errorMessage(""),              // NUEVO: Inicializar sistema de errores
	  showError(false),
	  invalidMovePosition(-1, -1),
	  gameOverButtonsY(0),
	  gameOverButtonsPositionValid(false)
{
    // Cargar fuente personalizada
    if (!font.loadFromFile("fonts/LEMONMILK-Medium.otf")) {
        // Si no encuentra archivo, continuar sin fuente personalizada
        std::cout << "Warning: Could not load font fonts/street_drips.ttf, using default" << std::endl;
        // SFML puede funcionar sin fuente explícita
    } else {
        std::cout << "✓ Custom font loaded: fonts/street_drips.ttf" << std::endl;
    }
    
    window.setFramerateLimit(60);
    std::cout << "GUI Renderer initialized: " << WINDOW_WIDTH << "x" << WINDOW_HEIGHT << std::endl;
}

GuiRenderer::~GuiRenderer() {
    if (window.isOpen()) {
        window.close();
    }
}

bool GuiRenderer::isWindowOpen() const {
    return window.isOpen();
}

void GuiRenderer::processEvents() {
    while (window.pollEvent(event)) {
        switch (event.type) {
            case sf::Event::Closed:
                window.close();
                break;
                
            case sf::Event::MouseButtonPressed:
                if (event.mouseButton.button == sf::Mouse::Left) {
                    int x = event.mouseButton.x;
                    int y = event.mouseButton.y;
                    
                    switch (currentState) {
                        case MENU:
                            handleMenuClick(x, y);
                            break;
                        case PLAYING:
                            handleGameClick(x, y);
                            break;
                        case GAME_OVER:
                            handleGameOverClick(x, y);
                            break;
                    }
                }
                break;
                
            case sf::Event::MouseMoved:
                handleMouseMove(event.mouseMove.x, event.mouseMove.y);
                break;
                
            case sf::Event::KeyPressed:
                if (event.key.code == sf::Keyboard::Escape) {
                    if (currentState != MENU) {
                        setState(MENU);
                    } else {
                        window.close();
                    }
                }
                break;
                
            default:
                break;
        }
    }
}

void GuiRenderer::render(const GameState& state, int aiTimeMs) {
    window.clear(backgroundColor);
    
    switch (currentState) {
        case MENU:
            renderMenu();
            break;
        case PLAYING:
            renderGame(state, aiTimeMs);
            break;
        case GAME_OVER:
            renderGameOver(state);  // CORREGIDO: Pasar state
            break;
    }
    
    window.display();
}

// ===============================================
// MENU METHODS - Implementación básica
// ===============================================

GuiRenderer::MenuOption GuiRenderer::showMenuAndGetChoice() {
    setState(MENU);
    
    // Este método será non-blocking, la elección se captura en handleMenuClick
    if (selectedMenuOption == 0) return VS_AI;
    if (selectedMenuOption == 1) return VS_HUMAN; 
    if (selectedMenuOption == 2) return QUIT;
    
    return NONE; // Sin selección aún
}

void GuiRenderer::renderMenu() {
    // 1. Título principal
    drawText("=== GOMOKU AI ===", WINDOW_WIDTH/2 - 120, 100, 36, sf::Color::White);
    drawText("5 in a row with advanced AI", WINDOW_WIDTH/2 - 100, 150, 18, sf::Color(200, 200, 200));
    
    // 2. Botones del menú (centrados)
    int buttonWidth = 250;
    int buttonHeight = 60;
    int buttonX = WINDOW_WIDTH/2 - buttonWidth/2;
    int buttonSpacing = 80;
    
    // CORREGIDO: Usar hoveredMenuOption para efectos visuales
    bool vsAiHover = (hoveredMenuOption == 0);
    drawButton("Play vs AI", buttonX, 250, buttonWidth, buttonHeight, vsAiHover);
    
    bool vsHumanHover = (hoveredMenuOption == 1);
    drawButton("Play vs Human", buttonX, 250 + buttonSpacing, buttonWidth, buttonHeight, vsHumanHover);
    
    bool quitHover = (hoveredMenuOption == 2);
    drawButton("Exit", buttonX, 250 + buttonSpacing * 2, buttonWidth, buttonHeight, quitHover);
    
    // 3. Información adicional
    drawText("Features:", 50, 500, 20, sf::Color::Yellow);
    drawText("- Zobrist Hashing for maximum performance", 70, 530, 16, sf::Color::White);
    drawText("- Minimax search with Alpha-Beta pruning", 70, 550, 16, sf::Color::White);
    drawText("- Adaptive depth up to 10 levels", 70, 570, 16, sf::Color::White);
    drawText("- Complete rules: captures + double-three", 70, 590, 16, sf::Color::White);
    
    // 4. Controles
    drawText("Use mouse to select", WINDOW_WIDTH/2 - 100, 650, 16, sf::Color(150, 150, 150));
    drawText("ESC = Exit", WINDOW_WIDTH - 100, WINDOW_HEIGHT - 30, 14, sf::Color(100, 100, 100));
}

void GuiRenderer::handleMenuClick(int x, int y) {
    int buttonWidth = 250;
    int buttonHeight = 60;
    int buttonX = WINDOW_WIDTH/2 - buttonWidth/2;
    
    // Botón VS AI (posición Y: 250)
    if (x >= buttonX && x <= buttonX + buttonWidth && 
        y >= 250 && y <= 250 + buttonHeight) {
        selectedMenuOption = 0;
        std::cout << "Seleccionado: VS AI" << std::endl;
        return;
    }
    
    // Botón VS Human (posición Y: 330)
    if (x >= buttonX && x <= buttonX + buttonWidth && 
        y >= 330 && y <= 330 + buttonHeight) {
        selectedMenuOption = 1;
        std::cout << "Seleccionado: VS Human" << std::endl;
        return;
    }
    
    // Botón Quit (posición Y: 410)
    if (x >= buttonX && x <= buttonX + buttonWidth && 
        y >= 410 && y <= 410 + buttonHeight) {
        selectedMenuOption = 2;
        std::cout << "Seleccionado: Quit" << std::endl;
        return;
    }
    
    // Click fuera de botones
    selectedMenuOption = -1;
}

// ===============================================
// GAME METHODS - Esqueletos
// ===============================================

Move GuiRenderer::waitForUserMove(const GameState& state) {
    moveReady = false;
    setState(PLAYING);
    
    // REDUCIR TIMEOUT para evitar congelamientos
    auto timeout = std::chrono::milliseconds(100); // 100ms timeout
    auto startTime = std::chrono::steady_clock::now();
    
    // Este será el método "blocking" que espera hasta que el usuario haga click
    while (isWindowOpen() && !moveReady) {
        processEvents();
        render(state);
        
        // NUEVO: Timeout para evitar congelamiento
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

void GuiRenderer::renderGame(const GameState& state, int aiTimeMs) {
    drawBoard();
    drawPieces(state);
	drawSuggestionIndicator();
	drawInvalidMoveIndicator(); // NUEVO: Mostrar indicador de movimiento inválido
    drawHoverIndicator(); // NUEVO: Mostrar hover antes de la info
    drawGameInfo(state, aiTimeMs);
}

void GuiRenderer::handleGameClick(int x, int y) {
    if (!isPointInBoard(x, y)) return;
    
    auto [boardX, boardY] = pixelToBoardPosition(x, y);
    pendingMove = Move(boardX, boardY);
    moveReady = true;
    
    std::cout << "Movimiento capturado: " << char('A' + boardY) << (boardX + 1) << std::endl;
}

// ===============================================
// RENDERING HELPERS - Esqueletos básicos
// ===============================================

// ===============================================
// RENDERING HELPERS - Board and pieces
// ===============================================

void GuiRenderer::drawHoverIndicator() {
    // Solo mostrar hover si estamos en una posición válida
    if (!hoverPosition.isValid()) return;
    
    // No mostrar hover en celdas ocupadas
    // (Necesitaríamos el state aquí, por simplicidad lo omitimos por ahora)
    
    sf::Vector2i pos = boardPositionToPixel(hoverPosition.x, hoverPosition.y);
    int cellSize = CELL_SIZE - 4; // Mismo tamaño que las celdas del tablero
    
    // 1. Fondo de la celda con transparencia
    sf::RectangleShape hoverBg(sf::Vector2f(cellSize, cellSize));
    hoverBg.setPosition(pos.x - cellSize/2 + 2, pos.y - cellSize/2 + 2);
    hoverBg.setFillColor(sf::Color(255, 255, 255, 50)); // Blanco semi-transparente
    window.draw(hoverBg);
    
    // 2. Borde brillante
    sf::RectangleShape hoverBorder(sf::Vector2f(cellSize, cellSize));
    hoverBorder.setPosition(pos.x - cellSize/2 + 2, pos.y - cellSize/2 + 2);
    hoverBorder.setFillColor(sf::Color::Transparent);
    hoverBorder.setOutlineThickness(2);
    hoverBorder.setOutlineColor(sf::Color(255, 255, 0, 150)); // Amarillo semi-transparente
    window.draw(hoverBorder);
    
    // 3. Pequeño círculo en el centro para indicar donde se colocaría la ficha
    sf::CircleShape hoverIndicator(6);
    hoverIndicator.setPosition(pos.x - 6, pos.y - 6);
    hoverIndicator.setFillColor(sf::Color(255, 255, 255, 100));
    hoverIndicator.setOutlineThickness(1);
    hoverIndicator.setOutlineColor(sf::Color(255, 255, 0, 200));
    window.draw(hoverIndicator);
}

void GuiRenderer::drawBoard() {
    // 1. Sombra del tablero (efecto 3D)
    sf::RectangleShape boardShadow(sf::Vector2f(BOARD_SIZE_PX + 8, BOARD_SIZE_PX + 8));
    boardShadow.setPosition(BOARD_OFFSET_X + 4, BOARD_OFFSET_Y + 4); // Offset para sombra
    boardShadow.setFillColor(sf::Color(0, 0, 0, 80)); // Negro semi-transparente
    window.draw(boardShadow);
    
    // 2. Fondo principal del tablero (más claro para contraste)
    sf::RectangleShape boardBg(sf::Vector2f(BOARD_SIZE_PX, BOARD_SIZE_PX));
    boardBg.setPosition(BOARD_OFFSET_X, BOARD_OFFSET_Y);
    boardBg.setFillColor(sf::Color(245, 222, 179)); // Wheat - más claro
    window.draw(boardBg);
    
    // 3. Borde biselado (efecto 3D)
    sf::RectangleShape topBevel(sf::Vector2f(BOARD_SIZE_PX, 3));
    topBevel.setPosition(BOARD_OFFSET_X, BOARD_OFFSET_Y);
    topBevel.setFillColor(sf::Color(255, 248, 220)); // Highlight superior
    window.draw(topBevel);
    
    sf::RectangleShape leftBevel(sf::Vector2f(3, BOARD_SIZE_PX));
    leftBevel.setPosition(BOARD_OFFSET_X, BOARD_OFFSET_Y);
    leftBevel.setFillColor(sf::Color(255, 248, 220)); // Highlight izquierdo
    window.draw(leftBevel);
    
    sf::RectangleShape bottomBevel(sf::Vector2f(BOARD_SIZE_PX, 3));
    bottomBevel.setPosition(BOARD_OFFSET_X, BOARD_OFFSET_Y + BOARD_SIZE_PX - 3);
    bottomBevel.setFillColor(sf::Color(160, 130, 98)); // Sombra inferior
    window.draw(bottomBevel);
    
    sf::RectangleShape rightBevel(sf::Vector2f(3, BOARD_SIZE_PX));
    rightBevel.setPosition(BOARD_OFFSET_X + BOARD_SIZE_PX - 3, BOARD_OFFSET_Y);
    rightBevel.setFillColor(sf::Color(160, 130, 98)); // Sombra derecha
    window.draw(rightBevel);
    
    // 4. Celdas individuales con efecto hundido
    for (int i = 0; i < GameState::BOARD_SIZE; i++) {
        for (int j = 0; j < GameState::BOARD_SIZE; j++) {
            int cellX = BOARD_OFFSET_X + j * CELL_SIZE + 2; // +2 para margen interno
            int cellY = BOARD_OFFSET_Y + i * CELL_SIZE + 2;
            int cellSize = CELL_SIZE - 4; // -4 para el margen
            
            // Celda base (ligeramente más oscura)
            sf::RectangleShape cell(sf::Vector2f(cellSize, cellSize));
            cell.setPosition(cellX, cellY);
            cell.setFillColor(sf::Color(210, 180, 140)); // Tan - más oscuro que el fondo
            window.draw(cell);
            
            // Efecto hundido - sombra interior
            sf::RectangleShape innerShadowTop(sf::Vector2f(cellSize, 1));
            innerShadowTop.setPosition(cellX, cellY);
            innerShadowTop.setFillColor(sf::Color(160, 130, 98)); // Sombra superior
            window.draw(innerShadowTop);
            
            sf::RectangleShape innerShadowLeft(sf::Vector2f(1, cellSize));
            innerShadowLeft.setPosition(cellX, cellY);
            innerShadowLeft.setFillColor(sf::Color(160, 130, 98)); // Sombra izquierda
            window.draw(innerShadowLeft);
            
            // Highlight inferior derecho (para efecto hundido)
            sf::RectangleShape innerHighlightBottom(sf::Vector2f(cellSize, 1));
            innerHighlightBottom.setPosition(cellX, cellY + cellSize - 1);
            innerHighlightBottom.setFillColor(sf::Color(235, 210, 175)); // Highlight inferior
            window.draw(innerHighlightBottom);
            
            sf::RectangleShape innerHighlightRight(sf::Vector2f(1, cellSize));
            innerHighlightRight.setPosition(cellX + cellSize - 1, cellY);
            innerHighlightRight.setFillColor(sf::Color(235, 210, 175)); // Highlight derecho
            window.draw(innerHighlightRight);
        }
    }
    
    // 5. Coordenadas del tablero (A-S horizontalmente, 1-19 verticalmente)
    // Letras horizontales (A-S)
    for (int j = 0; j < GameState::BOARD_SIZE; j++) {
        char letter = 'A' + j;
        std::string coordText(1, letter);
        
        // Coordenada superior
        drawText(coordText, 
                BOARD_OFFSET_X + j * CELL_SIZE + CELL_SIZE/2 - 6, 
                BOARD_OFFSET_Y - 25, 
                16, sf::Color(100, 50, 0));
        
        // Coordenada inferior
        drawText(coordText, 
                BOARD_OFFSET_X + j * CELL_SIZE + CELL_SIZE/2 - 6, 
                BOARD_OFFSET_Y + BOARD_SIZE_PX + 8, 
                16, sf::Color(100, 50, 0));
    }
    
    // Números verticales (1-19)
    for (int i = 0; i < GameState::BOARD_SIZE; i++) {
        std::string coordText = std::to_string(i + 1);
        
        // Coordenada izquierda
        drawText(coordText, 
                BOARD_OFFSET_X - 25, 
                BOARD_OFFSET_Y + i * CELL_SIZE + CELL_SIZE/2 - 8, 
                16, sf::Color(100, 50, 0));
        
        // Coordenada derecha
        drawText(coordText, 
                BOARD_OFFSET_X + BOARD_SIZE_PX + 8, 
                BOARD_OFFSET_Y + i * CELL_SIZE + CELL_SIZE/2 - 8, 
                16, sf::Color(100, 50, 0));
    }
}

void GuiRenderer::drawPieces(const GameState& state) {
    for (int i = 0; i < GameState::BOARD_SIZE; i++) {
        for (int j = 0; j < GameState::BOARD_SIZE; j++) {
            int piece = state.getPiece(i, j);
            if (piece != GameState::EMPTY) {
                sf::Vector2i pos = boardPositionToPixel(i, j);
                
                // Ficha principal (ligeramente más grande para ocupar bien la celda)
                int pieceRadius = (CELL_SIZE / 2) - 4; // Margen de 4px desde el borde de la celda
                sf::CircleShape pieceMain(pieceRadius);
                pieceMain.setPosition(pos.x - pieceRadius, pos.y - pieceRadius);
                pieceMain.setFillColor(getPieceColor(piece));
                
                // Efecto 3D para las fichas
                // Sombra de la ficha
                sf::CircleShape pieceShadow(pieceRadius);
                pieceShadow.setPosition(pos.x - pieceRadius + 2, pos.y - pieceRadius + 2); // Offset sombra
                pieceShadow.setFillColor(sf::Color(0, 0, 0, 100)); // Negro semi-transparente
                window.draw(pieceShadow);
                
                // Ficha principal
                window.draw(pieceMain);
                
                // Highlight superior para efecto 3D
                sf::CircleShape pieceHighlight(pieceRadius - 3);
                pieceHighlight.setPosition(pos.x - pieceRadius + 3, pos.y - pieceRadius + 3);
                
                // Color del highlight según el tipo de pieza
                if (piece == GameState::PLAYER1) {
                    // Azul más claro para highlight (más legible)
                    pieceHighlight.setFillColor(sf::Color(135, 206, 250)); // Light sky blue
                } else {
                    // Rojo más claro para highlight  
                    pieceHighlight.setFillColor(sf::Color(255, 99, 71)); // Tomato
                }
                
                window.draw(pieceHighlight);
                
                // Brillo pequeño en la esquina superior izquierda
                sf::CircleShape pieceShine(3);
                pieceShine.setPosition(pos.x - pieceRadius + 5, pos.y - pieceRadius + 5);
                pieceShine.setFillColor(sf::Color(255, 255, 255, 180)); // Blanco semi-transparente
                window.draw(pieceShine);
                
                // NUEVO: Resaltado especial para la última ficha de la IA
                if (piece == GameState::PLAYER2 && lastAiMove.x == i && lastAiMove.y == j) {
                    // Anillo pulsante alrededor de la última ficha de la IA
                    sf::CircleShape lastMoveRing(pieceRadius + 4);
                    lastMoveRing.setPosition(pos.x - pieceRadius - 4, pos.y - pieceRadius - 4);
                    lastMoveRing.setFillColor(sf::Color::Transparent);
                    lastMoveRing.setOutlineThickness(2);
                    
                    // Efecto pulsante usando el tiempo
                    static sf::Clock pulseClock;
                    float pulseTime = pulseClock.getElapsedTime().asSeconds();
                    float alpha = (sin(pulseTime * 3.0f) + 1.0f) * 0.3f + 0.4f; // Oscila entre 0.4 y 1.0
                    lastMoveRing.setOutlineColor(sf::Color(255, 255, 0, (sf::Uint8)(alpha * 255))); // Amarillo pulsante
                    
                    window.draw(lastMoveRing);
                }
            }
        }
    }
}

// ===============================================
// UI ELEMENTS METHODS
// ===============================================

void GuiRenderer::drawButton(const std::string& text, int x, int y, int width, int height, 
                           bool highlighted) {
    // 1. Sombra del botón (efecto 3D)
    sf::RectangleShape buttonShadow(sf::Vector2f(width + 4, height + 4));
    buttonShadow.setPosition(x + 2, y + 2);
    buttonShadow.setFillColor(sf::Color(0, 0, 0, 100));
    window.draw(buttonShadow);
    
    // 2. Fondo del botón
    sf::RectangleShape buttonBg(sf::Vector2f(width, height));
    buttonBg.setPosition(x, y);
    
    if (highlighted) {
        // Color más claro cuando está resaltado
        buttonBg.setFillColor(sf::Color(70, 130, 180)); // Steel blue
        buttonBg.setOutlineThickness(3);
        buttonBg.setOutlineColor(sf::Color::White);
    } else {
        // Color normal
        buttonBg.setFillColor(sf::Color(47, 79, 79)); // Dark slate gray
        buttonBg.setOutlineThickness(2);
        buttonBg.setOutlineColor(sf::Color(105, 105, 105)); // Dim gray
    }
    
    window.draw(buttonBg);
    
    // 3. Efecto biselado (borde 3D)
    if (!highlighted) {
        // Highlight superior e izquierdo
        sf::RectangleShape topHighlight(sf::Vector2f(width, 2));
        topHighlight.setPosition(x, y);
        topHighlight.setFillColor(sf::Color(200, 200, 200));
        window.draw(topHighlight);
        
        sf::RectangleShape leftHighlight(sf::Vector2f(2, height));
        leftHighlight.setPosition(x, y);
        leftHighlight.setFillColor(sf::Color(200, 200, 200));
        window.draw(leftHighlight);
        
        // Sombra inferior y derecha
        sf::RectangleShape bottomShadow(sf::Vector2f(width, 2));
        bottomShadow.setPosition(x, y + height - 2);
        bottomShadow.setFillColor(sf::Color(30, 30, 30));
        window.draw(bottomShadow);
        
        sf::RectangleShape rightShadow(sf::Vector2f(2, height));
        rightShadow.setPosition(x + width - 2, y);
        rightShadow.setFillColor(sf::Color(30, 30, 30));
        window.draw(rightShadow);
    }
    
    // 4. Texto del botón (centrado)
    sf::Color textColor = highlighted ? sf::Color::Yellow : sf::Color::White;
    int textSize = 20;
    
    // Aproximación del ancho del texto (para centrado)
    int textWidth = text.length() * textSize * 0.6; // Aproximación
    int textX = x + (width - textWidth) / 2;
    int textY = y + (height - textSize) / 2;
    
    drawText(text, textX, textY, textSize, textColor);
}

void GuiRenderer::drawText(const std::string& text, int x, int y, int size, sf::Color color) {
    sf::Text sfText;
    if (font.getInfo().family != "") {
        sfText.setFont(font);
    }
    sfText.setString(text);
    sfText.setCharacterSize(size);
    sfText.setFillColor(color);
    sfText.setPosition(x, y);
    window.draw(sfText);
}

void GuiRenderer::drawGameInfo(const GameState& state, int aiTimeMs) {
    // Panel lateral derecho con información del juego
    int panelX = BOARD_OFFSET_X + BOARD_SIZE_PX + 30;
    int panelY = BOARD_OFFSET_Y;
    int panelWidth = 250;
    
    // 1. Fondo del panel con efecto 3D
    sf::RectangleShape panelShadow(sf::Vector2f(panelWidth + 4, 400));
    panelShadow.setPosition(panelX + 2, panelY + 2);
    panelShadow.setFillColor(sf::Color(0, 0, 0, 100));
    window.draw(panelShadow);
    
    sf::RectangleShape panelBg(sf::Vector2f(panelWidth, 400));
    panelBg.setPosition(panelX, panelY);
    panelBg.setFillColor(sf::Color(45, 45, 45)); // Gris oscuro
    panelBg.setOutlineThickness(2);
    panelBg.setOutlineColor(sf::Color(100, 100, 100));
    window.draw(panelBg);
    
    // 2. Título del panel
    drawText("=== STATUS ===", panelX + 10, panelY + 10, 18, sf::Color::Yellow);
    
    int yOffset = panelY + 50;
    int lineHeight = 25;
    
    // 3. Información básica del juego
    std::string turnInfo = "Turn: " + std::to_string(state.turnCount);
    drawText(turnInfo, panelX + 10, yOffset, 16, sf::Color::White);
    yOffset += lineHeight;
    
    // Jugador actual
    std::string currentPlayerStr = (state.currentPlayer == GameState::PLAYER1) ? "Player 1 (O)" : "Player 2 (X)";
    sf::Color playerColor = (state.currentPlayer == GameState::PLAYER1) ? player1Color : player2Color;
    drawText("Player:", panelX + 10, yOffset, 16, sf::Color::White);
    drawText(currentPlayerStr, panelX + 10, yOffset + lineHeight, 16, playerColor);
    yOffset += lineHeight * 2 + 10;
    
    // NUEVO: Mostrar mensaje de error si está activo
    if (showError && errorTimer.getElapsedTime().asSeconds() < 3.0f) {
        // Fondo rojo para el mensaje de error
        sf::RectangleShape errorBg(sf::Vector2f(panelWidth - 20, 30));
        errorBg.setPosition(panelX + 10, yOffset - 5);
        errorBg.setFillColor(sf::Color(150, 0, 0, 100)); // Rojo semi-transparente
        errorBg.setOutlineThickness(1);
        errorBg.setOutlineColor(sf::Color::Red);
        window.draw(errorBg);
        
        // Texto del error con efecto pulsante
        float pulseTime = errorTimer.getElapsedTime().asSeconds();
        float alpha = (sin(pulseTime * 6.0f) + 1.0f) * 0.3f + 0.7f; // Oscila entre 0.7 y 1.0
        sf::Color errorColor(255, 100, 100, (sf::Uint8)(alpha * 255));
        
        drawText("! " + errorMessage, panelX + 15, yOffset, 14, errorColor);
        
        // Coordenada del movimiento inválido si está disponible
        if (invalidMovePosition.isValid()) {
            std::string posText = "Position: ";
            posText += char('A' + invalidMovePosition.y);
            posText += std::to_string(invalidMovePosition.x + 1);
            drawText(posText, panelX + 15, yOffset + 15, 12, sf::Color(200, 150, 150));
        }
        
        yOffset += 40;
    }
    
    // NUEVO: Mostrar sugerencia si está activa
    if (showSuggestion && currentSuggestion.isValid()) {
        // Separador
        sf::RectangleShape separator(sf::Vector2f(panelWidth - 20, 2));
        separator.setPosition(panelX + 10, yOffset);
        separator.setFillColor(sf::Color(100, 100, 100));
        window.draw(separator);
        yOffset += 20;
        
        // Título de sugerencia con icono
        drawText("AI SUGGESTION:", panelX + 10, yOffset, 16, sf::Color(255, 215, 0));
        yOffset += lineHeight + 5;
        
        // Coordenada sugerida
        std::string suggestionText = "Move to: ";
        suggestionText += char('A' + currentSuggestion.y);
        suggestionText += std::to_string(currentSuggestion.x + 1);
        
        drawText(suggestionText, panelX + 15, yOffset, 16, sf::Color::White);
        yOffset += lineHeight;
        
        // Nota informativa
        drawText("(You can ignore it)", panelX + 15, yOffset, 11, sf::Color(150, 150, 150));
        yOffset += lineHeight + 10;
    }
    
    // 4. Separador
    sf::RectangleShape separator(sf::Vector2f(panelWidth - 20, 2));
    separator.setPosition(panelX + 10, yOffset);
    separator.setFillColor(sf::Color(100, 100, 100));
    window.draw(separator);
    yOffset += 20;
    
    // 5. Capturas
    drawText("CAPTURES:", panelX + 10, yOffset, 16, sf::Color::Yellow);
    yOffset += lineHeight + 5;
    
    // Capturas del humano
    std::string humanCaptures = "You: " + std::to_string(state.captures[0]) + "/10";
    drawText(humanCaptures, panelX + 15, yOffset, 14, player1Color);
    if (state.captures[0] >= 8) {
        drawText("Close!", panelX + 120, yOffset, 14, sf::Color::Red);
    }
    yOffset += lineHeight;
    
    // Capturas de la IA
    std::string aiCaptures = "AI: " + std::to_string(state.captures[1]) + "/10";
    drawText(aiCaptures, panelX + 15, yOffset, 14, player2Color);
    if (state.captures[1] >= 8) {
        drawText("Danger!", panelX + 120, yOffset, 14, sf::Color::Red);
    }
    yOffset += lineHeight + 15;
    
    // 6. Estadísticas de la IA (si disponibles)
    if (aiTimeMs > 0) {
        drawText("AI STATS:", panelX + 10, yOffset, 16, sf::Color::Yellow);
        yOffset += lineHeight + 5;
        
        // Tiempo de pensamiento actual
        std::string timeStr = "Last: " + std::to_string(aiTimeMs) + "ms";
        sf::Color timeColor = sf::Color::White;
        if (aiTimeMs < 100) timeColor = sf::Color::Green;
        else if (aiTimeMs > 1000) timeColor = sf::Color::Red;
        drawText(timeStr, panelX + 15, yOffset, 14, timeColor);
        yOffset += lineHeight;
        
        // Tiempo promedio de la IA
        if (aiMoveCount > 0) {
            float avgTime = getAverageAiTime();
            std::string avgTimeStr = "Avg: " + std::to_string(static_cast<int>(avgTime)) + "ms";
            sf::Color avgColor = sf::Color::White;
            if (avgTime < 100) avgColor = sf::Color::Green;
            else if (avgTime > 1000) avgColor = sf::Color::Red;
            drawText(avgTimeStr, panelX + 15, yOffset, 14, avgColor);
            yOffset += lineHeight;
            
            // Mostrar número de movimientos evaluados
            std::string movesStr = "Moves: " + std::to_string(aiMoveCount);
            drawText(movesStr, panelX + 15, yOffset, 12, sf::Color(180, 180, 180));
            yOffset += lineHeight;
        }
        
        // Indicador de rendimiento (basado en último tiempo)
        std::string perfStr;
        if (aiTimeMs < 50) perfStr = "Ultra Fast";
        else if (aiTimeMs < 200) perfStr = "Fast"; 
        else if (aiTimeMs < 500) perfStr = "Normal";
        else perfStr = "Thinking...";
        
        drawText(perfStr, panelX + 15, yOffset, 12, sf::Color(150, 150, 150));
        yOffset += lineHeight + 10;
    }
    
    // 7. Controles
    sf::RectangleShape separator2(sf::Vector2f(panelWidth - 20, 2));
    separator2.setPosition(panelX + 10, yOffset);
    separator2.setFillColor(sf::Color(100, 100, 100));
    window.draw(separator2);
    yOffset += 20;
    
    drawText("CONTROLS:", panelX + 10, yOffset, 16, sf::Color::Yellow);
    yOffset += lineHeight + 5;
    
    drawText("- Click cell to move", panelX + 15, yOffset, 12, sf::Color::White);
    yOffset += lineHeight - 5;
    drawText("- ESC to return to menu", panelX + 15, yOffset, 12, sf::Color::White);
    yOffset += lineHeight - 5;
    
    // 8. Hash del estado (para debug)
    if (state.getZobristHash() != 0) {
        yOffset += 10;
        drawText("DEBUG:", panelX + 10, yOffset, 14, sf::Color(100, 100, 100));
        yOffset += lineHeight;
        
        std::stringstream hashStr;
        hashStr << "Hash: 0x" << std::hex << (state.getZobristHash() & 0xFFFF);
        drawText(hashStr.str(), panelX + 15, yOffset, 10, sf::Color(80, 80, 80));
    }
}

// ===============================================
// UTILITY FUNCTIONS
// ===============================================

sf::Vector2i GuiRenderer::boardPositionToPixel(int boardX, int boardY) const {
    // Retorna el CENTRO de la celda, no la intersección
    return sf::Vector2i(
        BOARD_OFFSET_X + boardY * CELL_SIZE + CELL_SIZE/2,
        BOARD_OFFSET_Y + boardX * CELL_SIZE + CELL_SIZE/2
    );
}

std::pair<int, int> GuiRenderer::pixelToBoardPosition(int x, int y) const {
    // Convertir pixel a posición de celda (no intersección)
    int boardX = (y - BOARD_OFFSET_Y) / CELL_SIZE;
    int boardY = (x - BOARD_OFFSET_X) / CELL_SIZE;
    
    // Verificar límites
    if (boardX < 0) boardX = 0;
    if (boardX >= GameState::BOARD_SIZE) boardX = GameState::BOARD_SIZE - 1;
    if (boardY < 0) boardY = 0;
    if (boardY >= GameState::BOARD_SIZE) boardY = GameState::BOARD_SIZE - 1;
    
    return {boardX, boardY};
}

bool GuiRenderer::isPointInBoard(int x, int y) const {
    return (x >= BOARD_OFFSET_X && x <= BOARD_OFFSET_X + BOARD_SIZE_PX &&
            y >= BOARD_OFFSET_Y && y <= BOARD_OFFSET_Y + BOARD_SIZE_PX);
}

sf::Color GuiRenderer::getPieceColor(int piece) const {
    switch (piece) {
        case GameState::PLAYER1: return player1Color;
        case GameState::PLAYER2: return player2Color;
        default: return sf::Color::Transparent;
    }
}

void GuiRenderer::showGameResult(int) {
    setState(GAME_OVER);
}

// En gui_renderer.cpp - reemplazar renderGameOver completo
void GuiRenderer::renderGameOver(const GameState& state) {
    // ============================================
    // PASO 1: Renderizar el tablero normal (izquierda)
    // ============================================
    drawBoard();
    drawPieces(state);
	int PIECE_RADIUS = (CELL_SIZE / 2) - 4;
    
    // ============================================
    // PASO 2: Resaltar la línea ganadora
    // ============================================
    if (!winningLine.empty()) {
        for (const Move& move : winningLine) {
            sf::Vector2i pos = boardPositionToPixel(move.x, move.y);
            
            // Halo brillante dorado
            sf::CircleShape highlight(PIECE_RADIUS + 8);
            highlight.setPosition(pos.x - PIECE_RADIUS - 8, pos.y - PIECE_RADIUS - 8);
            highlight.setFillColor(sf::Color::Transparent);
            highlight.setOutlineThickness(5);
            highlight.setOutlineColor(sf::Color(255, 215, 0, 255)); // Dorado sólido
            window.draw(highlight);
            
            // Segundo halo más grande
            sf::CircleShape highlight2(PIECE_RADIUS + 14);
            highlight2.setPosition(pos.x - PIECE_RADIUS - 14, pos.y - PIECE_RADIUS - 14);
            highlight2.setFillColor(sf::Color::Transparent);
            highlight2.setOutlineThickness(3);
            highlight2.setOutlineColor(sf::Color(255, 255, 0, 150)); // Amarillo brillante
            window.draw(highlight2);
        }
        
        // Línea conectando las fichas
        if (winningLine.size() >= 2) {
            sf::Vector2i start = boardPositionToPixel(winningLine.front().x, winningLine.front().y);
            sf::Vector2i end = boardPositionToPixel(winningLine.back().x, winningLine.back().y);
            
            // Calcular grosor basado en ángulo
            float dx = end.x - start.x;
            float dy = end.y - start.y;
            float length = std::sqrt(dx*dx + dy*dy);
            float angle = std::atan2(dy, dx) * 180.0f / 3.14159f;
            
            // Rectángulo como línea gruesa
            sf::RectangleShape line(sf::Vector2f(length, 6));
            line.setPosition(start.x, start.y);
            line.setRotation(angle);
            line.setFillColor(sf::Color(255, 215, 0, 200));
            window.draw(line);
        }
    }
    
    // ============================================
    // PASO 3: Panel lateral derecho (donde va el info panel)
    // ============================================
    int panelX = BOARD_OFFSET_X + BOARD_SIZE_PX + 30;
    int panelY = BOARD_OFFSET_Y;
    int panelWidth = 280;
    int panelHeight = BOARD_SIZE_PX; // Mismo alto que el tablero
    
    // Overlay oscuro solo en el panel derecho
    sf::RectangleShape overlay(sf::Vector2f(panelWidth + 20, panelHeight));
    overlay.setPosition(panelX - 10, panelY);
    overlay.setFillColor(sf::Color(0, 0, 0, 200));
    window.draw(overlay);
    
    // Fondo del panel
    sf::RectangleShape panelBg(sf::Vector2f(panelWidth, panelHeight));
    panelBg.setPosition(panelX, panelY);
    panelBg.setFillColor(sf::Color(30, 30, 30, 250));
    panelBg.setOutlineThickness(4);
    panelBg.setOutlineColor(sf::Color(255, 215, 0)); // Borde dorado
    window.draw(panelBg);
    
    // ============================================
    // PASO 4: Contenido del panel
    // ============================================
    int yOffset = panelY + 20;
    int centerX = panelX + panelWidth / 2;
    
    // Título GAME OVER
    drawText("GAME OVER", centerX - 90, yOffset, 32, sf::Color(255, 100, 100));
    yOffset += 50;
    
    // ============================================
    // PASO 5: Determinar ganador y razón
    // ============================================
    bool player1Wins = false;
    std::string winReason = "";
    std::string winDetails = "";
    
    if (state.captures[0] >= 10) {
        player1Wins = true;
        winReason = "BY CAPTURES";
        winDetails = "10 pairs captured";
    } else if (state.captures[1] >= 10) {
        winReason = "BY CAPTURES";
        winDetails = "AI got 10 pairs";
    } else {
        if (state.currentPlayer == GameState::PLAYER1) {
            winReason = "BY ALIGNMENT";
            if (!winningLine.empty()) {
                char col = 'A' + winningLine[0].y;
                int row = winningLine[0].x + 1;
                winDetails = "Line at " + std::string(1, col) + std::to_string(row);
            } else {
                winDetails = "5 in a row";
            }
        } else {
            player1Wins = true;
            winReason = "BY ALIGNMENT";
            if (!winningLine.empty()) {
                char col = 'A' + winningLine[0].y;
                int row = winningLine[0].x + 1;
                winDetails = "Line at " + std::string(1, col) + std::to_string(row);
            } else {
                winDetails = "5 in a row";
            }
        }
    }
    
    // Resultado principal
    if (player1Wins) {
        drawText("YOU WIN!", centerX - 70, yOffset, 28, sf::Color(100, 255, 100));
        yOffset += 35;
        drawText("Victory!", centerX - 40, yOffset, 18, sf::Color(200, 255, 200));
    } else {
        drawText("AI WINS", centerX - 55, yOffset, 28, sf::Color(255, 80, 80));
        yOffset += 35;
        drawText("Defeat", centerX - 35, yOffset, 18, sf::Color(255, 150, 150));
    }
    
    yOffset += 35;
    
    // Razón de victoria (centrada)
    drawText(winReason, centerX - winReason.length() * 6, yOffset, 20, sf::Color::White);
    yOffset += 25;
    drawText(winDetails, centerX - winDetails.length() * 4, yOffset, 14, sf::Color(180, 180, 180));
    
    yOffset += 40;
    
    // Separador
    sf::RectangleShape separator(sf::Vector2f(panelWidth - 30, 2));
    separator.setPosition(panelX + 15, yOffset);
    separator.setFillColor(sf::Color(255, 215, 0));
    window.draw(separator);
    
    yOffset += 25;
    
    // ============================================
    // PASO 6: Estadísticas finales
    // ============================================
    drawText("FINAL STATS", centerX - 60, yOffset, 18, sf::Color(255, 215, 0));
    yOffset += 30;
    
    // Stats compactas
    drawText("Your Captures:", panelX + 15, yOffset, 14, sf::Color::White);
    drawText(std::to_string(state.captures[0]), panelX + panelWidth - 40, yOffset, 14, sf::Color(150, 255, 150));
    yOffset += 22;
    
    drawText("AI Captures:", panelX + 15, yOffset, 14, sf::Color::White);
    drawText(std::to_string(state.captures[1]), panelX + panelWidth - 40, yOffset, 14, sf::Color(255, 150, 150));
    yOffset += 22;
    
    drawText("Total Moves:", panelX + 15, yOffset, 14, sf::Color::White);
    drawText(std::to_string(state.turnCount), panelX + panelWidth - 40, yOffset, 14, sf::Color(200, 200, 200));
    
    yOffset += 35;
    
    // Otro separador
    sf::RectangleShape separator2(sf::Vector2f(panelWidth - 30, 2));
    separator2.setPosition(panelX + 15, yOffset);
    separator2.setFillColor(sf::Color(100, 100, 100));
    window.draw(separator2);
    
    yOffset += 25;
    
    // ============================================
    // PASO 7: Botones con hover effect
    // ============================================
    int buttonWidth = panelWidth - 30;
    int buttonHeight = 50;
    int buttonX = panelX + 15;
    
    // GUARDAR posición exacta de botones para usar en mouse events
    gameOverButtonsY = yOffset;
    gameOverButtonsPositionValid = true;
    
    // Botón "New Game" con hover
    bool newGameHover = (hoveredMenuOption == 0);
    drawButton("NEW GAME", buttonX, gameOverButtonsY, buttonWidth, buttonHeight, newGameHover);
    
    // Botón "Main Menu" con hover
    bool menuHover = (hoveredMenuOption == 1);
    drawButton("MAIN MENU", buttonX, gameOverButtonsY + buttonHeight + 15, buttonWidth, buttonHeight, menuHover);
    
    // Instrucción (usar posición calculada)
    yOffset = gameOverButtonsY + (buttonHeight + 15) * 2 + 20;
    
    yOffset += buttonHeight + 20;
    
    // Instrucción
    drawText("Press ESC for menu", centerX - 80, yOffset, 12, sf::Color(120, 120, 120));
}

void GuiRenderer::handleMouseMove(int x, int y) {
    if (currentState == MENU) {
        // CORREGIDO: Usar hoveredMenuOption para efectos visuales
        int buttonWidth = 250;
        int buttonHeight = 60;
        int buttonX = WINDOW_WIDTH/2 - buttonWidth/2;
        
        // Reset hover state
        int previousHover = hoveredMenuOption;
        hoveredMenuOption = -1;
        
        // Check each button
        if (x >= buttonX && x <= buttonX + buttonWidth) {
            if (y >= 250 && y <= 250 + buttonHeight) {
                hoveredMenuOption = 0; // VS AI hover
            } else if (y >= 330 && y <= 330 + buttonHeight) {
                hoveredMenuOption = 1; // VS Human hover
            } else if (y >= 410 && y <= 410 + buttonHeight) {
                hoveredMenuOption = 2; // Quit hover
            }
        }
        
        // Solo logear si cambió el hover (evitar spam)
        if (previousHover != hoveredMenuOption && hoveredMenuOption != -1) {
            std::string options[] = {"VS AI", "VS Human", "Quit"};
            std::cout << "Hover: " << options[hoveredMenuOption] << std::endl;
        }
    }
    
    if (currentState == PLAYING) {
        // NUEVO: Actualizar hover position en el tablero
        if (isPointInBoard(x, y)) {
            auto [boardX, boardY] = pixelToBoardPosition(x, y);
            hoverPosition = Move(boardX, boardY);
        } else {
            hoverPosition = Move(-1, -1); // Fuera del tablero
        }
    }

	if (currentState == GAME_OVER && gameOverButtonsPositionValid) {
        int panelX = BOARD_OFFSET_X + BOARD_SIZE_PX + 30;
        int panelWidth = 280;
        
        int buttonWidth = panelWidth - 30;
        int buttonHeight = 50;
        int buttonX = panelX + 15;
        
        // Usar posición exacta guardada desde renderGameOver
        int button1Y = gameOverButtonsY;
        int button2Y = gameOverButtonsY + buttonHeight + 15;
        
        // Reset hover
        int previousHover = hoveredMenuOption;
        hoveredMenuOption = -1;
        
        // Check botón NEW GAME
        if (x >= buttonX && x <= buttonX + buttonWidth && 
            y >= button1Y && y <= button1Y + buttonHeight) {
            hoveredMenuOption = 0;
        }
        // Check botón MAIN MENU
        else if (x >= buttonX && x <= buttonX + buttonWidth && 
                 y >= button2Y && y <= button2Y + buttonHeight) {
            hoveredMenuOption = 1;
        }
        
        // Debug hover (solo si cambió)
        if (previousHover != hoveredMenuOption && hoveredMenuOption != -1) {
            std::cout << "Game Over Hover: " << (hoveredMenuOption == 0 ? "New Game" : "Main Menu") << std::endl;
        }
    }
}

// En gui_renderer.cpp - Reemplazar handleGameOverClick
void GuiRenderer::handleGameOverClick(int x, int y) {
    if (!gameOverButtonsPositionValid) return; // No hay posiciones válidas aún
    
    int panelX = BOARD_OFFSET_X + BOARD_SIZE_PX + 30;
    int panelWidth = 280;
    
    int buttonWidth = panelWidth - 30;
    int buttonHeight = 50;
    int buttonX = panelX + 15;
    
    // Usar posición exacta guardada desde renderGameOver
    int button1Y = gameOverButtonsY;
    int button2Y = gameOverButtonsY + buttonHeight + 15;
    
    // Click en "NEW GAME"
    if (x >= buttonX && x <= buttonX + buttonWidth && 
        y >= button1Y && y <= button1Y + buttonHeight) {
        selectedMenuOption = 0;
        std::cout << "✓ Seleccionado: Nuevo Juego" << std::endl;
        return;
    }
    
    // Click en "MAIN MENU"
    if (x >= buttonX && x <= buttonX + buttonWidth && 
        y >= button2Y && y <= button2Y + buttonHeight) {
        setState(MENU);
        selectedMenuOption = -1;
        std::cout << "✓ Volviendo al menú" << std::endl;
        return;
    }
    
    std::cout << "✗ Click fuera de los botones" << std::endl;
}

char GuiRenderer::getPieceSymbol(int piece) const {
    switch (piece) {
        case GameState::PLAYER1: return 'O';
        case GameState::PLAYER2: return 'X';
        default: return '.';
    }
}

// ===============================================
// AI STATS METHODS - NUEVO
// ===============================================

void GuiRenderer::addAiTime(int timeMs) {
    if (timeMs > 0) {
        aiTimes.push_back(timeMs);
        totalAiTime += timeMs;
        aiMoveCount++;
    }
}

float GuiRenderer::getAverageAiTime() const {
    if (aiMoveCount == 0) return 0.0f;
    return static_cast<float>(totalAiTime) / static_cast<float>(aiMoveCount);
}

void GuiRenderer::resetAiStats() {
    aiTimes.clear();
    totalAiTime = 0;
    aiMoveCount = 0;
}

void GuiRenderer::drawSuggestionIndicator() {
    // Solo mostrar si hay una sugerencia válida
    if (!showSuggestion || !currentSuggestion.isValid()) return;
    
    sf::Vector2i pos = boardPositionToPixel(currentSuggestion.x, currentSuggestion.y);
    int cellSize = CELL_SIZE - 4;
    
    // 1. Fondo de la celda con color distintivo (amarillo/dorado)
    sf::RectangleShape suggestionBg(sf::Vector2f(cellSize, cellSize));
    suggestionBg.setPosition(pos.x - cellSize/2 + 2, pos.y - cellSize/2 + 2);
    suggestionBg.setFillColor(sf::Color(255, 215, 0, 80)); // Dorado semi-transparente
    window.draw(suggestionBg);
    
    // 2. Borde pulsante más grueso
    sf::RectangleShape suggestionBorder(sf::Vector2f(cellSize, cellSize));
    suggestionBorder.setPosition(pos.x - cellSize/2 + 2, pos.y - cellSize/2 + 2);
    suggestionBorder.setFillColor(sf::Color::Transparent);
    suggestionBorder.setOutlineThickness(3);
    
    // Efecto pulsante usando el tiempo
    static sf::Clock pulseClock;
    float pulseTime = pulseClock.getElapsedTime().asSeconds();
    float alpha = (sin(pulseTime * 4.0f) + 1.0f) * 0.4f + 0.2f; // Oscila entre 0.2 y 1.0
    suggestionBorder.setOutlineColor(sf::Color(255, 215, 0, (sf::Uint8)(alpha * 255))); // Dorado pulsante
    window.draw(suggestionBorder);
    
    // 3. Icono de bombilla en el centro
    sf::CircleShape bulbOuter(8);
    bulbOuter.setPosition(pos.x - 8, pos.y - 8);
    bulbOuter.setFillColor(sf::Color(255, 255, 0, 200)); // Amarillo brillante
    window.draw(bulbOuter);
    
    sf::CircleShape bulbInner(5);
    bulbInner.setPosition(pos.x - 5, pos.y - 5);
    bulbInner.setFillColor(sf::Color(255, 255, 255, 250)); // Blanco brillante (centro)
    window.draw(bulbInner);
    
    // 4. Texto "?" en el centro
    drawText("?", pos.x - 5, pos.y - 8, 14, sf::Color(50, 50, 50));
}

// NUEVO: Método para mostrar error de movimiento inválido
void GuiRenderer::showInvalidMoveError(const Move& move) {
    errorMessage = "Invalid move!";
    invalidMovePosition = move;
    showError = true;
    errorTimer.restart();
}

// NUEVO: Método para limpiar error de movimiento inválido
void GuiRenderer::clearInvalidMoveError() {
    showError = false;
    errorMessage = "";
    invalidMovePosition = Move(-1, -1);
}

// NUEVO: Método para dibujar indicador de movimiento inválido en el tablero
void GuiRenderer::drawInvalidMoveIndicator() {
    // Solo mostrar si hay un error activo y han pasado menos de 2 segundos
    if (!showError || errorTimer.getElapsedTime().asSeconds() > 2.0f) {
        if (showError) clearInvalidMoveError(); // Auto-limpiar después de 2 segundos
        return;
    }
    
    if (!invalidMovePosition.isValid()) return;
    
    sf::Vector2i pos = boardPositionToPixel(invalidMovePosition.x, invalidMovePosition.y);
    int cellSize = CELL_SIZE - 4;
    
    // 1. Fondo rojo pulsante
    sf::RectangleShape errorBg(sf::Vector2f(cellSize, cellSize));
    errorBg.setPosition(pos.x - cellSize/2 + 2, pos.y - cellSize/2 + 2);
    
    // Efecto pulsante rojo usando el tiempo
    float pulseTime = errorTimer.getElapsedTime().asSeconds();
    float alpha = (sin(pulseTime * 8.0f) + 1.0f) * 0.3f + 0.1f; // Oscila más rápido
    errorBg.setFillColor(sf::Color(255, 0, 0, (sf::Uint8)(alpha * 255))); // Rojo pulsante
    window.draw(errorBg);
    
    // 2. Borde rojo grueso
    sf::RectangleShape errorBorder(sf::Vector2f(cellSize, cellSize));
    errorBorder.setPosition(pos.x - cellSize/2 + 2, pos.y - cellSize/2 + 2);
    errorBorder.setFillColor(sf::Color::Transparent);
    errorBorder.setOutlineThickness(4);
    errorBorder.setOutlineColor(sf::Color(255, 0, 0, 200)); // Rojo sólido
    window.draw(errorBorder);
}
