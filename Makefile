CFLAGS=-Wall -pedantic -g

.PHONY: all
all: a.out my-malloc.so

a.out: main.c my-malloc.so
	gcc $(CFLAGS) -o a.out main.c my-malloc.so

my-malloc.so: my-malloc.c
	gcc $(CFLAGS) -rdynamic -shared -fPIC -o my-malloc.so my-malloc.c

.PHONY: clean
clean:
	rm -f my-malloc.so a.out
