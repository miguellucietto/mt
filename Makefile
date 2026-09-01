CC = gcc
CLANG_FORMAT ?= clang-format

SDL_CFLAGS := $(shell pkg-config --cflags sdl3 sdl3-ttf)
SDL_LIBS := $(shell pkg-config --libs sdl3 sdl3-ttf) -ldl
CFLAGS = -Wall -Wextra -Wpedantic -std=c17 -D_POSIX_C_SOURCE=200809L -ggdb -Iinclude -MMD -MP $(SDL_CFLAGS)
TARGET ?= mt
TEST_TARGET := build/tests

CFILES = $(wildcard src/*.c)
OBJECTS = $(patsubst src/%.c,build/%.o,$(CFILES))
DEPS = $(OBJECTS:.o=.d)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) -rdynamic $(OBJECTS) -o $@ $(SDL_LIBS)

build/%.o: src/%.c
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	rm -rf build $(TARGET)

format:
	$(CLANG_FORMAT) -i $(CFILES) $(wildcard include/*.h) $(wildcard tests/*.c) $(wildcard examples/*.c)

format-check:
	$(CLANG_FORMAT) --dry-run --Werror $(CFILES) $(wildcard include/*.h) $(wildcard tests/*.c) $(wildcard examples/*.c)

$(TEST_TARGET): tests/tests.c src/document.c src/text.c src/command.c src/keymap.c src/highlight.c src/buffer.c src/minibuffer.c
	@mkdir -p build
	$(CC) $(CFLAGS) $^ -o $@ $(SDL_LIBS)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

package-example:
	@mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared examples/hello-package.c -o build/hello-package.so

.PHONY: all clean format format-check test package-example
