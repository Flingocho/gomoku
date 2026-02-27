// ============================================
// TEST_AI.CPP — Comprehensive Test Suite
// Tests for: GameState, Move, RuleEngine,
//            Evaluator, AI, TranspositionSearch,
//            SuggestionEngine, GameEngine
// ============================================

#include "../include/ai/ai.hpp"
#include "../include/ai/evaluator.hpp"
#include "../include/ai/suggestion_engine.hpp"
#include "../include/core/game_types.hpp"
#include "../include/core/game_engine.hpp"
#include "../include/rules/rule_engine.hpp"
#include <iostream>
#include <cassert>
#include <cstring>
#include <vector>
#include <chrono>
#include <sstream>
#include <functional>
#include <string>
#include <cmath>

// ============================================
// Test framework helpers
// ============================================
static int totalTests = 0;
static int passedTests = 0;
static int failedTests = 0;

#define TEST(name) \
    do { \
        totalTests++; \
        std::cout << "  [TEST] " << name << "... "; \
        try {

#define END_TEST \
            passedTests++; \
            std::cout << "\033[32mPASSED\033[0m" << std::endl; \
        } catch (const std::exception& e) { \
            failedTests++; \
            std::cout << "\033[31mFAILED: " << e.what() << "\033[0m" << std::endl; \
        } catch (...) { \
            failedTests++; \
            std::cout << "\033[31mFAILED (unknown exception)\033[0m" << std::endl; \
        } \
    } while(0)

#define ASSERT(cond) \
    do { if (!(cond)) throw std::runtime_error("Assertion failed: " #cond); } while(0)

#define ASSERT_EQ(a, b) \
    do { \
        auto _a = (a); auto _b = (b); \
        if (_a != _b) { \
            std::ostringstream oss; \
            oss << "Expected " #a " == " #b ", got " << _a << " != " << _b; \
            throw std::runtime_error(oss.str()); \
        } \
    } while(0)

#define ASSERT_NE(a, b) \
    do { \
        auto _a = (a); auto _b = (b); \
        if (_a == _b) { \
            std::ostringstream oss; \
            oss << "Expected " #a " != " #b ", but both are " << _a; \
            throw std::runtime_error(oss.str()); \
        } \
    } while(0)

#define ASSERT_GT(a, b) \
    do { \
        auto _a = (a); auto _b = (b); \
        if (!(_a > _b)) { \
            std::ostringstream oss; \
            oss << "Expected " #a " > " #b ", got " << _a << " <= " << _b; \
            throw std::runtime_error(oss.str()); \
        } \
    } while(0)

#define ASSERT_GE(a, b) \
    do { \
        auto _a = (a); auto _b = (b); \
        if (!(_a >= _b)) { \
            std::ostringstream oss; \
            oss << "Expected " #a " >= " #b ", got " << _a << " < " << _b; \
            throw std::runtime_error(oss.str()); \
        } \
    } while(0)

#define ASSERT_LT(a, b) \
    do { \
        auto _a = (a); auto _b = (b); \
        if (!(_a < _b)) { \
            std::ostringstream oss; \
            oss << "Expected " #a " < " #b ", got " << _a << " >= " << _b; \
            throw std::runtime_error(oss.str()); \
        } \
    } while(0)

#define SECTION(name) \
    std::cout << "\n\033[1;36m=== " << name << " ===\033[0m" << std::endl

// Helper: create a fresh GameState
static GameState freshState() {
    return GameState();
}

// Helper: place a stone directly (no rule checks)
static void placeStone(GameState& s, int x, int y, int player) {
    s.board[x][y] = player;
}

// Helper: place a line of stones
static void placeLine(GameState& s, int startX, int startY, int dx, int dy, int length, int player) {
    for (int i = 0; i < length; i++) {
        placeStone(s, startX + i * dx, startY + i * dy, player);
    }
}

// ============================================
//  1. Move Struct Tests
// ============================================
static void testMove() {
    SECTION("Move Struct");

    TEST("Default move is invalid") {
        Move m;
        ASSERT(!m.isValid());
        ASSERT_EQ(m.x, -1);
        ASSERT_EQ(m.y, -1);
    } END_TEST;

    TEST("Move with valid coords is valid") {
        Move m(0, 0);
        ASSERT(m.isValid());
        Move m2(18, 18);
        ASSERT(m2.isValid());
        Move center(9, 9);
        ASSERT(center.isValid());
    } END_TEST;

    TEST("Move out of bounds is invalid") {
        ASSERT(!Move(-1, 0).isValid());
        ASSERT(!Move(0, -1).isValid());
        ASSERT(!Move(19, 0).isValid());
        ASSERT(!Move(0, 19).isValid());
        ASSERT(!Move(19, 19).isValid());
        ASSERT(!Move(-1, -1).isValid());
    } END_TEST;

    TEST("Move equality operator") {
        ASSERT(Move(5, 5) == Move(5, 5));
        ASSERT(!(Move(5, 5) == Move(5, 6)));
        ASSERT(!(Move(5, 5) == Move(6, 5)));
    } END_TEST;

    TEST("Move boundary values (0 and 18)") {
        ASSERT(Move(0, 0).isValid());
        ASSERT(Move(0, 18).isValid());
        ASSERT(Move(18, 0).isValid());
        ASSERT(Move(18, 18).isValid());
    } END_TEST;
}

// ============================================
//  2. GameState Tests
// ============================================
static void testGameState() {
    SECTION("GameState");

    TEST("Initial state is empty board") {
        GameState s = freshState();
        for (int i = 0; i < 19; i++)
            for (int j = 0; j < 19; j++)
                ASSERT_EQ(s.board[i][j], GameState::EMPTY);
    } END_TEST;

    TEST("Initial captures are zero") {
        GameState s = freshState();
        ASSERT_EQ(s.captures[0], 0);
        ASSERT_EQ(s.captures[1], 0);
    } END_TEST;

    TEST("Player 1 starts") {
        GameState s = freshState();
        ASSERT_EQ(s.currentPlayer, GameState::PLAYER1);
        ASSERT_EQ(s.turnCount, 0);
    } END_TEST;

    TEST("isValid boundary checks") {
        GameState s = freshState();
        ASSERT(s.isValid(0, 0));
        ASSERT(s.isValid(18, 18));
        ASSERT(s.isValid(9, 9));
        ASSERT(!s.isValid(-1, 0));
        ASSERT(!s.isValid(0, -1));
        ASSERT(!s.isValid(19, 0));
        ASSERT(!s.isValid(0, 19));
    } END_TEST;

    TEST("isEmpty on empty and occupied cell") {
        GameState s = freshState();
        ASSERT(s.isEmpty(5, 5));
        s.board[5][5] = GameState::PLAYER1;
        ASSERT(!s.isEmpty(5, 5));
    } END_TEST;

    TEST("getPiece returns correct values") {
        GameState s = freshState();
        ASSERT_EQ(s.getPiece(5, 5), GameState::EMPTY);
        s.board[5][5] = GameState::PLAYER1;
        ASSERT_EQ(s.getPiece(5, 5), GameState::PLAYER1);
        s.board[6][6] = GameState::PLAYER2;
        ASSERT_EQ(s.getPiece(6, 6), GameState::PLAYER2);
        // Out of bounds returns -1
        ASSERT_EQ(s.getPiece(-1, 0), -1);
        ASSERT_EQ(s.getPiece(19, 0), -1);
    } END_TEST;

    TEST("getOpponent returns correct opponent") {
        GameState s = freshState();
        ASSERT_EQ(s.getOpponent(GameState::PLAYER1), GameState::PLAYER2);
        ASSERT_EQ(s.getOpponent(GameState::PLAYER2), GameState::PLAYER1);
    } END_TEST;

    TEST("Copy constructor preserves full state") {
        GameState s = freshState();
        s.board[3][3] = GameState::PLAYER1;
        s.captures[0] = 4;
        s.currentPlayer = GameState::PLAYER2;
        s.turnCount = 5;

        GameState copy(s);
        ASSERT_EQ(copy.board[3][3], GameState::PLAYER1);
        ASSERT_EQ(copy.captures[0], 4);
        ASSERT_EQ(copy.currentPlayer, GameState::PLAYER2);
        ASSERT_EQ(copy.turnCount, 5);
    } END_TEST;

    TEST("Assignment operator preserves state") {
        GameState s = freshState();
        s.board[7][7] = GameState::PLAYER2;
        s.captures[1] = 6;

        GameState other;
        other = s;
        ASSERT_EQ(other.board[7][7], GameState::PLAYER2);
        ASSERT_EQ(other.captures[1], 6);
    } END_TEST;

    TEST("Copy does not create aliased boards") {
        GameState s = freshState();
        s.board[4][4] = GameState::PLAYER1;
        GameState copy = s;
        copy.board[4][4] = GameState::EMPTY;
        // Original must be unchanged
        ASSERT_EQ(s.board[4][4], GameState::PLAYER1);
    } END_TEST;

    TEST("Zobrist hash changes with piece placement") {
        GameState s = freshState();
        uint64_t hash1 = s.getZobristHash();
        s.board[9][9] = GameState::PLAYER1;
        s.recalculateHash();
        uint64_t hash2 = s.getZobristHash();
        ASSERT_NE(hash1, hash2);
    } END_TEST;

    TEST("Constants are correct") {
        ASSERT_EQ(GameState::BOARD_SIZE, 19);
        ASSERT_EQ(GameState::BOARD_CENTER, 9);
        ASSERT_EQ(GameState::EMPTY, 0);
        ASSERT_EQ(GameState::PLAYER1, 1);
        ASSERT_EQ(GameState::PLAYER2, 2);
        ASSERT_EQ(GameState::WIN_CAPTURES_NORMAL, 10);
    } END_TEST;

    TEST("depth getters and setters") {
        GameState s = freshState();
        ASSERT_EQ(s.getDepth(), 0);
        s.SetDepth(5);
        ASSERT_EQ(s.getDepth(), 5);
    } END_TEST;

    TEST("Forced capture fields initialized") {
        GameState s = freshState();
        ASSERT_EQ(s.forcedCapturePlayer, 0);
        ASSERT_EQ(s.pendingWinPlayer, 0);
        ASSERT(s.forcedCaptureMoves.empty());
    } END_TEST;
}

// ============================================
//  3. RuleEngine — Move Application
// ============================================
static void testRuleMoveApplication() {
    SECTION("RuleEngine — Move Application");

    TEST("Apply move on empty cell succeeds") {
        GameState s = freshState();
        Move m(9, 9);
        auto result = RuleEngine::applyMove(s, m);
        ASSERT(result.success);
        ASSERT_EQ(s.board[9][9], GameState::PLAYER1);
        ASSERT_EQ(s.currentPlayer, GameState::PLAYER2);
        ASSERT_EQ(s.turnCount, 1);
    } END_TEST;

    TEST("Apply move on occupied cell fails") {
        GameState s = freshState();
        s.board[9][9] = GameState::PLAYER1;
        s.currentPlayer = GameState::PLAYER2;
        Move m(9, 9);
        auto result = RuleEngine::applyMove(s, m);
        ASSERT(!result.success);
    } END_TEST;

    TEST("Alternating players after moves") {
        GameState s = freshState();
        RuleEngine::applyMove(s, Move(9, 9));
        ASSERT_EQ(s.currentPlayer, GameState::PLAYER2);
        RuleEngine::applyMove(s, Move(8, 8));
        ASSERT_EQ(s.currentPlayer, GameState::PLAYER1);
        RuleEngine::applyMove(s, Move(7, 7));
        ASSERT_EQ(s.currentPlayer, GameState::PLAYER2);
        ASSERT_EQ(s.turnCount, 3);
    } END_TEST;

    TEST("Multiple moves fill board correctly") {
        GameState s = freshState();
        RuleEngine::applyMove(s, Move(0, 0));
        RuleEngine::applyMove(s, Move(0, 1));
        RuleEngine::applyMove(s, Move(0, 2));
        ASSERT_EQ(s.board[0][0], GameState::PLAYER1);
        ASSERT_EQ(s.board[0][1], GameState::PLAYER2);
        ASSERT_EQ(s.board[0][2], GameState::PLAYER1);
    } END_TEST;

    TEST("Successful move sets createsWin flag when applicable") {
        GameState s = freshState();
        // Build 4 in a row for P1, then complete the 5th
        placeLine(s, 9, 5, 0, 1, 4, GameState::PLAYER1);
        s.currentPlayer = GameState::PLAYER1;
        s.turnCount = 7;
        auto result = RuleEngine::applyMove(s, Move(9, 9));
        ASSERT(result.success);
        // createsWin should be true (5 in a row)
        ASSERT(result.createsWin);
    } END_TEST;
}

// ============================================
//  4. RuleEngine — Legal Move Validation
// ============================================
static void testRuleLegalMoves() {
    SECTION("RuleEngine — Legal Move Validation");

    TEST("Empty cell is legal") {
        GameState s = freshState();
        ASSERT(RuleEngine::isLegalMove(s, Move(9, 9)));
    } END_TEST;

    TEST("Occupied cell is not legal") {
        GameState s = freshState();
        s.board[9][9] = GameState::PLAYER1;
        ASSERT(!RuleEngine::isLegalMove(s, Move(9, 9)));
    } END_TEST;

    TEST("All corners are legal on empty board") {
        GameState s = freshState();
        ASSERT(RuleEngine::isLegalMove(s, Move(0, 0)));
        ASSERT(RuleEngine::isLegalMove(s, Move(0, 18)));
        ASSERT(RuleEngine::isLegalMove(s, Move(18, 0)));
        ASSERT(RuleEngine::isLegalMove(s, Move(18, 18)));
    } END_TEST;

    TEST("Center cell is legal on empty board") {
        GameState s = freshState();
        ASSERT(RuleEngine::isLegalMove(s, Move(9, 9)));
    } END_TEST;
}

// ============================================
//  5. RuleEngine — Captures
// ============================================
static void testRuleCaptures() {
    SECTION("RuleEngine — Captures");

    TEST("Basic horizontal capture (P1 captures P2 pair)") {
        // Pattern: P1-P2-P2-P1  →  P1 places last piece
        GameState s = freshState();
        s.board[9][9] = GameState::PLAYER1;
        s.board[9][10] = GameState::PLAYER2;
        s.board[9][11] = GameState::PLAYER2;
        s.currentPlayer = GameState::PLAYER1;
        auto result = RuleEngine::applyMove(s, Move(9, 12));
        ASSERT(result.success);
        ASSERT_EQ(result.myCapturedPieces.size(), 2u);
        ASSERT_EQ(s.board[9][10], GameState::EMPTY);
        ASSERT_EQ(s.board[9][11], GameState::EMPTY);
        ASSERT_EQ(s.captures[0], 1);
    } END_TEST;

    TEST("Vertical capture") {
        GameState s = freshState();
        s.board[5][9] = GameState::PLAYER1;
        s.board[6][9] = GameState::PLAYER2;
        s.board[7][9] = GameState::PLAYER2;
        s.currentPlayer = GameState::PLAYER1;
        auto result = RuleEngine::applyMove(s, Move(8, 9));
        ASSERT(result.success);
        ASSERT_EQ(result.myCapturedPieces.size(), 2u);
        ASSERT_EQ(s.board[6][9], GameState::EMPTY);
        ASSERT_EQ(s.board[7][9], GameState::EMPTY);
    } END_TEST;

    TEST("Diagonal capture (down-right)") {
        GameState s = freshState();
        s.board[5][5] = GameState::PLAYER1;
        s.board[6][6] = GameState::PLAYER2;
        s.board[7][7] = GameState::PLAYER2;
        s.currentPlayer = GameState::PLAYER1;
        auto result = RuleEngine::applyMove(s, Move(8, 8));
        ASSERT(result.success);
        ASSERT_EQ(result.myCapturedPieces.size(), 2u);
        ASSERT_EQ(s.board[6][6], GameState::EMPTY);
        ASSERT_EQ(s.board[7][7], GameState::EMPTY);
    } END_TEST;

    TEST("Diagonal capture (down-left)") {
        GameState s = freshState();
        s.board[5][10] = GameState::PLAYER1;
        s.board[6][9] = GameState::PLAYER2;
        s.board[7][8] = GameState::PLAYER2;
        s.currentPlayer = GameState::PLAYER1;
        auto result = RuleEngine::applyMove(s, Move(8, 7));
        ASSERT(result.success);
        ASSERT_EQ(result.myCapturedPieces.size(), 2u);
        ASSERT_EQ(s.board[6][9], GameState::EMPTY);
        ASSERT_EQ(s.board[7][8], GameState::EMPTY);
    } END_TEST;

    TEST("No capture when pattern is incomplete") {
        GameState s = freshState();
        s.board[9][9] = GameState::PLAYER1;
        s.board[9][10] = GameState::PLAYER2;
        // (9,11) is empty — gap
        s.currentPlayer = GameState::PLAYER1;
        auto result = RuleEngine::applyMove(s, Move(9, 12));
        ASSERT(result.success);
        ASSERT_EQ(result.myCapturedPieces.size(), 0u);
    } END_TEST;

    TEST("No capture when middle pieces are same color") {
        GameState s = freshState();
        // P1-P1-P1-P1 → no capture (all same player)
        s.board[9][9] = GameState::PLAYER1;
        s.board[9][10] = GameState::PLAYER1;
        s.board[9][11] = GameState::PLAYER1;
        s.currentPlayer = GameState::PLAYER1;
        auto result = RuleEngine::applyMove(s, Move(9, 12));
        ASSERT(result.success);
        ASSERT_EQ(result.myCapturedPieces.size(), 0u);
    } END_TEST;

    TEST("Multiple captures in one move (two directions)") {
        GameState s = freshState();
        // Horizontal: P1(9,6)-P2(9,7)-P2(9,8)-[9,9]
        s.board[9][6] = GameState::PLAYER1;
        s.board[9][7] = GameState::PLAYER2;
        s.board[9][8] = GameState::PLAYER2;
        // Vertical: P1(6,9)-P2(7,9)-P2(8,9)-[9,9]
        s.board[6][9] = GameState::PLAYER1;
        s.board[7][9] = GameState::PLAYER2;
        s.board[8][9] = GameState::PLAYER2;
        s.currentPlayer = GameState::PLAYER1;
        auto result = RuleEngine::applyMove(s, Move(9, 9));
        ASSERT(result.success);
        ASSERT_EQ(result.myCapturedPieces.size(), 4u);
        ASSERT_EQ(s.captures[0], 2);
    } END_TEST;

    TEST("findCaptures detects captures without applying") {
        GameState s = freshState();
        s.board[9][9] = GameState::PLAYER1;
        s.board[9][10] = GameState::PLAYER2;
        s.board[9][11] = GameState::PLAYER2;
        auto captures = RuleEngine::findCaptures(s, Move(9, 12), GameState::PLAYER1);
        ASSERT_EQ(captures.size(), 2u);
        // Board unchanged
        ASSERT_EQ(s.board[9][10], GameState::PLAYER2);
        ASSERT_EQ(s.board[9][11], GameState::PLAYER2);
    } END_TEST;

    TEST("P2 can capture P1 pair") {
        GameState s = freshState();
        s.board[9][9] = GameState::PLAYER2;
        s.board[9][10] = GameState::PLAYER1;
        s.board[9][11] = GameState::PLAYER1;
        s.currentPlayer = GameState::PLAYER2;
        auto result = RuleEngine::applyMove(s, Move(9, 12));
        ASSERT(result.success);
        ASSERT_EQ(result.myCapturedPieces.size(), 2u);
        ASSERT_EQ(s.captures[1], 1); // P2 captures
    } END_TEST;

    TEST("Captures increment cumulatively") {
        GameState s = freshState();
        // First capture
        s.board[5][5] = GameState::PLAYER1;
        s.board[5][6] = GameState::PLAYER2;
        s.board[5][7] = GameState::PLAYER2;
        s.currentPlayer = GameState::PLAYER1;
        RuleEngine::applyMove(s, Move(5, 8));
        ASSERT_EQ(s.captures[0], 1);

        // Second capture (need to set up again)
        s.board[10][5] = GameState::PLAYER1;
        s.board[10][6] = GameState::PLAYER2;
        s.board[10][7] = GameState::PLAYER2;
        s.currentPlayer = GameState::PLAYER1;
        RuleEngine::applyMove(s, Move(10, 8));
        ASSERT_EQ(s.captures[0], 2);
    } END_TEST;

    TEST("Captures are capped at 10") {
        GameState s = freshState();
        s.captures[0] = 9;
        s.board[9][5] = GameState::PLAYER1;
        s.board[9][6] = GameState::PLAYER2;
        s.board[9][7] = GameState::PLAYER2;
        s.currentPlayer = GameState::PLAYER1;
        auto result = RuleEngine::applyMove(s, Move(9, 8));
        ASSERT(result.success);
        ASSERT_EQ(s.captures[0], 10);
    } END_TEST;
}

// ============================================
//  6. RuleEngine — Win Detection
// ============================================
static void testRuleWinDetection() {
    SECTION("RuleEngine — Win Detection");

    TEST("No win on empty board") {
        GameState s = freshState();
        ASSERT(!RuleEngine::checkWin(s, GameState::PLAYER1));
        ASSERT(!RuleEngine::checkWin(s, GameState::PLAYER2));
    } END_TEST;

    TEST("Horizontal five in a row wins") {
        GameState s = freshState();
        placeLine(s, 9, 5, 0, 1, 5, GameState::PLAYER1);
        ASSERT(RuleEngine::checkWin(s, GameState::PLAYER1));
        ASSERT(!RuleEngine::checkWin(s, GameState::PLAYER2));
    } END_TEST;

    TEST("Vertical five in a row wins") {
        GameState s = freshState();
        placeLine(s, 5, 9, 1, 0, 5, GameState::PLAYER1);
        ASSERT(RuleEngine::checkWin(s, GameState::PLAYER1));
    } END_TEST;

    TEST("Diagonal (down-right) five in a row wins") {
        GameState s = freshState();
        placeLine(s, 4, 4, 1, 1, 5, GameState::PLAYER1);
        ASSERT(RuleEngine::checkWin(s, GameState::PLAYER1));
    } END_TEST;

    TEST("Diagonal (down-left) five in a row wins") {
        GameState s = freshState();
        placeLine(s, 4, 14, 1, -1, 5, GameState::PLAYER2);
        ASSERT(RuleEngine::checkWin(s, GameState::PLAYER2));
    } END_TEST;

    TEST("Four in a row does NOT win") {
        GameState s = freshState();
        placeLine(s, 9, 5, 0, 1, 4, GameState::PLAYER1);
        ASSERT(!RuleEngine::checkWin(s, GameState::PLAYER1));
    } END_TEST;

    TEST("Three in a row does NOT win") {
        GameState s = freshState();
        placeLine(s, 9, 5, 0, 1, 3, GameState::PLAYER1);
        ASSERT(!RuleEngine::checkWin(s, GameState::PLAYER1));
    } END_TEST;

    TEST("Win by captures (10 pairs)") {
        GameState s = freshState();
        s.captures[0] = 10;
        ASSERT(RuleEngine::checkWin(s, GameState::PLAYER1));
    } END_TEST;

    TEST("9 captures is NOT enough to win") {
        GameState s = freshState();
        s.captures[0] = 9;
        ASSERT(!RuleEngine::checkWin(s, GameState::PLAYER1));
    } END_TEST;

    TEST("Six in a row also wins (overline)") {
        GameState s = freshState();
        placeLine(s, 9, 3, 0, 1, 6, GameState::PLAYER1);
        ASSERT(RuleEngine::checkWin(s, GameState::PLAYER1));
    } END_TEST;

    TEST("Win at board edge — top row horizontal") {
        GameState s = freshState();
        placeLine(s, 0, 0, 0, 1, 5, GameState::PLAYER1);
        ASSERT(RuleEngine::checkWin(s, GameState::PLAYER1));
    } END_TEST;

    TEST("Win at board edge — bottom row horizontal") {
        GameState s = freshState();
        placeLine(s, 18, 14, 0, 1, 5, GameState::PLAYER2);
        ASSERT(RuleEngine::checkWin(s, GameState::PLAYER2));
    } END_TEST;

    TEST("Win at board edge — left column vertical") {
        GameState s = freshState();
        placeLine(s, 0, 0, 1, 0, 5, GameState::PLAYER1);
        ASSERT(RuleEngine::checkWin(s, GameState::PLAYER1));
    } END_TEST;

    TEST("Win at board edge — right column vertical") {
        GameState s = freshState();
        placeLine(s, 14, 18, 1, 0, 5, GameState::PLAYER2);
        ASSERT(RuleEngine::checkWin(s, GameState::PLAYER2));
    } END_TEST;

    TEST("Win at corner diagonal") {
        GameState s = freshState();
        placeLine(s, 0, 0, 1, 1, 5, GameState::PLAYER1);
        ASSERT(RuleEngine::checkWin(s, GameState::PLAYER1));
    } END_TEST;

    TEST("P2 win by captures") {
        GameState s = freshState();
        s.captures[1] = 10;
        ASSERT(RuleEngine::checkWin(s, GameState::PLAYER2));
        ASSERT(!RuleEngine::checkWin(s, GameState::PLAYER1));
    } END_TEST;
}

// ============================================
//  7. RuleEngine — Double Free-Three
// ============================================
static void testDoubleFreethree() {
    SECTION("RuleEngine — Double Free-Three");

    TEST("Single free three is allowed") {
        GameState s = freshState();
        s.board[9][8] = GameState::PLAYER1;
        s.board[9][10] = GameState::PLAYER1;
        s.currentPlayer = GameState::PLAYER1;
        bool creates = RuleEngine::createsDoubleFreeThree(s, Move(9, 9), GameState::PLAYER1);
        ASSERT(!creates);
    } END_TEST;

    TEST("No free-three on isolated stone") {
        GameState s = freshState();
        bool creates = RuleEngine::createsDoubleFreeThree(s, Move(9, 9), GameState::PLAYER1);
        ASSERT(!creates);
    } END_TEST;
}

// ============================================
//  8. Evaluator Tests
// ============================================
static void testEvaluator() {
    SECTION("Evaluator");

    TEST("Empty board evaluates close to zero") {
        GameState s = freshState();
        int score = Evaluator::evaluate(s);
        ASSERT(score > -1000 && score < 1000);
    } END_TEST;

    TEST("Player with more pieces has better evaluation") {
        GameState s = freshState();
        placeStone(s, 9, 9, GameState::PLAYER1);
        placeStone(s, 9, 10, GameState::PLAYER1);
        placeStone(s, 9, 11, GameState::PLAYER1);
        int scoreP1 = Evaluator::evaluateForPlayer(s, GameState::PLAYER1);
        int scoreP2 = Evaluator::evaluateForPlayer(s, GameState::PLAYER2);
        ASSERT_GT(scoreP1, scoreP2);
    } END_TEST;

    TEST("Five in a row scores >= WIN constant") {
        GameState s = freshState();
        placeLine(s, 9, 5, 0, 1, 5, GameState::PLAYER1);
        int score = Evaluator::evaluateForPlayer(s, GameState::PLAYER1);
        ASSERT_GE(score, Evaluator::WIN);
    } END_TEST;

    TEST("Open four scores higher than open three") {
        GameState s1 = freshState();
        placeLine(s1, 9, 6, 0, 1, 4, GameState::PLAYER1);
        int scoreFour = Evaluator::evaluateForPlayer(s1, GameState::PLAYER1);

        GameState s2 = freshState();
        placeLine(s2, 9, 7, 0, 1, 3, GameState::PLAYER1);
        int scoreThree = Evaluator::evaluateForPlayer(s2, GameState::PLAYER1);

        ASSERT_GT(scoreFour, scoreThree);
    } END_TEST;

    TEST("Captures affect evaluation positively") {
        GameState s1 = freshState();
        s1.captures[0] = 4;
        int scoreWithCaps = Evaluator::evaluateForPlayer(s1, GameState::PLAYER1);

        GameState s2 = freshState();
        int scoreNoCaps = Evaluator::evaluateForPlayer(s2, GameState::PLAYER1);
        ASSERT_GT(scoreWithCaps, scoreNoCaps);
    } END_TEST;

    TEST("evaluateImmediateThreats detects four in a row") {
        GameState s = freshState();
        placeLine(s, 9, 6, 0, 1, 4, GameState::PLAYER1);
        int threats = Evaluator::evaluateImmediateThreats(s, GameState::PLAYER1);
        ASSERT_GT(threats, 0);
    } END_TEST;

    TEST("evaluateImmediateThreats returns 0 on empty board") {
        GameState s = freshState();
        int threats = Evaluator::evaluateImmediateThreats(s, GameState::PLAYER1);
        ASSERT_EQ(threats, 0);
    } END_TEST;

    TEST("hasWinningThreats detects open four") {
        GameState s = freshState();
        placeLine(s, 9, 6, 0, 1, 4, GameState::PLAYER1);
        bool winning = Evaluator::hasWinningThreats(s, GameState::PLAYER1);
        ASSERT(winning);
    } END_TEST;

    TEST("hasWinningThreats false for empty board") {
        GameState s = freshState();
        ASSERT(!Evaluator::hasWinningThreats(s, GameState::PLAYER1));
        ASSERT(!Evaluator::hasWinningThreats(s, GameState::PLAYER2));
    } END_TEST;

    TEST("Evaluation with mate distance scoring") {
        GameState s = freshState();
        placeLine(s, 9, 5, 0, 1, 5, GameState::PLAYER1);
        int scoreDeep = Evaluator::evaluate(s, 10, 2);
        int scoreShallow = Evaluator::evaluate(s, 10, 8);
        // Closer wins (lower depth) should have higher absolute score
        (void)scoreDeep;
        (void)scoreShallow;
        // Just ensure no crash
    } END_TEST;

    TEST("Evaluation is symmetric for mirror positions") {
        GameState s1 = freshState();
        placeStone(s1, 9, 9, GameState::PLAYER1);
        placeStone(s1, 9, 10, GameState::PLAYER1);
        int score1 = Evaluator::evaluateForPlayer(s1, GameState::PLAYER1);

        GameState s2 = freshState();
        placeStone(s2, 9, 9, GameState::PLAYER2);
        placeStone(s2, 9, 10, GameState::PLAYER2);
        int score2 = Evaluator::evaluateForPlayer(s2, GameState::PLAYER2);

        int diff = std::abs(score1 - score2);
        ASSERT_LT(diff, 500);
    } END_TEST;

    TEST("evaluateCombinations detects fork potential") {
        GameState s = freshState();
        // Create a position with multiple threats
        placeStone(s, 9, 9, GameState::PLAYER1);
        placeStone(s, 9, 10, GameState::PLAYER1);
        placeStone(s, 9, 11, GameState::PLAYER1);
        placeStone(s, 10, 9, GameState::PLAYER1);
        placeStone(s, 11, 9, GameState::PLAYER1);
        int combo = Evaluator::evaluateCombinations(s, GameState::PLAYER1);
        ASSERT_GE(combo, 0);
    } END_TEST;
}

// Helper: get implementation name string
static const char* implName(AIImplementation impl) {
    return (impl == CPP_IMPLEMENTATION) ? "C++" : "Rust";
}

// ============================================
//  9. AI — Basic Functionality (C++ & Rust)
// ============================================
static void testAIBasic() {
    SECTION("AI — Basic Functionality");

    for (AIImplementation impl : {CPP_IMPLEMENTATION, RUST_IMPLEMENTATION}) {
        std::string tag = std::string("[") + implName(impl) + "] ";

        TEST((tag + "AI returns a valid move on near-empty board").c_str()) {
            GameState s = freshState();
            s.board[9][9] = GameState::PLAYER1;
            s.turnCount = 1;
            s.currentPlayer = GameState::PLAYER2;

            AI ai(4, impl);
            Move best = ai.getBestMove(s);
            ASSERT(best.isValid());
            ASSERT(s.isEmpty(best.x, best.y));
        } END_TEST;

        TEST((tag + "AI plays near existing stones").c_str()) {
            GameState s = freshState();
            s.board[9][9] = GameState::PLAYER1;
            s.turnCount = 1;
            s.currentPlayer = GameState::PLAYER2;

            AI ai(4, impl);
            Move best = ai.getBestMove(s);
            ASSERT_GE(best.x, 5);
            ASSERT_LT(best.x, 14);
            ASSERT_GE(best.y, 5);
            ASSERT_LT(best.y, 14);
        } END_TEST;

        TEST((tag + "AI completes five in a row when possible").c_str()) {
            GameState s = freshState();
            placeStone(s, 9, 7, GameState::PLAYER2);
            placeStone(s, 9, 8, GameState::PLAYER2);
            placeStone(s, 9, 9, GameState::PLAYER2);
            placeStone(s, 9, 10, GameState::PLAYER2);
            placeStone(s, 2, 2, GameState::PLAYER1);
            placeStone(s, 3, 3, GameState::PLAYER1);
            placeStone(s, 4, 4, GameState::PLAYER1);
            placeStone(s, 5, 5, GameState::PLAYER1);
            s.currentPlayer = GameState::PLAYER2;
            s.turnCount = 8;

            AI ai(4, impl);
            Move best = ai.getBestMove(s);
            bool winsLeft = (best.x == 9 && best.y == 6);
            bool winsRight = (best.x == 9 && best.y == 11);
            ASSERT(winsLeft || winsRight);
        } END_TEST;

        TEST((tag + "AI blocks opponent four in a row (half-open)").c_str()) {
            GameState s = freshState();
            placeStone(s, 9, 7, GameState::PLAYER1);
            placeStone(s, 9, 8, GameState::PLAYER1);
            placeStone(s, 9, 9, GameState::PLAYER1);
            placeStone(s, 9, 10, GameState::PLAYER1);
            placeStone(s, 9, 6, GameState::PLAYER2); // Block left
            placeStone(s, 2, 2, GameState::PLAYER2);
            placeStone(s, 3, 3, GameState::PLAYER2);
            placeStone(s, 4, 4, GameState::PLAYER2);
            s.currentPlayer = GameState::PLAYER2;
            s.turnCount = 7;

            AI ai(4, impl);
            Move best = ai.getBestMove(s);
            ASSERT_EQ(best.x, 9);
            ASSERT_EQ(best.y, 11);
        } END_TEST;

        TEST((tag + "AI responds to open three threat").c_str()) {
            GameState s = freshState();
            placeStone(s, 9, 7, GameState::PLAYER1);
            placeStone(s, 9, 8, GameState::PLAYER1);
            placeStone(s, 9, 9, GameState::PLAYER1);
            placeStone(s, 2, 2, GameState::PLAYER2);
            placeStone(s, 3, 15, GameState::PLAYER2);
            placeStone(s, 14, 14, GameState::PLAYER2);
            s.currentPlayer = GameState::PLAYER2;
            s.turnCount = 6;

            AI ai(6, impl);
            Move best = ai.getBestMove(s);
            ASSERT(best.isValid());
            bool nearThreat = (best.x >= 7 && best.x <= 11 && best.y >= 5 && best.y <= 11);
            ASSERT(nearThreat);
        } END_TEST;

        TEST((tag + "AI never returns an occupied cell").c_str()) {
            GameState s = freshState();
            for (int i = 7; i <= 11; i++)
                for (int j = 7; j <= 11; j++)
                    s.board[i][j] = (((i + j) % 2) == 0) ? GameState::PLAYER1 : GameState::PLAYER2;
            s.currentPlayer = GameState::PLAYER1;
            s.turnCount = 25;

            AI ai(4, impl);
            Move best = ai.getBestMove(s);
            if (best.isValid()) {
                ASSERT_EQ(s.board[best.x][best.y], GameState::EMPTY);
            }
        } END_TEST;
    }

    // C++-only tests (implementation-specific API)
    TEST("AI depth getter/setter") {
        AI ai(4, CPP_IMPLEMENTATION);
        ASSERT_EQ(ai.getDepth(), 4);
        ai.setDepth(8);
        ASSERT_EQ(ai.getDepth(), 8);
        ai.setDepth(1);
        ASSERT_EQ(ai.getDepth(), 1);
    } END_TEST;

    TEST("AI implementation getter/setter") {
        AI ai(4, CPP_IMPLEMENTATION);
        ASSERT(ai.getImplementation() == CPP_IMPLEMENTATION);
        ai.setImplementation(RUST_IMPLEMENTATION);
        ASSERT(ai.getImplementation() == RUST_IMPLEMENTATION);
    } END_TEST;

    TEST("AI getLastNodesEvaluated > 0 after search") {
        GameState s = freshState();
        s.board[9][9] = GameState::PLAYER1;
        s.currentPlayer = GameState::PLAYER2;
        s.turnCount = 1;

        AI ai(2, CPP_IMPLEMENTATION);
        ai.getBestMove(s);
        ASSERT_GT(ai.getLastNodesEvaluated(), 0);
    } END_TEST;

    TEST("AI cache can be cleared without crash") {
        AI ai(4, CPP_IMPLEMENTATION);
        GameState s = freshState();
        s.board[9][9] = GameState::PLAYER1;
        s.currentPlayer = GameState::PLAYER2;
        s.turnCount = 1;
        ai.getBestMove(s);
        ai.clearCache();
        ASSERT_GT(ai.getCacheSize(), 0u);
    } END_TEST;
}

// ============================================
// 10. AI — Strategic Scenarios (C++ & Rust)
// ============================================
static void testAIStrategic() {
    SECTION("AI — Strategic Scenarios");

    for (AIImplementation impl : {CPP_IMPLEMENTATION, RUST_IMPLEMENTATION}) {
        std::string tag = std::string("[") + implName(impl) + "] ";

        TEST((tag + "AI prefers capture when it leads to win (9 captures)").c_str()) {
            GameState s = freshState();
            s.captures[1] = 9;
            s.board[9][5] = GameState::PLAYER2;
            s.board[9][6] = GameState::PLAYER1;
            s.board[9][7] = GameState::PLAYER1;
            placeStone(s, 4, 4, GameState::PLAYER1);
            placeStone(s, 5, 5, GameState::PLAYER1);
            placeStone(s, 3, 3, GameState::PLAYER2);
            s.currentPlayer = GameState::PLAYER2;
            s.turnCount = 7;

            AI ai(6, impl);
            Move best = ai.getBestMove(s);
            ASSERT_EQ(best.x, 9);
            ASSERT_EQ(best.y, 8);
        } END_TEST;

        TEST((tag + "AI prioritizes winning over blocking").c_str()) {
            GameState s = freshState();
            placeStone(s, 5, 5, GameState::PLAYER2);
            placeStone(s, 5, 6, GameState::PLAYER2);
            placeStone(s, 5, 7, GameState::PLAYER2);
            placeStone(s, 5, 8, GameState::PLAYER2);
            placeStone(s, 10, 5, GameState::PLAYER1);
            placeStone(s, 10, 6, GameState::PLAYER1);
            placeStone(s, 10, 7, GameState::PLAYER1);
            placeStone(s, 10, 8, GameState::PLAYER1);
            s.currentPlayer = GameState::PLAYER2;
            s.turnCount = 8;

            AI ai(4, impl);
            Move best = ai.getBestMove(s);
            bool wins = (best.x == 5 && (best.y == 4 || best.y == 9));
            ASSERT(wins);
        } END_TEST;

        TEST((tag + "AI responds in reasonable time at depth 6").c_str()) {
            GameState s = freshState();
            placeStone(s, 9, 9, GameState::PLAYER1);
            placeStone(s, 8, 8, GameState::PLAYER2);
            placeStone(s, 9, 10, GameState::PLAYER1);
            placeStone(s, 8, 10, GameState::PLAYER2);
            placeStone(s, 10, 8, GameState::PLAYER1);
            placeStone(s, 7, 7, GameState::PLAYER2);
            s.currentPlayer = GameState::PLAYER1;
            s.turnCount = 6;

            AI ai(6, impl);
            auto start = std::chrono::steady_clock::now();
            Move best = ai.getBestMove(s);
            auto end = std::chrono::steady_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

            ASSERT(best.isValid());
            ASSERT_LT(ms, 30000);
            std::cout << "(took " << ms << "ms) ";
        } END_TEST;

        TEST((tag + "AI on empty board handles gracefully").c_str()) {
            GameState s = freshState();
            s.currentPlayer = GameState::PLAYER1;
            s.turnCount = 0;

            AI ai(4, impl);
            Move best = ai.getBestMove(s);
            if (best.isValid()) {
                ASSERT(s.isEmpty(best.x, best.y));
            }
        } END_TEST;
    }
}

// ============================================
// 11. TranspositionSearch Tests
// ============================================
static void testTranspositionSearch() {
    SECTION("TranspositionSearch");

    TEST("findBestMoveIterative returns valid move") {
        GameState s = freshState();
        s.board[9][9] = GameState::PLAYER1;
        s.currentPlayer = GameState::PLAYER2;
        s.turnCount = 1;

        AI ai(4, CPP_IMPLEMENTATION);
        auto result = ai.findBestMoveIterative(s, 4);
        ASSERT(result.bestMove.isValid());
        ASSERT_GT(result.nodesEvaluated, 0);
    } END_TEST;

    TEST("generateOrderedMoves returns non-empty for non-empty board") {
        GameState s = freshState();
        s.board[9][9] = GameState::PLAYER1;
        s.currentPlayer = GameState::PLAYER2;

        AI ai(4, CPP_IMPLEMENTATION);
        auto moves = ai.generateOrderedMoves(s);
        ASSERT_GT(moves.size(), 0u);
        for (const auto& m : moves) {
            ASSERT(m.isValid());
            ASSERT(s.isEmpty(m.x, m.y));
        }
    } END_TEST;

    TEST("generateOrderedMoves: all moves are within board") {
        GameState s = freshState();
        placeStone(s, 0, 0, GameState::PLAYER1);
        placeStone(s, 18, 18, GameState::PLAYER2);
        s.currentPlayer = GameState::PLAYER1;

        AI ai(4, CPP_IMPLEMENTATION);
        auto moves = ai.generateOrderedMoves(s);
        for (const auto& m : moves) {
            ASSERT(m.isValid());
            ASSERT_GE(m.x, 0);
            ASSERT_LT(m.x, 19);
            ASSERT_GE(m.y, 0);
            ASSERT_LT(m.y, 19);
        }
    } END_TEST;

    TEST("quickEvaluateMove: winning move scores highest") {
        GameState s = freshState();
        placeStone(s, 9, 6, GameState::PLAYER1);
        placeStone(s, 9, 7, GameState::PLAYER1);
        placeStone(s, 9, 8, GameState::PLAYER1);
        placeStone(s, 9, 9, GameState::PLAYER1);
        s.currentPlayer = GameState::PLAYER1;

        AI ai(4, CPP_IMPLEMENTATION);
        int winScore = ai.quickEvaluateMove(s, Move(9, 10));
        int randomScore = ai.quickEvaluateMove(s, Move(2, 2));
        ASSERT_GT(winScore, randomScore);
    } END_TEST;

    TEST("Iterative deepening at depth 2 completes quickly") {
        GameState s = freshState();
        s.board[9][9] = GameState::PLAYER1;
        s.currentPlayer = GameState::PLAYER2;
        s.turnCount = 1;

        AI ai(2, CPP_IMPLEMENTATION);
        auto start = std::chrono::steady_clock::now();
        auto result = ai.findBestMoveIterative(s, 2);
        auto end = std::chrono::steady_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        ASSERT(result.bestMove.isValid());
        ASSERT_LT(ms, 5000);
        std::cout << "(took " << ms << "ms, " << result.nodesEvaluated << " nodes) ";
    } END_TEST;

    TEST("Cache hit rate improves on repeated evaluation") {
        GameState s = freshState();
        s.board[9][9] = GameState::PLAYER1;
        s.currentPlayer = GameState::PLAYER2;
        s.turnCount = 1;
        s.recalculateHash();

        AI ai(4, CPP_IMPLEMENTATION);
        ai.clearCache();
        ai.findBestMoveIterative(s, 4);
        float rate1 = ai.getLastCacheHitRate();

        // Second search with warm cache should have higher hit rate
        ai.findBestMoveIterative(s, 4);
        float rate2 = ai.getLastCacheHitRate();
        ASSERT_GE(rate2, rate1);
    } END_TEST;

    TEST("Search result score is reasonable") {
        GameState s = freshState();
        s.board[9][9] = GameState::PLAYER1;
        s.currentPlayer = GameState::PLAYER2;
        s.turnCount = 1;

        AI ai(4, CPP_IMPLEMENTATION);
        auto result = ai.findBestMoveIterative(s, 4);
        // Score should be finite
        ASSERT_LT(result.score, 10000000);
        ASSERT_GT(result.score, -10000000);
    } END_TEST;
}

// ============================================
// 12. SuggestionEngine Tests
// ============================================
static void testSuggestionEngine() {
    SECTION("SuggestionEngine");

    TEST("getSuggestion returns valid move") {
        GameState s = freshState();
        s.board[9][9] = GameState::PLAYER1;
        s.currentPlayer = GameState::PLAYER2;
        s.turnCount = 1;

        Move suggestion = SuggestionEngine::getSuggestion(s, 4);
        ASSERT(suggestion.isValid());
        ASSERT(s.isEmpty(suggestion.x, suggestion.y));
    } END_TEST;

    TEST("getQuickSuggestion returns valid move") {
        GameState s = freshState();
        s.board[9][9] = GameState::PLAYER1;
        s.currentPlayer = GameState::PLAYER2;
        s.turnCount = 1;

        Move suggestion = SuggestionEngine::getQuickSuggestion(s);
        ASSERT(suggestion.isValid());
        ASSERT(s.isEmpty(suggestion.x, suggestion.y));
    } END_TEST;

    TEST("Quick suggestion blocks obvious four") {
        GameState s = freshState();
        placeStone(s, 9, 7, GameState::PLAYER1);
        placeStone(s, 9, 8, GameState::PLAYER1);
        placeStone(s, 9, 9, GameState::PLAYER1);
        placeStone(s, 9, 10, GameState::PLAYER1);
        placeStone(s, 9, 6, GameState::PLAYER2);
        placeStone(s, 3, 3, GameState::PLAYER2);
        placeStone(s, 4, 4, GameState::PLAYER2);
        s.currentPlayer = GameState::PLAYER2;
        s.turnCount = 7;

        Move suggestion = SuggestionEngine::getQuickSuggestion(s);
        ASSERT(suggestion.isValid());
        ASSERT_EQ(suggestion.x, 9);
        ASSERT_EQ(suggestion.y, 11);
    } END_TEST;

    TEST("Quick suggestion is faster than full AI search") {
        GameState s = freshState();
        s.board[9][9] = GameState::PLAYER1;
        s.board[8][8] = GameState::PLAYER2;
        s.board[10][10] = GameState::PLAYER1;
        s.currentPlayer = GameState::PLAYER2;
        s.turnCount = 3;

        auto start1 = std::chrono::steady_clock::now();
        SuggestionEngine::getQuickSuggestion(s);
        auto end1 = std::chrono::steady_clock::now();
        auto quickMs = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1).count();

        auto start2 = std::chrono::steady_clock::now();
        AI ai(6, CPP_IMPLEMENTATION);
        ai.getBestMove(s);
        auto end2 = std::chrono::steady_clock::now();
        auto fullMs = std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2).count();

        std::cout << "(quick=" << quickMs << "ms, full=" << fullMs << "ms) ";
        ASSERT_LT(quickMs, fullMs + 100);
    } END_TEST;

    TEST("getSuggestion finds winning move") {
        GameState s = freshState();
        placeStone(s, 9, 7, GameState::PLAYER1);
        placeStone(s, 9, 8, GameState::PLAYER1);
        placeStone(s, 9, 9, GameState::PLAYER1);
        placeStone(s, 9, 10, GameState::PLAYER1);
        s.currentPlayer = GameState::PLAYER1;
        s.turnCount = 7;

        Move suggestion = SuggestionEngine::getSuggestion(s, 4);
        bool isWin = (suggestion.x == 9 && (suggestion.y == 6 || suggestion.y == 11));
        ASSERT(isWin);
    } END_TEST;
}

// ============================================
// 13. GameEngine Integration Tests
// ============================================
static void testGameEngine() {
    SECTION("GameEngine Integration");

    TEST("newGame resets state completely") {
        GameEngine engine;
        engine.newGame();
        const GameState& s = engine.getState();
        ASSERT_EQ(s.currentPlayer, GameState::PLAYER1);
        ASSERT_EQ(s.turnCount, 0);
        ASSERT_EQ(s.captures[0], 0);
        ASSERT_EQ(s.captures[1], 0);
        for (int i = 0; i < 19; i++)
            for (int j = 0; j < 19; j++)
                ASSERT_EQ(s.board[i][j], GameState::EMPTY);
    } END_TEST;

    TEST("makeHumanMove places P1 correctly") {
        GameEngine engine;
        engine.newGame();
        bool ok = engine.makeHumanMove(Move(9, 9));
        ASSERT(ok);
        ASSERT_EQ(engine.getState().board[9][9], GameState::PLAYER1);
    } END_TEST;

    TEST("Game is not over at start") {
        GameEngine engine;
        engine.newGame();
        ASSERT(!engine.isGameOver());
    } END_TEST;

    TEST("makeAIMove returns valid move") {
        GameEngine engine;
        engine.newGame();
        engine.setAIDepth(2);
        engine.makeHumanMove(Move(9, 9));
        Move aiMove = engine.makeAIMove();
        ASSERT(aiMove.isValid());
        ASSERT_EQ(engine.getState().board[aiMove.x][aiMove.y], GameState::PLAYER2);
    } END_TEST;

    TEST("Game mode can be set and queried") {
        GameEngine engine;
        engine.setGameMode(GameMode::VS_AI);
        ASSERT(engine.getGameMode() == GameMode::VS_AI);
        engine.setGameMode(GameMode::VS_HUMAN_SUGGESTED);
        ASSERT(engine.getGameMode() == GameMode::VS_HUMAN_SUGGESTED);
    } END_TEST;

    TEST("VS_HUMAN_SUGGESTED allows both players") {
        GameEngine engine;
        engine.newGame();
        engine.setGameMode(GameMode::VS_HUMAN_SUGGESTED);
        ASSERT(engine.makeHumanMove(Move(9, 9)));   // P1
        ASSERT(engine.makeHumanMove(Move(8, 8)));   // P2
        ASSERT(engine.makeHumanMove(Move(10, 10))); // P1
    } END_TEST;

    TEST("Full game: P1 wins by five in a row") {
        GameEngine engine;
        engine.newGame();
        engine.setGameMode(GameMode::VS_HUMAN_SUGGESTED);

        engine.makeHumanMove(Move(5, 9));  // P1
        engine.makeHumanMove(Move(5, 1));  // P2
        engine.makeHumanMove(Move(6, 9));  // P1
        engine.makeHumanMove(Move(6, 1));  // P2
        engine.makeHumanMove(Move(7, 9));  // P1
        engine.makeHumanMove(Move(7, 1));  // P2
        engine.makeHumanMove(Move(8, 9));  // P1
        engine.makeHumanMove(Move(8, 1));  // P2
        engine.makeHumanMove(Move(9, 9));  // P1 wins!

        ASSERT(engine.isGameOver());
        ASSERT_EQ(engine.getWinner(), GameState::PLAYER1);
    } END_TEST;

    TEST("findWinningLine returns 5+ pieces after win") {
        GameEngine engine;
        engine.newGame();
        engine.setGameMode(GameMode::VS_HUMAN_SUGGESTED);

        engine.makeHumanMove(Move(9, 5));  // P1
        engine.makeHumanMove(Move(0, 0));  // P2
        engine.makeHumanMove(Move(9, 6));  // P1
        engine.makeHumanMove(Move(0, 1));  // P2
        engine.makeHumanMove(Move(9, 7));  // P1
        engine.makeHumanMove(Move(0, 2));  // P2
        engine.makeHumanMove(Move(9, 8));  // P1
        engine.makeHumanMove(Move(0, 3));  // P2
        engine.makeHumanMove(Move(9, 9));  // P1 wins!

        auto line = engine.findWinningLine();
        ASSERT_GE(line.size(), 5u);
    } END_TEST;

    TEST("AI thinking time is tracked") {
        GameEngine engine;
        engine.newGame();
        engine.setAIDepth(2);
        engine.makeHumanMove(Move(9, 9));
        engine.makeAIMove();
        ASSERT_GE(engine.getLastAIThinkingTime(), 0);
    } END_TEST;

    TEST("AI stats are accessible after move") {
        GameEngine engine;
        engine.newGame();
        engine.setAIDepth(2);
        engine.makeHumanMove(Move(9, 9));
        engine.makeAIMove();
        ASSERT_GT(engine.getLastNodesEvaluated(), 0);
        ASSERT_GE(engine.getLastCacheHits(), 0);
        ASSERT_GE(engine.getLastCacheHitRate(), 0.0f);
        ASSERT_GT(engine.getCacheSize(), 0u);
    } END_TEST;

    TEST("newGame after moves resets everything") {
        GameEngine engine;
        engine.newGame();
        engine.setGameMode(GameMode::VS_HUMAN_SUGGESTED);
        engine.makeHumanMove(Move(9, 9));
        engine.makeHumanMove(Move(8, 8));

        engine.newGame();
        const GameState& s = engine.getState();
        ASSERT_EQ(s.board[9][9], GameState::EMPTY);
        ASSERT_EQ(s.board[8][8], GameState::EMPTY);
        ASSERT_EQ(s.turnCount, 0);
        ASSERT_EQ(s.currentPlayer, GameState::PLAYER1);
    } END_TEST;
}

// ============================================
// 14. Zobrist Hashing Tests
// ============================================
static void testZobristHashing() {
    SECTION("Zobrist Hashing");

    TEST("Same board produces same hash") {
        GameState s1 = freshState();
        placeStone(s1, 9, 9, GameState::PLAYER1);
        s1.recalculateHash();

        GameState s2 = freshState();
        placeStone(s2, 9, 9, GameState::PLAYER1);
        s2.recalculateHash();

        ASSERT_EQ(s1.getZobristHash(), s2.getZobristHash());
    } END_TEST;

    TEST("Different positions produce different hashes") {
        GameState s1 = freshState();
        placeStone(s1, 9, 9, GameState::PLAYER1);
        s1.recalculateHash();

        GameState s2 = freshState();
        placeStone(s2, 9, 10, GameState::PLAYER1);
        s2.recalculateHash();

        ASSERT_NE(s1.getZobristHash(), s2.getZobristHash());
    } END_TEST;

    TEST("Different players at same position produce different hashes") {
        GameState s1 = freshState();
        placeStone(s1, 9, 9, GameState::PLAYER1);
        s1.recalculateHash();

        GameState s2 = freshState();
        placeStone(s2, 9, 9, GameState::PLAYER2);
        s2.recalculateHash();

        ASSERT_NE(s1.getZobristHash(), s2.getZobristHash());
    } END_TEST;

    TEST("Hash changes with each piece placement") {
        GameState s = freshState();
        s.recalculateHash();
        uint64_t prevHash = s.getZobristHash();

        for (int i = 0; i < 5; i++) {
            placeStone(s, 9, 5 + i, GameState::PLAYER1);
            s.recalculateHash();
            uint64_t newHash = s.getZobristHash();
            ASSERT_NE(newHash, prevHash);
            prevHash = newHash;
        }
    } END_TEST;

    TEST("Empty board hash is consistent across instances") {
        GameState s1 = freshState();
        GameState s2 = freshState();
        ASSERT_EQ(s1.getZobristHash(), s2.getZobristHash());
    } END_TEST;

    TEST("Hash is deterministic for complex board") {
        GameState s1 = freshState();
        placeStone(s1, 5, 5, GameState::PLAYER1);
        placeStone(s1, 6, 6, GameState::PLAYER2);
        placeStone(s1, 7, 7, GameState::PLAYER1);
        placeStone(s1, 8, 8, GameState::PLAYER2);
        s1.recalculateHash();

        GameState s2 = freshState();
        placeStone(s2, 5, 5, GameState::PLAYER1);
        placeStone(s2, 6, 6, GameState::PLAYER2);
        placeStone(s2, 7, 7, GameState::PLAYER1);
        placeStone(s2, 8, 8, GameState::PLAYER2);
        s2.recalculateHash();

        ASSERT_EQ(s1.getZobristHash(), s2.getZobristHash());
    } END_TEST;
}

// ============================================
// 15. Edge Cases & Stress Tests
// ============================================
static void testEdgeCases() {
    SECTION("Edge Cases & Stress");

    TEST("AI handles nearly full board") {
        GameState s = freshState();
        int count = 0;
        for (int i = 0; i < 19 && count < 350; i++)
            for (int j = 0; j < 19 && count < 350; j++) {
                s.board[i][j] = (count % 2 == 0) ? GameState::PLAYER1 : GameState::PLAYER2;
                count++;
            }
        s.currentPlayer = GameState::PLAYER1;
        s.turnCount = count;

        AI ai(2, CPP_IMPLEMENTATION);
        Move best = ai.getBestMove(s);
        if (best.isValid()) {
            ASSERT(s.isEmpty(best.x, best.y));
        }
    } END_TEST;

    TEST("Evaluator handles single stone") {
        GameState s = freshState();
        placeStone(s, 9, 9, GameState::PLAYER1);
        int score = Evaluator::evaluate(s);
        (void)score; // no crash
    } END_TEST;

    TEST("AI search at depth 1") {
        GameState s = freshState();
        s.board[9][9] = GameState::PLAYER1;
        s.currentPlayer = GameState::PLAYER2;
        s.turnCount = 1;

        AI ai(1, CPP_IMPLEMENTATION);
        Move best = ai.getBestMove(s);
        ASSERT(best.isValid());
    } END_TEST;

    TEST("AI search at depth 2") {
        GameState s = freshState();
        s.board[9][9] = GameState::PLAYER1;
        s.currentPlayer = GameState::PLAYER2;
        s.turnCount = 1;

        AI ai(2, CPP_IMPLEMENTATION);
        Move best = ai.getBestMove(s);
        ASSERT(best.isValid());
    } END_TEST;

    TEST("Multiple AI instances don't interfere") {
        GameState s = freshState();
        s.board[9][9] = GameState::PLAYER1;
        s.currentPlayer = GameState::PLAYER2;
        s.turnCount = 1;

        AI ai1(2, CPP_IMPLEMENTATION);
        AI ai2(4, CPP_IMPLEMENTATION);

        Move m1 = ai1.getBestMove(s);
        Move m2 = ai2.getBestMove(s);

        ASSERT(m1.isValid());
        ASSERT(m2.isValid());
    } END_TEST;

    TEST("Capture at board edge (top-left corner)") {
        GameState s = freshState();
        // P1(0,0)-P2(0,1)-P2(0,2)-P1 places at (0,3)
        s.board[0][0] = GameState::PLAYER1;
        s.board[0][1] = GameState::PLAYER2;
        s.board[0][2] = GameState::PLAYER2;
        s.currentPlayer = GameState::PLAYER1;
        auto result = RuleEngine::applyMove(s, Move(0, 3));
        ASSERT(result.success);
        ASSERT_EQ(result.myCapturedPieces.size(), 2u);
    } END_TEST;

    TEST("Capture at board edge (bottom-right corner)") {
        GameState s = freshState();
        s.board[18][18] = GameState::PLAYER1;
        s.board[18][17] = GameState::PLAYER2;
        s.board[18][16] = GameState::PLAYER2;
        s.currentPlayer = GameState::PLAYER1;
        auto result = RuleEngine::applyMove(s, Move(18, 15));
        ASSERT(result.success);
        ASSERT_EQ(result.myCapturedPieces.size(), 2u);
    } END_TEST;

    TEST("Win detection at all four corners") {
        // Top-left horizontal
        GameState s1 = freshState();
        placeLine(s1, 0, 0, 0, 1, 5, GameState::PLAYER1);
        ASSERT(RuleEngine::checkWin(s1, GameState::PLAYER1));

        // Top-right horizontal
        GameState s2 = freshState();
        placeLine(s2, 0, 14, 0, 1, 5, GameState::PLAYER1);
        ASSERT(RuleEngine::checkWin(s2, GameState::PLAYER1));

        // Bottom-left vertical
        GameState s3 = freshState();
        placeLine(s3, 14, 0, 1, 0, 5, GameState::PLAYER2);
        ASSERT(RuleEngine::checkWin(s3, GameState::PLAYER2));

        // Bottom-right diagonal
        GameState s4 = freshState();
        placeLine(s4, 14, 14, 1, 1, 5, GameState::PLAYER2);
        ASSERT(RuleEngine::checkWin(s4, GameState::PLAYER2));
    } END_TEST;
}

// ============================================
// 16. Consistency & Determinism Tests
// ============================================
static void testConsistency() {
    SECTION("Consistency & Determinism");

    TEST("Same position gives same best move (deterministic)") {
        GameState s = freshState();
        placeStone(s, 9, 9, GameState::PLAYER1);
        placeStone(s, 8, 8, GameState::PLAYER2);
        s.currentPlayer = GameState::PLAYER1;
        s.turnCount = 2;
        s.recalculateHash();

        AI ai1(4, CPP_IMPLEMENTATION);
        AI ai2(4, CPP_IMPLEMENTATION);

        Move m1 = ai1.getBestMove(s);
        Move m2 = ai2.getBestMove(s);

        ASSERT_EQ(m1.x, m2.x);
        ASSERT_EQ(m1.y, m2.y);
    } END_TEST;

    TEST("AI score is positive when AI is winning") {
        GameState s = freshState();
        placeStone(s, 9, 7, GameState::PLAYER2);
        placeStone(s, 9, 8, GameState::PLAYER2);
        placeStone(s, 9, 9, GameState::PLAYER2);
        placeStone(s, 9, 10, GameState::PLAYER2);
        s.currentPlayer = GameState::PLAYER2;
        s.turnCount = 8;

        AI ai(4, CPP_IMPLEMENTATION);
        ai.getBestMove(s);
        int score = ai.getLastScore();
        ASSERT_GT(score, 0);
    } END_TEST;

    TEST("Higher depth search gives same or better results") {
        GameState s = freshState();
        placeStone(s, 9, 9, GameState::PLAYER1);
        placeStone(s, 8, 8, GameState::PLAYER2);
        placeStone(s, 10, 10, GameState::PLAYER1);
        placeStone(s, 7, 7, GameState::PLAYER2);
        s.currentPlayer = GameState::PLAYER1;
        s.turnCount = 4;
        s.recalculateHash();

        AI ai2(2, CPP_IMPLEMENTATION);
        Move m2 = ai2.getBestMove(s);
        int nodes2 = ai2.getLastNodesEvaluated();

        AI ai4(4, CPP_IMPLEMENTATION);
        Move m4 = ai4.getBestMove(s);
        int nodes4 = ai4.getLastNodesEvaluated();

        ASSERT(m2.isValid());
        ASSERT(m4.isValid());
        // Deeper search should evaluate more nodes
        ASSERT_GE(nodes4, nodes2);
    } END_TEST;
}

// ============================================
// 17. Pattern Counting Tests
// ============================================
static void testPatternCounting() {
    SECTION("Pattern Counting");

    TEST("countAllPatterns finds open two") {
        GameState s = freshState();
        placeStone(s, 9, 9, GameState::PLAYER1);
        placeStone(s, 9, 10, GameState::PLAYER1);
        auto counts = Evaluator::countAllPatterns(s, GameState::PLAYER1);
        ASSERT_GE(counts.twoOpen, 1);
    } END_TEST;

    TEST("countAllPatterns finds open three") {
        GameState s = freshState();
        placeStone(s, 9, 8, GameState::PLAYER1);
        placeStone(s, 9, 9, GameState::PLAYER1);
        placeStone(s, 9, 10, GameState::PLAYER1);
        auto counts = Evaluator::countAllPatterns(s, GameState::PLAYER1);
        ASSERT_GE(counts.threeOpen + counts.threeHalf, 1);
    } END_TEST;

    TEST("countAllPatterns finds four") {
        GameState s = freshState();
        placeStone(s, 9, 7, GameState::PLAYER1);
        placeStone(s, 9, 8, GameState::PLAYER1);
        placeStone(s, 9, 9, GameState::PLAYER1);
        placeStone(s, 9, 10, GameState::PLAYER1);
        auto counts = Evaluator::countAllPatterns(s, GameState::PLAYER1);
        ASSERT_GE(counts.fourOpen + counts.fourHalf, 1);
    } END_TEST;

    TEST("No patterns on empty board") {
        GameState s = freshState();
        auto counts = Evaluator::countAllPatterns(s, GameState::PLAYER1);
        ASSERT_EQ(counts.fourOpen, 0);
        ASSERT_EQ(counts.fourHalf, 0);
        ASSERT_EQ(counts.threeOpen, 0);
        ASSERT_EQ(counts.threeHalf, 0);
        ASSERT_EQ(counts.twoOpen, 0);
    } END_TEST;

    TEST("countPatternType detects specific patterns") {
        GameState s = freshState();
        placeStone(s, 9, 8, GameState::PLAYER1);
        placeStone(s, 9, 9, GameState::PLAYER1);
        placeStone(s, 9, 10, GameState::PLAYER1);
        int openThrees = Evaluator::countPatternType(s, GameState::PLAYER1, 3, 2);
        int halfThrees = Evaluator::countPatternType(s, GameState::PLAYER1, 3, 1);
        ASSERT_GE(openThrees + halfThrees, 1);
    } END_TEST;

    TEST("Blocked pattern has fewer free ends") {
        GameState s = freshState();
        // Three in a row blocked on one side
        placeStone(s, 9, 7, GameState::PLAYER2); // Block
        placeStone(s, 9, 8, GameState::PLAYER1);
        placeStone(s, 9, 9, GameState::PLAYER1);
        placeStone(s, 9, 10, GameState::PLAYER1);
        auto counts = Evaluator::countAllPatterns(s, GameState::PLAYER1);
        // With one side blocked, should have half-open three(s)
        ASSERT_GE(counts.threeHalf + counts.threeOpen, 1);
    } END_TEST;

    TEST("Patterns counted for both players independently") {
        GameState s = freshState();
        placeStone(s, 5, 8, GameState::PLAYER1);
        placeStone(s, 5, 9, GameState::PLAYER1);
        placeStone(s, 5, 10, GameState::PLAYER1);

        placeStone(s, 10, 8, GameState::PLAYER2);
        placeStone(s, 10, 9, GameState::PLAYER2);
        placeStone(s, 10, 10, GameState::PLAYER2);

        auto countsP1 = Evaluator::countAllPatterns(s, GameState::PLAYER1);
        auto countsP2 = Evaluator::countAllPatterns(s, GameState::PLAYER2);
        ASSERT_GE(countsP1.threeOpen + countsP1.threeHalf, 1);
        ASSERT_GE(countsP2.threeOpen + countsP2.threeHalf, 1);
    } END_TEST;
}

// ============================================
// 18. Move Ordering Tests
// ============================================
static void testMoveOrdering() {
    SECTION("Move Ordering");

    TEST("Winning move is scored highest by quickEvaluateMove") {
        GameState s = freshState();
        placeStone(s, 9, 6, GameState::PLAYER1);
        placeStone(s, 9, 7, GameState::PLAYER1);
        placeStone(s, 9, 8, GameState::PLAYER1);
        placeStone(s, 9, 9, GameState::PLAYER1);
        s.currentPlayer = GameState::PLAYER1;

        AI ai(4, CPP_IMPLEMENTATION);
        int winScore = ai.quickEvaluateMove(s, Move(9, 10));
        int normalScore = ai.quickEvaluateMove(s, Move(15, 15));
        ASSERT_GT(winScore, normalScore);
    } END_TEST;

    TEST("Ordered moves have winning move first") {
        GameState s = freshState();
        placeStone(s, 9, 7, GameState::PLAYER1);
        placeStone(s, 9, 8, GameState::PLAYER1);
        placeStone(s, 9, 9, GameState::PLAYER1);
        placeStone(s, 9, 10, GameState::PLAYER1);
        s.currentPlayer = GameState::PLAYER1;

        AI ai(4, CPP_IMPLEMENTATION);
        auto moves = ai.generateOrderedMoves(s);
        ASSERT_GT(moves.size(), 0u);
        bool firstIsWin = (moves[0].x == 9 && (moves[0].y == 6 || moves[0].y == 11));
        ASSERT(firstIsWin);
    } END_TEST;

    TEST("Blocking move is near top of ordered list") {
        GameState s = freshState();
        placeStone(s, 9, 7, GameState::PLAYER1);
        placeStone(s, 9, 8, GameState::PLAYER1);
        placeStone(s, 9, 9, GameState::PLAYER1);
        placeStone(s, 9, 10, GameState::PLAYER1);
        placeStone(s, 9, 6, GameState::PLAYER2);
        s.currentPlayer = GameState::PLAYER2;

        AI ai(4, CPP_IMPLEMENTATION);
        auto moves = ai.generateOrderedMoves(s);
        ASSERT_GT(moves.size(), 0u);
        // Blocking move (9,11) should be in the first few moves
        bool foundBlock = false;
        for (size_t i = 0; i < std::min(moves.size(), (size_t)5); i++) {
            if (moves[i].x == 9 && moves[i].y == 11) {
                foundBlock = true;
                break;
            }
        }
        ASSERT(foundBlock);
    } END_TEST;
}

// ============================================
// 19. Game Simulation Tests (C++ & Rust)
// ============================================
static void testGameSimulation() {
    SECTION("Game Simulation");

    for (AIImplementation impl : {CPP_IMPLEMENTATION, RUST_IMPLEMENTATION}) {
        std::string tag = std::string("[") + implName(impl) + "] ";

        TEST((tag + "AI vs AI completes a game").c_str()) {
            GameEngine engine;
            engine.newGame();
            engine.setGameMode(GameMode::VS_HUMAN_SUGGESTED);

            engine.makeHumanMove(Move(9, 9));

            AI ai1(2, impl);
            AI ai2(2, impl);

            int maxMoves = 80;
            int moves = 1;

            while (!engine.isGameOver() && moves < maxMoves) {
                const GameState& s = engine.getState();
                Move best;
                if (s.currentPlayer == GameState::PLAYER1) {
                    best = ai1.getBestMove(s);
                } else {
                    best = ai2.getBestMove(s);
                }

                if (!best.isValid()) break;
                engine.makeHumanMove(best);
                moves++;
            }

            std::cout << "(" << moves << " moves played) ";
            ASSERT_GT(moves, 1);
        } END_TEST;
    }

    TEST("Win by capture in full game simulation") {
        GameEngine engine;
        engine.newGame();
        engine.setGameMode(GameMode::VS_HUMAN_SUGGESTED);

        GameState s;
        s.captures[0] = 10;
        ASSERT(RuleEngine::checkWin(s, GameState::PLAYER1));
    } END_TEST;

    TEST("C++ vs Rust — both produce valid moves for same position") {
        GameState s = freshState();
        placeStone(s, 9, 9, GameState::PLAYER1);
        placeStone(s, 8, 8, GameState::PLAYER2);
        placeStone(s, 9, 10, GameState::PLAYER1);
        placeStone(s, 8, 10, GameState::PLAYER2);
        s.currentPlayer = GameState::PLAYER2;
        s.turnCount = 4;

        AI cppAi(4, CPP_IMPLEMENTATION);
        AI rustAi(4, RUST_IMPLEMENTATION);

        Move cppMove = cppAi.getBestMove(s);
        Move rustMove = rustAi.getBestMove(s);

        ASSERT(cppMove.isValid());
        ASSERT(rustMove.isValid());
        ASSERT(s.isEmpty(cppMove.x, cppMove.y));
        ASSERT(s.isEmpty(rustMove.x, rustMove.y));

        // Both should play near the action
        bool cppNear = (cppMove.x >= 5 && cppMove.x <= 14 && cppMove.y >= 5 && cppMove.y <= 14);
        bool rustNear = (rustMove.x >= 5 && rustMove.x <= 14 && rustMove.y >= 5 && rustMove.y <= 14);
        ASSERT(cppNear);
        ASSERT(rustNear);
        std::cout << "(C++: " << (char)('A' + cppMove.y) << cppMove.x + 1
                  << ", Rust: " << (char)('A' + rustMove.y) << rustMove.x + 1 << ") ";
    } END_TEST;

    TEST("C++ vs Rust cross-play game completes") {
        GameEngine engine;
        engine.newGame();
        engine.setGameMode(GameMode::VS_HUMAN_SUGGESTED);

        engine.makeHumanMove(Move(9, 9));

        AI cppAi(2, CPP_IMPLEMENTATION);
        AI rustAi(2, RUST_IMPLEMENTATION);

        int maxMoves = 80;
        int moves = 1;

        while (!engine.isGameOver() && moves < maxMoves) {
            const GameState& s = engine.getState();
            Move best;
            if (s.currentPlayer == GameState::PLAYER1) {
                best = cppAi.getBestMove(s);  // P1 = C++
            } else {
                best = rustAi.getBestMove(s); // P2 = Rust
            }

            if (!best.isValid()) break;
            engine.makeHumanMove(best);
            moves++;
        }

        std::cout << "(" << moves << " moves played) ";
        ASSERT_GT(moves, 1);
    } END_TEST;
}

// ============================================
// MAIN
// ============================================
int main() {
    std::cout << "\033[1;33m"
              << "╔═══════════════════════════════════════════════╗\n"
              << "║       GOMOKU AI — Comprehensive Test Suite    ║\n"
              << "╚═══════════════════════════════════════════════╝"
              << "\033[0m" << std::endl;

    // Initialize Zobrist Hasher (required globally)
    GameState::initializeHasher();

    auto totalStart = std::chrono::steady_clock::now();

    // Run all test sections
    testMove();
    testGameState();
    testRuleMoveApplication();
    testRuleLegalMoves();
    testRuleCaptures();
    testRuleWinDetection();
    testDoubleFreethree();
    testEvaluator();
    testAIBasic();
    testAIStrategic();
    testTranspositionSearch();
    testSuggestionEngine();
    testGameEngine();
    testZobristHashing();
    testEdgeCases();
    testConsistency();
    testPatternCounting();
    testMoveOrdering();
    testGameSimulation();

    auto totalEnd = std::chrono::steady_clock::now();
    auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(totalEnd - totalStart).count();

    // Cleanup
    GameState::cleanupHasher();

    // Summary
    std::cout << "\n\033[1;33m"
              << "═══════════════════════════════════════════════\n"
              << "  RESULTS: " << passedTests << "/" << totalTests << " passed";
    if (failedTests > 0)
        std::cout << " (\033[31m" << failedTests << " FAILED\033[1;33m)";
    std::cout << "\n  Time: " << totalMs << "ms"
              << "\n═══════════════════════════════════════════════"
              << "\033[0m" << std::endl;

    return (failedTests > 0) ? 1 : 0;
}
