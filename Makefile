CFLAGS=-Wall -pedantic -g

my-malloc.so: my-malloc.o
	gcc -g -Wall -pedantic -rdynamic -shared -fPIC -o my-malloc.so my-malloc.c

.PHONY: clean
clean:
	rm -f my-malloc.so
