import kurve
import stdops
from nc.nc import *
import nc.iso

output('test.tap')
program_begin(123, 'Test program')
absolute()
metric()
set_plane(0)

clearance = 5
rapid_down_to_height = 2
final_depth = -0.1
tool_diameter = 3
spindle(7000)
feedrate(100)
tool_change(1)
k1 = kurve.new()
kurve.add_point(k1, 0, -7, 8, 0, 0)
kurve.add_point(k1, 0, -3, 9, 0, 0)
kurve.add_point(k1, 0, 2, 8, 0, 0)
kurve.add_point(k1, 0, 7, 6, 0, 0)

stdops.profile(k1, 'left', tool_diameter/2, clearance, rapid_down_to_height, final_depth)
