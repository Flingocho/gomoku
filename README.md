# 🎮 Gomoku AI

A feature-rich **Gomoku (Five-in-a-Row)** game on a 19×19 board, featuring a powerful AI engine with dual C++ and Rust implementations, a polished SFML graphical interface, and complete game rules including captures and forbidden moves.

![C++17](https://img.shields.io/badge/C%2B%2B-17-blue)
![Rust](https://img.shields.io/badge/Rust-FFI-orange)
![SFML](https://img.shields.io/badge/SFML-2.5-green)
![Tests](https://img.shields.io/badge/Tests-148%20passing-brightgreen)

---

## 📋 Table of Contents

- [Features](#-features)
- [Game Modes](#-game-modes)
- [AI Architecture](#-ai-architecture)
- [Project Structure](#-project-structure)
- [Prerequisites](#-prerequisites)
- [Build & Run](#-build--run)
- [Testing](#-testing)
- [Controls](#-controls)
- [Audio](#-audio)
- [Debug System](#-debug-system)

---

## ✨ Features

- **Dual AI Implementations** — C++ and Rust engines, switchable from the main menu
- **Advanced Search** — Minimax with Alpha-Beta pruning, Iterative Deepening, and Zobrist hashing
- **Pattern Evaluation** — Detects open-fours, half-fours, open-threes, and strategic combinations
- **Transposition Table** — 64 MB cache with importance-based replacement and generation aging
- **Adaptive Depth** — Automatically adjusts search depth based on game phase (6 → 8 → 10)
- **Move Ordering** — History heuristic, killer moves, geometric/centrality bonuses for faster pruning
- **Complete Rules** — Captures, double free-three prohibition, breakable five-in-a-row, forced capture mechanism
- **SFML GUI** — 1000×800 window with particle effects, glow, hover indicator, winning line highlight, and animations
- **Colorblind Mode** — Accessible color scheme toggle
- **Sound & Music** — Background music, sound effects for every action, volume controls
- **Suggestion Engine** — AI-powered move suggestions in human vs human mode
- **148 Unit Tests** — Comprehensive test suite covering both AI implementations

---

## 🕹️ Game Modes

| Mode | Description |
|------|-------------|
| **Play vs AI** | Human (Player 1) vs AI (Player 2). The AI plays automatically after each human move. |
| **Play vs Human** | Two humans on the same machine. The AI provides move suggestions in the background at reduced depth. |
| **Rust AI** | Same as Play vs AI, but uses the Rust implementation of the AI engine via FFI. |

All modes play on a **19×19 board**. A player wins by aligning **5 stones in a row** (horizontal, vertical, or diagonal) or by capturing **10 opponent stones** (5 capture pairs).

### Capture Mechanic
When a player places a stone that sandwiches exactly two opponent stones between their own, those two stones are removed from the board. If a 5-in-a-row can be broken by a capture, the opponent gets a **forced capture turn** to attempt it.

---

## 🧠 AI Architecture

The AI engine uses a multi-layered approach to find the best move:

### Search Algorithm
- **Minimax with Alpha-Beta Pruning** — Explores the game tree while cutting off branches that cannot improve the result
- **Iterative Deepening** — Searches at increasing depths (1, 2, ..., N) for better move ordering and time control
- **Transposition Table** — Zobrist hash-based cache (64 MB) that stores previously evaluated positions to avoid redundant computation

### Adaptive Depth
The search depth adjusts automatically based on the game phase:

| Phase | Turn Count | Search Depth |
|-------|-----------|-------------|
| Opening | < 6 | 6 |
| Midgame | 6 – 12 | 8 |
| Endgame | > 12 | 10 |

### Pattern Evaluation
The evaluator assigns scores to board patterns:

| Pattern | Score |
|---------|-------|
| Win (5-in-a-row) | 600,000 |
| Open Four | 50,000 |
| Half Four | 25,000 |
| Open Three | 10,000 |
| Half Three | 1,500 |
| Open Two | 100 |
| Capture Opportunity | 5,000 |
| Capture Threat | 6,000 |

Additional heuristics evaluate threat combinations, positional value, connectivity, and proximity to the center.

### Move Ordering
Fast pruning relies on examining the best moves first. The engine uses:
- **Quick move categorization** — Immediate wins, blocks, captures, and threats
- **History heuristic** — Tracks moves that caused beta cutoffs across the search
- **Killer moves** — Two slots per depth for moves that caused cutoffs at sibling nodes
- **Geometric & centrality bonuses** — Prefer central and well-connected moves
- **Adaptive candidate generation** — Search radius and candidate count vary by game phase

### Rust Implementation
The `gomoku_ai_rust` crate is a full parallel implementation of the C++ AI, compiled as a static library and linked via FFI. It uses the same algorithms and evaluation logic with zero external dependencies. The board state is passed as a flat `int[361]` array through C-compatible functions.

---

## 📁 Project Structure

```
gomoku/
├── src/
│   ├── main.cpp                    # Entry point, game loop state machine
│   ├── ai_engine/                  # AI logic
│   │   ├── ai_engine_core.cpp      # AI initialization, implementation dispatch
│   │   ├── search_minimax.cpp      # Minimax + Alpha-Beta + Iterative Deepening
│   │   ├── search_ordering.cpp     # Move ordering heuristics
│   │   ├── search_transposition.cpp # Transposition table management
│   │   ├── evaluator_patterns.cpp  # Pattern detection (fours, threes, twos)
│   │   ├── evaluator_threats.cpp   # Threat combination analysis
│   │   ├── evaluator_position.cpp  # Positional and centrality scoring
│   │   └── suggestion_engine.cpp   # Move suggestions for hotseat mode
│   ├── core/                       # Game state and engine
│   │   ├── game_engine.cpp         # Game flow, AI integration, forced captures
│   │   └── game_types.cpp          # Board state, Zobrist hash management
│   ├── gui/                        # SFML rendering
│   │   ├── gui_renderer_core.cpp   # Window setup, main render loop
│   │   ├── gui_renderer_board.cpp  # Board grid, stones, coordinates
│   │   ├── gui_renderer_menu.cpp   # Main menu and options screen
│   │   ├── gui_renderer_game.cpp   # In-game HUD, stats, captures
│   │   ├── gui_renderer_effects.cpp # Particles, glow, hover effects
│   │   ├── gui_renderer_ui.cpp     # Buttons, text, UI elements
│   │   └── gui_renderer_gameover.cpp # Win/defeat animations
│   ├── rule_engine/                # Game rules
│   │   ├── rules_core.cpp          # Move application, main rule logic
│   │   ├── rules_validation.cpp    # Legal move checking, double free-three
│   │   ├── rules_capture.cpp       # Capture detection and execution
│   │   └── rules_win.cpp           # Win conditions, breakable five-in-a-row
│   ├── ui/                         # Audio and display
│   │   ├── audio_manager.cpp       # Music streaming, sound effects
│   │   └── display.cpp             # Display utilities
│   ├── debug/                      # Debug and analysis
│   │   ├── debug_core.cpp          # Debug initialization and logging
│   │   ├── debug_analyzer.cpp      # Move analysis and game snapshots
│   │   └── debug_formatter.cpp     # Debug output formatting
│   └── utils/
│       └── zobrist_hasher.cpp      # Zobrist hash key generation
├── include/                        # Headers (mirrors src/ structure)
├── gomoku_ai_rust/                 # Rust AI implementation
│   ├── Cargo.toml
│   └── src/
│       ├── lib.rs                  # FFI entry points
│       ├── ai.rs                   # Minimax search
│       ├── evaluator.rs            # Pattern evaluation
│       ├── game_types.rs           # Board and state types
│       ├── move_ordering.rs        # Move ordering heuristics
│       └── transposition_table.rs  # Zobrist-based cache
├── tests/
│   ├── test_ai.cpp                 # 148 tests (both implementations)
│   └── Makefile
├── external/
│   └── sfml/                       # Bundled SFML 2.5 (auto-fetched)
├── fonts/                          # UI fonts
├── imgs/                           # Win/defeat animation frames
├── sounds/                         # Music and sound effects (OGG)
├── scripts/
│   └── setup.sh                    # Dependency fetcher
└── Makefile                        # Main build system
```

---

## 📦 Prerequisites

- **C++ Compiler** — `g++` or `clang++` with C++17 support
- **Make** — Build automation
- **Rust Toolchain** — `cargo` for building the Rust AI library
- **System Libraries** — Standard Linux libraries (usually pre-installed)

> SFML and its audio dependencies (OpenAL, Vorbis, FLAC, OGG) are **bundled locally** in `external/sfml/` — no system-wide installation required.

---

## 🔨 Build & Run

### Quick Start

```bash
git clone <repository_url>
cd gomoku
make        # Fetches SFML, builds Rust lib, compiles C++
make run    # Launches the game (sets LD_LIBRARY_PATH automatically)
```

The first build will:
1. Run `scripts/setup.sh` to download SFML headers and libraries into `external/sfml/`
2. Compile the Rust static library via `cargo build --release`
3. Compile all C++ sources with `-Wall -Wextra -Werror -g3 -O3 -std=c++17`

### Manual Run

If you prefer to run the binary directly:

```bash
export LD_LIBRARY_PATH=$(pwd)/external/sfml/lib:$LD_LIBRARY_PATH
./Gomoku
```

### Build Targets

| Command | Description |
|---------|-------------|
| `make` | Full build (setup + Rust + C++) |
| `make run` | Build and run with correct library paths |
| `make clean` | Remove object files |
| `make fclean` | Remove objects and binary |
| `make re` | Full rebuild from scratch |

---

## 🧪 Testing

The project includes **148 unit tests** covering pattern evaluation, threat detection, capture mechanics, win conditions, and AI decision-making — for both the C++ and Rust implementations.

```bash
cd tests
make
export LD_LIBRARY_PATH=$(pwd)/../external/sfml/lib:$(pwd)/../gomoku_ai_rust/target/release:$LD_LIBRARY_PATH
./test_ai
```

Tests validate:
- Pattern detection (open-fours, half-fours, open-threes, etc.)
- Capture opportunities and threats
- Win condition evaluation
- Threat combination scoring
- AI move selection quality
- Rust/C++ implementation parity

---

## 🎛️ Controls

| Input | Action |
|-------|--------|
| **Left Click** | Place a stone / Select menu option |
| **M** | Toggle mute |
| **ESC** | Return to menu / Exit |

### Options Menu
Accessible from the main menu, with controls for:
- Music volume (±10%)
- Sound effects volume (±10%)
- Music and sound toggles
- Debug mode toggle

---

## 🔊 Audio

All audio files are in **OGG Vorbis** format (SFML requirement).

| Sound | Usage | Default Volume |
|-------|-------|---------------|
| `main_theme.ogg` | Background music (loops) | 50% |
| `place_piece.ogg` | Valid stone placement | 70% |
| `invalid_move.ogg` | Invalid move attempt | 70% |
| `click_menu.ogg` | Menu button click | 70% |
| `victory.ogg` | Player wins | 70% |
| `defeat.ogg` | Player loses | 70% |

The game degrades gracefully if audio files are missing — it will display a console warning but continue running normally.

---

## 🐛 Debug System

The built-in debug analyzer provides 5 levels of detail:

| Level | Output |
|-------|--------|
| 0 | Off |
| 1 | Root best moves per iteration |
| 2 | Threat and capture analysis |
| 3 | Full evaluation breakdown (patterns, position, captures) |
| 4 | Board state snapshots |

Debug output is logged to `gomoku_debug.log` and can be toggled from the options menu. The debug system tracks per-move scores, nodes evaluated, search depth, and provides human-readable explanations of the AI's reasoning.

---

<div align="center">
  Created with ❤️ by <a href="https://github.com/jainavas">jainavas</a> and <a href="https://github.com/Flingocho">Flingocho</a>
</div>