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
#include <cstdlib>  // NUEVO: Para rand() en efectos visuales

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
	  gameOverButtonsPositionValid(false),
	  isColorblindMode(false)        // NUEVO: Inicializar modo colorblind
{
    // Inicializar partículas para efectos visuales
    particles.resize(50);
    particleLife.resize(50);
    for (int i = 0; i < 50; i++) {
        particles[i] = sf::Vector2f(rand() % WINDOW_WIDTH, rand() % WINDOW_HEIGHT);
        particleLife[i] = static_cast<float>(rand() % 100) / 100.0f;
    }
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
                        resetColorblindMode(); // Resetear modo colorblind al volver al menú
                        selectedMenuOption = -1; // Resetear selección de menú
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
    if (selectedMenuOption == 2) return COLORBLIND;
    if (selectedMenuOption == 3) return RUST_AI;
    if (selectedMenuOption == 4) return CAPTURE_MODE;  // NUEVO
    if (selectedMenuOption == 5) return QUIT;  // ACTUALIZADO índice
    
    return NONE; // Sin selección aún
}

void GuiRenderer::renderMenu() {
    // 0. Fondo moderno con efectos
    drawModernBackground();
    
    // 1. Título principal con efectos de brillo
    std::string mainTitle = "=== GOMOKU AI ===";
    int titleSize = 36;
    
    sf::Text titleText;
    if (font.getInfo().family != "") {
        titleText.setFont(font);
    }
    titleText.setString(mainTitle);
    titleText.setCharacterSize(titleSize);
    titleText.setFillColor(sf::Color::White);
    
    // Centrar usando setOrigin - más preciso
    sf::FloatRect titleBounds = titleText.getLocalBounds();
    titleText.setOrigin(titleBounds.width / 2.0f, 0);
    titleText.setPosition(WINDOW_WIDTH / 2.0f, 100);
    
    // Efecto de brillo pulsante
    float time = animationClock.getElapsedTime().asSeconds();
    float pulse = sin(time * 2.0f) * 0.3f + 1.0f;
    sf::Color glowColor = sf::Color(255, 215, 0, static_cast<sf::Uint8>(pulse * 100)); // Dorado
    
    drawGlowEffect(titleText, glowColor);
    window.draw(titleText);
    
    // 2. Subtítulo centrado
    std::string subtitle = "5 in a row with advanced AI";
    int subtitleSize = 18;
    
    sf::Text subtitleText;
    if (font.getInfo().family != "") {
        subtitleText.setFont(font);
    }
    subtitleText.setString(subtitle);
    subtitleText.setCharacterSize(subtitleSize);
    subtitleText.setFillColor(sf::Color(200, 200, 200));
    
    sf::FloatRect subtitleBounds = subtitleText.getLocalBounds();
    subtitleText.setOrigin(subtitleBounds.width / 2.0f, 0);
    subtitleText.setPosition(WINDOW_WIDTH / 2.0f, 150);
    window.draw(subtitleText);
    
    // 2. Botones del menú (centrados con precisión flotante)
    int buttonWidth = 250;
    int buttonHeight = 50;  // REDUCIDO de 60 a 50 para acomodar más botones
    int buttonX = WINDOW_WIDTH/2.0f - buttonWidth/2.0f;  // Usar flotantes para más precisión
    int buttonSpacing = 65;  // REDUCIDO de 80 a 65 para mejor distribución
    
    // CORREGIDO: Usar hoveredMenuOption para efectos visuales
    bool vsAiHover = (hoveredMenuOption == 0);
    drawButton("Play vs AI", buttonX, 230, buttonWidth, buttonHeight, vsAiHover);
    
    bool vsHumanHover = (hoveredMenuOption == 1);
    drawButton("Play vs Human", buttonX, 230 + buttonSpacing, buttonWidth, buttonHeight, vsHumanHover);
    
    bool colorblindHover = (hoveredMenuOption == 2);
    drawButton("Colorblind Mode", buttonX, 230 + buttonSpacing * 2, buttonWidth, buttonHeight, colorblindHover);
    
    bool rustAiHover = (hoveredMenuOption == 3);
    drawButton("Rust AI", buttonX, 230 + buttonSpacing * 3, buttonWidth, buttonHeight, rustAiHover);
    
    bool captureModeHover = (hoveredMenuOption == 4);
    drawButton("Capture Mode", buttonX, 230 + buttonSpacing * 4, buttonWidth, buttonHeight, captureModeHover);
    
    bool quitHover = (hoveredMenuOption == 5);
    drawButton("Exit", buttonX, 230 + buttonSpacing * 5, buttonWidth, buttonHeight, quitHover);
    
    // 3. Información adicional (compacta)
    drawText("Features:", 50, 680, 16, sf::Color::Yellow);
    drawText("- Zobrist Hashing + Alpha-Beta pruning", 70, 705, 14, sf::Color::White);
    drawText("- Adaptive depth + Complete rules", 70, 725, 14, sf::Color::White);
    
    // 4. Controles
    drawText("ESC = Exit", WINDOW_WIDTH - 100, WINDOW_HEIGHT - 30, 14, sf::Color(100, 100, 100));
}

void GuiRenderer::handleMenuClick(int x, int y) {
    int buttonWidth = 250;
    int buttonHeight = 50;  // ACTUALIZADO para coincidir con renderMenu
    int buttonX = WINDOW_WIDTH/2 - buttonWidth/2;
    int buttonSpacing = 65;  // ACTUALIZADO para coincidir con renderMenu
    
    // Botón VS AI (posición Y: 230)
    if (x >= buttonX && x <= buttonX + buttonWidth && 
        y >= 230 && y <= 230 + buttonHeight) {
        selectedMenuOption = 0;
        isColorblindMode = false;  // Resetear modo colorblind
        std::cout << "Seleccionado: VS AI" << std::endl;
        return;
    }
    
    // Botón VS Human (posición Y: 230 + 65)
    if (x >= buttonX && x <= buttonX + buttonWidth && 
        y >= 230 + buttonSpacing && y <= 230 + buttonSpacing + buttonHeight) {
        selectedMenuOption = 1;
        isColorblindMode = false;  // Resetear modo colorblind
        std::cout << "Seleccionado: VS Human" << std::endl;
        return;
    }
    
    // Botón Colorblind Mode (posición Y: 230 + 130)
    if (x >= buttonX && x <= buttonX + buttonWidth && 
        y >= 230 + buttonSpacing * 2 && y <= 230 + buttonSpacing * 2 + buttonHeight) {
        selectedMenuOption = 2;
        isColorblindMode = true;  // Activar modo colorblind
        std::cout << "Seleccionado: Colorblind Mode (VS AI)" << std::endl;
        return;
    }
    
    // Botón Rust AI (posición Y: 230 + 195)
    if (x >= buttonX && x <= buttonX + buttonWidth && 
        y >= 230 + buttonSpacing * 3 && y <= 230 + buttonSpacing * 3 + buttonHeight) {
        selectedMenuOption = 3;
        std::cout << "Seleccionado: Rust AI" << std::endl;
        return;
    }
    
    // Botón Capture Mode (posición Y: 230 + 260) - NUEVO
    if (x >= buttonX && x <= buttonX + buttonWidth && 
        y >= 230 + buttonSpacing * 4 && y <= 230 + buttonSpacing * 4 + buttonHeight) {
        selectedMenuOption = 4;
        isColorblindMode = false;
        std::cout << "Seleccionado: Capture Mode" << std::endl;
        return;
    }
    
    // Botón Quit (posición Y: 230 + 325)
    if (x >= buttonX && x <= buttonX + buttonWidth && 
        y >= 230 + buttonSpacing * 5 && y <= 230 + buttonSpacing * 5 + buttonHeight) {
        selectedMenuOption = 5;  // ACTUALIZADO índice
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
    // Fondo moderno igual que el menú
    drawModernBackground();
    
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
    
    sf::Vector2i pos = boardPositionToPixel(hoverPosition.x, hoverPosition.y);
    int cellSize = CELL_SIZE - 4;
    float time = animationClock.getElapsedTime().asSeconds();
    
    // 1. Efecto de ondas expandiéndose
    for (int i = 1; i <= 3; i++) {
        float wavePhase = fmod(time * 3.0f + i * 0.5f, 2.0f);
        float waveSize = cellSize/2 + wavePhase * 15;
        float waveAlpha = (2.0f - wavePhase) * 40;
        
        sf::CircleShape wave(waveSize);
        wave.setPosition(pos.x - waveSize, pos.y - waveSize);
        wave.setFillColor(sf::Color::Transparent);
        wave.setOutlineThickness(2);
        wave.setOutlineColor(sf::Color(0, 255, 255, static_cast<sf::Uint8>(waveAlpha))); // Cian brillante
        window.draw(wave);
    }
    
    // 2. Fondo pulsante
    float pulse = sin(time * 4.0f) * 0.3f + 0.7f;
    sf::RectangleShape hoverBg(sf::Vector2f(cellSize, cellSize));
    hoverBg.setPosition(pos.x - cellSize/2 + 2, pos.y - cellSize/2 + 2);
    hoverBg.setFillColor(sf::Color(100, 200, 255, static_cast<sf::Uint8>(pulse * 80)));
    window.draw(hoverBg);
    
    // 3. Borde con brillo giratorio
    sf::RectangleShape hoverBorder(sf::Vector2f(cellSize, cellSize));
    hoverBorder.setPosition(pos.x - cellSize/2 + 2, pos.y - cellSize/2 + 2);
    hoverBorder.setFillColor(sf::Color::Transparent);
    hoverBorder.setOutlineThickness(3);
    hoverBorder.setOutlineColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(pulse * 200)));
    window.draw(hoverBorder);
    
    // 4. Partículas giratorias alrededor del centro
    for (int i = 0; i < 6; i++) {
        float angle = time * 2.0f + i * 1.047f; // 60 grados entre partículas
        float radius = 15;
        float sparkX = pos.x + cos(angle) * radius;
        float sparkY = pos.y + sin(angle) * radius;
        
        sf::CircleShape spark(3);
        spark.setPosition(sparkX - 3, sparkY - 3);
        spark.setFillColor(sf::Color(255, 255, 100, static_cast<sf::Uint8>(pulse * 255)));
        window.draw(spark);
    }
    
    // 5. Círculo central brillante
    sf::CircleShape hoverIndicator(8 + pulse * 3);
    hoverIndicator.setPosition(pos.x - (8 + pulse * 3), pos.y - (8 + pulse * 3));
    hoverIndicator.setFillColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(pulse * 150)));
    hoverIndicator.setOutlineThickness(2);
    hoverIndicator.setOutlineColor(sf::Color(0, 255, 255, 255));
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
                
                // En modo colorblind, todas las piezas tienen el mismo color, EXCEPTO cuando el juego terminó
                if (isColorblindMode && currentState != GAME_OVER) {
                    pieceMain.setFillColor(sf::Color(128, 128, 128)); // Gris neutral
                } else {
                    pieceMain.setFillColor(getPieceColor(piece)); // Colores reales (normal o revelados al final)
                }
                
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
                if (isColorblindMode && currentState != GAME_OVER) {
                    // En modo colorblind, highlight ligeramente más claro, EXCEPTO al final del juego
                    pieceHighlight.setFillColor(sf::Color(160, 160, 160)); // Gris más claro
                } else if (piece == GameState::PLAYER1) {
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
    float time = animationClock.getElapsedTime().asSeconds();
    
    // 1. Sombra suave y moderna
    sf::RectangleShape buttonShadow(sf::Vector2f(width + 8, height + 8));
    buttonShadow.setPosition(x + 4, y + 4);
    buttonShadow.setFillColor(sf::Color(0, 0, 0, 60));
    window.draw(buttonShadow);
    
    // 2. Fondo del botón con gradiente
    sf::RectangleShape buttonBg(sf::Vector2f(width, height));
    buttonBg.setPosition(x, y);
    
    if (highlighted) {
        // Efecto hover animado
        float pulse = sin(time * 4.0f) * 0.2f + 0.8f;
        sf::Uint8 brightness = static_cast<sf::Uint8>(pulse * 255);
        buttonBg.setFillColor(sf::Color(30, 144, 255, brightness)); // Azul brillante
        
        // Borde con brillo
        buttonBg.setOutlineThickness(3);
        sf::Color glowBorder = sf::Color(255, 255, 255, static_cast<sf::Uint8>(pulse * 200));
        buttonBg.setOutlineColor(glowBorder);
        
        // Efecto de partículas en hover
        for (int i = 0; i < 8; i++) {
            float angle = (time * 2.0f + i * 0.785f);
            float sparkX = x + width/2 + cos(angle) * (width/2 + 10);
            float sparkY = y + height/2 + sin(angle) * (height/2 + 10);
            
            sf::CircleShape spark(1.5f);
            spark.setPosition(sparkX, sparkY);
            spark.setFillColor(sf::Color(255, 255, 255, 150));
            window.draw(spark);
        }
    } else {
        // Estilo moderno normal con gradiente sutil
        buttonBg.setFillColor(sf::Color(45, 45, 65, 220)); // Azul oscuro translúcido
        buttonBg.setOutlineThickness(2);
        buttonBg.setOutlineColor(sf::Color(100, 149, 237, 150)); // Cornflower blue
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
    
    // 4. Texto del botón (centrado perfectamente)
    sf::Color textColor = highlighted ? sf::Color::Yellow : sf::Color::White;
    int textSize = 20;
    
    // Centrado perfecto usando setOrigin
    sf::Text buttonText;
    if (font.getInfo().family != "") {
        buttonText.setFont(font);
    }
    buttonText.setString(text);
    buttonText.setCharacterSize(textSize);
    buttonText.setFillColor(textColor);
    
    sf::FloatRect textBounds = buttonText.getLocalBounds();
    buttonText.setOrigin(textBounds.width / 2.0f, textBounds.height / 2.0f);
    buttonText.setPosition(x + width / 2.0f, y + height / 2.0f);
    window.draw(buttonText);
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
    
    // 1. Fondo del panel con efectos modernos
    float time = animationClock.getElapsedTime().asSeconds();
    
    // Sombra suave y amplia
    sf::RectangleShape panelShadow(sf::Vector2f(panelWidth + 12, 400 + 12));
    panelShadow.setPosition(panelX + 6, panelY + 6);
    panelShadow.setFillColor(sf::Color(0, 0, 0, 80));
    window.draw(panelShadow);
    
    // Fondo translúcido con efecto de cristal
    sf::RectangleShape panelBg(sf::Vector2f(panelWidth, 400));
    panelBg.setPosition(panelX, panelY);
    panelBg.setFillColor(sf::Color(20, 25, 40, 200)); // Azul oscuro translúcido
    
    // Borde con brillo animado
    panelBg.setOutlineThickness(3);
    float pulse = sin(time * 1.5f) * 0.3f + 0.7f;
    sf::Color glowBorder = sf::Color(100, 149, 237, static_cast<sf::Uint8>(pulse * 180));
    panelBg.setOutlineColor(glowBorder);
    window.draw(panelBg);
    
    // Línea de brillo superior
    sf::RectangleShape topGlow(sf::Vector2f(panelWidth - 20, 2));
    topGlow.setPosition(panelX + 10, panelY + 5);
    topGlow.setFillColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(pulse * 100)));
    window.draw(topGlow);
    
    // 2. Título del panel con efectos de brillo
    sf::Text statusTitle;
    if (font.getInfo().family != "") {
        statusTitle.setFont(font);
    }
    statusTitle.setString("=== STATUS ===");
    statusTitle.setCharacterSize(18);
    statusTitle.setFillColor(sf::Color(255, 215, 0)); // Dorado
    statusTitle.setPosition(panelX + 10, panelY + 15);
    
    // Efecto de brillo para el título del panel
    sf::Color titleGlow = sf::Color(255, 255, 0, static_cast<sf::Uint8>(pulse * 80));
    drawGlowEffect(statusTitle, titleGlow);
    window.draw(statusTitle);
    
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
    // PASO 0: Fondo moderno espectacular
    // ============================================
    drawModernBackground();
    
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
        int buttonHeight = 50;  // ACTUALIZADO
        int buttonX = WINDOW_WIDTH/2 - buttonWidth/2;
        int buttonSpacing = 65;  // ACTUALIZADO
        
        // Reset hover state
        int previousHover = hoveredMenuOption;
        hoveredMenuOption = -1;
        
        // Check each button
        if (x >= buttonX && x <= buttonX + buttonWidth) {
            if (y >= 230 && y <= 230 + buttonHeight) {
                hoveredMenuOption = 0; // VS AI hover
            } else if (y >= 230 + buttonSpacing && y <= 230 + buttonSpacing + buttonHeight) {
                hoveredMenuOption = 1; // VS Human hover
            } else if (y >= 230 + buttonSpacing * 2 && y <= 230 + buttonSpacing * 2 + buttonHeight) {
                hoveredMenuOption = 2; // Colorblind hover
            } else if (y >= 230 + buttonSpacing * 3 && y <= 230 + buttonSpacing * 3 + buttonHeight) {
                hoveredMenuOption = 3; // Rust AI hover
            } else if (y >= 230 + buttonSpacing * 4 && y <= 230 + buttonSpacing * 4 + buttonHeight) {
                hoveredMenuOption = 4; // Capture Mode hover
            } else if (y >= 230 + buttonSpacing * 5 && y <= 230 + buttonSpacing * 5 + buttonHeight) {
                hoveredMenuOption = 5; // Quit hover
            }
        }
        
        // Solo logear si cambió el hover (evitar spam)
        if (previousHover != hoveredMenuOption && hoveredMenuOption != -1) {
            std::string options[] = {"VS AI", "VS Human", "Colorblind Mode", "Rust AI", "Capture Mode", "Quit"};
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

// ===============================================
// MODERN VISUAL EFFECTS - Efectos Visuales Modernos
// ===============================================

void GuiRenderer::drawModernBackground() {
    // 1. Fondo con gradiente dinámico
    float time = animationClock.getElapsedTime().asSeconds();
    
    // Gradiente vertical animado
    for (int y = 0; y < WINDOW_HEIGHT; y += 4) {
        float ratio = static_cast<float>(y) / WINDOW_HEIGHT;
        float wave = sin(time * 0.5f + ratio * 3.14159f) * 0.3f + 0.7f;
        
        sf::Uint8 r = static_cast<sf::Uint8>(20 + 40 * wave);
        sf::Uint8 g = static_cast<sf::Uint8>(25 + 35 * wave);
        sf::Uint8 b = static_cast<sf::Uint8>(60 + 60 * wave);
        
        sf::RectangleShape gradientLine(sf::Vector2f(WINDOW_WIDTH, 4));
        gradientLine.setPosition(0, y);
        gradientLine.setFillColor(sf::Color(r, g, b));
        window.draw(gradientLine);
    }
    
    // 2. Partículas flotantes
    updateParticles();
    for (size_t i = 0; i < particles.size(); i++) {
        float alpha = particleLife[i] * 100;
        sf::CircleShape particle(2 + particleLife[i] * 3);
        particle.setPosition(particles[i]);
        particle.setFillColor(sf::Color(200, 220, 255, static_cast<sf::Uint8>(alpha)));
        window.draw(particle);
    }
    
    // 3. Grid de puntos brillantes de fondo
    for (int x = 50; x < WINDOW_WIDTH; x += 80) {
        for (int y = 50; y < WINDOW_HEIGHT; y += 80) {
            float distance = sqrt((x - WINDOW_WIDTH/2) * (x - WINDOW_WIDTH/2) + 
                                (y - WINDOW_HEIGHT/2) * (y - WINDOW_HEIGHT/2));
            float pulse = sin(time * 2.0f + distance * 0.01f) * 0.5f + 0.5f;
            
            sf::CircleShape dot(1.5f);
            dot.setPosition(x, y);
            dot.setFillColor(sf::Color(100, 150, 255, static_cast<sf::Uint8>(pulse * 80)));
            window.draw(dot);
        }
    }
}

void GuiRenderer::updateParticles() {
    float deltaTime = 0.016f; // ~60 FPS
    
    for (size_t i = 0; i < particles.size(); i++) {
        // Movimiento flotante
        particles[i].y -= 20 * deltaTime;
        particles[i].x += sin(animationClock.getElapsedTime().asSeconds() + i) * 10 * deltaTime;
        
        // Actualizar vida
        particleLife[i] -= deltaTime * 0.3f;
        
        // Resetear partícula si murió
        if (particleLife[i] <= 0 || particles[i].y < 0) {
            particles[i] = sf::Vector2f(rand() % WINDOW_WIDTH, WINDOW_HEIGHT + 10);
            particleLife[i] = 1.0f;
        }
    }
}

void GuiRenderer::drawGlowEffect(const sf::Text& text, sf::Color glowColor) {
    // Efecto de glow/brillo alrededor del texto
    for (int i = 1; i <= 5; i++) {
        sf::Text glowText = text;
        glowText.setFillColor(sf::Color(glowColor.r, glowColor.g, glowColor.b, 50 - i * 8));
        glowText.setPosition(text.getPosition().x + i, text.getPosition().y);
        window.draw(glowText);
        glowText.setPosition(text.getPosition().x - i, text.getPosition().y);
        window.draw(glowText);
        glowText.setPosition(text.getPosition().x, text.getPosition().y + i);
        window.draw(glowText);
        glowText.setPosition(text.getPosition().x, text.getPosition().y - i);
        window.draw(glowText);
    }
}
