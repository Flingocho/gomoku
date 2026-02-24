# Gomoku AI Project

A comprehensive Gomoku (Five-in-a-Row) game featuring a powerful AI engine, pattern-based evaluation, and a polished graphical interface using SFML.

## Features

*   **Hybrid AI Engine:** Utilizes Minimax with Alpha-Beta Pruning, Iterative Deepening, and Zobrist Hashing for efficient search.
*   **Rust Integration:** Includes a Rust module for performance-critical components.
*   **Sophisticated Evaluator:** Detects winning threats, open-fours, and strategic patterns.
*   **Graphical Interface:** Built with SFML for smooth gameplay and visual feedback.
*   **Cross-Platform Build:** Includes scripts to handle dependencies automatically on Linux systems.

## Project Structure

*   `src/`: C++ Source code.
    *   `ai_engine/`: Core AI logic (Minimax, Evaluator, Move Ordering).
    *   `core/`: Game state management and basic types.
    *   `gui/`: Rendering and UI components.
    *   `rule_engine/`: Rules validation and win condition checking.
*   `include/`: Header files corresponding to source modules.
*   `gomoku_ai_rust/`: Rust library for AI enhancements.
*   `scripts/`: Helper scripts for setup and maintenance.
*   `external/`: Third-party dependencies (SFML) managed by the setup script.
*   `tests/`: Test harnesses for validating AI logic.

## Prerequisites

*   **C++ Compiler:** `g++` or `clang++` supporting C++17.
*   **Make:** Build automation tool.
*   **Rust Toolchain:** `cargo` (for the Rust module).
*   **System Libraries:** Basic system libraries (usually present on most Linux distros).

*   **SFML (bundled):** SFML runtime libraries are fetched and stored locally in `external/sfml` by the project's setup script. You generally do not need to install SFML system-wide — the build and `make run` will use the bundled libraries. If you run the executable directly, you may need to set `LD_LIBRARY_PATH` to `./external/sfml/lib` (see Run section).

## Installation

1.  **Clone the repository:**
    ```bash
    git clone <repository_url>
    cd <repository_directory>
    ```

2.  **Build the project:**
    The `Makefile` is configured to automatically download and configure SFML and its dependencies locally.
    ```bash
    make
    ```
    *   The first run will execute `scripts/setup.sh` to fetch headers and libraries into `external/sfml`.
    *   The Rust library will be compiled automatically.

3.  **Run the game:**
    ```bash
    make run
    ```
    This command sets up the necessary library paths (`LD_LIBRARY_PATH`) automatically.

    Or manually:
    ```bash
    # Absolute paths (recommended for scripts)
    export LD_LIBRARY_PATH=$(pwd)/external/sfml/lib:$(pwd)/gomoku_ai_rust/target/release:$LD_LIBRARY_PATH
    ./gomoku

    # Or, if you're already in the repo root, a shorter relative form also works:
    export LD_LIBRARY_PATH=./external/sfml/lib:$LD_LIBRARY_PATH
    ./gomoku
    ```

## Development & Testing

*   **Running Tests:**
    To verify the AI engine without the GUI (useful for headless environments):
    ```bash
    cd tests
    make
    export LD_LIBRARY_PATH=$(pwd)/../external/sfml/lib:$(pwd)/../gomoku_ai_rust/target/release:$LD_LIBRARY_PATH
    ./test_ai
    ```

*   **Clean Build:**
    ```bash
    make clean   # Removes objects
    make fclean  # Removes executables
    make re      # Rebuilds everything
    ```

## Improvements & Fixes

*   **Robust Dependency Management:** The project now automatically manages SFML and its dependencies (OpenAL, Vorbis, FLAC) locally, ensuring it compiles and runs on standard environments without requiring root privileges or system package installations.
*   **AI Stability:** Fixed a critical segmentation fault in the AI engine related to move generation and Zobrist hashing initialization.
*   **Refactoring:**
    *   Replaced magic numbers in `Evaluator` with named constants for better readability and tuning.
    *   Improved move ordering safety by explicitly initializing memory.
    *   Cleaned up codebase documentation.

## Authors
*   42 School Project Team
