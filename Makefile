psearch: psearch.c
	gcc -g -pthread -Wall psearch.c -o psearch

clean:
	rm -f psearch
