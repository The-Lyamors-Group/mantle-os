ifeq ($(origin CC),default)
CC := gcc
endif
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -Werror -D_FORTIFY_SOURCE=2

.PHONY: all test clean
all: test-state
test-state: mantle-shell-state.o test-state.o
	$(CC) $(CFLAGS) -o $@ $^
mantle-shell-state.o: mantle-shell-state.c mantle-shell-state.h
	$(CC) $(CFLAGS) -c -o $@ $<
test-state.o: test-state.c mantle-shell-state.h
	$(CC) $(CFLAGS) -c -o $@ $<
test: test-state
	./test-state
clean:
	rm -f *.o test-state
