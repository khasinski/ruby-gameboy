# Ruby Snake on Game Boy - Makefile
GBDK_HOME = $(HOME)/gbdk
LCC = $(GBDK_HOME)/bin/lcc
MRBC = mrbc

# Compiler flags
CFLAGS = -Wa-l -Wl-m -Wl-j -Isrc

# Source files
VM_SRCS = src/mrbz/vm.c src/mrbz/builtins.c
GB_SRCS = src/gb/main.c src/gb/platform.c src/gb/tiles.c

.PHONY: all clean run color run-color demo run-demo

# Default target - snake game (DMG)
all: snake.gb

# Game Boy Color target
color: snake-color.gb

# Demo target
demo: demo-color.gb

# Compile Ruby to bytecode
src/game/snake.ruby.c: src/game/snake.rb
	$(MRBC) -B snake_bytecode -o $@ $<

src/game/demo.ruby.c: src/game/demo.rb
	$(MRBC) -B demo_bytecode -o $@ $<

# Snake game ROM (DMG)
snake.gb: $(VM_SRCS) $(GB_SRCS) src/game/snake.ruby.c
	$(LCC) $(CFLAGS) -o $@ $(VM_SRCS) $(GB_SRCS)

# Snake game ROM (Game Boy Color)
snake-color.gb: $(VM_SRCS) $(GB_SRCS) src/game/snake.ruby.c
	$(LCC) $(CFLAGS) -DCGB -Wm-yC -o $@ $(VM_SRCS) $(GB_SRCS)

# Run in mGBA
run: snake.gb
	open -a mGBA snake.gb

run-color: snake-color.gb
	open -a mGBA snake-color.gb

# Demo ROM (Game Boy Color)
demo-color.gb: $(VM_SRCS) $(GB_SRCS) src/game/demo.ruby.c
	$(LCC) $(CFLAGS) -DGAME_DEMO -DCGB -Wm-yC -o $@ $(VM_SRCS) $(GB_SRCS)

run-demo: demo-color.gb
	open -a mGBA demo-color.gb

# Clean build artifacts
clean:
	rm -f *.gb *.map *.sym *.noi *.o *.asm *.lst
	rm -f src/game/*.ruby.c
