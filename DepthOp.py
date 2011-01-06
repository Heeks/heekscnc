from SpeedOp import SpeedOp
from consts import *

class DepthOp(SpeedOp):
    def __init__(self):
        SpeedOp.__init__(self)
        self.abs_mode = ABS_MODE_ABSOLUTE
        self.clearance_height = 0.0
        self.start_depth = 0.0
        self.step_down = 0.0
        self.final_depth = 0.0
        self.rapid_down_to_height = 0.0