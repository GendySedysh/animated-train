NAME		=	ircserv
SRCS		=	main.cpp Server.cpp User.cpp Command.cpp Chanel.cpp ft_split.cpp
OBJS		=	$(SRCS:.cpp=.o)
DEPS		=	$(SRCS:.cpp=.d)
CC			=	clang++
RM			=	rm -f
CFLAGS		=	-std=c++98 -Wall -Wextra -Werror

.cpp.o			:
				$(CC) $(CFLAGS) -c $< -o $@ -MD

all			:	$(NAME)

$(NAME)		:	$(OBJS)
				$(CC) -o $(NAME) $(OBJS) -fsanitize=address
clean		:
				$(RM) $(OBJS) $(DEPS)

fclean		:	clean
				$(RM) $(NAME)

re:			fclean all

.PHONY:		all clean fclean re

-include $(DEPS)