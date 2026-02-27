// ============================================
// DEBUG_ANALYZER.CPP  
// Move analysis, evaluation, and game snapshots
// ============================================

#include "../../include/debug/debug_analyzer.hpp"
#include "../../include/ai/evaluator.hpp"
#include "../../include/rules/rule_engine.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <cmath>

extern EvaluationDebugCapture g_evalDebug;

void DebugAnalyzer::analyzeRootMove(const Move& move, int score, const EvaluationBreakdown& breakdown) {
    if (currentLevel == DEBUG_OFF) return;
    
    MoveAnalysis analysis(move);
    analysis.score = score;
    analysis.breakdown = breakdown;
    analysis.reasoning = breakdown.explanation;
    
    rootMoveAnalyses.push_back(analysis);
}

void DebugAnalyzer::setChosenMove(const Move& move, int finalScore) {
    if (currentLevel == DEBUG_OFF && finalScore != 0) return;
    
    // Mark the chosen move
    for (auto& analysis : rootMoveAnalyses) {
        if (analysis.move.x == move.x && analysis.move.y == move.y) {
            analysis.wasChosenAsRoot = true;
            break;
        }
    }
}

void DebugAnalyzer::createSnapshot(const GameState& state, int totalTime, int totalNodes) {
    if (currentLevel == DEBUG_OFF) return;
    
    lastSnapshot.state = state;
    lastSnapshot.totalTime = totalTime;
    lastSnapshot.totalNodes = totalNodes;
    lastSnapshot.gamePhase = analyzeGamePhase(state);
    lastSnapshot.criticalThreats = findCriticalThreats(state);
    
    // Copy move analyses and sort by score
    lastSnapshot.topMoves = rootMoveAnalyses;
    std::sort(lastSnapshot.topMoves.begin(), lastSnapshot.topMoves.end(),
              [](const MoveAnalysis& a, const MoveAnalysis& b) {
                  return a.score > b.score;
              });
    
    // Find chosen move
    for (const auto& analysis : rootMoveAnalyses) {
        if (analysis.wasChosenAsRoot) {
            lastSnapshot.chosenMove = analysis.move;
            break;
        }
    }
    
    // Print analysis if debug level allows it
    if (currentLevel >= DEBUG_TOP_MOVES) {
        printCurrentAnalysis();
    }
    
    // Clear for next turn
    clear();
}

EvaluationBreakdown DebugAnalyzer::evaluateWithBreakdown(
    const GameState& state, const Move& move, int player) {
    
    EvaluationBreakdown breakdown(move);
    
    // Create temporary state for evaluation
    GameState tempState = state;
    tempState.board[move.x][move.y] = player;
    
    // CRITICAL! Check win conditions FIRST
    if (RuleEngine::checkWin(tempState, player)) {
        breakdown.totalScore = 500000;
        breakdown.isWinning = true;
        breakdown.explanation = "WINNING MOVE! ";
        return breakdown;
    }
    if (RuleEngine::checkWin(tempState, state.getOpponent(player))) {
        breakdown.totalScore = -500000;
        breakdown.isLosing = true;
        breakdown.explanation = "LOSING MOVE! ";
        return breakdown;
    }
    
    // 1. Enable real evaluator debug capture
    g_evalDebug.reset();
    g_evalDebug.active = true;
    g_evalDebug.currentMove = move;
    
    // Evaluate patterns with automatic debug capture
    breakdown.patternScore = Evaluator::evaluateForPlayer(tempState, player) - 
                           Evaluator::evaluateForPlayer(tempState, state.getOpponent(player));
    
    // Transfer captured information from the real evaluator
    if (g_evalDebug.active) {
        if (player == GameState::PLAYER2) {
            breakdown.heuristicDebug.threeOpenCount = g_evalDebug.aiThreeOpen;
            breakdown.heuristicDebug.fourHalfCount = g_evalDebug.aiFourHalf; 
            breakdown.heuristicDebug.fourOpenCount = g_evalDebug.aiFourOpen;
            breakdown.heuristicDebug.twoOpenCount = g_evalDebug.aiTwoOpen;
            
            breakdown.heuristicDebug.threeOpenScore = g_evalDebug.aiThreeOpen * Evaluator::THREE_OPEN;
            breakdown.heuristicDebug.fourHalfScore = g_evalDebug.aiFourHalf * Evaluator::FOUR_HALF;
            breakdown.heuristicDebug.fourOpenScore = g_evalDebug.aiFourOpen * Evaluator::FOUR_OPEN;
            breakdown.heuristicDebug.twoOpenScore = g_evalDebug.aiTwoOpen * Evaluator::TWO_OPEN;
            
            std::ostringstream desc;
            if (g_evalDebug.aiThreeOpen > 0) desc << "AI_3_OPEN:" << g_evalDebug.aiThreeOpen << "(" << breakdown.heuristicDebug.threeOpenScore << ") ";
            if (g_evalDebug.aiFourHalf > 0) desc << "AI_4_HALF:" << g_evalDebug.aiFourHalf << "(" << breakdown.heuristicDebug.fourHalfScore << ") ";
            if (g_evalDebug.humanThreeOpen > 0) desc << "HU_3_OPEN:" << g_evalDebug.humanThreeOpen << "(" << g_evalDebug.humanThreeOpen * Evaluator::THREE_OPEN << ") ";
            if (g_evalDebug.humanFourHalf > 0) desc << "HU_4_HALF:" << g_evalDebug.humanFourHalf << "(" << g_evalDebug.humanFourHalf * Evaluator::FOUR_HALF << ") ";
            breakdown.heuristicDebug.patternDetails = desc.str();
        } else {
            breakdown.heuristicDebug.threeOpenCount = g_evalDebug.humanThreeOpen;
            breakdown.heuristicDebug.fourHalfCount = g_evalDebug.humanFourHalf;
            breakdown.heuristicDebug.fourOpenCount = g_evalDebug.humanFourOpen;
            breakdown.heuristicDebug.twoOpenCount = g_evalDebug.humanTwoOpen;
            
            breakdown.heuristicDebug.threeOpenScore = g_evalDebug.humanThreeOpen * Evaluator::THREE_OPEN;
            breakdown.heuristicDebug.fourHalfScore = g_evalDebug.humanFourHalf * Evaluator::FOUR_HALF;
            breakdown.heuristicDebug.fourOpenScore = g_evalDebug.humanFourOpen * Evaluator::FOUR_OPEN;
            breakdown.heuristicDebug.twoOpenScore = g_evalDebug.humanTwoOpen * Evaluator::TWO_OPEN;
            
            std::ostringstream desc;
            if (g_evalDebug.humanThreeOpen > 0) desc << "HU_3_OPEN:" << g_evalDebug.humanThreeOpen << "(" << breakdown.heuristicDebug.threeOpenScore << ") ";
            if (g_evalDebug.humanFourHalf > 0) desc << "HU_4_HALF:" << g_evalDebug.humanFourHalf << "(" << breakdown.heuristicDebug.fourHalfScore << ") ";
            if (g_evalDebug.aiThreeOpen > 0) desc << "AI_3_OPEN:" << g_evalDebug.aiThreeOpen << "(" << g_evalDebug.aiThreeOpen * Evaluator::THREE_OPEN << ") ";
            if (g_evalDebug.aiFourHalf > 0) desc << "AI_4_HALF:" << g_evalDebug.aiFourHalf << "(" << g_evalDebug.aiFourHalf * Evaluator::FOUR_HALF << ") ";
            breakdown.heuristicDebug.patternDetails = desc.str();
        }
        
        g_evalDebug.active = false;
    }
    
    // 2. Evaluate captures
    auto captures = RuleEngine::findCaptures(tempState, move, player);
    breakdown.captureScore = captures.size() * 1000;
    
    // 3. Evaluate immediate threats
    breakdown.threatScore = Evaluator::evaluateImmediateThreats(tempState, player);
    
    // 4. Evaluate position
    int centerDist = std::max(std::abs(move.x - 9), std::abs(move.y - 9));
    breakdown.positionScore = (9 - centerDist) * 10;
    
    // 5. Total score
    breakdown.totalScore = breakdown.patternScore + breakdown.captureScore + 
                          breakdown.threatScore + breakdown.positionScore;
    
    // 6. Criticality analysis
    breakdown.isWinning = breakdown.totalScore > 50000;
    breakdown.isLosing = breakdown.totalScore < -50000;
    breakdown.isCriticalThreat = std::abs(breakdown.totalScore) > 10000;
    
    // 7. Generate explanation
    std::ostringstream explanation;
    
    if (breakdown.isWinning) {
        explanation << "WINNING MOVE! ";
    } else if (breakdown.isLosing) {
        explanation << "LOSING MOVE! ";
    }
    
    if (breakdown.captureScore > 0) {
        explanation << "Captures:" << (breakdown.captureScore/1000) << " ";
    }
    
    if (breakdown.threatScore > 20000) {
        explanation << "CRITICAL_THREAT ";
    } else if (breakdown.threatScore > 5000) {
        explanation << "threat ";
    }
    
    if (breakdown.patternScore > 1000) {
        explanation << "good_pattern ";
    } else if (breakdown.patternScore < -1000) {
        explanation << "bad_pattern ";
    }
    
    explanation << "[P:" << breakdown.patternScore 
                << " C:" << breakdown.captureScore 
                << " T:" << breakdown.threatScore 
                << " Pos:" << breakdown.positionScore << "]";
                
    if (breakdown.heuristicDebug.threeOpenCount > 0 || breakdown.heuristicDebug.fourHalfCount > 0 || 
        breakdown.heuristicDebug.fourOpenCount > 0) {
        explanation << " " << breakdown.heuristicDebug.patternDetails;
    }
    
    breakdown.explanation = explanation.str();
    
    return breakdown;
}

void DebugAnalyzer::printCurrentAnalysis() const {
    if (rootMoveAnalyses.empty() || currentLevel == DEBUG_OFF) return;
    
    std::ostringstream analysisLog;
    
    analysisLog << "\n🤖 AI DECISION ANALYSIS\n";
    analysisLog << "========================\n";
    analysisLog << "Turn: " << lastSnapshot.state.turnCount << "\n";
    analysisLog << "Current Player: " << (lastSnapshot.state.currentPlayer == GameState::PLAYER1 ? "HUMAN (O)" : "AI (X)") << "\n";
    analysisLog << "Game Phase: " << lastSnapshot.gamePhase << "\n";
    analysisLog << "Captures: HUMAN=" << lastSnapshot.state.captures[0] << " AI=" << lastSnapshot.state.captures[1] << "\n";
    
    if (!lastSnapshot.criticalThreats.empty()) {
        analysisLog << "⚠️  Critical Threats: " << lastSnapshot.criticalThreats << "\n";
    }
    
    analysisLog << "\nCURRENT BOARD STATE:\n   ";
    for (int j = 0; j < GameState::BOARD_SIZE; j++) {
        analysisLog << std::setw(2) << char('A' + j) << " ";
    }
    analysisLog << "\n";
    
    for (int i = 0; i < GameState::BOARD_SIZE; i++) {
        analysisLog << std::setw(2) << (i + 1) << " ";
        for (int j = 0; j < GameState::BOARD_SIZE; j++) {
            char piece = '.';
            if (lastSnapshot.state.board[i][j] == GameState::PLAYER1) piece = 'O';
            else if (lastSnapshot.state.board[i][j] == GameState::PLAYER2) piece = 'X';
            analysisLog << piece << "  ";
        }
        analysisLog << "\n";
    }
    
    analysisLog << "\nTOP 10 MOVE CANDIDATES:\n";
    analysisLog << std::left << std::setw(8) << "Rank" 
                << std::setw(6) << "Move" 
                << std::setw(12) << "Score" 
                << std::setw(50) << "Reason" << "\n";
    analysisLog << std::string(80, '-') << "\n";
    
    int count = 0;
    for (const auto& analysis : lastSnapshot.topMoves) {
        if (count >= 10) break;
        
        std::string marker = analysis.wasChosenAsRoot ? "👑CHOSEN" : std::to_string(count + 1);
        
        analysisLog << std::left << std::setw(8) << marker
                    << std::setw(6) << formatMove(analysis.move)
                    << std::setw(12) << formatScore(analysis.score)
                    << analysis.breakdown.explanation << "\n";
        
        if (currentLevel >= DEBUG_HEURISTIC) {
            const auto& b = analysis.breakdown;
            analysisLog << "        └─ Pattern:" << b.patternScore 
                        << " Capture:" << b.captureScore
                        << " Threat:" << b.threatScore 
                        << " Position:" << b.positionScore << "\n";
                        
            if (analysis.wasChosenAsRoot) {
                const auto& h = b.heuristicDebug;
                if (h.threeOpenCount > 0 || h.fourHalfCount > 0 || h.fourOpenCount > 0 || h.twoOpenCount > 0) {
                    analysisLog << "        ★ HEURISTIC PATTERNS BREAKDOWN:\n";
                    if (h.fourOpenCount > 0) 
                        analysisLog << "          └─ FOUR_OPEN: " << h.fourOpenCount << " patterns = " << h.fourOpenScore << " points (critical value!)\n";
                    if (h.fourHalfCount > 0) 
                        analysisLog << "          └─ FOUR_HALF (4 closed): " << h.fourHalfCount << " patterns = " << h.fourHalfScore << " points (forced threat)\n";
                    if (h.threeOpenCount > 0) 
                        analysisLog << "          └─ THREE_OPEN (3 open): " << h.threeOpenCount << " patterns = " << h.threeOpenScore << " points (strong threat)\n";
                    if (h.twoOpenCount > 0) 
                        analysisLog << "          └─ TWO_OPEN: " << h.twoOpenCount << " patterns = " << h.twoOpenScore << " points (development)\n";
                }
            }
        }
        
        if (analysis.wasChosenAsRoot) {
            analysisLog << "        ★ FINAL DECISION: " << formatMove(analysis.move) 
                        << " with score " << analysis.score << "\n";
            analysisLog << "        ★ REASONING: " << analysis.breakdown.explanation << "\n";
            if (analysis.breakdown.isWinning) {
                analysisLog << "        ★ THIS IS A WINNING MOVE!\n";
            } else if (analysis.breakdown.isLosing) {
                analysisLog << "        ★ WARNING: This might be a losing move!\n";
            } else if (analysis.breakdown.isCriticalThreat) {
                analysisLog << "        ★ Critical threat situation!\n";
            }
        }
        
        count++;
    }
    
    analysisLog << "\nPERFORMANCE STATS:\n";
    analysisLog << "Time: " << lastSnapshot.totalTime << "ms\n";
    analysisLog << "Nodes evaluated: " << lastSnapshot.totalNodes << "\n";
    analysisLog << "Nodes per second: " << (lastSnapshot.totalTime > 0 ? (lastSnapshot.totalNodes * 1000 / lastSnapshot.totalTime) : 0) << "\n";
    analysisLog << std::string(80, '=') << "\n\n";
    
    logToFile(analysisLog.str());
    // Print full analysis to console so the user can see it in real-time
    std::cout << analysisLog.str();
}

void GameSnapshot::saveToFile(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Could not save snapshot to " << filename << std::endl;
        return;
    }
    
    file << "GOMOKU AI DETAILED SNAPSHOT\n";
    file << "===========================\n";
    file << "Turn: " << state.turnCount << "\n";
    file << "Current Player: " << (state.currentPlayer == GameState::PLAYER1 ? "HUMAN (O)" : "AI (X)") << "\n";
    file << "Game Phase: " << gamePhase << "\n";
    file << "Captures: HUMAN=" << state.captures[0] << " AI=" << state.captures[1] << "\n";
    file << "Time: " << totalTime << "ms\n";
    file << "Nodes: " << totalNodes << "\n";
    file << "Performance: " << (totalTime > 0 ? (totalNodes * 1000 / totalTime) : 0) << " nodes/second\n";
    
    if (!criticalThreats.empty()) {
        file << "⚠️  Critical Threats: " << criticalThreats << "\n";
    }
    
    file << "\nBOARD STATE:\n   ";
    for (int j = 0; j < GameState::BOARD_SIZE; j++) {
        file << std::setw(2) << char('A' + j) << " ";
    }
    file << "\n";
    
    for (int i = 0; i < GameState::BOARD_SIZE; i++) {
        file << std::setw(2) << (i + 1) << " ";
        for (int j = 0; j < GameState::BOARD_SIZE; j++) {
            char piece = '.';
            if (state.board[i][j] == GameState::PLAYER1) piece = 'O';
            else if (state.board[i][j] == GameState::PLAYER2) piece = 'X';
            file << piece << "  ";
        }
        file << "\n";
    }
    
    file << "\nTOP 10 MOVE ANALYSIS:\n";
    file << std::left << std::setw(8) << "Rank" 
         << std::setw(6) << "Move" 
         << std::setw(12) << "Score" 
         << std::setw(50) << "Reasoning" << "\n";
    file << std::string(80, '-') << "\n";
    
    int count = 0;
    for (const auto& moveAnalysis : topMoves) {
        if (count >= 10) break;
        
        std::string marker = moveAnalysis.wasChosenAsRoot ? "👑CHOSEN" : std::to_string(count + 1);
        
        file << std::left << std::setw(8) << marker
             << std::setw(6) << (char('A' + moveAnalysis.move.y)) << (moveAnalysis.move.x + 1)
             << std::setw(12) << moveAnalysis.score
             << moveAnalysis.reasoning << "\n";
        
        if (moveAnalysis.wasChosenAsRoot) {
            file << "        ★ FINAL DECISION with detailed breakdown\n";
            const auto& b = moveAnalysis.breakdown;
            file << "        └─ Pattern:" << b.patternScore 
                 << " Capture:" << b.captureScore
                 << " Threat:" << b.threatScore 
                 << " Position:" << b.positionScore << "\n";
                 
            const auto& h = b.heuristicDebug;
            if (h.threeOpenCount > 0 || h.fourHalfCount > 0 || h.fourOpenCount > 0 || h.twoOpenCount > 0) {
                file << "        ★ HEURISTIC BREAKDOWN:\n";
                if (h.fourOpenCount > 0) 
                    file << "          - FOUR_OPEN: " << h.fourOpenCount << " patterns = " << h.fourOpenScore << " points\n";
                if (h.fourHalfCount > 0) 
                    file << "          - FOUR_HALF (4 closed): " << h.fourHalfCount << " patterns = " << h.fourHalfScore << " points\n";
                if (h.threeOpenCount > 0) 
                    file << "          - THREE_OPEN (3 open): " << h.threeOpenCount << " patterns = " << h.threeOpenScore << " points\n";
                if (h.twoOpenCount > 0) 
                    file << "          - TWO_OPEN: " << h.twoOpenCount << " patterns = " << h.twoOpenScore << " points\n";
                if (!h.patternDetails.empty())
                    file << "          - Details: " << h.patternDetails << "\n";
            }
        }
        
        count++;
    }
    
    file.close();
}

void DebugAnalyzer::analyzeHeuristicPatterns(const GameState& state, int player, EvaluationBreakdown::HeuristicDebug& debug) {
    debug.threeOpenCount = Evaluator::countPatternType(state, player, 3, 2);
    debug.threeOpenScore = debug.threeOpenCount * Evaluator::THREE_OPEN;
    
    debug.fourHalfCount = Evaluator::countPatternType(state, player, 4, 1);
    debug.fourHalfScore = debug.fourHalfCount * Evaluator::FOUR_HALF;
    
    debug.fourOpenCount = Evaluator::countPatternType(state, player, 4, 2);
    debug.fourOpenScore = debug.fourOpenCount * Evaluator::FOUR_OPEN;
    
    debug.twoOpenCount = Evaluator::countPatternType(state, player, 2, 2);
    debug.twoOpenScore = debug.twoOpenCount * Evaluator::TWO_OPEN;
    
    std::ostringstream details;
    if (debug.fourOpenCount > 0) details << "4OPEN:" << debug.fourOpenCount << " ";
    if (debug.fourHalfCount > 0) details << "4HALF:" << debug.fourHalfCount << " ";
    if (debug.threeOpenCount > 0) details << "3OPEN:" << debug.threeOpenCount << " ";
    if (debug.twoOpenCount > 0) details << "2OPEN:" << debug.twoOpenCount << " ";
    
    debug.patternDetails = details.str();
}
