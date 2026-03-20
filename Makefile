# Ruby Snake on Game Boy - Makefile
GBDK_HOME = $(HOME)/gbdk
LCC = $(GBDK_HOME)/bin/lcc
MRBC = mrbc

# Compiler flags
CFLAGS = -Wa-l -Wl-m -Wl-j -Isrc

# Source files
VM_SRCS = src/mrbz/vm.c src/mrbz/builtins.c
GB_SRCS = src/gb/main.c src/gb/platform.c src/gb/tiles.c

.PHONY: all clean run color run-color ruby3d ruby3d-color run-ruby3d run-ruby3d-color

# Default target - snake game (DMG)
all: snake.gb

# Game Boy Color target
color: snake-color.gb

# Ruby 3D demo targets
ruby3d: ruby3d.gb
ruby3d-color: ruby3d-color.gb

# Compile Ruby to bytecode
src/game/snake.ruby.c: src/game/snake.rb
	$(MRBC) -B snake_bytecode -o $@ $<

src/game/ruby3d.ruby.c: src/game/ruby3d.rb
	$(MRBC) -B ruby3d_bytecode -o $@ $<

# Snake game ROM (DMG)
snake.gb: $(VM_SRCS) $(GB_SRCS) src/game/snake.ruby.c
	$(LCC) $(CFLAGS) -o $@ $(VM_SRCS) $(GB_SRCS)

# Snake game ROM (Game Boy Color)
snake-color.gb: $(VM_SRCS) $(GB_SRCS) src/game/snake.ruby.c
	$(LCC) $(CFLAGS) -DCGB -Wm-yC -o $@ $(VM_SRCS) $(GB_SRCS)

# Ruby 3D demo ROM (DMG)
ruby3d.gb: $(VM_SRCS) $(GB_SRCS) src/game/ruby3d.ruby.c
	$(LCC) $(CFLAGS) -DGAME_RUBY3D -o $@ $(VM_SRCS) $(GB_SRCS)

# Ruby 3D demo ROM (Game Boy Color)
ruby3d-color.gb: $(VM_SRCS) $(GB_SRCS) src/game/ruby3d.ruby.c
	$(LCC) $(CFLAGS) -DGAME_RUBY3D -DCGB -Wm-yC -o $@ $(VM_SRCS) $(GB_SRCS)

# Run in mGBA
run: snake.gb
	open -a mGBA snake.gb

run-color: snake-color.gb
	open -a mGBA snake-color.gb

run-ruby3d: ruby3d.gb
	open -a mGBA ruby3d.gb

run-ruby3d-color: ruby3d-color.gb
	open -a mGBA ruby3d-color.gb

# Clean build artifacts
clean:
	rm -f *.gb *.map *.sym *.noi *.o *.asm *.lst
	rm -f src/game/*.ruby.c
