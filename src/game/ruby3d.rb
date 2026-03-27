# 3D Ruby Gem - Pre-rendered rotating gemstone
# 8 frames covering 45 degrees, loops by 8-fold symmetry

@frame = 0

while true
  show_frame(@frame)
  wait_vbl
  wait_vbl
  wait_vbl
  @frame += 1
  @frame = 0 if @frame >= 8
end
