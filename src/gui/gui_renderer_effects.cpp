// ===============================================
// GUI Renderer - Visual Effects Module
// ===============================================
// Handles: Hover effects, suggestion indicators, error displays, 
//          background effects, particles, animations
// Dependencies: Core module, board module
// ===============================================

#include "../../include/gui/gui_renderer.hpp"
#include <cmath>
#include <iostream>

// ===============================================
// HOVER INDICATOR
// ===============================================

void GuiRenderer::drawHoverIndicator() {
    // Only show hover on a valid position
    if (!hoverPosition.isValid()) return;
    
    sf::Vector2i pos = boardPositionToPixel(hoverPosition.x, hoverPosition.y);
    int cellSize = CELL_SIZE - 4;
    float time = animationClock.getElapsedTime().asSeconds();
    
    // 1. Expanding wave effect
    for (int i = 1; i <= 3; i++) {
        float wavePhase = fmod(time * 3.0f + i * 0.5f, 2.0f);
        float waveSize = cellSize/2 + wavePhase * 15;
        float waveAlpha = (2.0f - wavePhase) * 40;
        
        sf::CircleShape wave(waveSize);
        wave.setPosition(pos.x - waveSize, pos.y - waveSize);
        wave.setFillColor(sf::Color::Transparent);
        wave.setOutlineThickness(2);
        wave.setOutlineColor(sf::Color(0, 255, 255, static_cast<sf::Uint8>(waveAlpha))); // Bright cyan
        window.draw(wave);
    }
    
    // 2. Pulsating background
    float pulse = sin(time * 4.0f) * 0.3f + 0.7f;
    sf::RectangleShape hoverBg(sf::Vector2f(cellSize, cellSize));
    hoverBg.setPosition(pos.x - cellSize/2 + 2, pos.y - cellSize/2 + 2);
    hoverBg.setFillColor(sf::Color(100, 200, 255, static_cast<sf::Uint8>(pulse * 80)));
    window.draw(hoverBg);
    
    // 3. Border with rotating glow
    sf::RectangleShape hoverBorder(sf::Vector2f(cellSize, cellSize));
    hoverBorder.setPosition(pos.x - cellSize/2 + 2, pos.y - cellSize/2 + 2);
    hoverBorder.setFillColor(sf::Color::Transparent);
    hoverBorder.setOutlineThickness(3);
    hoverBorder.setOutlineColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(pulse * 200)));
    window.draw(hoverBorder);
    
    // 4. Rotating particles around center
    for (int i = 0; i < 6; i++) {
        float angle = time * 2.0f + i * 1.047f; // 60 degrees between particles
        float radius = 15;
        float sparkX = pos.x + cos(angle) * radius;
        float sparkY = pos.y + sin(angle) * radius;
        
        sf::CircleShape spark(3);
        spark.setPosition(sparkX - 3, sparkY - 3);
        spark.setFillColor(sf::Color(255, 255, 100, static_cast<sf::Uint8>(pulse * 255)));
        window.draw(spark);
    }
    
    // 5. Bright center circle
    sf::CircleShape hoverIndicator(8 + pulse * 3);
    hoverIndicator.setPosition(pos.x - (8 + pulse * 3), pos.y - (8 + pulse * 3));
    hoverIndicator.setFillColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(pulse * 150)));
    hoverIndicator.setOutlineThickness(2);
    hoverIndicator.setOutlineColor(sf::Color(0, 255, 255, 255));
    window.draw(hoverIndicator);
}

// ===============================================
// SUGGESTION INDICATOR
// ===============================================

void GuiRenderer::drawSuggestionIndicator() {
    // Only show if there is a valid suggestion
    if (!showSuggestion || !currentSuggestion.isValid()) return;
    
    sf::Vector2i pos = boardPositionToPixel(currentSuggestion.x, currentSuggestion.y);
    int cellSize = CELL_SIZE - 4;
    
    // 1. Cell background with distinctive color (yellow/gold)
    sf::RectangleShape suggestionBg(sf::Vector2f(cellSize, cellSize));
    suggestionBg.setPosition(pos.x - cellSize/2 + 2, pos.y - cellSize/2 + 2);
    suggestionBg.setFillColor(sf::Color(255, 215, 0, 80)); // Semi-transparent gold
    window.draw(suggestionBg);
    
    // 2. Thicker pulsating border
    sf::RectangleShape suggestionBorder(sf::Vector2f(cellSize, cellSize));
    suggestionBorder.setPosition(pos.x - cellSize/2 + 2, pos.y - cellSize/2 + 2);
    suggestionBorder.setFillColor(sf::Color::Transparent);
    suggestionBorder.setOutlineThickness(3);
    
    // Pulsating effect using time
    static sf::Clock pulseClock;
    float pulseTime = pulseClock.getElapsedTime().asSeconds();
    float alpha = (sin(pulseTime * 4.0f) + 1.0f) * 0.4f + 0.2f; // Oscillates between 0.2 and 1.0
    suggestionBorder.setOutlineColor(sf::Color(255, 215, 0, (sf::Uint8)(alpha * 255))); // Pulsating gold
    window.draw(suggestionBorder);
    
    // 3. Light bulb icon in center
    sf::CircleShape bulbOuter(8);
    bulbOuter.setPosition(pos.x - 8, pos.y - 8);
    bulbOuter.setFillColor(sf::Color(255, 255, 0, 200)); // Bright yellow
    window.draw(bulbOuter);
    
    sf::CircleShape bulbInner(5);
    bulbInner.setPosition(pos.x - 5, pos.y - 5);
    bulbInner.setFillColor(sf::Color(255, 255, 255, 250)); // Bright white (center)
    window.draw(bulbInner);
    
    // 4. "?" text in center
    drawText("?", pos.x - 5, pos.y - 8, 14, sf::Color(50, 50, 50));
}

// ===============================================
// INVALID MOVE INDICATOR
// ===============================================

void GuiRenderer::showInvalidMoveError(const Move& move) {
    errorMessage = "Invalid move!";
    invalidMovePosition = move;
    showError = true;
    errorTimer.restart();
    
    // Play invalid move sound
    audioManager.playSound("invalid_move");
}

void GuiRenderer::clearInvalidMoveError() {
    showError = false;
    errorMessage = "";
    invalidMovePosition = Move(-1, -1);
}

void GuiRenderer::drawInvalidMoveIndicator() {
    // Only show if there is an active error and less than 2 seconds have passed
    if (!showError || errorTimer.getElapsedTime().asSeconds() > 2.0f) {
        if (showError) clearInvalidMoveError(); // Auto-clear after 2 seconds
        return;
    }
    
    if (!invalidMovePosition.isValid()) return;
    
    sf::Vector2i pos = boardPositionToPixel(invalidMovePosition.x, invalidMovePosition.y);
    int cellSize = CELL_SIZE - 4;
    
    // 1. Pulsating red background
    sf::RectangleShape errorBg(sf::Vector2f(cellSize, cellSize));
    errorBg.setPosition(pos.x - cellSize/2 + 2, pos.y - cellSize/2 + 2);
    
    // Pulsating red effect using time
    float pulseTime = errorTimer.getElapsedTime().asSeconds();
    float alpha = (sin(pulseTime * 8.0f) + 1.0f) * 0.3f + 0.1f; // Oscillates faster
    errorBg.setFillColor(sf::Color(255, 0, 0, (sf::Uint8)(alpha * 255))); // Pulsating red
    window.draw(errorBg);
    
    // 2. Thick red border
    sf::RectangleShape errorBorder(sf::Vector2f(cellSize, cellSize));
    errorBorder.setPosition(pos.x - cellSize/2 + 2, pos.y - cellSize/2 + 2);
    errorBorder.setFillColor(sf::Color::Transparent);
    errorBorder.setOutlineThickness(4);
    errorBorder.setOutlineColor(sf::Color(255, 0, 0, 200)); // Solid red
    window.draw(errorBorder);
}

// ===============================================
// MODERN BACKGROUND EFFECTS
// ===============================================

void GuiRenderer::drawModernBackground() {
    // 1. Dynamic gradient background
    float time = animationClock.getElapsedTime().asSeconds();
    
    // Animated vertical gradient
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
    
    // 2. Floating particles
    updateParticles();
    for (size_t i = 0; i < particles.size(); i++) {
        float alpha = particleLife[i] * 100;
        sf::CircleShape particle(2 + particleLife[i] * 3);
        particle.setPosition(particles[i]);
        particle.setFillColor(sf::Color(200, 220, 255, static_cast<sf::Uint8>(alpha)));
        window.draw(particle);
    }
    
    // 3. Background grid of bright dots
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

// ===============================================
// PARTICLE SYSTEM
// ===============================================

void GuiRenderer::updateParticles() {
    float deltaTime = 0.016f; // ~60 FPS
    
    for (size_t i = 0; i < particles.size(); i++) {
        // Floating movement
        particles[i].y -= 20 * deltaTime;
        particles[i].x += sin(animationClock.getElapsedTime().asSeconds() + i) * 10 * deltaTime;
        
        // Update lifetime
        particleLife[i] -= deltaTime * 0.3f;
        
        // Reset particle if expired
        if (particleLife[i] <= 0 || particles[i].y < 0) {
            particles[i] = sf::Vector2f(rand() % WINDOW_WIDTH, WINDOW_HEIGHT + 10);
            particleLife[i] = 1.0f;
        }
    }
}

// ===============================================
// GLOW EFFECTS
// ===============================================

void GuiRenderer::drawGlowEffect(const sf::Text& text, sf::Color glowColor) {
    // Glow/shine effect around text
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
