build: main.c
	cc -Wall -Wextra -Wno-missing-field-initializers -lraylib -o grapher main.c -ggdb

run:
	./grapher
