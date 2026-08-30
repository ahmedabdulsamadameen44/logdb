bin_logdb: src/main.c src/hashtable.c src/row.c src/index.c src/parser.c
	gcc -g -Wall -o bin_logdb src/main.c src/hashtable.c src/row.c src/index.c src/parser.c

clean:
	rm -f bin_logdb