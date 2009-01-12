import siegkx1
import sys
from math import *
from stdops import *

clearance = 5
rapid_down_to_height = 2
final_depth = -0.1
spindle(7000)
rate(100, 100)
tool(1, 3, 0)
k1 = kurve.new()
kurve.add_point(k1, 0, 5, -3, 0, 0)
kurve.add_point(k1, 0, 14, 2, 0, 0)
kurve.add_point(k1, 0, 8, 8, 0, 0)
kurve.add_point(k1, 0, 0, 3, 0, 0)
kurve.add_point(k1, 0, 5, -3, 0, 0)
profile(k1, 'left', clearance, rapid_down_to_height, final_depth)

end()
