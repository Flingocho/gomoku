// ============================================
// AI Engine — Exact match of search_minimax.cpp
//   + transposition_search.hpp (search wrapper)
// Minimax with alpha-beta, LMR, killer moves,
//   history heuristic, iterative deepening
// ============================================

use crate::evaluator::{Evaluator, WIN};
use crate::game_types::*;
use crate::move_ordering::MoveOrdering;
use crate::rule_engine::RuleEngine;
use crate::transposition_table::*;

pub struct AI {
    tt: TranspositionTable,
    history_table: [[i32; BOARD_SIZE]; BOARD_SIZE],
    killer_moves: [[Move; 2]; MAX_SEARCH_DEPTH],
    previous_best_move: Move,
    pub nodes_evaluated: i32,
    pub cache_hits: i32,
}

impl AI {
    pub fn new() -> Self {
        AI {
            tt: TranspositionTable::new(),
            history_table: [[0i32; BOARD_SIZE]; BOARD_SIZE],
            killer_moves: [[Move::invalid(); 2]; MAX_SEARCH_DEPTH],
            previous_best_move: Move::invalid(),
            nodes_evaluated: 0,
            cache_hits: 0,
        }
    }

    // ============================================
    // ITERATIVE DEEPENING — findBestMoveIterative
    // ============================================

    pub fn find_best_move_iterative(&mut self, state: &GameState, max_depth: i32) -> SearchResult {
        let mut result = SearchResult::new();

        // Pre-check: immediate victory
        let candidates = MoveOrdering::generate_ordered_moves(
            state,
            &Move::invalid(),
            &self.history_table,
        );

        for mv in &candidates {
            let mut new_state = state.clone();
            let move_result = RuleEngine::apply_move(&mut new_state, mv);
            if move_result.success && move_result.creates_win {
                result.best_move = *mv;
                result.score = WIN;
                result.depth_searched = 1;
                return result;
            }
        }

        // Iterative deepening
        let mut best_move = Move::invalid();

        for depth in 1..=max_depth {
            self.nodes_evaluated = 0;
            self.cache_hits = 0;
            self.tt.increment_generation();

            let mut current_best = Move::invalid();
            let maximizing = state.current_player == PLAYER2;

            let score = self.minimax(
                state,
                depth,
                i32::MIN + 1,
                i32::MAX - 1,
                maximizing,
                depth,
                &mut current_best,
            );

            if current_best.is_valid() {
                best_move = current_best;
                self.previous_best_move = current_best;
            }

            result.best_move = best_move;
            result.score = score;
            result.depth_searched = depth;
            result.nodes_evaluated = self.nodes_evaluated;
            result.cache_hits = self.cache_hits;

            // Early exit if winning/losing position found
            if score.abs() > 300000 {
                break;
            }

            // Age history table between iterations
            for i in 0..BOARD_SIZE {
                for j in 0..BOARD_SIZE {
                    self.history_table[i][j] >>= 1;
                }
            }
        }

        result
    }

    // ============================================
    // MINIMAX — with alpha-beta, LMR, killer moves
    // ============================================

    fn minimax(
        &mut self,
        state: &GameState,
        depth: i32,
        mut alpha: i32,
        mut beta: i32,
        maximizing: bool,
        original_max_depth: i32,
        best_move_out: &mut Move,
    ) -> i32 {
        self.nodes_evaluated += 1;

        // Base case: terminal evaluation
        if depth <= 0 {
            let current_depth = original_max_depth - depth;
            return Evaluator::evaluate(state, original_max_depth, current_depth);
        }

        // Transposition table lookup
        let tt_key = state.zobrist_hash;
        if let Some((cached_score, cached_move)) = self.tt.lookup(tt_key, depth, alpha, beta) {
            self.cache_hits += 1;
            *best_move_out = cached_move;
            return cached_score;
        }

        // Get cached best move for move ordering
        let tt_best_move = self.tt.get_best_move(tt_key).unwrap_or(Move::invalid());

        // Generate and order moves
        let prev_best = if tt_best_move.is_valid() {
            tt_best_move
        } else {
            self.previous_best_move
        };

        let mut moves =
            MoveOrdering::generate_ordered_moves(state, &prev_best, &self.history_table);

        // Promote killer moves
        let depth_idx = depth as usize;
        if depth_idx < MAX_SEARCH_DEPTH {
            for k in (0..2).rev() {
                let killer = self.killer_moves[depth_idx][k];
                if killer.is_valid() {
                    if let Some(pos) = moves.iter().position(|m| *m == killer) {
                        let km = moves.remove(pos);
                        moves.insert(0.min(moves.len()), km);
                    }
                }
            }
        }

        if moves.is_empty() {
            let current_depth = original_max_depth - depth;
            return Evaluator::evaluate(state, original_max_depth, current_depth);
        }

        let mut best_score;
        let mut local_best_move = Move::invalid();
        let mut move_index = 0;

        if maximizing {
            best_score = i32::MIN + 1;

            for mv in &moves {
                let mut new_state = state.clone();
                let move_result = RuleEngine::apply_move(&mut new_state, mv);

                if !move_result.success {
                    continue;
                }

                if move_result.creates_win {
                    let current_depth = original_max_depth - depth;
                    let win_score = WIN - (original_max_depth - current_depth);
                    *best_move_out = *mv;

                    // Store in TT
                    self.tt.store(
                        tt_key,
                        win_score,
                        depth,
                        *mv,
                        CacheEntryType::Exact,
                    );

                    return win_score;
                }

                let mut score;
                let mut dummy_move = Move::invalid();

                // Late Move Reduction (LMR)
                let use_lmr = move_index >= 2 && depth >= 3;

                if use_lmr {
                    // Reduced-depth search first
                    score = self.minimax(
                        &new_state,
                        depth - 2,
                        alpha,
                        alpha + 1,
                        false,
                        original_max_depth,
                        &mut dummy_move,
                    );

                    // Re-search at full depth if score is promising
                    if score > alpha {
                        score = self.minimax(
                            &new_state,
                            depth - 1,
                            alpha,
                            beta,
                            false,
                            original_max_depth,
                            &mut dummy_move,
                        );
                    }
                } else {
                    score = self.minimax(
                        &new_state,
                        depth - 1,
                        alpha,
                        beta,
                        false,
                        original_max_depth,
                        &mut dummy_move,
                    );
                }

                if score > best_score {
                    best_score = score;
                    local_best_move = *mv;
                }

                if best_score > alpha {
                    alpha = best_score;
                }

                if alpha >= beta {
                    // Beta cutoff — update killer moves and history
                    if depth_idx < MAX_SEARCH_DEPTH {
                        if self.killer_moves[depth_idx][0] != *mv {
                            self.killer_moves[depth_idx][1] = self.killer_moves[depth_idx][0];
                            self.killer_moves[depth_idx][0] = *mv;
                        }
                    }
                    self.history_table[mv.x as usize][mv.y as usize] += depth * depth;
                    break;
                }

                move_index += 1;
            }
        } else {
            // Minimizing
            best_score = i32::MAX - 1;

            for mv in &moves {
                let mut new_state = state.clone();
                let move_result = RuleEngine::apply_move(&mut new_state, mv);

                if !move_result.success {
                    continue;
                }

                if move_result.creates_win {
                    let current_depth = original_max_depth - depth;
                    let win_score = -WIN + (original_max_depth - current_depth);
                    *best_move_out = *mv;

                    self.tt.store(
                        tt_key,
                        win_score,
                        depth,
                        *mv,
                        CacheEntryType::Exact,
                    );

                    return win_score;
                }

                let mut score;
                let mut dummy_move = Move::invalid();

                // Late Move Reduction (LMR)
                let use_lmr = move_index >= 2 && depth >= 3;

                if use_lmr {
                    score = self.minimax(
                        &new_state,
                        depth - 2,
                        beta - 1,
                        beta,
                        true,
                        original_max_depth,
                        &mut dummy_move,
                    );

                    if score < beta {
                        score = self.minimax(
                            &new_state,
                            depth - 1,
                            alpha,
                            beta,
                            true,
                            original_max_depth,
                            &mut dummy_move,
                        );
                    }
                } else {
                    score = self.minimax(
                        &new_state,
                        depth - 1,
                        alpha,
                        beta,
                        true,
                        original_max_depth,
                        &mut dummy_move,
                    );
                }

                if score < best_score {
                    best_score = score;
                    local_best_move = *mv;
                }

                if best_score < beta {
                    beta = best_score;
                }

                if alpha >= beta {
                    // Alpha cutoff — update killer moves and history
                    if depth_idx < MAX_SEARCH_DEPTH {
                        if self.killer_moves[depth_idx][0] != *mv {
                            self.killer_moves[depth_idx][1] = self.killer_moves[depth_idx][0];
                            self.killer_moves[depth_idx][0] = *mv;
                        }
                    }
                    self.history_table[mv.x as usize][mv.y as usize] += depth * depth;
                    break;
                }

                move_index += 1;
            }
        }

        // Store in transposition table
        let entry_type = if best_score <= alpha {
            CacheEntryType::UpperBound
        } else if best_score >= beta {
            CacheEntryType::LowerBound
        } else {
            CacheEntryType::Exact
        };

        if local_best_move.is_valid() {
            *best_move_out = local_best_move;
        }

        self.tt
            .store(tt_key, best_score, depth, local_best_move, entry_type);

        best_score
    }
}
