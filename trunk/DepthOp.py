from SpeedOp import SpeedOp
from consts import *
from CNCConfig import CNCConfig

class DepthOp(SpeedOp):
    def __init__(self):
        SpeedOp.__init__(self)
        
    def ReadDefaultValues(self):
        SpeedOp.ReadDefaultValues(self)
        config = CNCConfig()
        self.abs_mode = config.ReadInt("DepthOpAbsMode", ABS_MODE_ABSOLUTE)
        self.clearance_height = config.ReadFloat("DepthOpClearance", 5.0)
        self.start_depth = config.ReadFloat("DepthOpStartDepth", 0.0)
        self.step_down = config.ReadFloat("DepthOpStepDown", 1.0)
        self.final_depth = config.ReadFloat("DepthOpFinalDepth", -1.0)
        self.rapid_down_to_height = config.ReadFloat("DepthOpRapidSpace", 2.0)

    def WriteDefaultValues(self):
        SpeedOp.WriteDefaultValues(self)
        config = CNCConfig()
        config.WriteInt("DepthOpAbsMode", self.abs_mode)
        config.WriteFloat("DepthOpClearance", self.clearance_height)
        config.WriteFloat("DepthOpStartDepth", self.start_depth)
        config.WriteFloat("DepthOpStepDown", self.step_down)
        config.WriteFloat("DepthOpFinalDepth", self.final_depth)
        config.WriteFloat("DepthOpRapidSpace", self.rapid_down_to_height)
