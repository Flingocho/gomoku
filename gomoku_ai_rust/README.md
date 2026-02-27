# 🦀 Gomoku AI — Rust Implementation

## What is this?

This crate is a **direct translation** of the C++ AI engine (`src/ai_engine/`) into Rust. It adds no new features and doesn't change any logic: same algorithm, same heuristics, same weights, rewritten in Rust.

### Why rewrite in Rust?

Porting a C++ engine to Rust is a common industry practice for several reasons:

- **Memory safety** — Rust eliminates bugs like use-after-free, data races, and buffer overflows at compile time, with zero runtime cost.
- **Interoperability** — Rust compiles to a static library (`libgomoku_ai_rust.a`) that links with C++ via FFI, making migration incremental and transparent.
- **Comparable performance** — Rust generates native code with similar optimizations to C++ (LLVM backend).
- **Maintainability** — Rust's type system (ownership, borrowing) catches many logic errors at compile time.

> This is the same pattern used in projects like Firefox (Servo → Stylo), the Linux kernel, and Cloudflare, where performance-critical modules are rewritten from C/C++ to Rust without changing their external interface.

## Structure

```
src/
├── lib.rs                    # FFI interface (C → Rust)
├── game_types.rs             # Core types: Move, GameState, Board
├── ai.rs                     # Minimax + iterative deepening
├── evaluator.rs              # Position evaluation & pattern detection
├── transposition_table.rs    # Transposition table (Zobrist hashing)
└── move_ordering.rs          # Candidate generation & move ordering
```

## Build

```bash
cargo build --release
```

The library is generated at `target/release/libgomoku_ai_rust.a` and linked automatically by the project's main Makefile.

## C++ equivalence

| C++ component | Rust equivalent |
|---|---|
| `search_minimax.cpp` | `ai.rs` |
| `evaluator_*.cpp` | `evaluator.rs` |
| `search_transposition.cpp` | `transposition_table.rs` |
| `search_ordering.cpp` | `move_ordering.rs` |
| `game_types.cpp` | `game_types.rs` |

The FFI interface exports a single function:

```rust
pub extern "C" fn rust_ai_find_best_move(...) -> Move;
```

Called from C++ through `RustAIWrapper::getBestMove()` when the player selects the Rust AI.

## Tests

Tests for this AI live in the main project test suite (`tests/test_ai.cpp`). Every AI test runs for **both implementations** (C++ and Rust), verifying they produce equivalent results. See [tests.md](../tests.md) for details.