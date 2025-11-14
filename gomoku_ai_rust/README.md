# 🦀 Gomoku AI (Rust Implementation)

## Overview

This is the high-performance Rust-based AI engine for Gomoku. It implements advanced game tree search algorithms with pattern recognition and evaluation functions, designed to be called from C++ via FFI (Foreign Function Interface).

## 🎯 Features

### Core AI Components

- **🔍 Minimax Search with Alpha-Beta Pruning**
  - Efficient game tree exploration
  - Optimal move selection with pruning
  - Depth-limited search

- **⚡ Iterative Deepening**
  - Progressive depth increase (1 → max_depth)
  - Better time management
  - Early move ordering benefits

- **🛡️ Immediate Threat Detection**
  - Pre-search check for AI winning moves
  - **Critical: Opponent win blocking** - prevents obvious losses
  - Returns immediately if threat detected

- **💾 Transposition Table**
  - 64MB hash table (configurable)
  - Zobrist hashing for position identification
  - Stores exact scores, upper/lower bounds
  - Generation-based replacement strategy

- **📊 Sophisticated Evaluation**
  - Pattern recognition (2/3/4/5-in-a-row)
  - Gap pattern analysis (X-XXX, XX-X, etc.)
  - Free-end counting
  - Capture evaluation with context
  - Mate distance calculation

- **🎲 Move Ordering**
  - Previous best move first (PV-move)
  - Adaptive candidate generation
  - Radius-based search around existing pieces
  - Tactical zone expansion near opponent's last move

- **🚀 Performance Optimizations**
  - In-place move application (no cloning)
  - Zero-copy state updates
  - Efficient undo mechanism
  - Cache-friendly data structures

## 📂 Module Structure

```
gomoku_ai_rust/
├── src/
│   ├── lib.rs                    # FFI interface & module exports
│   ├── game_types.rs             # Core types (Move, GameState, Board)
│   ├── ai.rs                     # Search engine (minimax, iterative deepening)
│   ├── evaluator.rs              # Position evaluation & pattern detection
│   ├── transposition_table.rs    # Cache management with Zobrist hashing
│   └── move_ordering.rs          # Candidate generation & move ordering
├── Cargo.toml                    # Rust package configuration
└── README.md                     # This file
```

## 🔧 Building

### Debug Build (Fast compilation, slower execution)
```bash
cargo build
```

### Release Build (Optimized for performance)
```bash
cargo build --release
```

The release build is **10-100x faster** than debug due to optimizations.

## 🧪 Testing

### Run all tests
```bash
cargo test
```

### Run specific test
```bash
cargo test test_name
```

### Run with output
```bash
cargo test -- --nocapture
```

## 📊 Performance Characteristics

### Time Complexity
- **Best case**: O(b^(d/2)) with perfect move ordering
- **Average case**: O(b^(3d/4)) with good move ordering  
- **Worst case**: O(b^d) without pruning

Where:
- `b` = branching factor (~5-15 with candidate generation)
- `d` = search depth (typically 6-10)

### Space Complexity
- **Transposition Table**: 64 MB (configurable)
- **Stack Space**: O(d) for recursion depth
- **State Size**: ~3.6 KB per GameState

### Typical Performance (depth 8)
- **Nodes Evaluated**: 50,000 - 200,000
- **Cache Hit Rate**: 50-70%
- **Time**: 1-8 seconds (depends on position)

## 🔌 FFI Interface

The Rust AI is called from C++ through a C-compatible FFI interface:

### Exported Functions

```rust
#[no_mangle]
pub extern "C" fn rust_ai_find_best_move(
    board: *const [[i32; 19]; 19],
    current_player: i32,
    captures_p1: i32,
    captures_p2: i32,
    last_move_x: i32,
    last_move_y: i32,
    last_human_move_x: i32,
    last_human_move_y: i32,
    max_depth: i32,
    nodes_evaluated: *mut u64,
    cache_hits: *mut u64,
) -> Move;
```

### Safety Guarantees
- Pointer validity checks before dereferencing
- Null pointer guards
- Bounds checking on array access
- No memory leaks or use-after-free

## 🎨 Pattern Evaluation Weights

| Pattern | Free Ends | Score | Description |
|---------|-----------|-------|-------------|
| 5+ pieces | Any | 600,000 | Victory |
| 4 pieces | 2 | 50,000 | Unstoppable threat (open four) |
| 4 pieces | 1 | 25,000 | Strong threat (half-open four) |
| 3 pieces | 2 | 10,000 | Developing threat (open three) |
| 3 pieces | 1 | 1,500 | Half-open three |
| 2 pieces | 2 | 100 | Early position |

### Immediate Threat Bonuses
- **AI has winning threats**: +90,000
- **Opponent has winning threats**: -105,000
- **Opponent has open four**: -80,000
- **Opponent has half-open four**: -60,000
- **AI has open four**: +70,000
- **AI has half-open four**: +40,000

### Capture Evaluation
- **10 captures (win)**: ±600,000
- **9 captures**: ±300,000
- **8 captures**: ±150,000
- **7 captures**: ±50,000
- **6 captures**: ±15,000
- **5 captures**: ±5,000
- **<5 captures**: ±500-1,500

## 🔍 Algorithm Details

### Iterative Deepening Flow

```rust
1. Pre-Search Checks:
   a. Check if AI can win immediately → play winning move
   b. Check if opponent can win next turn → BLOCK
   
2. For depth = 1 to max_depth:
   a. Run minimax(depth)
   b. Store best move found at this depth
   c. If time limit reached, return best move so far
   
3. Return best move from deepest completed search
```

### Minimax with Alpha-Beta

```rust
fn minimax(state, depth, alpha, beta, maximizing):
    // Transposition table lookup
    if cached_position:
        return cached_score
    
    // Base cases
    if depth == 0 or game_over:
        return evaluate(state)
    
    // Generate and order moves
    moves = generate_candidates(state)
    moves = order_moves(moves, previous_best)
    
    if maximizing:
        max_eval = -∞
        for move in moves:
            apply_move(move)
            eval = minimax(state, depth-1, alpha, beta, false)
            undo_move(move)
            
            max_eval = max(max_eval, eval)
            alpha = max(alpha, eval)
            if beta <= alpha:
                break  // β cutoff
        
        cache_result(max_eval)
        return max_eval
    else:
        // Symmetric for minimizing player
```

### Transposition Table

```rust
struct CacheEntry {
    zobrist_key: u64,      // Position hash
    score: i32,            // Evaluation score
    depth: i32,            // Search depth
    best_move: Move,       // Best move found
    entry_type: Type,      // Exact/LowerBound/UpperBound
    generation: u8,        // For replacement strategy
    importance: u8,        // Priority (0-255)
}
```

**Replacement Strategy:**
1. Always replace if same position (deeper search)
2. If different position:
   - Prefer keeping deeper searches
   - Prefer keeping exact scores over bounds
   - Prefer keeping recent generations
   - Use importance score as tiebreaker

## 🚀 Optimization Techniques

### 1. In-Place Move Application
```rust
// Old (slow): Clone entire state
let mut new_state = state.clone();
new_state.make_move(mv);
let score = minimax(new_state, ...);

// New (fast): Apply and undo in-place
let captures = state.apply_move(mv);
let score = minimax(state, ...);
state.unapply_move(mv, captures);
```

**Speedup**: 2-3x faster by avoiding clones

### 2. Adaptive Candidate Generation
```rust
// Early game: Small radius (3-5 cells)
// Mid game: Medium radius (4-6 cells)
// Late game: Larger radius but fewer candidates
// Extended zone around opponent's last move
```

**Speedup**: 10x fewer nodes evaluated

### 3. Move Ordering
```rust
// Order moves by:
1. Previous best move (from transposition table)
2. Quick evaluation score:
   - Distance to center (centrality bonus)
   - Adjacent pieces (connectivity)
   - Immediate patterns created
```

**Speedup**: 5x better alpha-beta pruning

## 🐛 Debugging

### Enable Debug Output
```rust
println!("Immediate win at ({}, {})", mv.x, mv.y);
println!("Blocking opponent win at ({}, {})", mv.x, mv.y);
```

### Common Issues

**AI plays random moves:**
- Check evaluation function is working
- Verify transposition table not corrupted
- Ensure depth > 0

**AI too slow:**
- Increase candidate generation radius limits
- Reduce max_depth
- Check for infinite loops

**AI doesn't block obvious threats:**
- ✅ Fixed: Added pre-search opponent win detection
- Verify `has_winning_threats()` function
- Check pattern detection weights

## 📈 Future Improvements

- [ ] **Opening Book**: Pre-computed optimal openings
- [ ] **Endgame Database**: Perfect play in late game
- [ ] **Multi-threading**: Parallel search at root node
- [ ] **Neural Network Evaluation**: ML-based position scoring
- [ ] **Faster Hash Functions**: XXHash or similar
- [ ] **Bitboard Representation**: Compact board state
- [ ] **Principal Variation Search**: More efficient than alpha-beta

## 📝 License

MIT License - See root README.md for details

## 👥 Contributing

Contributions welcome! Areas needing improvement:
1. **Performance**: Further optimizations
2. **Testing**: More comprehensive test coverage
3. **Documentation**: Code comments and examples
4. **Evaluation**: Better pattern recognition

## 🙏 Acknowledgments

- Rust programming language for safety and performance
- Chess programming wiki for algorithm references
- Gomoku community for game insights
