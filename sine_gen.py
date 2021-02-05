import math 

angle = 0
sine_table = []

for x in range(176):
  sine_table.append(20 + round(20 * math.sin(x*2*math.pi/176)))

print(sine_table)