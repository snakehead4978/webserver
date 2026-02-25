NAME = parser

CC = c++

CFLAGS = -Wall -Wextra -Werror -g3 -MMD -MP
CFLAGS += -std=c++98

INC = -Iinclude

vpath %.cpp src

OBJ_DIR = obj/

SRC = parser.cpp Message.cpp server.cpp ACustomSocket.cpp ServerSocket.cpp ClientSocket.cpp misc.cpp Cgi.cpp

OBJ = $(addprefix $(OBJ_DIR), $(SRC:.cpp=.o))

$(OBJ_DIR)%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

$(NAME): $(OBJ)
	$(CC) $(CFLAGS) $(INC) $^ -o $@
-include $(OBJ:.o=.d)

all: $(NAME)

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -rf $(NAME)

re: fclean all

test: re
	clear
	./$(NAME)

valg: re
	clear
	valgrind ./$(NAME)

.PHONY: all clean fclean re valg test


# valgrind --track-fds=yes --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose