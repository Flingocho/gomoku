// ============================================
// RULES_VALIDATION.CPP
// Validación de movimientos: double free-three
// Detección de patrones válidos de free-three
// ============================================

#include "../../include/rules/rule_engine.hpp"

using namespace Directions;

bool RuleEngine::createsDoubleFreeThree(const GameState &state, const Move &move, int player)
{
	// Crear copia temporal para testear
	GameState tempState = state;
	tempState.board[move.x][move.y] = player;

	auto freeThrees = findFreeThrees(tempState, move, player);
	return freeThrees.size() >= 2;
}

std::vector<Move> RuleEngine::findFreeThrees(const GameState &state, const Move &move, int player)
{
	std::vector<Move> freeThrees;

	// Verificar las 4 direcciones principales
	for (int d = 0; d < MAIN_COUNT; d++)
	{
		if (isFreeThree(state, move, MAIN[d][0], MAIN[d][1], player))
		{
			freeThrees.push_back(Move(MAIN[d][0], MAIN[d][1])); // Direction as identifier
		}
	}

	return freeThrees;
}

bool RuleEngine::isFreeThree(const GameState &state, const Move &move,
							 int dx, int dy, int player)
{
	// Un free-three es cualquier patrón de 3 fichas en una ventana de 5 posiciones
	// donde ambos extremos están libres y se puede formar una amenaza de 4
	// Ahora incluye patrones con gaps como -XX-X- o -X-XX-
	
	// Buscar todas las ventanas de 5 posiciones que contengan el movimiento
	for (int offset = -4; offset <= 0; offset++)
	{
		Move windowStart(move.x + offset * dx, move.y + offset * dy);
		
		// Verificar que la ventana de 5 está dentro del tablero
		bool validWindow = true;
		for (int i = 0; i < 5; i++)
		{
			Move pos(windowStart.x + i * dx, windowStart.y + i * dy);
			if (!state.isValid(pos.x, pos.y))
			{
				validWindow = false;
				break;
			}
		}
		
		if (!validWindow) continue;
		
		// Verificar que el movimiento está dentro de esta ventana
		bool moveInWindow = false;
		for (int i = 0; i < 5; i++)
		{
			Move pos(windowStart.x + i * dx, windowStart.y + i * dy);
			if (pos.x == move.x && pos.y == move.y)
			{
				moveInWindow = true;
				break;
			}
		}
		
		if (!moveInWindow) continue;
		
		// Crear array de estados de la ventana
		int windowState[5];
		for (int i = 0; i < 5; i++)
		{
			Move pos(windowStart.x + i * dx, windowStart.y + i * dy);
			
			if (pos.x == move.x && pos.y == move.y)
			{
				windowState[i] = player; // El movimiento cuenta como ficha del jugador
			}
			else
			{
				windowState[i] = state.getPiece(pos.x, pos.y);
			}
		}
		
		// Contar fichas del jugador y del oponente
		int playerPieces = 0;
		int opponentPieces = 0;
		int emptySpaces = 0;
		
		for (int i = 0; i < 5; i++)
		{
			if (windowState[i] == player)
				playerPieces++;
			else if (windowState[i] == state.getOpponent(player))
				opponentPieces++;
			else
				emptySpaces++;
		}
		
		// Para ser free-three: exactamente 3 fichas del jugador, 0 del oponente, 2 vacías
		if (playerPieces == 3 && opponentPieces == 0 && emptySpaces == 2)
		{
			// Verificar que los extremos están libres
			Move leftExtreme(windowStart.x - dx, windowStart.y - dy);
			Move rightExtreme(windowStart.x + 5 * dx, windowStart.y + 5 * dy);
			
			bool leftFree = state.isValid(leftExtreme.x, leftExtreme.y) && 
							state.isEmpty(leftExtreme.x, leftExtreme.y);
			bool rightFree = state.isValid(rightExtreme.x, rightExtreme.y) && 
							 state.isEmpty(rightExtreme.x, rightExtreme.y);
			
			// Ambos extremos deben estar libres para ser "free"
			if (leftFree && rightFree)
			{
				// Verificar que es un patrón válido de free-three
				// Debe ser posible formar un cuatro consecutivo llenando los espacios
				if (isValidFreeThreePattern(windowState, player))
				{
					return true;
				}
			}
		}
	}
	
	return false;
}

bool RuleEngine::isValidFreeThreePattern(const int windowState[5], int player)
{
    // Definir todos los patrones válidos de free-three con 3 fichas y 2 espacios vacíos
    // Donde X = ficha del jugador, - = espacio vacío
    
    // Patrones válidos de free-three:
    // XXX-- , XX-X- , XX--X , X-XX- , X-X-X , X--XX , -XXX- , -XX-X , -X-XX , --XXX
    
    int empty = 0; // Representamos espacio vacío como 0
    
    // Crear array de patrones válidos
    int validPatterns[][5] = {
        {player, player, player, empty, empty},  // XXX--
        {player, player, empty, player, empty},  // XX-X-
        {player, player, empty, empty, player},  // XX--X
        {player, empty, player, player, empty},  // X-XX-
        {player, empty, player, empty, player},  // X-X-X
        {player, empty, empty, player, player},  // X--XX
        {empty, player, player, player, empty},  // -XXX-
        {empty, player, player, empty, player},  // -XX-X
        {empty, player, empty, player, player},  // -X-XX
        {empty, empty, player, player, player}   // --XXX
    };
    
    int numPatterns = sizeof(validPatterns) / sizeof(validPatterns[0]);
    
    // Verificar si el windowState coincide con algún patrón válido
    for (int p = 0; p < numPatterns; p++)
    {
        bool matches = true;
        for (int i = 0; i < 5; i++)
        {
            if (windowState[i] != validPatterns[p][i])
            {
                matches = false;
                break;
            }
        }
        
        if (matches)
        {
            // Verificar que este patrón puede formar una amenaza real
            // (puede completarse a 4 en línea de manera útil)
            return canFormThreat(validPatterns[p], player);
        }
    }
    
    return false;
}

bool RuleEngine::canFormThreat(const int pattern[5], int player)
{
    // Un patrón puede formar amenaza si:
    // 1. Tiene exactamente 3 fichas del jugador y 2 espacios vacíos
    // 2. Al menos una de las posiciones vacías puede completar una secuencia de 4
    
    int playerCount = 0;
    int emptyCount = 0;
    
    for (int i = 0; i < 5; i++)
    {
        if (pattern[i] == player) playerCount++;
        else if (pattern[i] == 0) emptyCount++;
    }
    
    // Debe tener exactamente 3 fichas y 2 espacios
    if (playerCount != 3 || emptyCount != 2) return false;
    
    // Verificar si podemos formar amenazas de 4
    // Simular llenar cada espacio vacío y ver si forma 4 consecutivos
    for (int i = 0; i < 5; i++)
    {
        if (pattern[i] == 0) // Espacio vacío
        {
            // Simular colocar ficha aquí
            int tempPattern[5];
            for (int j = 0; j < 5; j++)
            {
                tempPattern[j] = (j == i) ? player : pattern[j];
            }
            
            // Verificar si forma 4 consecutivos
            if (hasFourConsecutive(tempPattern, player))
            {
                return true;
            }
        }
    }
    
    return false;
}

bool RuleEngine::hasFourConsecutive(const int pattern[5], int player)
{
    // Buscar 4 fichas consecutivas del jugador en el patrón
    for (int i = 0; i <= 1; i++) // Solo puede empezar en posición 0 o 1 para tener 4 consecutivos
    {
        bool consecutive = true;
        for (int j = 0; j < 4; j++)
        {
            if (pattern[i + j] != player)
            {
                consecutive = false;
                break;
            }
        }
        
        if (consecutive) return true;
    }
    
    return false;
}
