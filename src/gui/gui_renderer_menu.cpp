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
    titleText.setOrigin(titleBounds.left + titleBounds.width / 2.0f, titleBounds.top);
    titleText.setPosition(WINDOW_WIDTH / 2.0f, 60);
    window.draw(titleText);
    
    int buttonWidth = 300;
    int buttonHeight = 40;
    int buttonX = WINDOW_WIDTH/2 - buttonWidth/2;
    int smallBtnW = 60;
    int smallBtnH = 35;
    
    // Layout Y positions - calculated with proper spacing
    int musicHeaderY   = 120;
    int musicToggleY   = 148;
    int musicVolLabelY = 198;
    int musicVolBtnY   = 220;
    
    int soundHeaderY   = 270;
    int soundToggleY   = 298;
    int soundVolLabelY = 348;
    int soundVolBtnY   = 370;
    
    int debugHeaderY   = 420;
    int debugToggleY   = 448;
    
    int backBtnY       = 520;
    
    // === MUSIC SECTION ===
    sf::Text musicHeader;
    if (font.getInfo().family != "") musicHeader.setFont(font);
    musicHeader.setString("MUSIC");
    musicHeader.setCharacterSize(16);
    musicHeader.setFillColor(sf::Color::Yellow);
    sf::FloatRect mhBounds = musicHeader.getLocalBounds();
    musicHeader.setOrigin(mhBounds.left + mhBounds.width / 2.0f, mhBounds.top);
    musicHeader.setPosition(WINDOW_WIDTH / 2.0f, musicHeaderY);
    window.draw(musicHeader);
    
    // Music toggle
    std::string musicText = musicEnabled ? "Music: ON" : "Music: OFF";
    drawButton(musicText, buttonX, musicToggleY, buttonWidth, buttonHeight, hoveredMenuOption == 0);
    
    // Music volume label (centered)
    std::string musicVolText = "Volume: " + std::to_string((int)musicVolume) + "%";
    sf::Text mvText;
    if (font.getInfo().family != "") mvText.setFont(font);
    mvText.setString(musicVolText);
    mvText.setCharacterSize(14);
    mvText.setFillColor(sf::Color::White);
    sf::FloatRect mvBounds = mvText.getLocalBounds();
    mvText.setOrigin(mvBounds.left + mvBounds.width / 2.0f, mvBounds.top);
    mvText.setPosition(WINDOW_WIDTH / 2.0f, musicVolLabelY);
    window.draw(mvText);
    
    // Music volume buttons
    drawButton("-", buttonX, musicVolBtnY, smallBtnW, smallBtnH, hoveredMenuOption == 1);
    drawButton("+", buttonX + buttonWidth - smallBtnW, musicVolBtnY, smallBtnW, smallBtnH, hoveredMenuOption == 2);
    
    // === SOUND FX SECTION ===
    sf::Text soundHeader;
    if (font.getInfo().family != "") soundHeader.setFont(font);
    soundHeader.setString("SOUND FX");
    soundHeader.setCharacterSize(16);
    soundHeader.setFillColor(sf::Color::Yellow);
    sf::FloatRect shBounds = soundHeader.getLocalBounds();
    soundHeader.setOrigin(shBounds.left + shBounds.width / 2.0f, shBounds.top);
    soundHeader.setPosition(WINDOW_WIDTH / 2.0f, soundHeaderY);
    window.draw(soundHeader);
    
    // Sound FX toggle
    std::string soundText = soundEnabled ? "Sound FX: ON" : "Sound FX: OFF";
    drawButton(soundText, buttonX, soundToggleY, buttonWidth, buttonHeight, hoveredMenuOption == 3);
    
    // FX volume label (centered)
    std::string fxVolText = "Volume: " + std::to_string((int)soundVolume) + "%";
    sf::Text fvText;
    if (font.getInfo().family != "") fvText.setFont(font);
    fvText.setString(fxVolText);
    fvText.setCharacterSize(14);
    fvText.setFillColor(sf::Color::White);
    sf::FloatRect fvBounds = fvText.getLocalBounds();
    fvText.setOrigin(fvBounds.left + fvBounds.width / 2.0f, fvBounds.top);
    fvText.setPosition(WINDOW_WIDTH / 2.0f, soundVolLabelY);
    window.draw(fvText);
    
    // FX volume buttons
    drawButton("-", buttonX, soundVolBtnY, smallBtnW, smallBtnH, hoveredMenuOption == 4);
    drawButton("+", buttonX + buttonWidth - smallBtnW, soundVolBtnY, smallBtnW, smallBtnH, hoveredMenuOption == 5);
    
    // === DEBUG SECTION ===
    sf::Text debugHeader;
    if (font.getInfo().family != "") debugHeader.setFont(font);
    debugHeader.setString("DEBUG");
    debugHeader.setCharacterSize(16);
    debugHeader.setFillColor(sf::Color::Yellow);
    sf::FloatRect dhBounds = debugHeader.getLocalBounds();
    debugHeader.setOrigin(dhBounds.left + dhBounds.width / 2.0f, dhBounds.top);
    debugHeader.setPosition(WINDOW_WIDTH / 2.0f, debugHeaderY);
    window.draw(debugHeader);
    
    std::string debugText = debugEnabled ? "Debug Mode: ON" : "Debug Mode: OFF";
    drawButton(debugText, buttonX, debugToggleY, buttonWidth, buttonHeight, hoveredMenuOption == 6);
    
    // Back button
    drawButton("Back to Menu", buttonX, backBtnY, buttonWidth, buttonHeight, hoveredMenuOption == 7);
    
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
    int smallBtnW = 60;
    int smallBtnH = 35;
    
    // Y positions must match renderOptions layout
    int musicToggleY   = 148;
    int musicVolBtnY   = 220;
    int soundToggleY   = 298;
    int soundVolBtnY   = 370;
    int debugToggleY   = 448;
    int backBtnY       = 520;
    
    // Music toggle
    if (x >= buttonX && x <= buttonX + buttonWidth && 
        y >= musicToggleY && y <= musicToggleY + buttonHeight) {
        audioManager.playSound("click_menu");
        toggleMusic();
        return;
    }
    
    // Music volume down
    if (x >= buttonX && x <= buttonX + smallBtnW && 
        y >= musicVolBtnY && y <= musicVolBtnY + smallBtnH) {
        audioManager.playSound("click_menu");
        musicVolume = std::max(0.0f, musicVolume - 10.0f);
        audioManager.setMusicVolume(musicEnabled ? musicVolume : 0);
        return;
    }
    
    // Music volume up
    if (x >= buttonX + buttonWidth - smallBtnW && x <= buttonX + buttonWidth && 
        y >= musicVolBtnY && y <= musicVolBtnY + smallBtnH) {
        audioManager.playSound("click_menu");
        musicVolume = std::min(100.0f, musicVolume + 10.0f);
        audioManager.setMusicVolume(musicEnabled ? musicVolume : 0);
        return;
    }
    
    // Sound FX toggle
    if (x >= buttonX && x <= buttonX + buttonWidth && 
        y >= soundToggleY && y <= soundToggleY + buttonHeight) {
        audioManager.playSound("click_menu");
        toggleSound();
        return;
    }
    
    // FX volume down
    if (x >= buttonX && x <= buttonX + smallBtnW && 
        y >= soundVolBtnY && y <= soundVolBtnY + smallBtnH) {
        audioManager.playSound("click_menu");
        soundVolume = std::max(0.0f, soundVolume - 10.0f);
        audioManager.setSoundVolume(soundEnabled ? soundVolume : 0);
        return;
    }
    
    // FX volume up
    if (x >= buttonX + buttonWidth - smallBtnW && x <= buttonX + buttonWidth && 
        y >= soundVolBtnY && y <= soundVolBtnY + smallBtnH) {
        audioManager.playSound("click_menu");
        soundVolume = std::min(100.0f, soundVolume + 10.0f);
        audioManager.setSoundVolume(soundEnabled ? soundVolume : 0);
        return;
    }
    
    // Debug toggle
    if (x >= buttonX && x <= buttonX + buttonWidth && 
        y >= debugToggleY && y <= debugToggleY + buttonHeight) {
        audioManager.playSound("click_menu");
        toggleDebug();
        return;
    }
    
    // Back button
    if (x >= buttonX && x <= buttonX + buttonWidth && 
        y >= backBtnY && y <= backBtnY + buttonHeight) {
        audioManager.playSound("click_menu");
        setState(MENU);
        selectedMenuOption = -1;
        return;
    }
}
