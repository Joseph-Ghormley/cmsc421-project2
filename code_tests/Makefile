CC = gcc
CFLAGS = -Wall -Wextra -std=c11
TARGETS = main_program test_program

all: $(TARGETS)

main_program: main_program.c
	$(CC) $(CFLAGS) -o main_program main_program.c

test_program: test_program.c
	$(CC) $(CFLAGS) -o test_program test_program.c

clean:
	rm -f $(TARGETS)
