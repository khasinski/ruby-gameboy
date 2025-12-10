# Ruby Snake on Game Boy - Makefile
GBDK_HOME = $(HOME)/gbdk
LCC = $(GBDK_HOME)/bin/lcc
MRBC = mrbc

# Compiler flags
CFLAGS = -Wa-l -Wl-m -Wl-j -Isrc

# Source files
VM_SRCS = src/mrbz/vm.c src/mrbz/builtins.c
GB_SRCS = src/gb/main.c src/gb/platform.c src/gb/tiles.c

.PHONY: all clean run hello test test-vm

# Default target - test VM
all: test-vm

# Hello World test ROM (no VM, just GBDK test)
hello: hello.gb
	@echo "Built hello.gb - test with: open -a mGBA hello.gb"

hello.gb: src/gb/main.c
	$(LCC) $(CFLAGS) -o $@ $<

# Compile test Ruby to bytecode
src/game/test.ruby.c: src/game/test.rb
	$(MRBC) -B test_bytecode -o $@ $<

# Compile snake Ruby to bytecode
src/game/snake.ruby.c: src/game/snake.rb
	$(MRBC) -B snake_bytecode -o $@ $<

# Test VM ROM - runs simple test.rb
test-vm: test.gb
	@echo "Built test.gb - test with: make run-test"

test.gb: $(VM_SRCS) $(GB_SRCS) src/game/test.ruby.c
	$(LCC) $(CFLAGS) -o $@ $(VM_SRCS) $(GB_SRCS)

# Full game ROM (once snake.rb is ready)
snake.gb: $(VM_SRCS) $(GB_SRCS) src/game/snake.ruby.c
	$(LCC) $(CFLAGS) -DUSE_SNAKE -o $@ $(VM_SRCS) $(GB_SRCS)

# Run in mGBA
run: snake.gb
	open -a mGBA snake.gb

run-test: test.gb
	open -a mGBA test.gb

run-hello: hello.gb
	open -a mGBA hello.gb

# Clean build artifacts
clean:
	rm -f *.gb *.map *.sym *.o *.asm *.lst
	rm -f src/game/*.ruby.c

# Test Ruby bytecode compilation only
test-mrbc:
	@echo "Testing mrbc..."
	@echo "return 42" > /tmp/test.rb
	$(MRBC) -B test_bytecode -o /tmp/test.ruby.c /tmp/test.rb
	@echo "mrbc works!"
	@cat /tmp/test.ruby.c | head -20
