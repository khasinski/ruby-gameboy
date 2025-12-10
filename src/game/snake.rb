# Snake Game for mrbz Game Boy VM
# Using parallel arrays for snake body

# Constants - using local variables since we don't have CONST support yet
grid_w = 20
grid_h = 18
max_snake = 50
frame_delay = 8  # Wait this many frames between moves

# Tile IDs (must match platform.h - offset 128 for game tiles)
tile_empty = 128
tile_head = 129
tile_body = 130
tile_food = 131

# Direction constants: 0=none, 1=up, 2=down, 3=left, 4=right
dir_none = 0
dir_up = 1
dir_down = 2
dir_left = 3
dir_right = 4

# Initialize game state using instance variables
@snake_x = Array.new(max_snake, 0)
@snake_y = Array.new(max_snake, 0)
@snake_len = 3
@direction = dir_right
@food_x = 15
@food_y = 9
@score = 0
@running = 1  # Use 1/0 instead of true/false for simplicity

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
  if i == 0
    draw_tile(@snake_x[i], @snake_y[i], tile_head)
  else
    draw_tile(@snake_x[i], @snake_y[i], tile_body)
  end
  i = i + 1
end
draw_tile(@food_x, @food_y, tile_food)

# Main game loop
while @running == 1
  # Wait for frames and poll input each frame
  frame_count = 0
  while frame_count < frame_delay
    wait_vbl

    # Read input every frame for responsiveness
    input = read_joypad
    if input == dir_up
      if @direction != dir_down
        @direction = dir_up
      end
    end
    if input == dir_down
      if @direction != dir_up
        @direction = dir_down
      end
    end
    if input == dir_left
      if @direction != dir_right
        @direction = dir_left
      end
    end
    if input == dir_right
      if @direction != dir_left
        @direction = dir_right
      end
    end

    frame_count = frame_count + 1
  end

  # Calculate new head position
  new_x = @snake_x[0]
  new_y = @snake_y[0]

  if @direction == dir_up
    new_y = new_y - 1
  end
  if @direction == dir_down
    new_y = new_y + 1
  end
  if @direction == dir_left
    new_x = new_x - 1
  end
  if @direction == dir_right
    new_x = new_x + 1
  end

  # Check wall collision
  if new_x < 0
    @running = 0
  end
  if new_x >= grid_w
    @running = 0
  end
  if new_y < 0
    @running = 0
  end
  if new_y >= grid_h
    @running = 0
  end

  # Check self collision (only if still running)
  if @running == 1
    i = 0
    while i < @snake_len
      if @snake_x[i] == new_x
        if @snake_y[i] == new_y
          @running = 0
        end
      end
      i = i + 1
    end
  end

  # Move snake (only if still running)
  if @running == 1
    # Check if eating food
    ate_food = 0
    if new_x == @food_x
      if new_y == @food_y
        ate_food = 1
        @score = @score + 10
      end
    end

    # Clear old tail (unless growing)
    if ate_food == 0
      tail_idx = @snake_len - 1
      clear_tile(@snake_x[tail_idx], @snake_y[tail_idx])
    else
      # Growing - increase length
      @snake_len = @snake_len + 1
    end

    # Shift body segments backward
    i = @snake_len - 1
    while i > 0
      @snake_x[i] = @snake_x[i - 1]
      @snake_y[i] = @snake_y[i - 1]
      i = i - 1
    end

    # Set new head position
    @snake_x[0] = new_x
    @snake_y[0] = new_y

    # Draw snake
    draw_tile(@snake_x[0], @snake_y[0], tile_head)
    if @snake_len > 1
      draw_tile(@snake_x[1], @snake_y[1], tile_body)
    end

    # Spawn new food if eaten
    if ate_food == 1
      @food_x = rand(grid_w)
      @food_y = rand(grid_h)
      # Simple collision avoidance - try a few times
      tries = 0
      while tries < 10
        collision = 0
        i = 0
        while i < @snake_len
          if @snake_x[i] == @food_x
            if @snake_y[i] == @food_y
              collision = 1
            end
          end
          i = i + 1
        end
        if collision == 1
          @food_x = rand(grid_w)
          @food_y = rand(grid_h)
        end
        tries = tries + 1
      end
      draw_tile(@food_x, @food_y, tile_food)
    end
  end
end

# Game over
game_over(@score)
