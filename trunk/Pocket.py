from DepthOp import DepthOp
import HeeksCNC
from consts import *
import wx
from CNCConfig import CNCConfig
import re
pattern_main = re.compile('([(!;].*|\s+|[a-zA-Z0-9_:](?:[+-])?\d*(?:\.\d*)?|\w\#\d+|\(.*?\)|\#\d+\=(?:[+-])?\d*(?:\.\d*)?)')

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
        HeeksCNC.program.python_program += "a = area.Area()\n"
        HeeksCNC.program.python_program += "entry_moves = []\n"

        for sketch in self.sketches:
            shape_str = HeeksCNC.cad.GetSketchShape(sketch)
            if shape_str:
                WriteAreaCurve(shape_str)

        # reorder the area, the outside curves must be made anti-clockwise and the insides clockwise
        HeeksCNC.program.python_program += "a.Reorder()\n"

        # start - assume we are at a suitable clearance height

        # make a parameter of area_funcs.pocket() eventually
        # 0..plunge, 1..ramp, 2..helical
        HeeksCNC.program.python_program += "entry_style = " + str(self.entry_move) + "\n"

        # Pocket the area
        HeeksCNC.program.python_program += "area_funcs.pocket(a, tool_diameter/2, "
        HeeksCNC.program.python_program += str(self.material_allowance / HeeksCNC.program.units)
        HeeksCNC.program.python_program += ", rapid_down_to_height, start_depth, final_depth, "
        HeeksCNC.program.python_program += str(self.step_over / HeeksCNC.program.units)
        HeeksCNC.program.python_program += ", step_down, clearance"
        HeeksCNC.program.python_program += (", True" if self.starting_place else ", False")
        HeeksCNC.program.python_program += (", True" if self.keep_tool_down_if_poss else ", False")
        HeeksCNC.program.python_program += (", True" if self.use_zig_zag else ", False")
        HeeksCNC.program.python_program += ", " + str(self.zig_angle)
        HeeksCNC.program.python_program += ")\n"

        # rapid back up to clearance plane
        HeeksCNC.program.python_program += "rapid(z = clearance)\n"

def WriteLine(words):
    if words[0][0] != "x":return
    x = words[0][1:]
    if words[1][0] != "y":return
    y = words[1][1:]
    HeeksCNC.program.python_program += "c.append(area.Point(" + x + ", " + y + "))\n"

def WriteArc(direction, words):
    type_str = "-1"
    if direction: type_str = "1"
    if words[1][0] != "x":return
    x = words[1][1:]
    if words[2][0] != "y":return
    y = words[2][1:]
    if words[3][0] != "x":return
    x = words[3][1:]
    if words[4][0] != "y":return
    y = words[4][1:]
    HeeksCNC.program.python_program += "c.append(area.Vertex(" + type_str + ", area.Point(" + x + ", " + y + "), area.Point(" + i + ", " + j + ")))\n"

def WriteSpan(span_str):
    global pattern_main
    words = pattern_main.findall(span_str)
    length = len(words)
    if length < 1:return
    if words[0][0] == 'a':
        if length != 5:return
        WriteArc(True, words)
    elif words[0][0] == 't':
        if length != 5:return
        WriteArc(False, words)
    else:
        if length != 2:return
        WriteLine(words)
    
def WriteAreaCurve(shape_str):
    length = len(shape_str)
    i = 0
    s = ""
    HeeksCNC.program.python_program += "c = area.Curve()\n"
    while i < length:
        if shape_str[i] == '\n':
            WriteSpan(s)
            s = ""
        else:
            s += shape_str[i]
        i = i + 1
    HeeksCNC.program.python_program += "a.append(c)\n"
