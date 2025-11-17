// ============================================================================
// GUI Renderer - Menu Module
// Contains: Menu rendering, menu click handling, menu selection
// ============================================================================

#include "../../include/gui/gui_renderer.hpp"
#include <iostream>
#include <cmath>

// ============================================================================
// MENU SELECTION
// ============================================================================

GuiRenderer::MenuOption GuiRenderer::showMenuAndGetChoice() {
    setState(MENU);
    
    // Este método será non-blocking, la elección se captura en handleMenuClick
    if (selectedMenuOption == 0) return VS_AI;
    if (selectedMenuOption == 1) return VS_HUMAN; 
    if (selectedMenuOption == 2) return COLORBLIND;
    if (selectedMenuOption == 3) return RUST_AI;
    if (selectedMenuOption == 4) return QUIT;
    
    return NONE; // Sin selección aún
}

// ============================================================================
// MENU RENDERING
// ============================================================================

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
    int buttonHeight = 50;
    int buttonX = WINDOW_WIDTH/2.0f - buttonWidth/2.0f;
    int buttonSpacing = 65;
    
    // Usar hoveredMenuOption para efectos visuales
    bool vsAiHover = (hoveredMenuOption == 0);
    drawButton("Play vs AI", buttonX, 230, buttonWidth, buttonHeight, vsAiHover);
    
    bool vsHumanHover = (hoveredMenuOption == 1);
    drawButton("Play vs Human", buttonX, 230 + buttonSpacing, buttonWidth, buttonHeight, vsHumanHover);
    
    bool colorblindHover = (hoveredMenuOption == 2);
    drawButton("Colorblind Mode", buttonX, 230 + buttonSpacing * 2, buttonWidth, buttonHeight, colorblindHover);
    
    bool rustAiHover = (hoveredMenuOption == 3);
    drawButton("Rust AI", buttonX, 230 + buttonSpacing * 3, buttonWidth, buttonHeight, rustAiHover);
    
    bool quitHover = (hoveredMenuOption == 4);
    drawButton("Exit", buttonX, 230 + buttonSpacing * 4, buttonWidth, buttonHeight, quitHover);
    
    // 3. Información adicional (compacta)
    drawText("Features:", 50, 680, 16, sf::Color::Yellow);
    drawText("- Zobrist Hashing + Alpha-Beta pruning", 70, 705, 14, sf::Color::White);
    drawText("- Adaptive depth + Complete rules", 70, 725, 14, sf::Color::White);
    
    // 4. Controles
    drawText("ESC = Exit", WINDOW_WIDTH - 100, WINDOW_HEIGHT - 30, 14, sf::Color(100, 100, 100));
}

// ============================================================================
// MENU CLICK HANDLING
// ============================================================================

void GuiRenderer::handleMenuClick(int x, int y) {
    int buttonWidth = 250;
    int buttonHeight = 50;
    int buttonX = WINDOW_WIDTH/2 - buttonWidth/2;
    int buttonSpacing = 65;
    
    // Botón VS AI (posición Y: 230)
    if (x >= buttonX && x <= buttonX + buttonWidth && 
        y >= 230 && y <= 230 + buttonHeight) {
        audioManager.playSound("click_menu");
        selectedMenuOption = 0;
        isColorblindMode = false;
        std::cout << "Seleccionado: VS AI" << std::endl;
        return;
    }
    
    // Botón VS Human (posición Y: 230 + 65)
    if (x >= buttonX && x <= buttonX + buttonWidth && 
        y >= 230 + buttonSpacing && y <= 230 + buttonSpacing + buttonHeight) {
        audioManager.playSound("click_menu");
        selectedMenuOption = 1;
        isColorblindMode = false;
        std::cout << "Seleccionado: VS Human" << std::endl;
        return;
    }
    
    // Botón Colorblind Mode (posición Y: 230 + 130)
    if (x >= buttonX && x <= buttonX + buttonWidth && 
        y >= 230 + buttonSpacing * 2 && y <= 230 + buttonSpacing * 2 + buttonHeight) {
        audioManager.playSound("click_menu");
        selectedMenuOption = 2;
        isColorblindMode = true;
        std::cout << "Seleccionado: Colorblind Mode (VS AI)" << std::endl;
        return;
    }
    
    // Botón Rust AI (posición Y: 230 + 195)
    if (x >= buttonX && x <= buttonX + buttonWidth && 
        y >= 230 + buttonSpacing * 3 && y <= 230 + buttonSpacing * 3 + buttonHeight) {
        audioManager.playSound("click_menu");
        selectedMenuOption = 3;
        std::cout << "Seleccionado: Rust AI" << std::endl;
        return;
    }
    
    // Botón Quit (posición Y: 230 + 260)
    if (x >= buttonX && x <= buttonX + buttonWidth && 
        y >= 230 + buttonSpacing * 4 && y <= 230 + buttonSpacing * 4 + buttonHeight) {
        audioManager.playSound("click_menu");
        selectedMenuOption = 4;
        std::cout << "Seleccionado: Quit" << std::endl;
        return;
    }
    
    // Click fuera de botones
    selectedMenuOption = -1;
}
