# Test program for mrbz VM
# Tests: arithmetic, comparisons, conditionals, loops

# Simple arithmetic
a = 5
b = 3
c = a + b  # 8

# Comparison and conditional
if c > 7
  result = c * 2  # 16
else
  result = c
end

# While loop - count down
i = 5
total = 0
while i > 0
  total = total + i
  i = i - 1
end
# total should be 15 (5+4+3+2+1)

# Final result
final = result + total  # 16 + 15 = 31
return final
