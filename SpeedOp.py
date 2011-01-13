from Operation import Operation
from CNCConfig import CNCConfig

ABS_MODE_ABSOLUTE = 1
ABS_MODE_INCREMENTAL = 2

class SpeedOp(Operation):
    def __init__(self):
        Operation.__init__(self)
        
    def ReadDefaultValues(self):
        Operation.ReadDefaultValues(self)
        config = CNCConfig()
        self.horizontal_feed_rate = config.ReadFloat("SpeedOpHFeedrate", 200.0)
        self.vertical_feed_rate = config.ReadFloat("SpeedOpVFeedrate", 50.0)
        self.spindle_speed = config.ReadFloat("SpeedOpSpindleSpeed", 7000.0)

    def WriteDefaultValues(self):
        Operation.WriteDefaultValues(self)
        config = CNCConfig()
        config.WriteFloat("SpeedOpHFeedrate", self.horizontal_feed_rate)
        config.WriteFloat("SpeedOpVFeedrate", self.vertical_feed_rate)
        config.WriteFloat("SpeedOpSpindleSpeed", self.spindle_speed)
