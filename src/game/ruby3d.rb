# 3D Ruby Gem - Rotating solid gemstone
# Controls: D-pad adjusts rotation speed (X and Y axes)
# All rendering done in C for speed

@angle_x = 0
@angle_y = 0
@speed_x = 0
@speed_y = 3
@prev_input = nil

while true
  input = read_joypad
  if input != @prev_input
    if input == :right
      @speed_y += 1 if @speed_y < 6
    end
    if input == :left
      @speed_y -= 1 if @speed_y > -6
    end
    if input == :down
      @speed_x += 1 if @speed_x < 6
    end
    if input == :up
      @speed_x -= 1 if @speed_x > -6
    end
  end
  @prev_input = input

  @angle_x += @speed_x
  @angle_y += @speed_y

  # Single C call: rotate, project, cull, shade, pixel-fill, VRAM copy
  update_and_render(@angle_x, @angle_y)
end
