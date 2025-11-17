#ifndef GUI_RENDERER_HPP
#define GUI_RENDERER_HPP

#include <SFML/Graphics.hpp>
#include "../core/game_types.hpp"
#include "../core/audio_manager.hpp"
#include <string>
#include <chrono>
#include <vector> // NUEVO: Para vector de tiempos de IA

class GuiRenderer
{
public:
	enum AppState
	{
		MENU,
		PLAYING,
		GAME_OVER
	};

	enum MenuOption
	{
		VS_AI,
		VS_HUMAN,
		COLORBLIND,
		RUST_AI,
		QUIT,
		NONE
	};

private:
	// Core SFML
	sf::RenderWindow window;
	sf::Font font;
	sf::Event event;
	std::vector<sf::Texture> winAnimationFrames;    // Frames de la animación de victoria
	sf::Sprite winAnimationSprite;                  // Sprite para mostrar la animación de victoria
	int currentWinFrame;                            // Frame actual de la animación de victoria
	sf::Clock winAnimationClock;                    // Reloj para controlar la animación de victoria
	std::vector<sf::Texture> defeatAnimationFrames; // Frames de la animación de derrota
	sf::Sprite defeatAnimationSprite;               // Sprite para mostrar la animación de derrota
	int currentDefeatFrame;                         // Frame actual de la animación de derrota
	sf::Clock defeatAnimationClock;                 // Reloj para controlar la animación de derrota

	// App state
	AppState currentState;
	int selectedMenuOption; // Para clicks reales
	int hoveredMenuOption;	// NUEVO: Para hover effects
	Move pendingMove;		// Para capturar clicks del usuario
	bool moveReady;
	Move hoverPosition; // NUEVO: Para hover en el tablero
	Move lastAiMove;	// NUEVO: Última ficha colocada por la IA
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
	sf::Color player1Color; // Humano
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
	void processEvents(); // Procesa eventos sin bloquear
	void render(const GameState &state, int aiTimeMs = 0);

	// Game flow control
	MenuOption showMenuAndGetChoice();
	Move waitForUserMove(const GameState &state); // Reemplaza getUserMove()
	void showGameResult(int winner);

	// State management
	void setState(AppState newState) { 
        currentState = newState; 
        if (newState != GAME_OVER) {
            gameOverButtonsPositionValid = false; // Reset cuando salimos de game over
        }
    }
	AppState getState() const { return currentState; }
	bool hasUserMove() const { return moveReady; }
	Move getUserMove(); // Non-blocking, returns pending move
	void clearUserMove() { moveReady = false; }
	void refreshSelectedMenuOption() { selectedMenuOption = -1; }
	void setLastAiMove(const Move &move) { lastAiMove = move; } // NUEVO: Para resaltar la última jugada de la IA
	void resetColorblindMode() { isColorblindMode = false; } // NUEVO: Resetear modo colorblind

	// NUEVO: Métodos para estadísticas de tiempo de IA
	    void addAiTime(int timeMs);     // Add a new AI time
	float getAverageAiTime() const; // Obtener tiempo promedio
	void resetAiStats();			// Reiniciar estadísticas

	void setSuggestion(const Move &move)
	{
		currentSuggestion = move;
		showSuggestion = move.isValid();
	}

	// NUEVO: Limpiar sugerencia
	void clearSuggestion()
	{
		currentSuggestion = Move(-1, -1);
		showSuggestion = false;
	}

	// NUEVO: Métodos para manejar errores de movimiento
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

private:
	// Internal rendering methods
	void renderMenu();
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
	void drawInvalidMoveIndicator(); // NUEVO: Para mostrar error en el tablero

	// Utility functions
	sf::Vector2i boardPositionToPixel(int boardX, int boardY) const;
	std::pair<int, int> pixelToBoardPosition(int x, int y) const;
	bool isPointInBoard(int x, int y) const;
	char getPieceSymbol(int piece) const;
	sf::Color getPieceColor(int piece) const;

	// Event handling helpers
	void handleMenuClick(int x, int y);
	void handleGameClick(int x, int y);
	void handleGameOverClick(int x, int y); // NUEVO
	void handleMouseMove(int x, int y);

	// Member variables for hover position

	    // NEW: Variables for AI time statistics
	std::vector<int> aiTimes; // Almacenar todos los tiempos de IA
	int totalAiTime;		  // Suma total de tiempos
	int aiMoveCount;		  // Número de movimientos de IA realizados

	Move currentSuggestion;
    bool showSuggestion;
	
	    // NEW: Variables for handling invalid move errors
	std::string errorMessage;
	sf::Clock errorTimer;
	bool showError;
	Move invalidMovePosition;
	
	// Posiciones exactas de botones de Game Over (calculadas en renderGameOver)
	int gameOverButtonsY;
	bool gameOverButtonsPositionValid;
	bool isColorblindMode;	// NUEVO: Flag para el modo colorblind
	
	// Audio
	AudioManager audioManager;
};

#endif