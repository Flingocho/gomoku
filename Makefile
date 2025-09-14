CXX = c++
CXXFLAGS = -Wall -Wextra -Werror

SRCS = src/ai.cpp\
		src/display.cpp\
		src/evaluator.cpp\
		src/game_engine.cpp\
		src/game_types.cpp\
		src/main.cpp\
		src/rule_engine.cpp\
		src/transposition_search.cpp\
		src/zobrist_hasher.cpp
OBJS = $(SRCS:.cpp=.o)

EXEC = gomoku

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CXX) $(OBJS) -o $(EXEC)

%.o: %.cpp %.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(EXEC)

re: fclean all

.PHONY: all clean fclean re
