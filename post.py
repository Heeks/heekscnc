from nc.nc import *
import nc.iso
from nc.attach import *
import kurve
import stdops

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
kurve.add_point(k1, 0, -41, -5, 0, 0)
kurve.add_point(k1, 0, -33, 2, 0, 0)
kurve.add_point(k1, 0, -23, 4, 0, 0)
kurve.add_point(k1, 0, -15, 5, 0, 0)
kurve.add_point(k1, 0, -1, 5, 0, 0)
kurve.add_point(k1, 0, 12, 4, 0, 0)
stdops.profile(k1, 'left', tool_diameter/2, clearance, rapid_down_to_height, final_depth)

rapid_home()

program_end()
