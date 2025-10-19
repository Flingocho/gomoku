CXX = c++
SFML_HOME := $(HOME)/sfml-2.5.1
RUST_LIB_DIR := gomoku_ai_rust/target/release
INCLUDES := -I$(SFML_HOME)/include -Igomoku_ai_rust/src
LIBS := -L$(SFML_HOME)/lib -L$(RUST_LIB_DIR) -lsfml-graphics -lsfml-window -lsfml-system -lgomoku_ai_rust -ldl -lpthread
CXXFLAGS := -Wall -Wextra -Werror -O3 -std=c++17 $(INCLUDES)

SRCS = src/ai.cpp\
		src/display.cpp\
		src/evaluator.cpp\
		src/game_engine.cpp\
		src/game_types.cpp\
		src/main.cpp\
		src/rule_engine.cpp\
		src/transposition_search.cpp\
		src/zobrist_hasher.cpp\
		src/debug_analyzer.cpp\
		src/gui_renderer.cpp\
		src/suggestion_engine.cpp

OBJ_DIR = objects
OBJS = $(SRCS:src/%.cpp=$(OBJ_DIR)/%.o)

EXEC = gomoku

all: rust_lib $(EXEC)

rust_lib:
	cd gomoku_ai_rust && cargo build --release

$(EXEC): $(OBJS)
	$(CXX) $(OBJS) -o $(EXEC) $(LIBS)

$(OBJ_DIR)/%.o: src/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR)
	cd gomoku_ai_rust && cargo clean

fclean: clean
	rm -f $(EXEC)

re: fclean all

.PHONY: all clean fclean re
