ping: main.o
	clang -std=c17 -Wall -Wextra -Wpedantic -Werror $^ -o $@

main.o: main.c
	clang -std=c17 -Wall -Wextra -Wpedantic -Werror -c $^

clean:
	rm -f *.o ping