from Operation import Operation
from CNCConfig import CNCConfig
import HeeksCNC

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
        
    def AppendTextToProgram(self):
        Operation.AppendTextToProgram(self)

        if self.spindle_speed != 0.0:
            HeeksCNC.program.python_program += "spindle(" + str(self.spindle_speed) + ")\n"

        HeeksCNC.program.python_program += "feedrate_hv(" + str(self.horizontal_feed_rate / HeeksCNC.program.units) + ", "
        HeeksCNC.program.python_program += str(self.vertical_feed_rate / HeeksCNC.program.units) + ")\n"
        HeeksCNC.program.python_program += "flush_nc()\n"
