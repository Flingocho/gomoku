// ============================================================================
// GUI Renderer - Core Module
// Contains: Constructor, Destructor, Main Loop, Event Processing
// ============================================================================

#include "../../include/gui/gui_renderer.hpp"
#include <iostream>
#include <cstdlib>  // For rand() in visual effects

// ============================================================================
// CONSTRUCTOR & DESTRUCTOR
// ============================================================================

GuiRenderer::GuiRenderer() 
    : window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Gomoku AI"),
      currentState(MENU),
      selectedMenuOption(-1),
      hoveredMenuOption(-1),
      moveReady(false),
      hoverPosition(-1, -1),
      lastAiMove(-1, -1),
      backgroundColor(sf::Color(40, 44, 52)),     // Dark blue-gray
      boardLineColor(sf::Color(139, 69, 19)),     // Saddle brown
      player1Color(sf::Color(30, 144, 255)),      // Human - Dodger Blue
      player2Color(sf::Color::Red),                // AI - Red  
      hoverColor(sf::Color(255, 255, 255, 100)),   // Transparent white
      totalAiTime(0),
      aiMoveCount(0),
      currentSuggestion(-1, -1),
      showSuggestion(false),
      errorMessage(""),
      showError(false),
      invalidMovePosition(-1, -1),
      gameOverButtonsY(0),
      gameOverButtonsPositionValid(false),
      isColorblindMode(false),
      showGameOverAnimation(true),
      nextButtonX(0), nextButtonY(0), nextButtonWidth(0), nextButtonHeight(0),
      debugEnabled(false),
      soundEnabled(true),
      musicEnabled(true),
      musicVolume(30.0f),
      soundVolume(70.0f)
{
    // Initialize particles for visual effects
    particles.resize(50);
    particleLife.resize(50);
    for (int i = 0; i < 50; i++) {
        particles[i] = sf::Vector2f(rand() % WINDOW_WIDTH, rand() % WINDOW_HEIGHT);
        particleLife[i] = static_cast<float>(rand() % 100) / 100.0f;
    }
    
    // Load custom font
    if (!font.loadFromFile("fonts/LEMONMILK-Medium.otf")) {
        std::cout << "Warning: Could not load font fonts/LEMONMILK-Medium.otf, using default" << std::endl;
    } else {
        std::cout << "✓ Custom font loaded: fonts/LEMONMILK-Medium.otf" << std::endl;
    }
    
    // Load victory animation frames
    currentWinFrame = 0;
    const int numWinFrames = 115; // Victory GIF has 115 frames
    winAnimationFrames.resize(numWinFrames);
    
    bool allWinFramesLoaded = true;
    for (int i = 0; i < numWinFrames; i++) {
        std::string framePath = "imgs/win_frames/win_frame_" + 
                               std::string(i < 10 ? "0" : "") + std::to_string(i) + ".png";
        if (!winAnimationFrames[i].loadFromFile(framePath)) {
            std::cout << "Warning: Could not load " << framePath << std::endl;
            allWinFramesLoaded = false;
        }
    }
    
    if (allWinFramesLoaded) {
        std::cout << "✓ Victory animation loaded: " << numWinFrames << " frames" << std::endl;
        winAnimationSprite.setTexture(winAnimationFrames[0]);
        // Adjust sprite size
        float scale = 300.0f / winAnimationFrames[0].getSize().x; // Scale to 300px width
        winAnimationSprite.setScale(scale, scale);
    }
    
    // Load defeat animation frames
    currentDefeatFrame = 0;
    const int numDefeatFrames = 41; // Defeat GIF has 41 frames
    defeatAnimationFrames.resize(numDefeatFrames);
    
    bool allDefeatFramesLoaded = true;
    for (int i = 0; i < numDefeatFrames; i++) {
        std::string framePath = "imgs/defeat_frames/defeat_frame_" + 
                               std::string(i < 10 ? "0" : "") + std::to_string(i) + ".png";
        if (!defeatAnimationFrames[i].loadFromFile(framePath)) {
            std::cout << "Warning: Could not load " << framePath << std::endl;
            allDefeatFramesLoaded = false;
        }
    }
    
    if (allDefeatFramesLoaded) {
        std::cout << "✓ Defeat animation loaded: " << numDefeatFrames << " frames" << std::endl;
        defeatAnimationSprite.setTexture(defeatAnimationFrames[0]);
        // Adjust sprite size
        float defeatScale = 300.0f / defeatAnimationFrames[0].getSize().x; // Scale to 300px width
        defeatAnimationSprite.setScale(defeatScale, defeatScale);
    }
    
    window.setFramerateLimit(60);
    std::cout << "GUI Renderer initialized: " << WINDOW_WIDTH << "x" << WINDOW_HEIGHT << std::endl;
    
    // Load audio files
    if (audioManager.loadMusic("sounds/main_theme.ogg")) {
        audioManager.playMusic(true); // Loop infinitely
    }
    
    audioManager.loadSound("place_piece", "sounds/place_piece.ogg");
    audioManager.loadSound("invalid_move", "sounds/invalid_move.ogg");
    audioManager.loadSound("click_menu", "sounds/click_menu.ogg");
    audioManager.loadSound("victory", "sounds/victory.ogg");
    audioManager.loadSound("defeat", "sounds/defeat.ogg");
    
    audioManager.setMusicVolume(30.0f);
    audioManager.setSoundVolume(70.0f);
}

GuiRenderer::~GuiRenderer() {
    if (window.isOpen()) {
        window.close();
    }
}

// ============================================================================
// CORE WINDOW MANAGEMENT
// ============================================================================

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
                        case OPTIONS:
                            handleOptionsClick(x, y);
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
                        // Clear all visual state when returning to menu
                        resetColorblindMode();
                        clearSuggestion();
                        clearInvalidMoveError();
                        setWinningLine(std::vector<Move>());
                        selectedMenuOption = -1;
                        setState(MENU);
                        std::cout << "✓ Returned to menu - all state cleaned" << std::endl;
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
            renderGameOver(state);
            break;
        case OPTIONS:
            renderOptions();
            break;
    }
    
    window.display();
}
