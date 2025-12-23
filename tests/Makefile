CXX = c++
SFML_HOME := ../external/sfml
RUST_LIB_DIR := ../gomoku_ai_rust/target/release
INCLUDES := -I$(SFML_HOME)/include -I../gomoku_ai_rust/src -I../include
LIBS := -L$(SFML_HOME)/lib -L$(RUST_LIB_DIR) -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio -lgomoku_ai_rust -ldl -lpthread -lopenal -lvorbisenc -lvorbisfile -lvorbis -logg -lFLAC -lsndio
CXXFLAGS := -Wall -Wextra -Werror -g3 -O3 -std=c++17 $(INCLUDES)

# Source files needed for AI
SRCS = ../src/ai_engine/ai_engine_core.cpp \
	../src/ai_engine/evaluator_patterns.cpp \
	../src/ai_engine/evaluator_position.cpp \
	../src/ai_engine/evaluator_threats.cpp \
	../src/ai_engine/search_minimax.cpp \
	../src/ai_engine/search_ordering.cpp \
	../src/ai_engine/search_transposition.cpp \
	../src/ai_engine/suggestion_engine.cpp \
	../src/core/game_engine.cpp \
	../src/core/game_types.cpp \
	../src/debug/debug_analyzer.cpp \
	../src/debug/debug_core.cpp \
	../src/debug/debug_formatter.cpp \
	../src/rule_engine/rules_capture.cpp \
	../src/rule_engine/rules_core.cpp \
	../src/rule_engine/rules_validation.cpp \
	../src/rule_engine/rules_win.cpp \
	../src/utils/zobrist_hasher.cpp \
    test_ai.cpp

OBJ_DIR = objects
OBJS = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(notdir $(SRCS)))

EXEC = test_ai

$(EXEC): $(OBJS)
	$(CXX) $(OBJS) -o $(EXEC) $(LIBS)

$(OBJ_DIR)/%.o: ../src/ai_engine/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: ../src/core/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: ../src/debug/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: ../src/rule_engine/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: ../src/utils/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: %.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR) $(EXEC)

.PHONY: clean
