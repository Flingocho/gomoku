/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   gui_renderer.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/01 00:00:00 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/23 17:17:26 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/gui_renderer.hpp"
#include <iostream>
#include <sstream>  // NUEVO: Para stringstream en drawGameInfo

GuiRenderer::GuiRenderer() 
    : window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Gomoku AI"),
      currentState(MENU),
      selectedMenuOption(-1),
      hoveredMenuOption(-1),
      moveReady(false),
      hoverPosition(-1, -1),    // NUEVO: Inicializar hover position
      backgroundColor(sf::Color(40, 44, 52)),     // Dark blue-gray
      boardLineColor(sf::Color(139, 69, 19)),     // Saddle brown
      player1Color(sf::Color::Blue),              // Human - Blue
      player2Color(sf::Color::Red),               // AI - Red  
      hoverColor(sf::Color(255, 255, 255, 100))   // Transparent white
{
    // Cargar fuente personalizada
    if (!font.loadFromFile("fonts/street_drips.ttf")) {
        // Si no encuentra archivo, continuar sin fuente personalizada
        std::cout << "Warning: No se pudo cargar fuente fonts/street_drips.ttf, usando por defecto" << std::endl;
        // SFML puede funcionar sin fuente explícita
    } else {
        std::cout << "✓ Fuente personalizada cargada: fonts/street_drips.ttf" << std::endl;
    }
    
    window.setFramerateLimit(60);
    std::cout << "GUI Renderer inicializado: " << WINDOW_WIDTH << "x" << WINDOW_HEIGHT << std::endl;
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
    drawText("5 en línea con IA avanzada", WINDOW_WIDTH/2 - 100, 150, 18, sf::Color(200, 200, 200));
    
    // 2. Botones del menú (centrados)
    int buttonWidth = 250;
    int buttonHeight = 60;
    int buttonX = WINDOW_WIDTH/2 - buttonWidth/2;
    int buttonSpacing = 80;
    
    // CORREGIDO: Usar hoveredMenuOption para efectos visuales
    bool vsAiHover = (hoveredMenuOption == 0);
    drawButton("🤖 Jugar vs IA", buttonX, 250, buttonWidth, buttonHeight, vsAiHover);
    
    bool vsHumanHover = (hoveredMenuOption == 1);
    drawButton("👥 Jugar vs Humano", buttonX, 250 + buttonSpacing, buttonWidth, buttonHeight, vsHumanHover);
    
    bool quitHover = (hoveredMenuOption == 2);
    drawButton("❌ Salir", buttonX, 250 + buttonSpacing * 2, buttonWidth, buttonHeight, quitHover);
    
    // 3. Información adicional
    drawText("Características:", 50, 500, 20, sf::Color::Yellow);
    drawText("• Zobrist Hashing para máximo rendimiento", 70, 530, 16, sf::Color::White);
    drawText("• Búsqueda Minimax con poda Alpha-Beta", 70, 550, 16, sf::Color::White);
    drawText("• Profundidad adaptativa hasta 10 niveles", 70, 570, 16, sf::Color::White);
    drawText("• Reglas completas: capturas + double-three", 70, 590, 16, sf::Color::White);
    
    // 4. Controles
    drawText("Usa el ratón para seleccionar", WINDOW_WIDTH/2 - 100, 650, 16, sf::Color(150, 150, 150));
    drawText("ESC = Salir", WINDOW_WIDTH - 100, WINDOW_HEIGHT - 30, 14, sf::Color(100, 100, 100));
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
                    // Azul más claro para highlight
                    pieceHighlight.setFillColor(sf::Color(100, 149, 237)); // Cornflower blue
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
    drawText("=== ESTADO ===", panelX + 10, panelY + 10, 18, sf::Color::Yellow);
    
    int yOffset = panelY + 50;
    int lineHeight = 25;
    
    // 3. Información básica del juego
    std::string turnInfo = "Turno: " + std::to_string(state.turnCount);
    drawText(turnInfo, panelX + 10, yOffset, 16, sf::Color::White);
    yOffset += lineHeight;
    
    // Jugador actual
    std::string currentPlayerStr = (state.currentPlayer == GameState::PLAYER1) ? "Humano (O)" : "IA (X)";
    sf::Color playerColor = (state.currentPlayer == GameState::PLAYER1) ? player1Color : player2Color;
    drawText("Jugador:", panelX + 10, yOffset, 16, sf::Color::White);
    drawText(currentPlayerStr, panelX + 10, yOffset + lineHeight, 16, playerColor);
    yOffset += lineHeight * 2 + 10;
    
    // 4. Separador
    sf::RectangleShape separator(sf::Vector2f(panelWidth - 20, 2));
    separator.setPosition(panelX + 10, yOffset);
    separator.setFillColor(sf::Color(100, 100, 100));
    window.draw(separator);
    yOffset += 20;
    
    // 5. Capturas
    drawText("CAPTURAS:", panelX + 10, yOffset, 16, sf::Color::Yellow);
    yOffset += lineHeight + 5;
    
    // Capturas del humano
    std::string humanCaptures = "Tu: " + std::to_string(state.captures[0]) + "/10";
    drawText(humanCaptures, panelX + 15, yOffset, 14, player1Color);
    if (state.captures[0] >= 8) {
        drawText("¡Cerca!", panelX + 120, yOffset, 14, sf::Color::Red);
    }
    yOffset += lineHeight;
    
    // Capturas de la IA
    std::string aiCaptures = "IA: " + std::to_string(state.captures[1]) + "/10";
    drawText(aiCaptures, panelX + 15, yOffset, 14, player2Color);
    if (state.captures[1] >= 8) {
        drawText("¡Peligro!", panelX + 120, yOffset, 14, sf::Color::Red);
    }
    yOffset += lineHeight + 15;
    
    // 6. Estadísticas de la IA (si disponibles)
    if (aiTimeMs > 0) {
        drawText("IA STATS:", panelX + 10, yOffset, 16, sf::Color::Yellow);
        yOffset += lineHeight + 5;
        
        // Tiempo de pensamiento
        std::string timeStr = "Tiempo: " + std::to_string(aiTimeMs) + "ms";
        sf::Color timeColor = sf::Color::White;
        if (aiTimeMs < 100) timeColor = sf::Color::Green;
        else if (aiTimeMs > 1000) timeColor = sf::Color::Red;
        drawText(timeStr, panelX + 15, yOffset, 14, timeColor);
        yOffset += lineHeight;
        
        // Indicador de rendimiento
        std::string perfStr;
        if (aiTimeMs < 50) perfStr = "Ultrarrápida";
        else if (aiTimeMs < 200) perfStr = "Rápida"; 
        else if (aiTimeMs < 500) perfStr = "Normal";
        else perfStr = "Pensando...";
        
        drawText(perfStr, panelX + 15, yOffset, 12, sf::Color(150, 150, 150));
        yOffset += lineHeight + 10;
    }
    
    // 7. Controles
    sf::RectangleShape separator2(sf::Vector2f(panelWidth - 20, 2));
    separator2.setPosition(panelX + 10, yOffset);
    separator2.setFillColor(sf::Color(100, 100, 100));
    window.draw(separator2);
    yOffset += 20;
    
    drawText("CONTROLES:", panelX + 10, yOffset, 16, sf::Color::Yellow);
    yOffset += lineHeight + 5;
    
    drawText("• Click en celda para mover", panelX + 15, yOffset, 12, sf::Color::White);
    yOffset += lineHeight - 5;
    drawText("• ESC para volver al menú", panelX + 15, yOffset, 12, sf::Color::White);
    yOffset += lineHeight - 5;
    
    // 8. Hash del estado (para debug)
    if (state.getZobristHash() != 0) {
        yOffset += 10;
        drawText("DEBUG:", panelX + 10, yOffset, 14, sf::Color(100, 100, 100));
        yOffset += lineHeight;
        
        std::stringstream hashStr;
        hashStr << "Hash: 0x" << std::hex << (state.getZobristHash() & 0xFFFF); // Solo últimos 4 dígitos
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

void GuiRenderer::renderGameOver(const GameState& state) {
    // 1. Overlay semi-transparente sobre todo
    sf::RectangleShape overlay(sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT));
    overlay.setPosition(0, 0);
    overlay.setFillColor(sf::Color(0, 0, 0, 180)); // Negro semi-transparente
    window.draw(overlay);
    
    // 2. Panel central del game over
    int panelWidth = 400;
    int panelHeight = 350;  // Más alto para mostrar más info
    int panelX = WINDOW_WIDTH/2 - panelWidth/2;
    int panelY = WINDOW_HEIGHT/2 - panelHeight/2;
    
    // Sombra del panel
    sf::RectangleShape panelShadow(sf::Vector2f(panelWidth + 8, panelHeight + 8));
    panelShadow.setPosition(panelX + 4, panelY + 4);
    panelShadow.setFillColor(sf::Color(0, 0, 0, 120));
    window.draw(panelShadow);
    
    // Panel principal
    sf::RectangleShape panel(sf::Vector2f(panelWidth, panelHeight));
    panel.setPosition(panelX, panelY);
    panel.setFillColor(sf::Color(60, 60, 60));
    panel.setOutlineThickness(3);
    panel.setOutlineColor(sf::Color::White);
    window.draw(panel);
    
    // 3. Título "GAME OVER"
    drawText("GAME OVER", panelX + panelWidth/2 - 80, panelY + 20, 32, sf::Color::Red);
    
    // 4. NUEVO: Determinar y mostrar el ganador real
    bool player1Wins = false, player2Wins = false;
    std::string winReason = "";
    
    // Verificar capturas
    if (state.captures[0] >= 10) {
        player1Wins = true;
        winReason = "por capturas";
    } else if (state.captures[1] >= 10) {
        player2Wins = true;
        winReason = "por capturas";
    }
    // Si no hay victoria por capturas, debe ser por alineación
    // (el game engine ya verificó que hay victoria)
    else {
        // Determinar ganador por el turno actual (el otro jugador ganó)
        if (state.currentPlayer == GameState::PLAYER1) {
            player2Wins = true; // La IA ganó en su último turno
        } else {
            player1Wins = true; // El humano ganó en su último turno
        }
        winReason = "por 5 en línea";
    }
    
    // 5. Mostrar resultado
    if (player1Wins) {
        drawText("¡GANASTE!", panelX + panelWidth/2 - 70, panelY + 70, 28, sf::Color::Green);
        drawText("🎉 ¡Felicidades! 🎉", panelX + panelWidth/2 - 80, panelY + 105, 18, sf::Color::Yellow);
    } else if (player2Wins) {
        drawText("IA GANA", panelX + panelWidth/2 - 50, panelY + 70, 28, sf::Color::Red);
        drawText("🤖 La máquina triunfa", panelX + panelWidth/2 - 85, panelY + 105, 18, sf::Color(200, 100, 100));
    }
    
    // 6. Mostrar razón de victoria
    std::string reasonText = "Victoria " + winReason;
    drawText(reasonText, panelX + panelWidth/2 - 60, panelY + 135, 16, sf::Color::White);
    
    // 7. Estadísticas finales
    drawText("Estadísticas finales:", panelX + panelWidth/2 - 80, panelY + 165, 16, sf::Color::Yellow);
    
    std::string turnosText = "Turnos jugados: " + std::to_string(state.turnCount);
    drawText(turnosText, panelX + 20, panelY + 190, 14, sf::Color::White);
    
    std::string capturesText = "Capturas - Tú: " + std::to_string(state.captures[0]) + 
                              ", IA: " + std::to_string(state.captures[1]);
    drawText(capturesText, panelX + 20, panelY + 210, 14, sf::Color::White);
    
    // 8. Botones de acción
    int buttonWidth = 120;
    int buttonHeight = 40;
    int button1X = panelX + 50;
    int button2X = panelX + panelWidth - 170;
    int buttonY = panelY + panelHeight - 80;
    
    // Botón "Nuevo Juego"
    drawButton("Nuevo Juego", button1X, buttonY, buttonWidth, buttonHeight, false);
    
    // Botón "Menú"
    drawButton("Menú", button2X, buttonY, buttonWidth, buttonHeight, false);
    
    // 9. Instrucciones
    drawText("Click en un botón o presiona ESC", panelX + panelWidth/2 - 120, 
             panelY + panelHeight + 20, 14, sf::Color(150, 150, 150));
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
}

void GuiRenderer::handleGameOverClick(int x, int y) {
    int panelWidth = 400;
    int panelHeight = 300;
    int panelX = WINDOW_WIDTH/2 - panelWidth/2;
    int panelY = WINDOW_HEIGHT/2 - panelHeight/2;
    
    int buttonWidth = 120;
    int buttonHeight = 40;
    int button1X = panelX + 50;         // Botón "Nuevo Juego"
    int button2X = panelX + panelWidth - 170;  // Botón "Menú"
    int buttonY = panelY + panelHeight - 80;
    
    // Click en "Nuevo Juego"
    if (x >= button1X && x <= button1X + buttonWidth && 
        y >= buttonY && y <= buttonY + buttonHeight) {
        selectedMenuOption = 0; // Señal para reiniciar juego
        std::cout << "Seleccionado: Nuevo Juego" << std::endl;
        return;
    }
    
    // Click en "Menú"
    if (x >= button2X && x <= button2X + buttonWidth && 
        y >= buttonY && y <= buttonY + buttonHeight) {
        setState(MENU);
        selectedMenuOption = -1; // Reset
        std::cout << "Volviendo al menú" << std::endl;
        return;
    }
    
    // Click fuera de botones - no hacer nada
}

char GuiRenderer::getPieceSymbol(int piece) const {
    switch (piece) {
        case GameState::PLAYER1: return 'O';
        case GameState::PLAYER2: return 'X';
        default: return '.';
    }
}
