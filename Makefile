NAME = ping
CC = clang

CFLAGS = -std=c17 \
         -Wall \
         -Wextra \
         -Wpedantic \
		 -Werror
		 
MATHS = -lm

SAN = -fsanitize=address,undefined \
	  -fno-sanitize-recover=undefined -g -O1

ping: main.c
	$(CC) $(CFLAGS) main.c -o $(NAME) $(MATHS)

san: main.c
	$(CC) $(CFLAGS) $(SAN) main.c -o $(NAME) $(MATHS)

clean:
	rm -f $(NAME)
