# Análisis Completo del Proyecto Gomoku — Revisión Profunda

## 1. Arquitectura General

El proyecto es un **Gomoku 19×19** con GUI SFML, motor de reglas completo (incluyendo capturas y double free-three) y una IA con Minimax + Alpha-Beta Pruning + Tabla de Transposición Zobrist. Hay ~30 ficheros C++ organizados en 7 módulos:

| Módulo | Ficheros | Responsabilidad |
|--------|----------|-----------------|
| **core** | `game_engine`, `game_types` | Estado del juego, flujo de turnos, lógica de modos |
| **rule_engine** | `rules_core`, `rules_capture`, `rules_validation`, `rules_win` | Reglas oficiales: colocar pieza, capturas, double-free-three, victoria |
| **ai_engine** | `ai_engine_core`, `search_minimax`, `search_ordering`, `search_transposition`, `evaluator_*`, `suggestion_engine` | IA principal: Minimax iterativo, move ordering, evaluación heurística |
| **gui** | `gui_renderer_core`, `_menu`, `_board`, `_game`, `_gameover`, `_effects`, `_ui` | Interfaz gráfica SFML completa |
| **ui** | `audio_manager`, `display` | Audio (OGG) y display de texto (consola, legacy) |
| **utils** | `zobrist_hasher`, `directions` | Hashing Zobrist incremental O(1), constantes de dirección |
| **debug** | `debug_core`, `debug_analyzer`, `debug_formatter` | Sistema de debug con breakdowns de evaluación y logging a fichero |

---

## 2. Modos de Juego

Definidos en `GameMode` (`include/core/game_engine.hpp`):

- **`VS_AI`** — Jugador 1 (humano, azul, `O`) vs Jugador 2 (IA, rojo, `X`). Player 1 siempre empieza. La IA juega automáticamente en el turno de Player 2.
- **`VS_HUMAN_SUGGESTED`** — Hotseat: ambos jugadores son humanos. La IA genera una **sugerencia** (profundidad 6) que se muestra como indicador dorado en el tablero (bombilla con `?`).

Variantes de menú que reutilizan `VS_AI`:
- **Colorblind Mode** — Activa `isColorblindMode`: todas las fichas se ven grises hasta el Game Over, donde se revelan los colores reales.
- **Rust AI** — Usa `RUST_IMPLEMENTATION` vía FFI (excluido del análisis actual).

---

## 3. Flujo Principal — `src/main.cpp`

```
MENU → [selección] → PLAYING → [game over detect] → GAME_OVER → [new game / menu]
              ↳ OPTIONS (audio, debug)
```

La máquina de estados usa `GuiRenderer::AppState` (`MENU`, `OPTIONS`, `PLAYING`, `GAME_OVER`). El bucle principal:
1. Procesa eventos SFML (`processEvents`)
2. En `PLAYING`:
   - Si `VS_HUMAN_SUGGESTED`: genera sugerencia para el jugador actual, espera click
   - Si `VS_AI` y turno Player 1: espera click humano
   - Si `VS_AI` y turno Player 2: llama `makeAIMove()`
3. Detecta `isGameOver()` → transición a `GAME_OVER` con línea ganadora resaltada y animación (GIF frame-by-frame)
4. ESC vuelve al menú desde cualquier estado

---

## 4. Condiciones de Victoria — `src/rule_engine/rules_win.cpp`

Dos formas de ganar:

1. **Victoria por capturas**: `captures[player-1] >= 10` (10 piezas capturadas = 5 pares). Comprobación directa.
2. **Victoria por alineación (5 en línea)**: Se verifica en las 4 direcciones principales desde cada casilla del jugador. **PERO** con dos verificaciones extra del estándar 42:
   - **¿El oponente puede romper la línea por captura?** → Si sí, `canBreakLineByCapture()` detecta las posiciones donde el oponente puede capturar una pieza de la línea de 5. Se establecen `forcedCaptureMoves` como *oportunidad* (no obligatoria) para el oponente.
   - **¿El oponente tiene ≥8 capturas y puede capturar más?** → Si el oponente está a punto de ganar por capturas, la línea de 5 **no se considera victoria**.

---

## 5. Sistema de Capturas — `src/rule_engine/rules_capture.cpp`

- Patrón de captura: `PLAYER-OPPONENT-OPPONENT-PLAYER` (patrón de flanqueo).
- Se busca en las **8 direcciones** (no solo las 4 principales).
- Dos variantes: hacia adelante (`NUEVA-OPP-OPP-MIA`) y hacia atrás (`MIA-OPP-OPP-NUEVA`).
- `findAllCaptures()` devuelve `CaptureInfo` con `myCapturedPieces` (las que YO capturo). Las piezas del oponente se quedan vacías (solo se aplican mis capturas).
- Las piezas capturadas se eliminan del tablero y se incrementa `captures[player-1]` por el número de pares.

---

## 6. Regla Double Free-Three — `src/rule_engine/rules_validation.cpp`

- Un movimiento es **ilegal** si crea ≥2 free-threes simultáneamente.
- Un *free-three* es un patrón de 3 fichas en una ventana de 5 posiciones, con ambos extremos libres, donde al menos una posición vacía permite completar 4 consecutivos.
- Se verifican los 10 patrones válidos (con y sin gaps): `XXX--`, `XX-X-`, `X-XX-`, `-XXX-`, etc.
- Se valida con `canFormThreat()` + `hasFourConsecutive()`.

---

## 7. Motor de IA — Arquitectura

### 7.1 Dispatcher — `src/ai_engine/ai_engine_core.cpp`
- `getBestMove()` despacha a C++ o Rust según `implementation`.
- Profundidad adaptativa por fase: turnos <6 → depth 6, ≤12 → depth 8, >12 → depth 10.

### 7.2 Búsqueda — `src/ai_engine/search_minimax.cpp`
- **Iterative Deepening** desde depth 1 hasta `maxDepth`.
- **Pre-check de victoria inmediata** antes de iniciar la búsqueda iterativa (escanea todos los candidatos buscando un movimiento que gane instantáneamente).
- **Minimax con Alpha-Beta Pruning**: ramas maximizing (Player 2/IA) y minimizing (Player 1/Humano).
- **Tabla de transposición Zobrist**: lookup antes de evaluar, con 3 tipos de entrada (`EXACT`, `LOWER_BOUND`, `UPPER_BOUND`).
- Early termination si `|score| > 300000` (mate detectado).
- Copia profunda del estado en cada nodo (`GameState newState = state; applyMove(newState, move)`).

### 7.3 Generación de Movimientos — `src/ai_engine/search_ordering.cpp`
- **Adaptive radius**: radio 1 alrededor de piezas existentes + radio extendido alrededor del último movimiento humano.
- **Move ordering** por evaluación rápida `quickEvaluateMove()`: centralidad, conectividad, patrones simples, detección de captura.
- Límite de candidatos por fase: ≤4 turnos → 3, ≤10 → 4, >10 → 5.
- El movimiento anterior de la iteración anterior se pone primero (killer move).

### 7.4 Tabla de Transposición — `src/ai_engine/search_transposition.cpp`
- Tamaño por defecto: 64 MB (~1.6M entradas de 40 bytes).
- Indexación: `hash & tableSizeMask` (potencia de 2).
- Estrategia de reemplazo: importancia basada en profundidad + tipo EXACT preferido + aging por generación.

### 7.5 Evaluador — `src/ai_engine/evaluator_*.cpp`
- **Evaluación principal**: `evaluate(state, maxDepth, currentDepth)` con distancia al mate.
- `evaluateForPlayer()` = amenazas inmediatas + `analyzePosition()`.
- **Patrones**: escaneo de líneas en 4 direcciones, detección de gaps (ventana de 6), scoring por tipo:
  - `WIN` = 600,000
  - `FOUR_OPEN` = 50,000
  - `FOUR_HALF` = 25,000
  - `THREE_OPEN` = 10,000
  - `THREE_HALF` = 1,500
  - `TWO_OPEN` = 100
  - `CAPTURE_OPPORTUNITY` = 5,000
  - `CAPTURE_THREAT` = 6,000
  - `CAPTURE_WIN` = 500,000
  - `CAPTURE_PREVENT_LOSS` = 400,000
- **Capturas**: `findAllCaptureOpportunities()` busca pares del oponente flanqueables, `evaluateCaptureContext()` puntúa según proximidad a victoria, ruptura de patrones, valor ofensivo.
- **Amenazas inmediatas**: detecta FOUR_OPEN, FOUR_HALF, múltiples THREE_OPEN.

### 7.6 Suggestion Engine — `src/ai_engine/suggestion_engine.cpp`
- Crea una `AI` temporal con depth 6 para generar sugerencias rápidas en modo hotseat.
- Fallback a `getQuickSuggestion()` con heurísticas simples (victoria → bloqueo → four → three → captura → patrón → centralidad).

---

## 8. Zobrist Hashing — `src/utils/zobrist_hasher.cpp`

- Tabla de 19×19×3 claves aleatorias de 64 bits + `turnHash` + `captureHashes[2][11]`.
- `computeFullHash()` O(n²) solo para inicialización.
- `updateHashAfterMove()` O(1): XOR de pieza colocada + piezas capturadas + cambio de turno + actualización de capturas.
- Inicialización con `std::random_device` + `std::mt19937_64` con entropía del sistema.
- El hasher es estático compartido (`GameState::hasher`), inicializado una vez en `main()`.

---

## 9. GUI SFML — Resumen Visual

- Ventana 1000×800, tablero 600×600 a la izquierda, panel de info a la derecha.
- **Tablero**: celdas individuales con efecto 3D (biseles, sombras interiores), coordenadas A-S / 1-19.
- **Fichas**: círculos con sombra + highlight 3D + brillo. Última jugada IA con anillo pulsante dorado.
- **Hover**: ondas cian expandiéndose + partículas giratorias + fondo pulsante.
- **Sugerencia**: fondo dorado + icono bombilla con `?`.
- **Error**: fondo rojo pulsante durante 2 segundos + sonido.
- **Fondo moderno**: gradiente dinámico animado + partículas flotantes + grid de puntos.
- **Game Over**: animación de victoria (115 frames) o derrota (41 frames) centrada en el tablero. Botón **NEXT** debajo de la animación para ocultarla y ver el estado final del tablero. Panel lateral con estadísticas + botones NEW GAME / MAIN MENU.
- **Menú**: 6 botones (VS AI, VS Human, Colorblind, Rust AI, Options, Exit) con hover effects y partículas.
- **Options**: controles de música, SFX, volumen (sliders ±10%), debug toggle.
- **Audio**: música de fondo en loop (OGG), 5 efectos de sonido (place_piece, invalid_move, click_menu, victory, defeat).

---

## 10. Sistema de Debug

- `DebugAnalyzer` global (`g_debugAnalyzer`) con 5 niveles: OFF → TOP_MOVES → CRITICAL → HEURISTIC → POSITIONS.
- Activable desde Options (toggle en GUI).
- Captura real del evaluador durante minimax (solo en nodos raíz) vía `g_evalDebug`.
- Logging a fichero `gomoku_debug.log` con análisis de los 10 mejores movimientos, breakdown de heurística, tablero, estadísticas de rendimiento.

---

## 11. Build System — `Makefile`

- C++17 con `-Wall -Wextra -Werror -g3 -O3`.
- `scripts/setup.sh` descarga SFML y dependencias en `external/sfml/`.
- Rust lib compilada con `cargo build --release`.
- `make run` configura `LD_LIBRARY_PATH` automáticamente.
- Para ejecución manual: `export LD_LIBRARY_PATH=./external/sfml/lib:$LD_LIBRARY_PATH && ./gomoku`

---

## 12. Mapa de Ficheros — Referencia Rápida

```
src/
├── main.cpp                          → Bucle principal, máquina de estados
├── core/
│   ├── game_engine.cpp               → newGame(), makeHumanMove(), makeAIMove(), isGameOver(), checkAndSetForcedCaptures()
│   └── game_types.cpp                → GameState constructor, operator=, hash management
├── rule_engine/
│   ├── rules_core.cpp                → applyMove(), isLegalMove()
│   ├── rules_capture.cpp             → findAllCaptures(), findCaptures(), canBreakLineByCapture()
│   ├── rules_validation.cpp          → createsDoubleFreeThree(), isFreeThree(), isValidFreeThreePattern()
│   └── rules_win.cpp                 → checkWin(), checkLineWinInDirection()
├── ai_engine/
│   ├── ai_engine_core.cpp            → getBestMove(), getDepthForGamePhase()
│   ├── search_minimax.cpp            → minimax(), findBestMoveIterative()
│   ├── search_ordering.cpp           → generateOrderedMoves(), quickEvaluateMove(), generateCandidatesAdaptiveRadius()
│   ├── search_transposition.cpp      → lookupTransposition(), storeTransposition(), clearCache()
│   ├── evaluator_patterns.cpp        → analyzeLine(), patternToScore(), countPatternType()
│   ├── evaluator_position.cpp        → evaluate(), evaluateForPlayer(), analyzePosition()
│   ├── evaluator_threats.cpp         → evaluateImmediateThreats(), hasWinningThreats(), evaluateCaptureContext()
│   └── suggestion_engine.cpp         → getSuggestion(), getQuickSuggestion()
├── gui/
│   ├── gui_renderer_core.cpp         → Constructor, processEvents(), render()
│   ├── gui_renderer_menu.cpp         → renderMenu(), renderOptions(), handleMenuClick(), handleOptionsClick()
│   ├── gui_renderer_board.cpp        → drawBoard(), drawPieces(), boardPositionToPixel()
│   ├── gui_renderer_game.cpp         → renderGame(), handleGameClick(), getUserMove()
│   ├── gui_renderer_gameover.cpp     → renderGameOver(), handleGameOverClick(), handleMouseMove()
│   ├── gui_renderer_effects.cpp      → drawHoverIndicator(), drawSuggestionIndicator(), drawModernBackground()
│   └── gui_renderer_ui.cpp           → drawButton(), drawText(), drawGameInfo()
├── ui/
│   ├── audio_manager.cpp             → loadMusic(), playSound(), toggleMute()
│   └── display.cpp                   → printBoard(), getUserMove() (legacy consola)
├── utils/
│   └── zobrist_hasher.cpp            → computeFullHash(), updateHashAfterMove()
└── debug/
    ├── debug_core.cpp                → DebugAnalyzer constructor, logging functions
    ├── debug_analyzer.cpp            → evaluateWithBreakdown(), createSnapshot(), printCurrentAnalysis()
    └── debug_formatter.cpp           → formatMove(), formatBoard(), analyzeGamePhase()
```
