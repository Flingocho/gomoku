# 🧪 Gomoku AI — Test Suite

> Documentación completa del test suite ubicado en `tests/test_ai.cpp`.

## Resumen

| Métrica | Valor |
|---------|-------|
| **Tests totales** | 148 |
| **Tests pasados** | ✅ 148 |
| **Tests fallidos** | ❌ 0 |
| **Tiempo total** | ~19.8s |
| **Compilador** | C++17 (`-Wall -Wextra -Werror -g3 -O3`) |
| **Dependencias** | SFML 2.5, `libgomoku_ai_rust` |

---

## Cómo ejecutar

```bash
# 1. Compilar la librería Rust (si no está compilada)
cd gomoku_ai_rust && cargo build --release && cd ..

# 2. Compilar los tests
cd tests && make clean && make

# 3. Ejecutar
LD_LIBRARY_PATH=../external/sfml/lib:../gomoku_ai_rust/target/release ./test_ai
```

---

## Framework de testing

Se utiliza un framework custom ligero basado en macros de C++:

| Macro | Descripción |
|-------|-------------|
| `TEST(name) { ... } END_TEST;` | Define un test con captura de excepciones |
| `ASSERT(cond)` | Verifica que la condición sea verdadera |
| `ASSERT_EQ(a, b)` | Verifica igualdad (`==`) |
| `ASSERT_NE(a, b)` | Verifica desigualdad (`!=`) |
| `ASSERT_GT(a, b)` | Verifica mayor que (`>`) |
| `ASSERT_GE(a, b)` | Verifica mayor o igual (`>=`) |
| `ASSERT_LT(a, b)` | Verifica menor que (`<`) |
| `SECTION(name)` | Imprime cabecera de sección |

Los tests atrapan excepciones (`std::exception` y `...`), mostrando `PASSED` en verde o `FAILED` en rojo con detalle del error.

---

## Secciones de test

### 1. Move Struct (5 tests)

Verifica la estructura básica `Move` que representa una jugada en el tablero.

| # | Test | Descripción | Resultado |
|---|------|-------------|-----------|
| 1 | Default move is invalid | Un `Move()` sin coordenadas tiene `x=-1, y=-1` y `isValid()` retorna `false` | ✅ PASSED |
| 2 | Move with valid coords is valid | `Move(0,0)`, `Move(18,18)`, `Move(9,9)` son válidos | ✅ PASSED |
| 3 | Move out of bounds is invalid | Coordenadas negativas o ≥19 son inválidas | ✅ PASSED |
| 4 | Move equality operator | Operador `==` compara correctamente `x` e `y` | ✅ PASSED |
| 5 | Move boundary values (0 and 18) | Las 4 esquinas del tablero son coordenadas válidas | ✅ PASSED |

---

### 2. GameState (14 tests)

Verifica el estado del juego: tablero 19×19, jugadores, capturas, hash Zobrist, constantes y copias.

| # | Test | Descripción | Resultado |
|---|------|-------------|-----------|
| 1 | Initial state is empty board | Todas las 361 celdas están `EMPTY` | ✅ PASSED |
| 2 | Initial captures are zero | `captures[0]` y `captures[1]` son 0 | ✅ PASSED |
| 3 | Player 1 starts | `currentPlayer == PLAYER1`, `turnCount == 0` | ✅ PASSED |
| 4 | isValid boundary checks | Verifica límites: `(0,0)` y `(18,18)` válidos, negativos y ≥19 inválidos | ✅ PASSED |
| 5 | isEmpty on empty and occupied cell | Celda vacía retorna `true`, celda ocupada retorna `false` | ✅ PASSED |
| 6 | getPiece returns correct values | Retorna `EMPTY`, `PLAYER1`, `PLAYER2`, o `-1` fuera de rango | ✅ PASSED |
| 7 | getOpponent returns correct opponent | P1→P2, P2→P1 | ✅ PASSED |
| 8 | Copy constructor preserves full state | La copia mantiene tablero, capturas, jugador y turno | ✅ PASSED |
| 9 | Assignment operator preserves state | Operador `=` copia correctamente | ✅ PASSED |
| 10 | Copy does not create aliased boards | Modificar la copia no altera el original | ✅ PASSED |
| 11 | Zobrist hash changes with piece placement | El hash cambia al colocar una pieza | ✅ PASSED |
| 12 | Constants are correct | `BOARD_SIZE=19`, `BOARD_CENTER=9`, `EMPTY=0`, `PLAYER1=1`, `PLAYER2=2`, `WIN_CAPTURES_NORMAL=10` | ✅ PASSED |
| 13 | depth getters and setters | `SetDepth(5)` → `getDepth() == 5` | ✅ PASSED |
| 14 | Forced capture fields initialized | `forcedCapturePlayer=0`, `pendingWinPlayer=0`, vector vacío | ✅ PASSED |

---

### 3. RuleEngine — Aplicación de movimientos (5 tests)

Verifica que `RuleEngine::applyMove` coloca piezas correctamente y alterna turnos.

| # | Test | Descripción | Resultado |
|---|------|-------------|-----------|
| 1 | Apply move on empty cell succeeds | Movimiento exitoso, pieza colocada, turno cambia | ✅ PASSED |
| 2 | Apply move on occupied cell fails | Movimiento en celda ocupada retorna `success=false` | ✅ PASSED |
| 3 | Alternating players after moves | P1→P2→P1, `turnCount` incrementa correctamente | ✅ PASSED |
| 4 | Multiple moves fill board correctly | Tres movimientos alternan P1/P2/P1 | ✅ PASSED |
| 5 | Successful move sets createsWin flag | Al completar 5 en línea, `result.createsWin == true` | ✅ PASSED |

---

### 4. RuleEngine — Validación de movimientos legales (4 tests)

| # | Test | Descripción | Resultado |
|---|------|-------------|-----------|
| 1 | Empty cell is legal | Celda vacía es jugada legal | ✅ PASSED |
| 2 | Occupied cell is not legal | Celda ocupada es jugada ilegal | ✅ PASSED |
| 3 | All corners are legal on empty board | Las 4 esquinas son legales en tablero vacío | ✅ PASSED |
| 4 | Center cell is legal on empty board | El centro (9,9) es legal | ✅ PASSED |

---

### 5. RuleEngine — Capturas (11 tests)

Verifica el sistema de capturas: patrón `X-OO-X` donde el jugador X encierra un par OO.

| # | Test | Descripción | Resultado |
|---|------|-------------|-----------|
| 1 | Basic horizontal capture | P1-P2-P2-P1 → captura horizontal, piezas eliminadas | ✅ PASSED |
| 2 | Vertical capture | Captura en dirección vertical | ✅ PASSED |
| 3 | Diagonal capture (down-right) | Captura en diagonal descendente-derecha | ✅ PASSED |
| 4 | Diagonal capture (down-left) | Captura en diagonal descendente-izquierda | ✅ PASSED |
| 5 | No capture when pattern is incomplete | Sin patrón completo, no hay captura | ✅ PASSED |
| 6 | No capture when middle pieces are same color | P1-P1-P1-P1 no produce captura | ✅ PASSED |
| 7 | Multiple captures in one move | Un movimiento captura en 2 direcciones (4 piezas, 2 pares) | ✅ PASSED |
| 8 | findCaptures detects without applying | Detecta capturas sin modificar el tablero | ✅ PASSED |
| 9 | P2 can capture P1 pair | P2 también puede capturar pares de P1 | ✅ PASSED |
| 10 | Captures increment cumulatively | Las capturas se acumulan entre turnos | ✅ PASSED |
| 11 | Captures are capped at 10 | El contador se detiene en 10 (victoria) | ✅ PASSED |

---

### 6. RuleEngine — Detección de victoria (16 tests)

Verifica condiciones de victoria: 5 en línea (horizontal, vertical, diagonal) y 10 capturas.

| # | Test | Descripción | Resultado |
|---|------|-------------|-----------|
| 1 | No win on empty board | Tablero vacío: nadie gana | ✅ PASSED |
| 2 | Horizontal five in a row wins | 5 horizontales → victoria | ✅ PASSED |
| 3 | Vertical five in a row wins | 5 verticales → victoria | ✅ PASSED |
| 4 | Diagonal (down-right) five wins | 5 diagonal ↘ → victoria | ✅ PASSED |
| 5 | Diagonal (down-left) five wins | 5 diagonal ↙ → victoria P2 | ✅ PASSED |
| 6 | Four in a row does NOT win | 4 en línea no es victoria | ✅ PASSED |
| 7 | Three in a row does NOT win | 3 en línea no es victoria | ✅ PASSED |
| 8 | Win by captures (10 pairs) | 10 capturas = victoria | ✅ PASSED |
| 9 | 9 captures is NOT enough | 9 capturas no es victoria | ✅ PASSED |
| 10 | Six in a row also wins | 6+ en línea también gana (overline) | ✅ PASSED |
| 11 | Win at top row | Victoria horizontal en fila 0 | ✅ PASSED |
| 12 | Win at bottom row | Victoria horizontal en fila 18 | ✅ PASSED |
| 13 | Win at left column | Victoria vertical en columna 0 | ✅ PASSED |
| 14 | Win at right column | Victoria vertical en columna 18 | ✅ PASSED |
| 15 | Win at corner diagonal | Victoria diagonal desde (0,0) | ✅ PASSED |
| 16 | P2 win by captures | P2 gana con 10 capturas, P1 no | ✅ PASSED |

---

### 7. RuleEngine — Doble tres libre (2 tests)

Verifica la regla de prohibición de doble tres libre.

| # | Test | Descripción | Resultado |
|---|------|-------------|-----------|
| 1 | Single free three is allowed | Un solo tres libre está permitido | ✅ PASSED |
| 2 | No free-three on isolated stone | Una piedra aislada no crea tres libre | ✅ PASSED |

---

### 8. Evaluator (12 tests)

Verifica el evaluador de posiciones: puntuación de tablero, amenazas, patrones y combinaciones.

| # | Test | Descripción | Resultado |
|---|------|-------------|-----------|
| 1 | Empty board evaluates close to zero | Tablero vacío ≈ 0 (rango ±1000) | ✅ PASSED |
| 2 | Player with more pieces has better eval | Más piezas → mejor evaluación | ✅ PASSED |
| 3 | Five in a row scores ≥ WIN | 5 en línea puntúa al menos `Evaluator::WIN` | ✅ PASSED |
| 4 | Open four > open three | 4 abierto puntúa más que 3 abierto | ✅ PASSED |
| 5 | Captures affect evaluation positively | Capturas mejoran la evaluación | ✅ PASSED |
| 6 | evaluateImmediateThreats detects four | 4 en línea detectado como amenaza | ✅ PASSED |
| 7 | evaluateImmediateThreats returns 0 empty | Tablero vacío = 0 amenazas | ✅ PASSED |
| 8 | hasWinningThreats detects open four | 4 abierto es amenaza ganadora | ✅ PASSED |
| 9 | hasWinningThreats false for empty | Sin amenazas en tablero vacío | ✅ PASSED |
| 10 | Evaluation with mate distance scoring | Evaluación con puntuación por distancia al mate (sin crash) | ✅ PASSED |
| 11 | Evaluation is symmetric for mirrors | Posiciones espejo tienen evaluación similar (diff < 500) | ✅ PASSED |
| 12 | evaluateCombinations detects fork | Posición con múltiples amenazas detectada | ✅ PASSED |

---

### 9. AI — Funcionalidad básica (16 tests — C++ & Rust)

Verifica operaciones fundamentales de la IA: movimientos válidos, completar victoria, bloquear amenazas. Cada test de comportamiento se ejecuta **dos veces** (una por implementación).

| # | Test | Descripción | C++ | Rust |
|---|------|-------------|:---:|:----:|
| 1–2 | AI returns valid move on near-empty board | IA retorna movimiento válido en tablero casi vacío | ✅ | ✅ |
| 3–4 | AI plays near existing stones | IA juega cerca de las piedras existentes (radio adaptativo) | ✅ | ✅ |
| 5–6 | AI completes five in a row | IA detecta victoria inmediata y la completa | ✅ | ✅ |
| 7–8 | AI blocks opponent four (half-open) | IA bloquea 4 en línea semi-abierto del oponente | ✅ | ✅ |
| 9–10 | AI responds to open three threat | IA responde a amenaza de 3 abierto | ✅ | ✅ |
| 11–12 | AI never returns an occupied cell | En tablero parcialmente lleno, nunca retorna celda ocupada | ✅ | ✅ |
| 13 | AI depth getter/setter | `setDepth()` / `getDepth()` funcionan | ✅ | — |
| 14 | AI implementation getter/setter | Cambio entre `CPP_IMPLEMENTATION` y `RUST_IMPLEMENTATION` | ✅ | — |
| 15 | AI getLastNodesEvaluated > 0 | Después de búsqueda, nodos evaluados > 0 | ✅ | — |
| 16 | AI cache can be cleared | `clearCache()` no causa crash | ✅ | — |

> **Nota:** Los tests 13–16 son exclusivos de la API C++ (getters/setters de profundidad, nodos, caché). Los tests 1–12 validan ambas implementaciones con el mismo escenario.

---

### 10. AI — Escenarios estratégicos (8 tests — C++ & Rust)

Verifica la capacidad estratégica de la IA en situaciones complejas. Cada test se ejecuta **dos veces** (una por implementación).

| # | Test | Descripción | C++ | Rust |
|---|------|-------------|:---:|:----:|
| 1–2 | AI prefers capture when it leads to win | Con 9 capturas, IA busca la captura #10 (victoria) | ✅ | ✅ |
| 3–4 | AI prioritizes winning over blocking | Con victoria disponible + amenaza rival, IA gana primero | ✅ | ✅ |
| 5–6 | AI responds in reasonable time at depth 6 | Respuesta en < 30s (C++: ~105ms, Rust: ~31ms) | ✅ | ✅ |
| 7–8 | AI on empty board handles gracefully | Tablero vacío sin candidatos → manejo sin crash | ✅ | ✅ |

---

### 11. TranspositionSearch (7 tests)

Verifica la tabla de transposición y la búsqueda iterativa con profundización progresiva.

| # | Test | Descripción | Resultado |
|---|------|-------------|-----------|
| 1 | findBestMoveIterative returns valid move | Búsqueda iterativa retorna movimiento válido + nodos > 0 | ✅ PASSED |
| 2 | generateOrderedMoves non-empty | Genera candidatos no vacíos para tablero no vacío | ✅ PASSED |
| 3 | generateOrderedMoves within board | Todos los movimientos están dentro del tablero 19×19 | ✅ PASSED |
| 4 | quickEvaluateMove: winning highest | Movimiento ganador tiene mayor puntuación rápida | ✅ PASSED |
| 5 | Iterative deepening at depth 2 fast | Profundidad 2 completa en < 5s (medido: 0ms, 13 nodos) | ✅ PASSED |
| 6 | Cache hit rate improves on repeated eval | Segunda búsqueda tiene tasa de cache hits ≥ primera (100%) | ✅ PASSED |
| 7 | Search result score is reasonable | Puntuación dentro de rango ±10M | ✅ PASSED |

---

### 12. SuggestionEngine (5 tests)

Verifica el motor de sugerencias: sugerencia rápida y completa para asistencia al jugador.

| # | Test | Descripción | Resultado |
|---|------|-------------|-----------|
| 1 | getSuggestion returns valid move | Sugerencia completa retorna movimiento válido | ✅ PASSED |
| 2 | getQuickSuggestion returns valid move | Sugerencia rápida retorna movimiento válido | ✅ PASSED |
| 3 | Quick suggestion blocks obvious four | Sugerencia rápida bloquea 4 en línea obvio | ✅ PASSED |
| 4 | Quick suggestion is faster than full AI | Sugerencia rápida más veloz que búsqueda completa (0ms vs 38ms) | ✅ PASSED |
| 5 | getSuggestion finds winning move | Sugerencia detecta movimiento ganador | ✅ PASSED |

---

### 13. GameEngine — Integración (12 tests)

Verifica el motor de juego completo: ciclo de vida, modos de juego, estadísticas de IA.

| # | Test | Descripción | Resultado |
|---|------|-------------|-----------|
| 1 | newGame resets state completely | Reset total: tablero vacío, capturas 0, P1 empieza | ✅ PASSED |
| 2 | makeHumanMove places P1 correctly | Movimiento humano coloca pieza de P1 | ✅ PASSED |
| 3 | Game is not over at start | Partida nueva no está terminada | ✅ PASSED |
| 4 | makeAIMove returns valid move | Movimiento de IA válido y colocado en tablero | ✅ PASSED |
| 5 | Game mode can be set and queried | `VS_AI`, `VS_HUMAN_SUGGESTED` configurables | ✅ PASSED |
| 6 | VS_HUMAN_SUGGESTED allows both players | Ambos jugadores pueden jugar manualmente | ✅ PASSED |
| 7 | Full game: P1 wins by five in a row | Partida completa: P1 gana con 5 verticales | ✅ PASSED |
| 8 | findWinningLine returns 5+ pieces | Línea ganadora tiene ≥ 5 piezas | ✅ PASSED |
| 9 | AI thinking time is tracked | Tiempo de pensamiento ≥ 0 registrado | ✅ PASSED |
| 10 | AI stats are accessible after move | Nodos, cache hits, tasa de cache, tamaño de cache accesibles | ✅ PASSED |
| 11 | newGame after moves resets everything | Segundo `newGame()` limpia todo correctamente | ✅ PASSED |

---

### 14. Zobrist Hashing (6 tests)

Verifica el hash Zobrist usado para la tabla de transposición.

| # | Test | Descripción | Resultado |
|---|------|-------------|-----------|
| 1 | Same board produces same hash | Misma posición → mismo hash | ✅ PASSED |
| 2 | Different positions produce different hashes | Posiciones distintas → hashes distintos | ✅ PASSED |
| 3 | Different players at same position | P1 vs P2 en misma celda → hashes distintos | ✅ PASSED |
| 4 | Hash changes with each piece | Cada piedra nueva cambia el hash | ✅ PASSED |
| 5 | Empty board hash consistent | Tableros vacíos distintos tienen mismo hash | ✅ PASSED |
| 6 | Hash deterministic for complex board | Tablero complejo → hash reproducible | ✅ PASSED |

---

### 15. Edge Cases & Stress (9 tests)

Verifica casos límite y situaciones de estrés.

| # | Test | Descripción | Resultado |
|---|------|-------------|-----------|
| 1 | AI handles nearly full board | 350 piezas en tablero → IA maneja sin crash | ✅ PASSED |
| 2 | Evaluator handles single stone | Evaluar tablero con 1 sola piedra | ✅ PASSED |
| 3 | AI search at depth 1 | Búsqueda mínima funciona | ✅ PASSED |
| 4 | AI search at depth 2 | Búsqueda a profundidad 2 funciona | ✅ PASSED |
| 5 | Multiple AI instances don't interfere | Dos instancias de IA independientes sin interferencia | ✅ PASSED |
| 6 | Capture at board edge (top-left) | Captura en esquina superior-izquierda | ✅ PASSED |
| 7 | Capture at board edge (bottom-right) | Captura en esquina inferior-derecha | ✅ PASSED |
| 8 | Win detection at all four corners | Victoria detectada desde las 4 esquinas | ✅ PASSED |

---

### 16. Consistency & Determinism (3 tests)

Verifica que la IA sea determinista y consistente.

| # | Test | Descripción | Resultado |
|---|------|-------------|-----------|
| 1 | Same position → same best move | Dos instancias dan el mismo mejor movimiento | ✅ PASSED |
| 2 | AI score positive when winning | Puntuación positiva cuando la IA tiene ventaja decisiva | ✅ PASSED |
| 3 | Higher depth ≥ nodes than lower | Mayor profundidad evalúa ≥ nodos que menor | ✅ PASSED |

---

### 17. Pattern Counting (7 tests)

Verifica el conteo de patrones del evaluador: dos abiertos, tres abiertos/semi-abiertos, cuatro.

| # | Test | Descripción | Resultado |
|---|------|-------------|-----------|
| 1 | countAllPatterns finds open two | 2 piedras adyacentes → `twoOpen ≥ 1` | ✅ PASSED |
| 2 | countAllPatterns finds open three | 3 piedras → `threeOpen + threeHalf ≥ 1` | ✅ PASSED |
| 3 | countAllPatterns finds four | 4 piedras → `fourOpen + fourHalf ≥ 1` | ✅ PASSED |
| 4 | No patterns on empty board | Tablero vacío = 0 patrones en todas las categorías | ✅ PASSED |
| 5 | countPatternType detects specific | Función específica detecta tres abiertos/semi-abiertos | ✅ PASSED |
| 6 | Blocked pattern has fewer free ends | Patrón bloqueado detectado como semi-abierto | ✅ PASSED |
| 7 | Patterns counted independently | P1 y P2 tienen sus propios conteos independientes | ✅ PASSED |

---

### 18. Move Ordering (3 tests)

Verifica que el ordenamiento de movimientos priorice movimientos críticos.

| # | Test | Descripción | Resultado |
|---|------|-------------|-----------|
| 1 | Winning move scored highest | `quickEvaluateMove` puntúa victoria > normal | ✅ PASSED |
| 2 | Ordered moves have winning first | Victoria aparece primero en lista ordenada | ✅ PASSED |
| 3 | Blocking move near top | Movimiento de bloqueo entre los 5 primeros | ✅ PASSED |

---

### 19. Game Simulation (5 tests — C++, Rust & Cross-play)

Simula partidas completas para verificar integración end-to-end. La simulación AI vs AI se ejecuta para ambas implementaciones, además de tests de interoperabilidad cruzada.

| # | Test | Descripción | C++ | Rust |
|---|------|-------------|:---:|:----:|
| 1–2 | AI vs AI completes a game | IA contra sí misma termina partida (C++: 31 mov, Rust: 32 mov) | ✅ | ✅ |
| 3 | Win by capture in full game simulation | Victoria por capturas es alcanzable (10 pares) | ✅ | — |
| 4 | C++ vs Rust — both produce valid moves | Misma posición: ambas IAs retornan movimientos válidos y cercanos | ✅ + 🦀 | ✅ + 🦀 |
| 5 | C++ vs Rust cross-play game completes | Partida cruzada C++ (P1) vs Rust (P2) termina correctamente (41 mov) | ✅ + 🦀 | ✅ + 🦀 |

> **Nota:** Los tests de cross-play (4–5) validan que ambas implementaciones son interoperables: comparten el mismo `GameState` y sus movimientos son mutuamente legales.

---

## Cobertura por módulo

| Módulo | Archivo(s) | Tests | Estado |
|--------|-----------|-------|--------|
| `Move` | `game_types.hpp/cpp` | 5 | ✅ 5/5 |
| `GameState` | `game_types.hpp/cpp` | 14 | ✅ 14/14 |
| `RuleEngine` | `rules_core.cpp`, `rules_capture.cpp`, `rules_validation.cpp`, `rules_win.cpp` | 38 | ✅ 38/38 |
| `Evaluator` | `evaluator_patterns.cpp`, `evaluator_position.cpp`, `evaluator_threats.cpp` | 12 | ✅ 12/12 |
| `AI` (C++ & Rust) | `ai_engine_core.cpp`, `search_minimax.cpp`, `search_ordering.cpp`, FFI `libgomoku_ai_rust` | 24 | ✅ 24/24 |
| `TranspositionSearch` | `search_transposition.cpp` | 7 | ✅ 7/7 |
| `SuggestionEngine` | `suggestion_engine.cpp` | 5 | ✅ 5/5 |
| `GameEngine` | `game_engine.cpp` | 11 | ✅ 11/11 |
| `Zobrist Hashing` | `zobrist_hasher.cpp` | 6 | ✅ 6/6 |
| Edge Cases / Stress | Transversal | 8 | ✅ 8/8 |
| Consistency | Transversal | 3 | ✅ 3/3 |
| Pattern Counting | `evaluator_patterns.cpp` | 7 | ✅ 7/7 |
| Move Ordering | `search_ordering.cpp` | 3 | ✅ 3/3 |
| Game Simulation (C++, Rust & Cross-play) | Integración completa + interoperabilidad | 5 | ✅ 5/5 |

---

## Rendimiento de la IA durante tests

Algunas métricas notables capturadas durante la ejecución:

| Escenario | Impl | Profundidad | Tiempo | Nodos | Cache Hit Rate |
|-----------|:----:|:-----------:|:------:|:-----:|:--------------:|
| Victoria inmediata | C++ | 8 | **0ms** | 1 | — |
| Victoria inmediata | Rust | 4 | **0ms** | 1 | — |
| Búsqueda básica (near-empty) | C++ | 6 | 9ms | 460 | 31.7% |
| Bloqueo de 4 en línea | C++ | 8 | 41ms | 1,406 | 43.0% |
| Respuesta a 3 abierto | C++ | 8 | 74ms | 2,545 | 33.8% |
| Escenario medio | C++ | 8 | 105ms | 5,127 | 39.6% |
| Escenario medio | Rust | 6 | 31ms | — | — |
| Segunda búsqueda (cache caliente) | C++ | 4 | 0ms | 4 | **100%** |
| AI vs AI (partida completa) | C++ | 6→10 | ~7s | ~200K+ | 30-100% |
| AI vs AI (partida completa) | Rust | 2 | ~1.5s | — | — |
| C++ vs Rust (cross-play) | Ambas | 2→10 | ~5s | — | — |

---

## Resultado final

```
╔═══════════════════════════════════════════════╗
║       GOMOKU AI — Comprehensive Test Suite    ║
╚═══════════════════════════════════════════════╝

  ═══════════════════════════════════════════════
    RESULTS: 148/148 passed
    Time: 19836ms
  ═══════════════════════════════════════════════
```

**✅ Todos los 148 tests pasan. Cada test de IA ejecuta ambas implementaciones (C++ y Rust) en el mismo escenario, validando paridad funcional y estratégica.**
