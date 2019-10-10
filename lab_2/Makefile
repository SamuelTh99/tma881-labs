.PHONY: all
all: newton

newton: newton.c
	gcc -O2 -o newton newton.c -lm -lpthread

.PHONY: clean
clean:
	rm -f newton
