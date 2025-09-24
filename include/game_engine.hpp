/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   game_engine.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/14 21:22:22 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/24 17:49:47 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef GAME_ENGINE_HPP
#define GAME_ENGINE_HPP

#include "game_types.hpp"
#include "rule_engine.hpp"
#include "ai.hpp"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

struct ConditionalResponse {
    Move aiMove;           // Movimiento IA que se consideró
    Move humanMove;        // Movimiento humano esperado
    Move aiResponse;       // Mejor respuesta IA después de la secuencia
    int sequenceScore;     // Score de toda la secuencia
    uint64_t stateHash;    // Hash del estado después del movimiento humano
    
    ConditionalResponse() : sequenceScore(0), stateHash(0) {}
    ConditionalResponse(Move ai, Move human, Move response, int score, uint64_t hash)
        : aiMove(ai), humanMove(human), aiResponse(response), sequenceScore(score), stateHash(hash) {}
};

class GameEngine {
public:
    GameEngine() : ai(10) {
        // Iniciar hilo de background
        backgroundThread = std::thread(&GameEngine::backgroundCalculationLoop, this);
    }
    
    ~GameEngine() {
        shouldStop = true;
        backgroundCV.notify_all();
        if (backgroundThread.joinable()) {
            backgroundThread.join();
        }
    }

	GameEngine(const GameEngine&) = delete;
    GameEngine& operator=(const GameEngine&) = delete;
    
    void newGame();
    
    bool makeHumanMove(const Move& move);
    
    Move makeAIMove();
    
    const GameState& getState() const { return state; }
    
    bool isGameOver() const;
    int getWinner() const;
    
    void setAIDepth(int depth) { ai.setDepth(depth); }
    
    int getLastAIThinkingTime() const { return lastAITime; }
    int getLastNodesEvaluated() const { return ai.getLastNodesEvaluated(); }
    int getLastCacheHits() const { return ai.getLastCacheHits(); }
    float getLastCacheHitRate() const { return ai.getLastCacheHitRate(); }
    size_t getCacheSize() const { return ai.getCacheSize(); }
    
    void clearAICache() { ai.clearCache(); }
    
private:
    GameState state;
    AI ai;
    int lastAITime = 0;

	std::thread backgroundThread;
    std::mutex backgroundMutex;
    std::condition_variable backgroundCV;
    std::atomic<bool> shouldCalculate{false};
    std::atomic<bool> shouldStop{false};
    std::atomic<bool> isCalculating{false};
    
    // Resultado del cálculo anticipado
    Move backgroundBestMove;
    int backgroundScore = 0;
    bool hasBackgroundResult = false;
    
    void backgroundCalculationLoop();
    void startBackgroundCalculation();
    Move getBackgroundResult();

	std::vector<ConditionalResponse> conditionalCache;
    static constexpr int TOP_AI_MOVES = 3;     // Top 3 movimientos IA a considerar
    static constexpr int TOP_HUMAN_MOVES = 5;  // Top 5 movimientos humanos por cada IA move
    
    Move pendingAIDecision;  // NUEVO: La IA no decide hasta ver al humano
    bool hasPendingDecision = false;
	Move lastHumanMove;
    
    // NUEVO: Métodos para respuestas condicionadas
    void buildConditionalCache();
    Move findBestAIMoveAfterHuman(const Move& humanMove);
    int quickEvaluateUnexpectedMove(const Move& humanMove);
    std::vector<Move> getTopAIMoves(const GameState& state);
    std::vector<Move> getTopHumanMovesAfterAI(const GameState& state);

};

#endif