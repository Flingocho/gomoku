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
    
    // Non-blocking method, the choice is captured in handleMenuClick
    if (selectedMenuOption == 0) return VS_AI;
    if (selectedMenuOption == 1) return VS_HUMAN;
    if (selectedMenuOption == 2) return COLORBLIND;
    if (selectedMenuOption == 3) return RUST_AI;
    if (selectedMenuOption == 4) return OPTIONS_MENU;
    if (selectedMenuOption == 5) return QUIT;
    
    return NONE; // No selection yet
}

// ============================================================================
// MENU RENDERING
// ============================================================================

void GuiRenderer::renderMenu() {
    // 0. Modern background with effects
    drawModernBackground();
    
    // 1. Main title with glow effects
    std::string mainTitle = "=== GOMOKU AI ===";
    int titleSize = 36;
    
    sf::Text titleText;
    if (font.getInfo().family != "") {
        titleText.setFont(font);
    }
    titleText.setString(mainTitle);
    titleText.setCharacterSize(titleSize);
    titleText.setFillColor(sf::Color::White);
    
    sf::FloatRect titleBounds = titleText.getLocalBounds();
    titleText.setOrigin(titleBounds.width / 2.0f, 0);
    titleText.setPosition(WINDOW_WIDTH / 2.0f, 100);
    
    // Pulsing glow effect
    float time = animationClock.getElapsedTime().asSeconds();
    float pulse = sin(time * 2.0f) * 0.3f + 1.0f;
    sf::Color glowColor = sf::Color(255, 215, 0, static_cast<sf::Uint8>(pulse * 100));
    
    drawGlowEffect(titleText, glowColor);
    window.draw(titleText);
    
    // 2. Centered subtitle
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
    
    // 3. Menu buttons
    int buttonWidth = 250;
    int buttonHeight = 45;
    int buttonX = WINDOW_WIDTH/2.0f - buttonWidth/2.0f;
    int buttonSpacing = 55;
    
    drawButton("Play vs AI", buttonX, 220, buttonWidth, buttonHeight, hoveredMenuOption == 0);
    drawButton("Play vs Human", buttonX, 220 + buttonSpacing, buttonWidth, buttonHeight, hoveredMenuOption == 1);
    drawButton("Colorblind Mode", buttonX, 220 + buttonSpacing * 2, buttonWidth, buttonHeight, hoveredMenuOption == 2);
    drawButton("Rust AI", buttonX, 220 + buttonSpacing * 3, buttonWidth, buttonHeight, hoveredMenuOption == 3);
    drawButton("Options", buttonX, 220 + buttonSpacing * 4, buttonWidth, buttonHeight, hoveredMenuOption == 4);
    drawButton("Exit", buttonX, 220 + buttonSpacing * 5, buttonWidth, buttonHeight, hoveredMenuOption == 5);
    
    // 4. Additional information
    drawText("Features:", 50, 680, 16, sf::Color::Yellow);
    drawText("- Zobrist Hashing + Alpha-Beta pruning", 70, 705, 14, sf::Color::White);
    drawText("- Adaptive depth + Complete rules", 70, 725, 14, sf::Color::White);
    
    // 5. Controls
    drawText("ESC = Exit", WINDOW_WIDTH - 100, WINDOW_HEIGHT - 30, 14, sf::Color(100, 100, 100));
}

// ============================================================================
// OPTIONS MENU RENDERING
// ============================================================================

void GuiRenderer::renderOptions() {
    drawModernBackground();
    
    // Title
    sf::Text titleText;
    if (font.getInfo().family != "") {
        titleText.setFont(font);
    }
    titleText.setString("OPTIONS");
    titleText.setCharacterSize(32);
    titleText.setFillColor(sf::Color::White);
    sf::FloatRect titleBounds = titleText.getLocalBounds();
    titleText.setOrigin(titleBounds.width / 2.0f, 0);
    titleText.setPosition(WINDOW_WIDTH / 2.0f, 80);
    window.draw(titleText);
    
    int buttonWidth = 300;
    int buttonHeight = 40;
    int buttonX = WINDOW_WIDTH/2 - buttonWidth/2;
    int startY = 150;
    int spacing = 50;
    
    // === MUSIC SECTION ===
    drawText("=== MUSIC ===", WINDOW_WIDTH/2 - 50, startY - 25, 16, sf::Color::Yellow);
    
    // Music toggle
    std::string musicText = musicEnabled ? "Music: ON" : "Music: OFF";
    drawButton(musicText, buttonX, startY, buttonWidth, buttonHeight, hoveredMenuOption == 0);
    
    // Music volume
    std::string musicVolText = "Music Vol: " + std::to_string((int)musicVolume) + "%";
    drawText(musicVolText, WINDOW_WIDTH/2 - 55, startY + spacing, 14, sf::Color::White);
    drawButton("-", buttonX, startY + spacing + 25, 60, 35, hoveredMenuOption == 1);
    drawButton("+", buttonX + buttonWidth - 60, startY + spacing + 25, 60, 35, hoveredMenuOption == 2);
    
    // === SOUND FX SECTION ===
    drawText("=== SOUND FX ===", WINDOW_WIDTH/2 - 60, startY + spacing * 2 + 35, 16, sf::Color::Yellow);
    
    // Sound FX toggle
    std::string soundText = soundEnabled ? "Sound FX: ON" : "Sound FX: OFF";
    drawButton(soundText, buttonX, startY + spacing * 3, buttonWidth, buttonHeight, hoveredMenuOption == 3);
    
    // FX volume
    std::string fxVolText = "FX Vol: " + std::to_string((int)soundVolume) + "%";
    drawText(fxVolText, WINDOW_WIDTH/2 - 40, startY + spacing * 4, 14, sf::Color::White);
    drawButton("-", buttonX, startY + spacing * 4 + 25, 60, 35, hoveredMenuOption == 4);
    drawButton("+", buttonX + buttonWidth - 60, startY + spacing * 4 + 25, 60, 35, hoveredMenuOption == 5);
    
    // === DEBUG SECTION ===
    drawText("=== DEBUG ===", WINDOW_WIDTH/2 - 50, startY + spacing * 5 + 35, 16, sf::Color::Yellow);
    
    std::string debugText = debugEnabled ? "Debug Mode: ON" : "Debug Mode: OFF";
    drawButton(debugText, buttonX, startY + spacing * 6 + 10, buttonWidth, buttonHeight, hoveredMenuOption == 6);
    
    // Back button
    drawButton("Back to Menu", buttonX, startY + spacing * 7 + 30, buttonWidth, buttonHeight, hoveredMenuOption == 7);
    
    // Help text
    drawText("ESC = Back", WINDOW_WIDTH - 100, WINDOW_HEIGHT - 30, 14, sf::Color(100, 100, 100));
}

// ============================================================================
// MENU CLICK HANDLING
// ============================================================================

void GuiRenderer::handleMenuClick(int x, int y) {
    int buttonWidth = 250;
    int buttonHeight = 45;
    int buttonX = WINDOW_WIDTH/2 - buttonWidth/2;
    int buttonSpacing = 55;
    
    // VS AI button
    if (x >= buttonX && x <= buttonX + buttonWidth && 
        y >= 220 && y <= 220 + buttonHeight) {
        audioManager.playSound("click_menu");
        selectedMenuOption = 0;
        isColorblindMode = false;
        return;
    }
    
    // VS Human button (with suggestions)
    if (x >= buttonX && x <= buttonX + buttonWidth && 
        y >= 220 + buttonSpacing && y <= 220 + buttonSpacing + buttonHeight) {
        audioManager.playSound("click_menu");
        selectedMenuOption = 1;
        isColorblindMode = false;
        return;
    }
    
    // Colorblind Mode button
    if (x >= buttonX && x <= buttonX + buttonWidth && 
        y >= 220 + buttonSpacing * 2 && y <= 220 + buttonSpacing * 2 + buttonHeight) {
        audioManager.playSound("click_menu");
        selectedMenuOption = 2;
        isColorblindMode = true;
        return;
    }
    
    // Rust AI button
    if (x >= buttonX && x <= buttonX + buttonWidth && 
        y >= 220 + buttonSpacing * 3 && y <= 220 + buttonSpacing * 3 + buttonHeight) {
        audioManager.playSound("click_menu");
        selectedMenuOption = 3;
        return;
    }
    
    // Options button
    if (x >= buttonX && x <= buttonX + buttonWidth && 
        y >= 220 + buttonSpacing * 4 && y <= 220 + buttonSpacing * 4 + buttonHeight) {
        audioManager.playSound("click_menu");
        selectedMenuOption = 4;
        return;
    }
    
    // Quit button
    if (x >= buttonX && x <= buttonX + buttonWidth && 
        y >= 220 + buttonSpacing * 5 && y <= 220 + buttonSpacing * 5 + buttonHeight) {
        audioManager.playSound("click_menu");
        selectedMenuOption = 5;
        return;
    }
    
    selectedMenuOption = -1;
}

// ============================================================================
// OPTIONS CLICK HANDLING
// ============================================================================

void GuiRenderer::handleOptionsClick(int x, int y) {
    int buttonWidth = 300;
    int buttonHeight = 40;
    int buttonX = WINDOW_WIDTH/2 - buttonWidth/2;
    int startY = 150;
    int spacing = 50;
    
    // Music toggle
    if (x >= buttonX && x <= buttonX + buttonWidth && 
        y >= startY && y <= startY + buttonHeight) {
        audioManager.playSound("click_menu");
        toggleMusic();
        return;
    }
    
    // Music volume down
    if (x >= buttonX && x <= buttonX + 60 && 
        y >= startY + spacing + 25 && y <= startY + spacing + 60) {
        audioManager.playSound("click_menu");
        musicVolume = std::max(0.0f, musicVolume - 10.0f);
        audioManager.setMusicVolume(musicEnabled ? musicVolume : 0);
        return;
    }
    
    // Music volume up
    if (x >= buttonX + buttonWidth - 60 && x <= buttonX + buttonWidth && 
        y >= startY + spacing + 25 && y <= startY + spacing + 60) {
        audioManager.playSound("click_menu");
        musicVolume = std::min(100.0f, musicVolume + 10.0f);
        audioManager.setMusicVolume(musicEnabled ? musicVolume : 0);
        return;
    }
    
    // Sound FX toggle
    if (x >= buttonX && x <= buttonX + buttonWidth && 
        y >= startY + spacing * 3 && y <= startY + spacing * 3 + buttonHeight) {
        audioManager.playSound("click_menu");
        toggleSound();
        return;
    }
    
    // FX volume down
    if (x >= buttonX && x <= buttonX + 60 && 
        y >= startY + spacing * 4 + 25 && y <= startY + spacing * 4 + 60) {
        audioManager.playSound("click_menu");
        soundVolume = std::max(0.0f, soundVolume - 10.0f);
        audioManager.setSoundVolume(soundEnabled ? soundVolume : 0);
        return;
    }
    
    // FX volume up
    if (x >= buttonX + buttonWidth - 60 && x <= buttonX + buttonWidth && 
        y >= startY + spacing * 4 + 25 && y <= startY + spacing * 4 + 60) {
        audioManager.playSound("click_menu");
        soundVolume = std::min(100.0f, soundVolume + 10.0f);
        audioManager.setSoundVolume(soundEnabled ? soundVolume : 0);
        return;
    }
    
    // Debug toggle
    if (x >= buttonX && x <= buttonX + buttonWidth && 
        y >= startY + spacing * 6 + 10 && y <= startY + spacing * 6 + 10 + buttonHeight) {
        audioManager.playSound("click_menu");
        toggleDebug();
        return;
    }
    
    // Back button
    if (x >= buttonX && x <= buttonX + buttonWidth && 
        y >= startY + spacing * 7 + 30 && y <= startY + spacing * 7 + 30 + buttonHeight) {
        audioManager.playSound("click_menu");
        setState(MENU);
        selectedMenuOption = -1;
        return;
    }
}
