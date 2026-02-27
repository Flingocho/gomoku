// ============================================
// Transposition Table — Exact match of
//   transposition_search.hpp / search_transposition.cpp
// Vec-based with power-of-2 sizing and mask indexing
// ============================================

use crate::game_types::*;

pub const MAX_SEARCH_DEPTH: usize = 20;

#[derive(Clone, Copy, PartialEq, Eq)]
pub enum CacheEntryType {
    Exact,
    LowerBound,
    UpperBound,
}

#[derive(Clone)]
pub struct CacheEntry {
    pub zobrist_key: u64,
    pub score: i32,
    pub depth: i32,
    pub best_move: Move,
    pub generation: u32,
    pub entry_type: CacheEntryType,
}

impl CacheEntry {
    fn empty() -> Self {
        CacheEntry {
            zobrist_key: 0,
            score: 0,
            depth: 0,
            best_move: Move::invalid(),
            generation: 0,
            entry_type: CacheEntryType::Exact,
        }
    }
}

pub struct SearchResult {
    pub best_move: Move,
    pub score: i32,
    pub depth_searched: i32,
    pub nodes_evaluated: i32,
    pub cache_hits: i32,
}

impl SearchResult {
    pub fn new() -> Self {
        SearchResult {
            best_move: Move::invalid(),
            score: 0,
            depth_searched: 0,
            nodes_evaluated: 0,
            cache_hits: 0,
        }
    }
}

pub struct TranspositionTable {
    table: Vec<CacheEntry>,
    table_size_mask: usize,
    current_generation: u32,
}

impl TranspositionTable {
    pub fn new() -> Self {
        // Power-of-2 sizing: 1 << 20 = 1048576 entries, same as C++
        let size: usize = 1 << 20;
        let mut table = Vec::with_capacity(size);
        for _ in 0..size {
            table.push(CacheEntry::empty());
        }

        TranspositionTable {
            table,
            table_size_mask: size - 1,
            current_generation: 0,
        }
    }

    pub fn lookup(
        &mut self, key: u64, depth: i32, alpha: i32, beta: i32,
    ) -> Option<(i32, Move)> {
        let index = (key as usize) & self.table_size_mask;
        let entry = &self.table[index];

        if entry.zobrist_key != key {
            return None;
        }

        // Store best move hint regardless of depth
        let best_move = entry.best_move;

        if entry.depth >= depth {
            match entry.entry_type {
                CacheEntryType::Exact => {
                    return Some((entry.score, best_move));
                }
                CacheEntryType::LowerBound => {
                    if entry.score >= beta {
                        return Some((entry.score, best_move));
                    }
                }
                CacheEntryType::UpperBound => {
                    if entry.score <= alpha {
                        return Some((entry.score, best_move));
                    }
                }
            }
        }

        // Return just the best move hint (no score usable)
        None
    }

    /// Get best move from cache without depth check (for move ordering)
    pub fn get_best_move(&self, key: u64) -> Option<Move> {
        let index = (key as usize) & self.table_size_mask;
        let entry = &self.table[index];
        if entry.zobrist_key == key && entry.best_move.is_valid() {
            Some(entry.best_move)
        } else {
            None
        }
    }

    pub fn store(
        &mut self, key: u64, score: i32, depth: i32, best_move: Move,
        entry_type: CacheEntryType,
    ) {
        let index = (key as usize) & self.table_size_mask;
        let existing = &self.table[index];

        // Replacement strategy
        if existing.zobrist_key == 0 {
            // Empty slot — always store
        } else if existing.zobrist_key == key {
            // Same position — replace if depth >= existing
            if depth < existing.depth {
                return;
            }
        } else {
            // Collision — importance-based replacement with aging
            let new_importance = depth;
            let existing_importance = existing.depth;
            let age_diff =
                self.current_generation.wrapping_sub(existing.generation) as i32;
            let adjusted_existing = existing_importance - age_diff * 10;

            if new_importance < adjusted_existing {
                return;
            }
        }

        self.table[index] = CacheEntry {
            zobrist_key: key,
            score,
            depth,
            best_move,
            generation: self.current_generation,
            entry_type,
        };
    }

    pub fn increment_generation(&mut self) {
        self.current_generation += 1;
    }

    pub fn clear(&mut self) {
        for entry in self.table.iter_mut() {
            *entry = CacheEntry::empty();
        }
        self.current_generation = 0;
    }
}
