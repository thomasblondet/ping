NAME = ping
CC = cc

CFLAGS = -std=c2x \
         -Wall \
         -Wextra \
         -Wpedantic \
		 -Werror
		 
MATHS = -lm

SAN = -fsanitize=address,undefined \
	  -fno-sanitize-recover=undefined -g -O1

ping: main.c packet.c
	$(CC) $(CFLAGS) $^ -o $(NAME) $(MATHS)

san: main.c packet.c
	$(CC) $(CFLAGS) $(SAN) $^ -o $(NAME) $(MATHS)

clean:
	rm -f $(NAME)
