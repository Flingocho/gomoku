// ===============================================
// GUI Renderer - UI Elements Module
// ===============================================
// Handles: Buttons, text rendering, game info panel, AI statistics
// Dependencies: Core module, SFML graphics
// ===============================================

#include "../../include/gui/gui_renderer.hpp"
#include <sstream>
#include <iomanip>
#include <cmath>

// ===============================================
// BUTTON RENDERING
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

// ===============================================
// TEXT RENDERING
// ===============================================

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

// ===============================================
// GAME INFO PANEL
// ===============================================

void GuiRenderer::drawGameInfo(const GameState& state, int aiTimeMs) {
    // Panel lateral derecho con información del juego
    int panelX = BOARD_OFFSET_X + BOARD_SIZE_PX + 30;
    int panelY = BOARD_OFFSET_Y;
    int panelWidth = 280;
    int panelHeight = 650; // Increased height to fit all content
    
    // 1. Fondo del panel con efectos modernos
    float time = animationClock.getElapsedTime().asSeconds();
    
    // Sombra suave y amplia
    sf::RectangleShape panelShadow(sf::Vector2f(panelWidth + 12, panelHeight + 12));
    panelShadow.setPosition(panelX + 6, panelY + 6);
    panelShadow.setFillColor(sf::Color(0, 0, 0, 80));
    window.draw(panelShadow);
    
    // Fondo translúcido con efecto de cristal
    sf::RectangleShape panelBg(sf::Vector2f(panelWidth, panelHeight));
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
    int lineHeight = 22;
    
    // 3. Información básica del juego
    std::string turnInfo = "Turn: " + std::to_string(state.turnCount);
    drawText(turnInfo, panelX + 10, yOffset, 16, sf::Color::White);
    yOffset += lineHeight;
    
    // Jugador actual
    std::string currentPlayerStr = (state.currentPlayer == GameState::PLAYER1) ? "Player 1 (O)" : "Player 2 (X)";
    sf::Color playerColor = (state.currentPlayer == GameState::PLAYER1) ? player1Color : player2Color;
    drawText("Player:", panelX + 10, yOffset, 16, sf::Color::White);
    yOffset += lineHeight;
    drawText(currentPlayerStr, panelX + 10, yOffset, 16, playerColor);
    yOffset += lineHeight + 15;
    
    // Separador
    sf::RectangleShape separator1(sf::Vector2f(panelWidth - 20, 2));
    separator1.setPosition(panelX + 10, yOffset);
    separator1.setFillColor(sf::Color(100, 100, 100));
    window.draw(separator1);
    yOffset += 20;
    
    // NUEVO: Mostrar mensaje de error si está activo
    if (showError && errorTimer.getElapsedTime().asSeconds() < 3.0f) {
        // Fondo rojo para el mensaje de error
        sf::RectangleShape errorBg(sf::Vector2f(panelWidth - 20, 50));
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
        yOffset += 20;
        
        // Coordenada del movimiento inválido si está disponible
        if (invalidMovePosition.isValid()) {
            std::string posText = "Position: ";
            posText += char('A' + invalidMovePosition.y);
            posText += std::to_string(invalidMovePosition.x + 1);
            drawText(posText, panelX + 15, yOffset, 12, sf::Color(200, 150, 150));
        }
        
        yOffset += 30;
        
        // Separador
        sf::RectangleShape separator2(sf::Vector2f(panelWidth - 20, 2));
        separator2.setPosition(panelX + 10, yOffset);
        separator2.setFillColor(sf::Color(100, 100, 100));
        window.draw(separator2);
        yOffset += 20;
    }
    
    // NUEVO: Mostrar sugerencia si está activa
    if (showSuggestion && currentSuggestion.isValid()) {
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
        yOffset += lineHeight + 15;
        
        // Separador
        sf::RectangleShape separator3(sf::Vector2f(panelWidth - 20, 2));
        separator3.setPosition(panelX + 10, yOffset);
        separator3.setFillColor(sf::Color(100, 100, 100));
        window.draw(separator3);
        yOffset += 20;
    }
    
    // 5. Capturas
    drawText("CAPTURES:", panelX + 10, yOffset, 16, sf::Color::Yellow);
    yOffset += lineHeight + 5;
    
    // Capturas del humano
    std::string humanCaptures = "You: " + std::to_string(state.captures[0]) + "/10";
    drawText(humanCaptures, panelX + 15, yOffset, 14, player1Color);
    if (state.captures[0] >= 8) {
        drawText("Close!", panelX + 150, yOffset, 14, sf::Color::Red);
    }
    yOffset += lineHeight;
    
    // Capturas de la IA
    std::string aiCaptures = "AI: " + std::to_string(state.captures[1]) + "/10";
    drawText(aiCaptures, panelX + 15, yOffset, 14, player2Color);
    if (state.captures[1] >= 8) {
        drawText("Danger!", panelX + 150, yOffset, 14, sf::Color::Red);
    }
    yOffset += lineHeight + 15;
    
    // Separador
    sf::RectangleShape separator4(sf::Vector2f(panelWidth - 20, 2));
    separator4.setPosition(panelX + 10, yOffset);
    separator4.setFillColor(sf::Color(100, 100, 100));
    window.draw(separator4);
    yOffset += 20;
    
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
    sf::RectangleShape separator5(sf::Vector2f(panelWidth - 20, 2));
    separator5.setPosition(panelX + 10, yOffset);
    separator5.setFillColor(sf::Color(100, 100, 100));
    window.draw(separator5);
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
// AI STATISTICS MANAGEMENT
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
