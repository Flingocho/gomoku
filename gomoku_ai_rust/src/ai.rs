// AI search engine - Similar to ai.hpp/cpp and transposition_search.hpp/cpp

use crate::game_types::*;
use crate::evaluator::Evaluator;
use crate::transposition_table::{TranspositionTable, CacheEntryType};
use crate::move_ordering::MoveOrdering;

pub struct AI {
    pub nodes_evaluated: u64,
    pub cache_hits: u64,
    transposition_table: TranspositionTable,
    previous_best_move: Move,
}

impl AI {
    pub fn new() -> Self {
        AI {
            nodes_evaluated: 0,
            cache_hits: 0,
            transposition_table: TranspositionTable::new(64), // 64 MB
            previous_best_move: Move::invalid(),
        }
    }

    pub fn get_best_move(&mut self, state: &GameState, max_depth: i32) -> Move {
        self.find_best_move_iterative(state, max_depth)
    }

    fn find_best_move_iterative(&mut self, state: &GameState, max_depth: i32) -> Move {
        self.nodes_evaluated = 0;
        self.cache_hits = 0;
        self.transposition_table.increment_generation();

        // Pre-check for immediate win
        let all_candidates = MoveOrdering::generate_candidates_adaptive_radius(state);
        for mv in &all_candidates {
            let mut test_state = state.clone();
            if test_state.make_move(*mv) {
                if test_state.is_game_over() {
                    println!("Immediate win detected at ({}, {})", mv.x, mv.y);
                    return *mv;
                }
            }
        }

        // CRITICAL: Check if opponent can win on their next move and block it!
        let opponent = state.get_opponent(state.current_player);
        for mv in &all_candidates {
            let mut test_state = state.clone();
            // Simulate opponent playing this move
            test_state.current_player = opponent;
            if test_state.make_move(*mv) {
                if test_state.is_game_over() {
                    println!("Blocking opponent win at ({}, {})", mv.x, mv.y);
                    return *mv;
                }
            }
        }

        let mut best_move = Move::invalid();

        // Iterative deepening
        for depth in 1..=max_depth {
            if best_move.is_valid() {
                self.previous_best_move = best_move;
            }

            let mut move_at_depth = Move::invalid();
            let score = self.minimax(
                &mut state.clone(),
                depth,
                i32::MIN,
                i32::MAX,
                state.current_player == PLAYER2,
                depth,
                Some(&mut move_at_depth)
            );

            best_move = move_at_depth;

            println!("Depth {}: score {} - {} nodes", depth, score, self.nodes_evaluated);

            // Early exit for mate
            if score.abs() > 300000 {
                println!("Mate detected at depth {}", depth);
                break;
            }
        }

        if best_move.is_valid() {
            best_move
        } else if !all_candidates.is_empty() {
            all_candidates[0]
        } else {
            Move::invalid()
        }
    }

    fn minimax(&mut self, state: &mut GameState, depth: i32, mut alpha: i32, mut beta: i32, maximizing: bool, original_max_depth: i32, best_move: Option<&mut Move>) -> i32 {
        self.nodes_evaluated += 1;

        // Transposition table lookup
        let zobrist_key = state.get_zobrist_hash();
        if let Some(entry) = self.transposition_table.lookup(zobrist_key) {
            self.cache_hits += 1;
            if entry.depth >= depth {
                match entry.entry_type {
                    CacheEntryType::Exact => {
                        if depth == original_max_depth {
                            if let Some(bm) = best_move {
                                *bm = entry.best_move;
                            }
                        }
                        return entry.score;
                    }
                    CacheEntryType::LowerBound => {
                        if entry.score >= beta {
                            return beta;
                        }
                    }
                    CacheEntryType::UpperBound => {
                        if entry.score <= alpha {
                            return alpha;
                        }
                    }
                }
            }
            if entry.best_move.is_valid() {
                self.previous_best_move = entry.best_move;
            }
        }

        // Base cases
        if depth == 0 || state.is_game_over() {
            let score = Evaluator::evaluate(state, original_max_depth, original_max_depth - depth);
            self.transposition_table.store(zobrist_key, score, depth, Move::invalid(), CacheEntryType::Exact);
            return score;
        }

        // Generate and order moves
        let moves = self.generate_ordered_moves(state);
        if moves.is_empty() {
            let score = Evaluator::evaluate(state, original_max_depth, original_max_depth - depth);
            self.transposition_table.store(zobrist_key, score, depth, Move::invalid(), CacheEntryType::Exact);
            return score;
        }

        let mut current_best_move = Move::invalid();
        let original_alpha = alpha;

        if maximizing {
            let mut max_eval = i32::MIN;

            for mv in &moves {
                if !state.make_move(*mv) {
                    continue;
                }

                let eval = self.minimax(state, depth - 1, alpha, beta, false, original_max_depth, None);
                state.unmake_move(*mv);

                if eval > max_eval {
                    max_eval = eval;
                    current_best_move = *mv;
                }

                alpha = alpha.max(eval);
                if beta <= alpha {
                    break; // Beta cutoff
                }
            }

            let entry_type = if max_eval <= original_alpha {
                CacheEntryType::UpperBound
            } else if max_eval >= beta {
                CacheEntryType::LowerBound
            } else {
                CacheEntryType::Exact
            };

            if !current_best_move.is_valid() && !moves.is_empty() {
                current_best_move = moves[0];
            }

            self.transposition_table.store(zobrist_key, max_eval, depth, current_best_move, entry_type);

            if depth == original_max_depth {
                if let Some(bm) = best_move {
                    *bm = current_best_move;
                }
            }

            max_eval
        } else {
            let mut min_eval = i32::MAX;

            for mv in &moves {
                if !state.make_move(*mv) {
                    continue;
                }

                let eval = self.minimax(state, depth - 1, alpha, beta, true, original_max_depth, None);
                state.unmake_move(*mv);

                if eval < min_eval {
                    min_eval = eval;
                    current_best_move = *mv;
                }

                beta = beta.min(eval);
                if beta <= alpha {
                    break; // Alpha cutoff
                }
            }

            let entry_type = if min_eval <= original_alpha {
                CacheEntryType::UpperBound
            } else if min_eval >= beta {
                CacheEntryType::LowerBound
            } else {
                CacheEntryType::Exact
            };

            if !current_best_move.is_valid() && !moves.is_empty() {
                current_best_move = moves[0];
            }

            self.transposition_table.store(zobrist_key, min_eval, depth, current_best_move, entry_type);

            if depth == original_max_depth {
                if let Some(bm) = best_move {
                    *bm = current_best_move;
                }
            }

            min_eval
        }
    }

    fn generate_ordered_moves(&mut self, state: &GameState) -> Vec<Move> {
        let mut candidates = MoveOrdering::generate_candidates_adaptive_radius(state);
        MoveOrdering::order_moves_with_previous_best(&mut candidates, state, self.previous_best_move);
        candidates
    }

    pub fn get_stats(&self) -> (u64, u64) {
        (self.nodes_evaluated, self.cache_hits)
    }
}
