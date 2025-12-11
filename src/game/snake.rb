# Snake Game for mrbz Game Boy VM

# Constants
GRID_W = 20
GRID_H = 18
MAX_SNAKE = 50
FRAME_DELAY = 8

# Tile IDs (offset 128 for game tiles)
TILE_HEAD = 129
TILE_BODY = 130
TILE_FOOD = 131

# Direction constants: 0=none, 1=up, 2=down, 3=left, 4=right
DIR_UP = 1
DIR_DOWN = 2
DIR_LEFT = 3
DIR_RIGHT = 4

# Initialize game state
@snake_x = Array.new(MAX_SNAKE, 0)
@snake_y = Array.new(MAX_SNAKE, 0)
@snake_len = 3
@direction = DIR_RIGHT
@food_x = 15
@food_y = 9
@score = 0
@running = true

# Set initial snake position (horizontal, moving right)
@snake_x[0] = 10
@snake_x[1] = 9
@snake_x[2] = 8
@snake_y[0] = 9
@snake_y[1] = 9
@snake_y[2] = 9

# Draw initial state
i = 0
while i < @snake_len
  tile = i == 0 ? TILE_HEAD : TILE_BODY
  draw_tile(@snake_x[i], @snake_y[i], tile)
  i += 1
end
draw_tile(@food_x, @food_y, TILE_FOOD)

# Main game loop
while @running
  # Remember last move direction (to prevent reversal)
  prev_dir = @direction
  turned = false

  # Wait for frames and poll input each frame
  frame_count = 0
  while frame_count < FRAME_DELAY
    wait_vbl
    input = read_joypad

    # Update direction (can't reverse, only one turn per move)
    unless turned
      if input == DIR_UP && prev_dir != DIR_DOWN
        @direction = DIR_UP
        turned = true
      end
      if input == DIR_DOWN && prev_dir != DIR_UP
        @direction = DIR_DOWN
        turned = true
      end
      if input == DIR_LEFT && prev_dir != DIR_RIGHT
        @direction = DIR_LEFT
        turned = true
      end
      if input == DIR_RIGHT && prev_dir != DIR_LEFT
        @direction = DIR_RIGHT
        turned = true
      end
    end

    frame_count += 1
  end

  # Calculate new head position
  new_x = @snake_x[0]
  new_y = @snake_y[0]

  new_y -= 1 if @direction == DIR_UP
  new_y += 1 if @direction == DIR_DOWN
  new_x -= 1 if @direction == DIR_LEFT
  new_x += 1 if @direction == DIR_RIGHT

  # Check wall collision
  @running = false if new_x < 0 || new_x >= GRID_W || new_y < 0 || new_y >= GRID_H

  # Check self collision
  if @running
    i = 0
    while i < @snake_len
      @running = false if @snake_x[i] == new_x && @snake_y[i] == new_y
      i += 1
    end
  end

  # Move snake
  if @running
    # Check if eating food
    ate_food = new_x == @food_x && new_y == @food_y
    @score += 10 if ate_food

    # Clear old tail (unless growing)
    if ate_food
      @snake_len += 1
    else
      tail_idx = @snake_len - 1
      clear_tile(@snake_x[tail_idx], @snake_y[tail_idx])
    end

    # Shift body segments backward
    i = @snake_len - 1
    while i > 0
      @snake_x[i] = @snake_x[i - 1]
      @snake_y[i] = @snake_y[i - 1]
      i -= 1
    end

    # Set new head position
    @snake_x[0] = new_x
    @snake_y[0] = new_y

    # Draw snake
    draw_tile(@snake_x[0], @snake_y[0], TILE_HEAD)
    draw_tile(@snake_x[1], @snake_y[1], TILE_BODY) if @snake_len > 1

    # Spawn new food if eaten
    if ate_food
      @food_x = rand(GRID_W)
      @food_y = rand(GRID_H)
      # Simple collision avoidance
      tries = 0
      while tries < 10
        collision = false
        i = 0
        while i < @snake_len
          collision = true if @snake_x[i] == @food_x && @snake_y[i] == @food_y
          i += 1
        end
        if collision
          @food_x = rand(GRID_W)
          @food_y = rand(GRID_H)
        end
        tries += 1
      end
      draw_tile(@food_x, @food_y, TILE_FOOD)
    end
  end
end

game_over(@score)
