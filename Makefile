bin_logdb: src/main.c src/hashtable.c
	gcc -g -Wall -o bin_logdb src/main.c src/hashtable.c

clean:
	rm -f bin_logdb