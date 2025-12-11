# Ruby Snake on Game Boy - Makefile
GBDK_HOME = $(HOME)/gbdk
LCC = $(GBDK_HOME)/bin/lcc
MRBC = mrbc

# Compiler flags
CFLAGS = -Wa-l -Wl-m -Wl-j -Isrc

# Source files
VM_SRCS = src/mrbz/vm.c src/mrbz/builtins.c
GB_SRCS = src/gb/main.c src/gb/platform.c src/gb/tiles.c

.PHONY: all clean run

# Default target - snake game
all: snake.gb

# Compile snake Ruby to bytecode
src/game/snake.ruby.c: src/game/snake.rb
	$(MRBC) -B snake_bytecode -o $@ $<

# Snake game ROM
snake.gb: $(VM_SRCS) $(GB_SRCS) src/game/snake.ruby.c
	$(LCC) $(CFLAGS) -o $@ $(VM_SRCS) $(GB_SRCS)

# Run in mGBA
run: snake.gb
	open -a mGBA snake.gb

# Clean build artifacts
clean:
	rm -f *.gb *.map *.sym *.o *.asm *.lst
	rm -f src/game/*.ruby.c
