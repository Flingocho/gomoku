CXX = c++
SFML_HOME := $(HOME)/sfml-2.5.1
INCLUDES := -I$(SFML_HOME)/include
LIBS := -L$(SFML_HOME)/lib -lsfml-graphics -lsfml-window -lsfml-system
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
OBJS = $(SRCS:.cpp=.o)

EXEC = gomoku

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CXX) $(OBJS) -o $(EXEC) $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(EXEC)

re: fclean all

.PHONY: all clean fclean re
