# Gomoku AI — Project Flow Diagrams

## 1. Application State Machine

The main game loop is a state machine with four states:

```mermaid
stateDiagram-v2
    [*] --> MENU : App starts

    MENU --> PLAYING : Play vs AI
    MENU --> PLAYING : Play vs Human
    MENU --> PLAYING : Rust AI
    MENU --> PLAYING : Colorblind Mode
    MENU --> OPTIONS : Options
    MENU --> [*] : Exit

    OPTIONS --> MENU : Back

    PLAYING --> GAME_OVER : Win / Capture Win detected
    PLAYING --> MENU : ESC

    GAME_OVER --> PLAYING : Play Again
    GAME_OVER --> MENU : Back to Menu
```

---

## 2. Game Loop — Playing State

```mermaid
flowchart TD
    A[Playing State] --> B{Game Over?}
    B -- Yes --> Z[Transition to GAME_OVER]
    B -- No --> C{Game Mode?}

    C -- VS_HUMAN_SUGGESTED --> D{Has Suggestion?}
    D -- No --> D1[SuggestionEngine generates hint at depth 6]
    D1 --> D2[Show suggestion on board]
    D -- Yes --> D2
    D2 --> E{Player clicked?}
    E -- No --> R[Render frame]
    E -- Yes --> F[Validate & apply move]

    C -- VS_AI --> G{Current Player?}
    G -- Player 1 Human --> E
    G -- Player 2 AI --> H[AI computes best move]
    H --> I[Apply AI move]
    I --> J[Record thinking time]
    J --> R

    F --> K{Move legal?}
    K -- No --> L[Show invalid move error]
    L --> R
    K -- Yes --> M[RuleEngine::applyMove]
    M --> N[Check forced captures]
    N --> R

    R --> A
```

---

## 3. Move Application Pipeline (RuleEngine)

```mermaid
flowchart TD
    A[applyMove called] --> B{Cell empty?}
    B -- No --> FAIL[Return: invalid move]
    B -- Yes --> C{Creates double free-three?}
    C -- Yes --> FAIL
    C -- No --> D[Place stone on board]
    D --> E[Find all captures]
    E --> F{Captures found?}
    F -- Yes --> G[Remove captured stones]
    G --> H[Update capture count]
    F -- No --> H
    H --> I[Check win condition]
    I --> J[Update Zobrist hash]
    J --> K[Switch current player]
    K --> L[Increment turn count]
    L --> SUCCESS[Return: success + capture info]

    SUCCESS --> M{5-in-a-row detected?}
    M -- No --> END[Turn ends normally]
    M -- Yes --> N{Can opponent break it by capture?}
    N -- No --> O[Game over — player wins]
    N -- Yes --> P[Set forced capture moves for opponent]
    P --> Q[Opponent MUST attempt capture next turn]
```

---

## 4. AI Search Pipeline

```mermaid
flowchart TD
    A[getBestMove called] --> B{Implementation?}

    B -- C++ --> C[Get adaptive depth for game phase]
    B -- Rust --> R1[Get adaptive depth for game phase]

    C --> D[Iterative Deepening: depth 1 → N]
    D --> E[Generate candidate moves]
    E --> F[Order moves by heuristics]

    F --> F1[Quick categorization]
    F1 --> F2[History heuristic]
    F2 --> F3[Killer moves]
    F3 --> F4[Centrality & connectivity bonus]

    F --> G[Minimax + Alpha-Beta]
    G --> H{Transposition table hit?}
    H -- Yes, exact --> I[Return cached score]
    H -- Yes, bounds --> J[Narrow alpha-beta window]
    H -- No --> K[Evaluate position]

    J --> K
    K --> L{Depth 0 or terminal?}
    L -- Yes --> M[Pattern evaluation]
    L -- No --> N[Recurse: generate & search child nodes]
    N --> G

    M --> M1[Count patterns per direction]
    M1 --> M2[Score threats & combinations]
    M2 --> M3[Add positional value]
    M3 --> M4[Add capture heuristics]
    M4 --> O[Return evaluation score]

    O --> P[Store in transposition table]
    P --> Q[Update best move for this depth]
    Q --> D

    D --> S[Return best move from deepest completed iteration]

    R1 --> R2[Send board as int 361 via FFI]
    R2 --> R3[Rust Minimax + Alpha-Beta]
    R3 --> R4[Return best move via FFI]
```

---

## 5. Pattern Evaluation Breakdown

```mermaid
flowchart TD
    A[Board State] --> B[Scan all 4 directions]
    B --> DIR["Horizontal — | Vertical | | Diagonal ╲ | Diagonal ╱"]
    DIR --> G[Count patterns per direction]

    G --> SCORES[Pattern Scores]
    G --> THREATS[Threat Combinations]
    G --> CAPS[Capture Scoring]

    subgraph SCORES_DETAIL ["Pattern Scores"]
        S1["fiveInRow → 600,000"]
        S2["fourOpen → 50,000"]
        S3["fourHalf → 25,000"]
        S4["threeOpen → 10,000"]
        S5["threeHalf → 1,500"]
        S6["twoOpen → 100"]
    end

    subgraph THREATS_DETAIL ["Threat Combinations"]
        T1["Double open-three"]
        T2["Four + three combo"]
        T3["Multiple half-fours"]
    end

    subgraph CAPS_DETAIL ["Capture Scoring"]
        C1["Opportunity → 5,000"]
        C2["Threat → 6,000"]
        C3["Capture win → 500,000"]
        C4["Prevent loss → 400,000"]
    end

    SCORES --> SCORES_DETAIL
    THREATS --> THREATS_DETAIL
    CAPS --> CAPS_DETAIL

    SCORES_DETAIL --> K[Sum all scores]
    THREATS_DETAIL --> K
    CAPS_DETAIL --> K
    K --> L[Add positional bonus]
    L --> M[Final evaluation score]
```

---

## 6. Module Architecture

```mermaid
graph TB
    subgraph GUI["GUI Layer (SFML)"]
        MENU_R[Menu Renderer]
        BOARD_R[Board Renderer]
        EFFECTS[Effects & Particles]
        GAME_R[Game HUD]
        GAMEOVER_R[Game Over Screen]
        AUDIO[Audio Manager]
    end

    subgraph CORE["Core Layer"]
        GE[GameEngine]
        GS[GameState]
        ZOBRIST[Zobrist Hasher]
    end

    subgraph RULES["Rule Engine"]
        RC[Rules Core]
        RV[Rules Validation]
        RCAP[Rules Capture]
        RWIN[Rules Win]
    end

    subgraph AI_CPP["AI Engine (C++)"]
        AI_CORE[AI Core]
        MINIMAX[Minimax Search]
        TT[Transposition Table]
        EVAL[Evaluator]
        ORDERING[Move Ordering]
        SUGGEST[Suggestion Engine]
    end

    subgraph AI_RUST["AI Engine (Rust FFI)"]
        RUST_W[Rust AI Wrapper]
        RUST_LIB["libgomoku_ai_rust.a"]
    end

    subgraph DEBUG["Debug System"]
        DCORE[Debug Core]
        DANALYZER[Debug Analyzer]
        DFMT[Debug Formatter]
    end

    MAIN([main.cpp]) --> GUI
    MAIN --> GE
    MAIN --> DEBUG

    GE --> GS
    GE --> RULES
    GE --> AI_CORE

    GS --> ZOBRIST

    RC --> RV
    RC --> RCAP
    RC --> RWIN

    AI_CORE -->|C++ path| MINIMAX
    AI_CORE -->|Rust path| RUST_W
    RUST_W --> RUST_LIB

    MINIMAX --> TT
    MINIMAX --> EVAL
    MINIMAX --> ORDERING

    SUGGEST --> EVAL

    DANALYZER --> DFMT

    GUI -.->|reads| GS
    GAME_R -.->|displays| AI_CORE
```

---

## 7. Build Pipeline

```mermaid
flowchart LR
    A[make] --> B[setup.sh]
    B --> B1[Download SFML headers]
    B1 --> B2[Download SFML libs]
    B2 --> B3[Download audio deps]

    A --> C[cargo build --release]
    C --> C1["libgomoku_ai_rust.a"]

    B3 --> D[g++ compile]
    C1 --> D
    D --> D1["C++17 -O3 -Wall -Wextra -Werror"]
    D1 --> E["Link: SFML + Rust + Audio libs"]
    E --> F["./Gomoku"]

    F --> G["make run"]
    G --> G1["Set LD_LIBRARY_PATH"]
    G1 --> F
```
