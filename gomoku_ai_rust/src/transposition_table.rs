// Transposition Table - Similar to transposition_search.hpp/cpp (table part)

use crate::game_types::Move;
use std::collections::HashMap;

// Cache entry types matching C++
#[derive(Clone, Copy, PartialEq)]
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
    pub fn new(zobrist_key: u64, score: i32, depth: i32, best_move: Move, entry_type: CacheEntryType, generation: u32) -> Self {
        CacheEntry {
            zobrist_key,
            score,
            depth,
            best_move,
            generation,
            entry_type,
        }
    }

    pub fn get_importance_value(&self) -> i32 {
        let mut value = self.depth * 100;
        if self.entry_type == CacheEntryType::Exact {
            value += 50;
        } else if self.entry_type == CacheEntryType::LowerBound {
            value += 25;
        }
        value
    }
}

pub struct TranspositionTable {
    table: HashMap<usize, CacheEntry>,
    table_size_mask: usize,
    current_generation: u32,
}

impl TranspositionTable {
    pub fn new(table_size_mb: usize) -> Self {
        let bytes_per_entry = std::mem::size_of::<CacheEntry>();
        let total_bytes = table_size_mb * 1024 * 1024;
        let num_entries = total_bytes / bytes_per_entry;
        
        // Round to nearest power of 2
        let mut power_of_2 = 1;
        while power_of_2 < num_entries {
            power_of_2 <<= 1;
        }
        if power_of_2 > num_entries {
            power_of_2 >>= 1;
        }
        
        TranspositionTable {
            table: HashMap::new(),
            table_size_mask: power_of_2 - 1,
            current_generation: 1,
        }
    }

    pub fn increment_generation(&mut self) {
        self.current_generation += 1;
    }

    #[allow(dead_code)]
    pub fn current_generation(&self) -> u32 {
        self.current_generation
    }

    pub fn lookup(&self, zobrist_key: u64) -> Option<CacheEntry> {
        let index = (zobrist_key as usize) & self.table_size_mask;
        if let Some(entry) = self.table.get(&index) {
            if entry.zobrist_key == zobrist_key {
                return Some(entry.clone());
            }
        }
        None
    }

    pub fn store(&mut self, zobrist_key: u64, score: i32, depth: i32, best_move: Move, entry_type: CacheEntryType) {
        let index = (zobrist_key as usize) & self.table_size_mask;
        
        let should_replace = if let Some(existing) = self.table.get(&index) {
            if existing.zobrist_key == 0 {
                true
            } else if existing.zobrist_key == zobrist_key {
                depth >= existing.depth
            } else {
                let new_entry = CacheEntry::new(zobrist_key, score, depth, best_move, entry_type, self.current_generation);
                let existing_importance = existing.get_importance_value() - ((self.current_generation - existing.generation) as i32 * 10);
                let new_importance = new_entry.get_importance_value();
                
                new_importance > existing_importance || 
                (new_importance == existing_importance && entry_type == CacheEntryType::Exact)
            }
        } else {
            true
        };

        if should_replace {
            self.table.insert(index, CacheEntry::new(zobrist_key, score, depth, best_move, entry_type, self.current_generation));
        }
    }
}
