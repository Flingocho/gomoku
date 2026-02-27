// ============================================
// SEARCH_ORDERING.CPP
// Candidate move generation and ordering
// Includes quick evaluation heuristics and adaptive filtering
// ============================================

#include "../../include/ai/transposition_search.hpp"
#include <algorithm>
#include <cmath>

using namespace Directions;

// ============================================
// MOVE GENERATION & ORDERING
// ============================================

std::vector<Move> TranspositionSearch::generateOrderedMoves(const GameState &state)
{
	// Generate candidates with adaptive filtering by game phase
	std::vector<Move> candidates = generateCandidatesAdaptiveRadius(state);

	return candidates;
}

void TranspositionSearch::orderMovesWithPreviousBest(std::vector<Move> &moves, const GameState &state)
{
	// If we have the best move from previous iteration, place it first
	if (previousBestMove.isValid())
	{
		auto it = std::find_if(moves.begin(), moves.end(),
							   [this](const Move &m)
							   {
								   return m.x == previousBestMove.x && m.y == previousBestMove.y;
							   });

		if (it != moves.end())
		{
			// Move to front
			std::iter_swap(moves.begin(), it);

			// Sort the rest normally
			if (moves.size() > 1)
			{
				std::sort(moves.begin() + 1, moves.end(), [&](const Move &a, const Move &b)
						  { return quickEvaluateMove(state, a) > quickEvaluateMove(state, b); });
			}

			return;
		}
	}

	// No previous best move found, use standard ordering
	orderMoves(moves, state);
}

void TranspositionSearch::orderMoves(std::vector<Move> &moves, const GameState &state)
{
	// For very few moves, ordering has minimal impact
	if (moves.size() <= 2)
	{
		return; // Skip sorting entirely
	}

	// Pre-compute scores once to avoid redundant calls during sort comparisons
	std::vector<int> scores(moves.size());
	for (size_t i = 0; i < moves.size(); i++)
		scores[i] = quickEvaluateMove(state, moves[i]);

	// Build index array, sort by score, then reorder moves
	std::vector<size_t> indices(moves.size());
	for (size_t i = 0; i < indices.size(); i++)
		indices[i] = i;

	std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b)
			  { return scores[a] > scores[b]; });

	std::vector<Move> sorted(moves.size());
	for (size_t i = 0; i < indices.size(); i++)
		sorted[i] = moves[indices[i]];
	moves = std::move(sorted);
}

// ============================================
// QUICK MOVE EVALUATION
// ============================================

int TranspositionSearch::quickEvaluateMove(const GameState& state, const Move& move) {
    int score = 0;
    int currentPlayer = state.currentPlayer;
    int opponent = state.getOpponent(currentPlayer);
    
    // ============================================
    // 1. CENTRALITY (O(1) - trivial)
    // ============================================
    int centerDist = std::max(std::abs(move.x - 9), std::abs(move.y - 9));
    score += (9 - centerDist) * 10;  // 0-90 points
    
    // ============================================
    // 2. IMMEDIATE CONNECTIVITY (O(8) - cheap)
    // ============================================
    // Count own and opponent adjacent pieces
    int myAdjacent = 0;
    int oppAdjacent = 0;
    
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            if (dx == 0 && dy == 0) continue;
            int nx = move.x + dx, ny = move.y + dy;
            if (state.isValid(nx, ny)) {
                int piece = state.getPiece(nx, ny);
                if (piece == currentPlayer) myAdjacent++;
                else if (piece == opponent) oppAdjacent++;
            }
        }
    }
    
    score += myAdjacent * 50;      // 0-400 points
    score += oppAdjacent * 20;     // Minor bonus for blocking
    
    // ============================================
    // 3. ACTIVE ZONE PRIORITY (O(1) - trivial)
    // ============================================
    // Is this near the opponent's last move?
    if (state.lastHumanMove.isValid()) {
        int distToLast = std::max(
            std::abs(move.x - state.lastHumanMove.x),
            std::abs(move.y - state.lastHumanMove.y)
        );
        
        if (distToLast <= 2) {
            score += 500;  // Tactical response
        }
    }
    
    // ============================================
    // 4. SIMPLE PATTERNS (O(4) - very cheap)
    // ============================================
    // Count consecutive pieces without checking open ends
    int maxMyLine = 0;
    int maxOppLine = 0;
    
    for (int d = 0; d < MAIN_COUNT; d++) {
        int dx = MAIN[d][0];
        int dy = MAIN[d][1];
        
        // Contar hacia ambos lados
        int myCount = 1;  // The piece being placed
        myCount += countConsecutiveInDirection(state, move.x, move.y, dx, dy, currentPlayer, 4);
        myCount += countConsecutiveInDirection(state, move.x, move.y, -dx, -dy, currentPlayer, 4);
        maxMyLine = std::max(maxMyLine, myCount);
        
        // Count opponent lines (for blocking evaluation)
        int oppCount = 0;
        oppCount += countConsecutiveInDirection(state, move.x, move.y, dx, dy, opponent, 4);
        oppCount += countConsecutiveInDirection(state, move.x, move.y, -dx, -dy, opponent, 4);
        maxOppLine = std::max(maxOppLine, oppCount);
    }
    
    // Exponential scoring for long lines
    if (maxMyLine >= 5) score += 100000;      // Win
    else if (maxMyLine == 4) score += 10000;  // Very dangerous
    else if (maxMyLine == 3) score += 1000;   // Dangerous
    else if (maxMyLine == 2) score += 100;    // Development
    
    // Blocking
    if (maxOppLine >= 4) score += 8000;       // Critical block
    else if (maxOppLine == 3) score += 800;   // Important block
    
    // ============================================
    // 5. QUICK CAPTURE CHECK (O(8) - cheap)
    // ============================================
    // Only check if a capture exists, without evaluating context
    for (int d = 0; d < ALL_COUNT; d++) {
        int dx = ALL[d][0];
        int dy = ALL[d][1];
        
        // Pattern: NEW + OPP + OPP + OWN
        int x1 = move.x + dx, y1 = move.y + dy;
        int x2 = move.x + 2*dx, y2 = move.y + 2*dy;
        int x3 = move.x + 3*dx, y3 = move.y + 3*dy;
        
        if (state.isValid(x1, y1) && state.isValid(x2, y2) && state.isValid(x3, y3)) {
            if (state.getPiece(x1, y1) == opponent &&
                state.getPiece(x2, y2) == opponent &&
                state.getPiece(x3, y3) == currentPlayer) {
                score += 2000;  // Capture available
                break;  // Stop searching
            }
        }
    }
    
    // ============================================
    // 6. HISTORY HEURISTIC (O(1) - trivial lookup)
    // ============================================
    // Moves that caused cutoffs in previous searches get a bonus
    score += historyTable[move.x][move.y];
    
    return score;
}

// ============================================
// PATTERN DETECTION HELPERS
// ============================================

bool TranspositionSearch::wouldCreateFiveInRow(const GameState& state, const Move& move, int player) {
    // Check 4 main directions
    for (int d = 0; d < MAIN_COUNT; d++) {
        int dx = MAIN[d][0], dy = MAIN[d][1];
        
        int count = 1; // The piece being placed
        count += countConsecutiveInDirection(state, move.x, move.y, dx, dy, player, 4);
        count += countConsecutiveInDirection(state, move.x, move.y, -dx, -dy, player, 4);
        
        if (count >= 5) return true;
    }
    
    return false;
}

bool TranspositionSearch::createsFourInRow(const GameState& state, const Move& move, int player) {
    for (int d = 0; d < MAIN_COUNT; d++) {
        int dx = MAIN[d][0], dy = MAIN[d][1];
        
        int count = 1;
        count += countConsecutiveInDirection(state, move.x, move.y, dx, dy, player, 3);
        count += countConsecutiveInDirection(state, move.x, move.y, -dx, -dy, player, 3);
        
        if (count == 4) {
            // Verify at least one end is open (to be a real threat)
            int startX = move.x - (count - 1) * dx;
            int startY = move.y - (count - 1) * dy;
            int endX = move.x + (count - 1) * dx;
            int endY = move.y + (count - 1) * dy;
            
            bool startFree = state.isValid(startX - dx, startY - dy) && 
                           state.isEmpty(startX - dx, startY - dy);
            bool endFree = state.isValid(endX + dx, endY + dy) && 
                         state.isEmpty(endX + dx, endY + dy);
            
            if (startFree || endFree) return true;
        }
    }
    
    return false;
}

bool TranspositionSearch::createsThreeInRow(const GameState& state, const Move& move, int player) {
    for (int d = 0; d < MAIN_COUNT; d++) {
        int dx = MAIN[d][0], dy = MAIN[d][1];
        
        int count = 1;
        count += countConsecutiveInDirection(state, move.x, move.y, dx, dy, player, 2);
        count += countConsecutiveInDirection(state, move.x, move.y, -dx, -dy, player, 2);
        
        if (count == 3) {
            // Verify both ends are open (free-three)
            int forward = countConsecutiveInDirection(state, move.x, move.y, dx, dy, player, 2);
            int backward = countConsecutiveInDirection(state, move.x, move.y, -dx, -dy, player, 2);
            
            int startX = move.x - backward * dx;
            int startY = move.y - backward * dy;
            int endX = move.x + forward * dx;
            int endY = move.y + forward * dy;
            
            bool startFree = state.isValid(startX - dx, startY - dy) && 
                           state.isEmpty(startX - dx, startY - dy);
            bool endFree = state.isValid(endX + dx, endY + dy) && 
                         state.isEmpty(endX + dx, endY + dy);
            
            if (startFree && endFree) return true;
        }
    }
    
    return false;
}

bool TranspositionSearch::hasImmediateCapture(const GameState& state, const Move& move, int player) {
    int opponent = state.getOpponent(player);
    
    // Check capture pattern in 8 directions: NEW + OPP + OPP + OWN
    for (int d = 0; d < ALL_COUNT; d++) {
        int dx = ALL[d][0], dy = ALL[d][1];
        
        // Forward pattern: NEW + OPP + OPP + OWN
        if (state.isValid(move.x + dx, move.y + dy) &&
            state.isValid(move.x + 2*dx, move.y + 2*dy) &&
            state.isValid(move.x + 3*dx, move.y + 3*dy)) {
            
            if (state.getPiece(move.x + dx, move.y + dy) == opponent &&
                state.getPiece(move.x + 2*dx, move.y + 2*dy) == opponent &&
                state.getPiece(move.x + 3*dx, move.y + 3*dy) == player) {
                return true;
            }
        }
        
        // Backward pattern: OWN + OPP + OPP + NEW
        if (state.isValid(move.x - dx, move.y - dy) &&
            state.isValid(move.x - 2*dx, move.y - 2*dy) &&
            state.isValid(move.x - 3*dx, move.y - 3*dy)) {
            
            if (state.getPiece(move.x - dx, move.y - dy) == opponent &&
                state.getPiece(move.x - 2*dx, move.y - 2*dy) == opponent &&
                state.getPiece(move.x - 3*dx, move.y - 3*dy) == player) {
                return true;
            }
        }
    }
    
    return false;
}

bool TranspositionSearch::isNearExistingPieces(const GameState& state, const Move& move) {
    // Check for pieces within radius 2
    for (int dx = -2; dx <= 2; dx++) {
        for (int dy = -2; dy <= 2; dy++) {
            if (dx == 0 && dy == 0) continue;
            int nx = move.x + dx, ny = move.y + dy;
            if (state.isValid(nx, ny) && state.getPiece(nx, ny) != GameState::EMPTY) {
                return true;
            }
        }
    }
    return false;
}

bool TranspositionSearch::blocksOpponentWin(const GameState& state, const Move& move, int opponent) {
    // Check if this position would create a win for the opponent
    return wouldCreateFiveInRow(state, move, opponent);
}

bool TranspositionSearch::blocksOpponentFour(const GameState& state, const Move& move, int opponent) {
    return createsFourInRow(state, move, opponent);
}

bool TranspositionSearch::blocksOpponentThree(const GameState& state, const Move& move, int opponent) {
    return createsThreeInRow(state, move, opponent);
}

int TranspositionSearch::countConsecutiveInDirection(const GameState& state, int x, int y, 
                                                   int dx, int dy, int player, int maxCount) {
    int count = 0;
    x += dx;
    y += dy;
    
    while (count < maxCount && state.isValid(x, y) && state.getPiece(x, y) == player) {
        count++;
        x += dx;
        y += dy;
    }
    
    return count;
}

// ============================================
// THREAT COUNTING HELPERS
// ============================================

bool TranspositionSearch::isBlocked(const GameState &state, int x, int y, int dx, int dy, int steps, int player)
{
	int nx = x + dx * steps;
	int ny = y + dy * steps;

	if (!state.isValid(nx, ny))
	{
		return true; // Blocked by board edge
	}

	int piece = state.getPiece(nx, ny);
	return (piece != GameState::EMPTY && piece != player); // Blocked by opponent
}

int TranspositionSearch::countThreats(const GameState &state, int player)
{
	int threats = 0;

	for (int i = 0; i < GameState::BOARD_SIZE; i += 2)
	{
		for (int j = 0; j < GameState::BOARD_SIZE; j += 2)
		{
			if (state.getPiece(i, j) == player)
			{
				threats += countLinesFromPosition(state, i, j, player);
			}
		}
	}

	return threats;
}

int TranspositionSearch::countLinesFromPosition(const GameState &state, int x, int y, int player)
{
	int lines = 0;

	for (int d = 0; d < MAIN_COUNT; d++)
	{
		int dx = MAIN[d][0], dy = MAIN[d][1];

		int count = 1; // The current piece
		count += countInDirection(state, x, y, dx, dy, player);
		count += countInDirection(state, x, y, -dx, -dy, player);

		if (count >= 3)
			lines++;
	}

	return lines;
}

int TranspositionSearch::countInDirection(const GameState &state, int x, int y, int dx, int dy, int player)
{
	int count = 0;
	x += dx;
	y += dy;

	while (state.isValid(x, y) && state.getPiece(x, y) == player)
	{
		count++;
		x += dx;
		y += dy;
	}

	return count;
}

// ============================================
// ADAPTIVE CANDIDATE GENERATION
// ============================================

std::vector<Move> TranspositionSearch::generateCandidatesAdaptiveRadius(const GameState &state)
{
    std::vector<Move> candidates;
    int searchRadius = getSearchRadiusForGamePhase(state.turnCount);
    
    // Pre-mark relevant zones
    bool relevantZone[GameState::BOARD_SIZE][GameState::BOARD_SIZE];
    for(int r=0; r<GameState::BOARD_SIZE; r++)
        for(int c=0; c<GameState::BOARD_SIZE; c++)
            relevantZone[r][c] = false;
    
    // Mark cells around existing pieces
    for (int i = 0; i < GameState::BOARD_SIZE; i++) {
        for (int j = 0; j < GameState::BOARD_SIZE; j++) {
            if (state.board[i][j] != GameState::EMPTY) {
                // Mark radius around this piece
                for (int di = -searchRadius; di <= searchRadius; di++) {
                    for (int dj = -searchRadius; dj <= searchRadius; dj++) {
                        int ni = i + di, nj = j + dj;
                        if (state.isValid(ni, nj) && state.isEmpty(ni, nj)) {
                            relevantZone[ni][nj] = true;
                        }
                    }
                }
            }
        }
    }
    
    // Mark zone around opponent's last move (tactical priority)
    if (state.lastHumanMove.isValid()) {
        int extendedRadius = searchRadius + 1; // Larger radius for responses
        for (int di = -extendedRadius; di <= extendedRadius; di++) {
            for (int dj = -extendedRadius; dj <= extendedRadius; dj++) {
                int ni = state.lastHumanMove.x + di;
                int nj = state.lastHumanMove.y + dj;
                if (state.isValid(ni, nj) && state.isEmpty(ni, nj)) {
                    relevantZone[ni][nj] = true;
                }
            }
        }
    }
    
    // Collect candidates from marked zones
    for (int i = 0; i < GameState::BOARD_SIZE; i++) {
        for (int j = 0; j < GameState::BOARD_SIZE; j++) {
            if (relevantZone[i][j]) {
                candidates.push_back(Move(i, j));
            }
        }
    }
    
    // Sort with move ordering
    orderMovesWithPreviousBest(candidates, state);
    
    // Limit number of candidates
    int maxCandidates = getMaxCandidatesForGamePhase(state);
    if (candidates.size() > (size_t)maxCandidates) {
        candidates.resize(maxCandidates);
    }
    
    return candidates;
}

int TranspositionSearch::getSearchRadiusForGamePhase(int pieceCount)
{
	if (pieceCount <= 6)
		return 2; // Opening: wide radius for exploration
	return 1;     // Mid/late game: narrow radius, dense play
}

int TranspositionSearch::getMaxCandidatesForGamePhase(const GameState &state)
{
	int pieceCount = state.turnCount;

	if (pieceCount <= 4)
	{
		return 3; // Opening: very selective to avoid combinatorial explosion
	}
	else if (pieceCount <= 10)
	{
		return 4; // Early game: moderately selective
	}
	else
	{
		return 5; // Mid/late game: more options available
	}
}

void TranspositionSearch::addCandidatesAroundLastHumanMove(std::vector<Move> &candidates, const GameState &state)
{
	// If no valid last human move exists, do nothing
	if (!state.lastHumanMove.isValid())
	{
		return;
	}

	int lastX = state.lastHumanMove.x;
	int lastY = state.lastHumanMove.y;

	// Generate all empty cells within radius 2 of the last human move
	int radius = 2; // Defensive response radius

	for (int dx = -radius; dx <= radius; dx++)
	{
		for (int dy = -radius; dy <= radius; dy++)
		{
			if (dx == 0 && dy == 0)
				continue; // Skip the last move's position

			int newX = lastX + dx;
			int newY = lastY + dy;

			// Verify it's within the board and is an empty cell
			if (state.isValid(newX, newY) && state.isEmpty(newX, newY))
			{
				Move candidate(newX, newY);

				// Check if already in the candidate list
				bool alreadyAdded = false;
				for (const Move &existing : candidates)
				{
					if (existing.x == newX && existing.y == newY)
					{
						alreadyAdded = true;
						break;
					}
				}

				// If not already added and is a legal move, add it
				if (!alreadyAdded && RuleEngine::isLegalMove(state, candidate))
				{
					candidates.push_back(candidate);
				}
			}
		}
	}
}

// ============================================
// GEOMETRIC MOVE VALUE CALCULATION
// ============================================

void TranspositionSearch::orderMovesByGeometricValue(std::vector<Move> &moves, const GameState &state)
{
	// Sort using geometric pattern evaluation
	std::sort(moves.begin(), moves.end(), [&](const Move &a, const Move &b)
			  { return calculateGeometricMoveValue(state, a) > calculateGeometricMoveValue(state, b); });
}

int TranspositionSearch::calculateGeometricMoveValue(const GameState &state, const Move &move)
{
	int value = 0;
	int currentPlayer = state.currentPlayer;
	int opponent = state.getOpponent(currentPlayer);

	// 1. POSITIONAL VALUE: Proximity to center (important in opening)
	int centralityBonus = calculateCentralityBonus(move);
	value += centralityBonus;

	// 2. PATTERN ANALYSIS: Evaluate potential alignments in 4 directions
	for (int d = 0; d < MAIN_COUNT; d++)
	{
		int dx = MAIN[d][0], dy = MAIN[d][1];

		// Contar piezas propias que se alinearían con este movimiento
		int myAlignment = 1; // The piece being placed
		myAlignment += countPiecesInDirection(state, move.x, move.y, dx, dy, currentPlayer);
		myAlignment += countPiecesInDirection(state, move.x, move.y, -dx, -dy, currentPlayer);

		// Count opponent pieces to evaluate disruptions
		int opponentInterruption = 0;
		opponentInterruption += countPiecesInDirection(state, move.x, move.y, dx, dy, opponent);
		opponentInterruption += countPiecesInDirection(state, move.x, move.y, -dx, -dy, opponent);

		// Score based on tactical alignment value
		value += calculateAlignmentValue(myAlignment);
		value += calculateInterruptionValue(opponentInterruption);
	}

	// 3. CONNECTIVITY: Bonus for being adjacent to own pieces
	int connectivityBonus = calculateConnectivityBonus(state, move, currentPlayer);
	value += connectivityBonus;

	return value;
}

int TranspositionSearch::calculateCentralityBonus(const Move &move)
{
	int centerDistance = std::max(std::abs(move.x - 9), std::abs(move.y - 9));
	return (9 - centerDistance) * 10;
}

int TranspositionSearch::calculateAlignmentValue(int alignmentLength)
{
	switch (alignmentLength)
	{
	case 5:
		return 10000; // Immediate win
	case 4:
		return 5000; // Critical threat
	case 3:
		return 1000; // Strong threat
	case 2:
		return 100; // Basic development
	default:
		return 0;
	}
}

int TranspositionSearch::calculateInterruptionValue(int interruptionLength)
{
	switch (interruptionLength)
	{
	case 4:
		return 80000; // Critical block must outweigh own attack
	case 3:
		return 15000; // Block strong threat
	case 2:
		return 1000; // Early prevention
	default:
		return 0;
	}
}

int TranspositionSearch::calculateConnectivityBonus(const GameState &state, const Move &move, int player)
{
	int connectivity = 0;

	// Check all 8 adjacent directions
	for (int dx = -1; dx <= 1; dx++)
	{
		for (int dy = -1; dy <= 1; dy++)
		{
			if (dx == 0 && dy == 0)
				continue;

			int adjX = move.x + dx, adjY = move.y + dy;
			if (state.isValid(adjX, adjY) && state.getPiece(adjX, adjY) == player)
			{
				connectivity += 30;
			}
		}
	}

	return connectivity;
}

// Optimized helper to count consecutive pieces in a direction
int TranspositionSearch::countPiecesInDirection(const GameState &state, int x, int y,
												int dx, int dy, int player)
{
	// Manual unroll for common cases (1-4 pieces)
	x += dx;
	y += dy;

	if (!state.isValid(x, y) || state.getPiece(x, y) != player)
		return 0;

	x += dx;
	y += dy;
	if (!state.isValid(x, y) || state.getPiece(x, y) != player)
		return 1;

	x += dx;
	y += dy;
	if (!state.isValid(x, y) || state.getPiece(x, y) != player)
		return 2;

	x += dx;
	y += dy;
	if (!state.isValid(x, y) || state.getPiece(x, y) != player)
		return 3;

	return 4; // Máximo que necesitamos
}
