#ifndef GUI_RENDERER_HPP
#define GUI_RENDERER_HPP

#include <SFML/Graphics.hpp>
#include "../core/game_types.hpp"
#include "../core/audio_manager.hpp"
#include <string>
#include <chrono>
#include <vector>

class GuiRenderer
{
public:
	enum AppState
	{
		MENU,
		OPTIONS,
		PLAYING,
		GAME_OVER
	};

	enum MenuOption
	{
		VS_AI,
		VS_HUMAN,
		COLORBLIND,
		RUST_AI,
		OPTIONS_MENU,
		QUIT,
		NONE
	};

private:
	// Core SFML
	sf::RenderWindow window;
	sf::Font font;
	sf::Event event;
	std::vector<sf::Texture> winAnimationFrames;    // Win animation frames
	sf::Sprite winAnimationSprite;                  // Win animation sprite
	int currentWinFrame;                            // Current win animation frame
	sf::Clock winAnimationClock;                    // Win animation timer
	std::vector<sf::Texture> defeatAnimationFrames; // Defeat animation frames
	sf::Sprite defeatAnimationSprite;               // Defeat animation sprite
	int currentDefeatFrame;                         // Current defeat animation frame
	sf::Clock defeatAnimationClock;                 // Defeat animation timer

	// App state
	AppState currentState;
	int selectedMenuOption; // Selected via click
	int hoveredMenuOption;	// For hover visual effects
	Move pendingMove;		// Captures user clicks
	bool moveReady;
	Move hoverPosition; // Current board hover position
	Move lastAiMove;	// Last AI piece placement
	std::vector<Move> winningLine;

	// Visual constants
	static constexpr int WINDOW_WIDTH = 1000;
	static constexpr int WINDOW_HEIGHT = 800;
	static constexpr int BOARD_SIZE_PX = 600;
	static constexpr int CELL_SIZE = BOARD_SIZE_PX / GameState::BOARD_SIZE;
	static constexpr int BOARD_OFFSET_X = 50;
	static constexpr int BOARD_OFFSET_Y = 100;

	// Colors
	sf::Color backgroundColor;
	sf::Color boardLineColor;
	sf::Color player1Color; // Human
	sf::Color player2Color; // AI
	sf::Color hoverColor;

	// Visual Effects
	sf::Clock animationClock;
	std::vector<sf::Vector2f> particles;
	std::vector<float> particleLife;

public:
	GuiRenderer();
	~GuiRenderer();

	// Core functionality - reemplaza Display
	bool isWindowOpen() const;
	void processEvents(); // Process events without blocking
	void render(const GameState &state, int aiTimeMs = 0);

	// Game flow control
	MenuOption showMenuAndGetChoice();
	Move waitForUserMove(const GameState &state); // Reemplaza getUserMove()
	void showGameResult(int winner);

	// State management
	void setState(AppState newState) { 
        currentState = newState; 
        if (newState == GAME_OVER) {
            showGameOverAnimation = true;  // Show animation on entering GAME_OVER
        } else {
            gameOverButtonsPositionValid = false; // Reset cuando salimos de game over
        }
    }
	AppState getState() const { return currentState; }
	bool hasUserMove() const { return moveReady; }
	Move getUserMove(); // Non-blocking, returns pending move
	void clearUserMove() { moveReady = false; }
	void refreshSelectedMenuOption() { selectedMenuOption = -1; }
	void setLastAiMove(const Move &move) { lastAiMove = move; } // Highlight the AI's last move
	void resetColorblindMode() { isColorblindMode = false; } // Reset colorblind mode

	// AI time statistics
	    void addAiTime(int timeMs);     // Add a new AI time
	float getAverageAiTime() const; // Get average AI thinking time
	void resetAiStats();			// Reset statistics

	void setSuggestion(const Move &move)
	{
		currentSuggestion = move;
		showSuggestion = move.isValid();
	}

	// Clear active suggestion
	void clearSuggestion()
	{
		currentSuggestion = Move(-1, -1);
		showSuggestion = false;
	}

	bool hasSuggestion() const { return showSuggestion; }

	// Invalid move error handling
	void showInvalidMoveError(const Move& move);
	void clearInvalidMoveError();
	void setWinningLine(const std::vector<Move>& line) { winningLine = line; }
	int getSelectedMenuOption() const { return selectedMenuOption; }
	
	// Audio control methods
	void playPlacePieceSound() { audioManager.playSound("place_piece"); }
	void playInvalidMoveSound() { audioManager.playSound("invalid_move"); }
	void playVictorySound() { audioManager.playSound("victory"); }
	void playDefeatSound() { audioManager.playSound("defeat"); }
	void toggleMute() { audioManager.toggleMute(); }
	
	// Options menu controls
	void setMusicVolume(float vol) { audioManager.setMusicVolume(vol); }
	void setSoundVolume(float vol) { audioManager.setSoundVolume(vol); }
	float getMusicVolume() const { return musicVolume; }
	float getSoundVolume() const { return soundVolume; }
	bool isSoundEnabled() const { return soundEnabled; }
	bool isMusicEnabled() const { return musicEnabled; }
	void toggleSound() { soundEnabled = !soundEnabled; }
	void toggleMusic() { musicEnabled = !musicEnabled; audioManager.setMusicVolume(musicEnabled ? musicVolume : 0); }
	bool isDebugEnabled() const { return debugEnabled; }
	void toggleDebug() { debugEnabled = !debugEnabled; }

private:
	// Internal rendering methods
	void renderMenu();
	void renderOptions();
	void renderGame(const GameState &state, int aiTimeMs);
    void renderGameOver(const GameState &state); // FIXED: Receive state to show winner
	
	// Visual Effects
	void drawModernBackground();
	void updateParticles();
	void drawGlowEffect(const sf::Text& text, sf::Color glowColor);

	// Board rendering
	void drawBoard();
	void drawPieces(const GameState &state);
	void drawHoverIndicator();
	void drawSuggestionIndicator(); 

	// UI elements
	void drawButton(const std::string &text, int x, int y, int width, int height,
					bool highlighted = false);
	void drawText(const std::string &text, int x, int y, int size = 24,
				  sf::Color color = sf::Color::White);
	void drawGameInfo(const GameState &state, int aiTimeMs);
	void drawInvalidMoveIndicator(); // Show error indicator on board

	// Utility functions
	sf::Vector2i boardPositionToPixel(int boardX, int boardY) const;
	std::pair<int, int> pixelToBoardPosition(int x, int y) const;
	bool isPointInBoard(int x, int y) const;
	char getPieceSymbol(int piece) const;
	sf::Color getPieceColor(int piece) const;

	// Event handling helpers
	void handleMenuClick(int x, int y);
	void handleOptionsClick(int x, int y);
	void handleGameClick(int x, int y);
	void handleGameOverClick(int x, int y);
	void handleMouseMove(int x, int y);

	// Member variables for hover position

	    // AI time statistics
	std::vector<int> aiTimes; // All AI thinking times
	int totalAiTime;		  // Total accumulated time
	int aiMoveCount;		  // Number of AI moves made

	Move currentSuggestion;
    bool showSuggestion;
	
	    // Invalid move error state
	std::string errorMessage;
	sf::Clock errorTimer;
	bool showError;
	Move invalidMovePosition;
	
	// Posiciones exactas de botones de Game Over (calculadas en renderGameOver)
	int gameOverButtonsY;
	bool gameOverButtonsPositionValid;
	bool isColorblindMode;	// Colorblind mode flag
	
	// Winner tracking (set by showGameResult, used by renderGameOver)
	int storedWinner = 0;
	
	// Animation skip control
	bool showGameOverAnimation;  // Whether to show win/defeat animation
	int nextButtonX, nextButtonY, nextButtonWidth, nextButtonHeight;  // NEXT button position
	
	// Options state
	bool debugEnabled;
	bool soundEnabled;
	bool musicEnabled;
	float musicVolume;
	float soundVolume;
	
	// Audio
	AudioManager audioManager;
};

#endif