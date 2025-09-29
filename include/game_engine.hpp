/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   game_engine.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/14 21:22:22 by jainavas          #+#    #+#             */
/*   Updated: 2025/09/29 19:55:40 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef GAME_ENGINE_HPP
#define GAME_ENGINE_HPP

#include "game_types.hpp"
#include "rule_engine.hpp"
#include "ai.hpp"

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

enum class GameMode {
    VS_AI,
    VS_HUMAN_SUGGESTED
};

class GameEngine {
public:
    GameEngine() : ai(10), currentMode(GameMode::VS_AI) {}
    
    ~GameEngine() = default;

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
	void setGameMode(GameMode mode) { currentMode = mode; }
    GameMode getGameMode() const { return currentMode; }
    
private:
    GameState state;
    AI ai;
    int lastAITime = 0;
    
    Move lastHumanMove;
	GameMode currentMode;
};

#endif