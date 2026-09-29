CC = gcc
CPPFLAGS = -D_POSIX_C_SOURCE=200809L -Iinclude
CFLAGS = -std=c99 -g -Wall -Wextra -Wpedantic
LDFLAGS =
SRCS := $(wildcard src/*.c)
OBJS := $(patsubst src/%.c,obj/%.o,$(SRCS))

all: bin/shell

bin/shell: $(OBJS) | bin
	$(CC) $(LDFLAGS) $(OBJS) -o $@

obj/%.o: src/%.c | obj
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

obj bin:
	mkdir -p $@

run: bin/shell
	./bin/shell

obj/test_execution: tests/test_execution.c $(filter-out obj/main.o obj/shell.o,$(OBJS))
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $^ -o $@

test: bin/shell obj/test_execution
	python3 tests/test_shell.py ./bin/shell
	python3 tests/run_execution_test.py ./obj/test_execution

clean:
	rm -f obj/*.o obj/*.d obj/test_execution bin/shell

-include $(OBJS:.o=.d)
.PHONY: all run test clean
