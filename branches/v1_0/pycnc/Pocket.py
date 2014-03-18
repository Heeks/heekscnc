from DepthOp import DepthOp
import HeeksCNC
from consts import *
import wx
from CNCConfig import CNCConfig

class Pocket(DepthOp):
    def __init__(self):
        DepthOp.__init__(self)
        self.sketches = [] # a list of strings
        
    def ReadDefaultValues(self):
        DepthOp.ReadDefaultValues(self)
        config = CNCConfig()
        self.step_over = config.ReadFloat("PocketStepover", 1.0)
        self.material_allowance = config.ReadFloat("PocketMaterialAllowance", 0.0)
        self.starting_place = config.ReadBool("PocketStartingPlace", True)
        self.keep_tool_down_if_poss = config.ReadBool("PocketKeepToolDown", True)
        self.use_zig_zag = config.ReadBool("PocketUseZigZag", True)
        self.zig_angle = config.ReadFloat("PocketZigAngle", 0.0)
        self.cut_mode = config.ReadInt("PocketCutMode", CUT_MODE_CONVENTIONAL)
        self.entry_move = config.ReadInt("PocketEntryStyle", ENTRY_STYLE_PLUNGE)
        
    def WriteDefaultValues(self):
        DepthOp.WriteDefaultValues(self)
        config = CNCConfig()
        config.WriteFloat("PocketStepover", self.step_over)
        config.WriteFloat("PocketMaterialAllowance", self.material_allowance)
        config.WriteBool("PocketStartingPlace", self.starting_place)
        config.WriteBool("PocketKeepToolDown", self.keep_tool_down_if_poss)
        config.WriteBool("PocketUseZigZag", self.use_zig_zag)
        config.WriteFloat("PocketZigAngle", self.zig_angle)
        config.WriteInt("PocketCutMode", self.cut_mode)
        config.WriteInt("PocketEntryStyle", self.entry_move)
        
    def TypeName(self):
        return "Pocket"
    
    def op_icon(self):
        # the name of the PNG file in the HeeksCNC icons folder
        return "pocket"
        
    def Edit(self):
        if HeeksCNC.widgets == HeeksCNC.WIDGETS_WX:
            from wxPocketDlg import PocketDlg
            dlg = PocketDlg(self)
            if dlg.ShowModal() == wx.ID_OK:
                self.WriteDefaultValues()
                return True
        return False
    
    def AppendTextToProgram(self):
        tool = HeeksCNC.program.tools.FindTool(self.tool_number)
        if tool == None:
            print "Cannot generate GCode for pocket without a tool assigned"
            return
        
        if len(self.sketches) == 0:
            return # to do, area_funcs.pocket crashes if given an empty area
        
        DepthOp.AppendTextToProgram(self)
        HeeksCNC.cad.WriteAreaToProgram(self.sketches)
        HeeksCNC.program.python_program += "entry_moves = []\n"

        # reorder the area, the outside curves must be made anti-clockwise and the insides clockwise
        HeeksCNC.program.python_program += "a.Reorder()\n"

        # start - assume we are at a suitable clearance height

        # make a parameter of area_funcs.pocket() eventually
        # 0..plunge, 1..ramp, 2..helical
        HeeksCNC.program.python_program += "entry_style = " + str(self.entry_move) + "\n"

        # Pocket the area
        HeeksCNC.program.python_program += "area_funcs.pocket(a, tool_diameter/2, "
        HeeksCNC.program.python_program += str(self.material_allowance / HeeksCNC.program.units)
        HeeksCNC.program.python_program += ", rapid_safety_space, start_depth, final_depth, "
        HeeksCNC.program.python_program += str(self.step_over / HeeksCNC.program.units)
        HeeksCNC.program.python_program += ", step_down, clearance"
        HeeksCNC.program.python_program += (", True" if self.starting_place else ", False")
        HeeksCNC.program.python_program += (", True" if self.keep_tool_down_if_poss else ", False")
        HeeksCNC.program.python_program += (", True" if self.use_zig_zag else ", False")
        HeeksCNC.program.python_program += ", " + str(self.zig_angle)
        HeeksCNC.program.python_program += ")\n"

        # rapid back up to clearance plane
        HeeksCNC.program.python_program += "rapid(z = clearance)\n"
