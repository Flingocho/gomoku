CXX = c++
SFML_HOME := ./external/sfml
RUST_LIB_DIR := gomoku_ai_rust/target/release
INCLUDES := -I$(SFML_HOME)/include -Igomoku_ai_rust/src
LIBS := -L$(SFML_HOME)/lib -L$(RUST_LIB_DIR) -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio -lgomoku_ai_rust -ldl -lpthread -lopenal -lvorbisenc -lvorbisfile -lvorbis -logg -lFLAC -lsndio
CXXFLAGS := -Wall -Wextra -Werror -g3 -O3 -std=c++17 $(INCLUDES)
DEPFLAGS = -MMD -MP

# Source files organized by folder
SRCS = src/main.cpp \
	src/ai_engine/ai_engine_core.cpp \
	src/ai_engine/evaluator_patterns.cpp \
	src/ai_engine/evaluator_position.cpp \
	src/ai_engine/evaluator_threats.cpp \
	src/ai_engine/search_minimax.cpp \
	src/ai_engine/search_ordering.cpp \
	src/ai_engine/search_transposition.cpp \
	src/ai_engine/suggestion_engine.cpp \
	src/core/game_engine.cpp \
	src/core/game_types.cpp \
	src/debug/debug_analyzer.cpp \
	src/debug/debug_core.cpp \
	src/debug/debug_formatter.cpp \
	src/gui/gui_renderer_board.cpp \
	src/gui/gui_renderer_core.cpp \
	src/gui/gui_renderer_effects.cpp \
	src/gui/gui_renderer_game.cpp \
	src/gui/gui_renderer_gameover.cpp \
	src/gui/gui_renderer_menu.cpp \
	src/gui/gui_renderer_ui.cpp \
	src/rule_engine/rules_capture.cpp \
	src/rule_engine/rules_core.cpp \
	src/rule_engine/rules_validation.cpp \
	src/rule_engine/rules_win.cpp \
	src/ui/audio_manager.cpp \
	src/ui/display.cpp \
	src/utils/zobrist_hasher.cpp

OBJ_DIR = objects
OBJS = $(patsubst src/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))

NAME = Gomoku

all: setup rust_lib $(NAME)

setup:
	@echo "Checking dependencies..."
	@bash scripts/setup.sh

$(RUST_LIB_DIR)/libgomoku_ai_rust.a: $(wildcard gomoku_ai_rust/src/*.rs) gomoku_ai_rust/Cargo.toml
	cd gomoku_ai_rust && cargo build --release

rust_lib: $(RUST_LIB_DIR)/libgomoku_ai_rust.a

$(NAME): $(OBJS) $(RUST_LIB_DIR)/libgomoku_ai_rust.a
	$(CXX) $(OBJS) -o $(NAME) $(LIBS)

$(OBJ_DIR)/%.o: src/%.cpp | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR)
	cd gomoku_ai_rust && cargo clean

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: run
run: $(NAME)
	LD_LIBRARY_PATH=$(SFML_HOME)/lib:$(RUST_LIB_DIR):$$LD_LIBRARY_PATH ./$(NAME)

.PHONY: all clean fclean re setup run

# Auto-generated header dependencies
DEPS = $(OBJS:.o=.d)
-include $(DEPS)
