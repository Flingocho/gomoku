CXX = c++
CXXFLAGS = -Wall -Wextra -Werror

SRCS = src/main.cpp src/display.cpp src/board.cpp src/ai.cpp
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
