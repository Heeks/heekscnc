from DepthOp import DepthOp
import HeeksCNC
from consts import *
import wx

class Pocket(DepthOp):
    def __init__(self):
        DepthOp.__init__(self)
        self.sketches = [] # a list of strings
        self.step_over = 0.0
        self.material_allowance = 0.0
        self.starting_place = True
        self.keep_tool_down_if_poss = True
        self.use_zig_zag = True
        self.zig_angle = 0.0
        self.cut_mode = CUT_MODE_CONVENTIONAL
        self.entry_move = ENTRY_STYLE_PLUNGE
        
    def TypeName(self):
        return "Pocket"
    
    def op_icon(self):
        # the name of the PNG file in the HeeksCNC icons folder
        return "pocket"
        
    def Edit(self):
        if HeeksCNC.widgets == HeeksCNC.WIDGETS_WX:
            from wxPocketDlg import PocketDlg
            dlg = PocketDlg(self)
            if dlg.ShowModal() == wx.ID_OK: return True
        return False
