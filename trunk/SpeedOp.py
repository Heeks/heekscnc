from Operation import Operation

ABS_MODE_ABSOLUTE = 1
ABS_MODE_INCREMENTAL = 2

class SpeedOp(Operation):
    def __init__(self):
        Operation.__init__(self)
        self.horizontal_feed_rate = 0.0
        self.vertical_feed_rate = 0.0
        self.spindle_speed = 0.0
