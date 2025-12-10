# Snake Game for mrbz Game Boy VM

# Constants
grid_w = 20
grid_h = 18
max_snake = 50
frame_delay = 8

# Tile IDs (offset 128 for game tiles)
tile_head = 129
tile_body = 130
tile_food = 131

# Direction constants: 0=none, 1=up, 2=down, 3=left, 4=right
dir_up = 1
dir_down = 2
dir_left = 3
dir_right = 4

# Initialize game state
@snake_x = Array.new(max_snake, 0)
@snake_y = Array.new(max_snake, 0)
@snake_len = 3
@direction = dir_right
@food_x = 15
@food_y = 9
@score = 0
@running = 1

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
  tile = i == 0 ? tile_head : tile_body
  draw_tile(@snake_x[i], @snake_y[i], tile)
  i += 1
end
draw_tile(@food_x, @food_y, tile_food)

# Main game loop
while @running == 1
  # Remember last move direction (to prevent reversal)
  prev_dir = @direction
  turned = 0

  # Wait for frames and poll input each frame
  frame_count = 0
  while frame_count < frame_delay
    wait_vbl
    input = read_joypad

    # Update direction (can't reverse, only one turn per move)
    if turned == 0
      if input == dir_up && prev_dir != dir_down
        @direction = dir_up
        turned = 1
      end
      if input == dir_down && prev_dir != dir_up
        @direction = dir_down
        turned = 1
      end
      if input == dir_left && prev_dir != dir_right
        @direction = dir_left
        turned = 1
      end
      if input == dir_right && prev_dir != dir_left
        @direction = dir_right
        turned = 1
      end
    end

    frame_count += 1
  end

  # Calculate new head position
  new_x = @snake_x[0]
  new_y = @snake_y[0]

  new_y -= 1 if @direction == dir_up
  new_y += 1 if @direction == dir_down
  new_x -= 1 if @direction == dir_left
  new_x += 1 if @direction == dir_right

  # Check wall collision
  @running = 0 if new_x < 0 || new_x >= grid_w || new_y < 0 || new_y >= grid_h

  # Check self collision
  if @running == 1
    i = 0
    while i < @snake_len
      @running = 0 if @snake_x[i] == new_x && @snake_y[i] == new_y
      i += 1
    end
  end

  # Move snake
  if @running == 1
    # Check if eating food
    ate_food = 0
    if new_x == @food_x && new_y == @food_y
      ate_food = 1
      @score += 10
    end

    # Clear old tail (unless growing)
    if ate_food == 0
      tail_idx = @snake_len - 1
      clear_tile(@snake_x[tail_idx], @snake_y[tail_idx])
    else
      @snake_len += 1
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
    draw_tile(@snake_x[0], @snake_y[0], tile_head)
    draw_tile(@snake_x[1], @snake_y[1], tile_body) if @snake_len > 1

    # Spawn new food if eaten
    if ate_food == 1
      @food_x = rand(grid_w)
      @food_y = rand(grid_h)
      # Simple collision avoidance
      tries = 0
      while tries < 10
        collision = 0
        i = 0
        while i < @snake_len
          collision = 1 if @snake_x[i] == @food_x && @snake_y[i] == @food_y
          i += 1
        end
        if collision == 1
          @food_x = rand(grid_w)
          @food_y = rand(grid_h)
        end
        tries += 1
      end
      draw_tile(@food_x, @food_y, tile_food)
    end
  end
end

game_over(@score)
