# Ruby on Game Boy - RubyKaigi 2026 Demo
# Tiles rain down, stack up, play pentatonic notes, reset when full

TILE = 130
COLS = 20
ROWS = 18

@heights = Array.new(COLS, 0)

while true
  # Pick a random column
  x = rand(COLS)

  if @heights[x] < ROWS
    landing_y = ROWS - 1 - @heights[x]

    # Animate tile falling from top to landing position
    y = 0
    while y < landing_y
      draw_tile(x, y, TILE)
      wait_vbl
      wait_vbl
      clear_tile(x, y)
      y += 1
    end

    # Place tile at final position
    draw_tile(x, landing_y, TILE)
    @heights[x] += 1

    # Play a note from the pentatonic scale
    play_note(x)
  end

  # Check if all columns are full
  full = true
  i = 0
  while i < COLS
    if @heights[i] < ROWS
      full = false
    end
    i += 1
  end

  # Clear and restart when full
  if full
    # Pause before clearing
    i = 0
    while i < 30
      wait_vbl
      i += 1
    end

    # Clear row by row from top
    y = 0
    while y < ROWS
      x = 0
      while x < COLS
        clear_tile(x, y)
        x += 1
      end
      wait_vbl
      y += 1
    end

    i = 0
    while i < COLS
      @heights[i] = 0
      i += 1
    end
  end
end
