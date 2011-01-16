import wx
import HeeksCNC
from Pocket import Pocket
from wxHDialog import HDialog
from wxPictureWindow import PictureWindow
from wxNiceTextCtrl import LengthCtrl
from wxNiceTextCtrl import DoubleCtrl
from wxNiceTextCtrl import GeomCtrl
from consts import *

ID_SKETCHES = 100
ID_STEP_OVER = 101
ID_MATERIAL_ALLOWANCE = 102
ID_STARTING_PLACE = 103
ID_KEEP_TOOL_DOWN = 104
ID_USE_ZIG_ZAG = 105
ID_ZIG_ANGLE = 106
ID_ABS_MODE = 107
ID_CLEARANCE_HEIGHT = 108
ID_RAPID_DOWN_TO_HEIGHT = 109
ID_START_DEPTH = 110
ID_FINAL_DEPTH = 111
ID_STEP_DOWN = 112
ID_HFEED = 113
ID_VFEED = 114
ID_SPINDLE_SPEED = 115
ID_COMMENT = 116
ID_ACTIVE = 117
ID_TITLE = 118
ID_TOOL = 119
ID_DESCENT_STRATGEY = 120
ID_PICK_SKETCHES = 121

class PocketDlg(HDialog):
    def __init__(self, pocket):
        HDialog.__init__(self, "Pocket Operation")
        self.pocket = pocket
        
        self.general_bitmap = None
        self.step_over_bitmap = None
        self.material_allowance_bitmap = None
        self.starting_center_bitmap = None
        self.starting_boundary_bitmap = None
        self.tool_down_bitmap = None
        self.not_tool_down_bitmap = None
        self.use_zig_zag_bitmap = None
        self.zig_angle_bitmap = None
        self.clearance_height_bitmap = None
        self.rapid_down_to_bitmap = None
        self.start_depth_bitmap = None
        self.final_depth_bitmap = None
        self.step_down_bitmap = None
        self.entry_move_bitmap = None
        
        self.ignore_event_functions = True
        sizerMain = wx.BoxSizer(wx.HORIZONTAL)
        
        #add left sizer
        sizerLeft = wx.BoxSizer(wx.VERTICAL)
        sizerMain.Add( sizerLeft, 0, wx.ALL, self.control_border )

        # add right sizer
        sizerRight = wx.BoxSizer(wx.VERTICAL)
        sizerMain.Add( sizerRight, 0, wx.ALL, self.control_border )

        # add picture to right side
        self.picture = PictureWindow(self, wx.Size(300, 200))
        pictureSizer = wx.BoxSizer(wx.VERTICAL)
        pictureSizer.Add(self.picture, 1, wx.GROW)
        sizerRight.Add( pictureSizer, 0, wx.ALL, self.control_border )

        # add some of the controls to the right side
        self.cmbAbsMode = wx.ComboBox(self, ID_ABS_MODE, choices = ["absolute", "incremental"])
        self.AddLabelAndControl(sizerRight, "absolute mode", self.cmbAbsMode)
        self.lgthHFeed = LengthCtrl(self, ID_HFEED)
        self.AddLabelAndControl(sizerRight, "horizontal feedrate", self.lgthHFeed)
        self.lgthVFeed = LengthCtrl(self, ID_VFEED)
        self.AddLabelAndControl(sizerRight, "vertical feedrate", self.lgthVFeed)
        self.dblSpindleSpeed = DoubleCtrl(self, ID_SPINDLE_SPEED)
        self.AddLabelAndControl(sizerRight, "spindle speed", self.dblSpindleSpeed)

        self.txtComment = wx.TextCtrl(self, ID_COMMENT)
        self.AddLabelAndControl(sizerRight, "comment", self.txtComment)
        self.chkActive = wx.CheckBox( self, ID_ACTIVE, "active" )
        sizerRight.Add( self.chkActive, 0, wx.ALL, self.control_border )
        self.txtTitle = wx.TextCtrl(self, ID_TITLE)
        self.AddLabelAndControl(sizerRight, "title", self.txtTitle)

        # add OK and Cancel to right side
        sizerOKCancel = self.MakeOkAndCancel(wx.HORIZONTAL)
        sizerRight.Add( sizerOKCancel, 0, wx.ALL + wx.ALIGN_RIGHT + wx.ALIGN_BOTTOM, self.control_border )

        # add all the controls to the left side
        self.idsSketches = GeomCtrl(self, ID_SKETCHES)
        self.AddLabelAndControl(sizerLeft, "sketches", self.idsSketches)
        
        btn_pick_sketches = wx.Button(self, ID_PICK_SKETCHES)
        self.AddLabelAndControl(sizerLeft, "pick sketches", btn_pick_sketches)
        self.lgthStepOver = LengthCtrl(self, ID_STEP_OVER)
        self.AddLabelAndControl(sizerLeft, "step over", self.lgthStepOver)
        self.lgthMaterialAllowance = LengthCtrl(self, ID_MATERIAL_ALLOWANCE)
        self.AddLabelAndControl(sizerLeft, "material allowance", self.lgthMaterialAllowance)

        self.cmbStartingPlace = wx.ComboBox(self, ID_STARTING_PLACE, choices = ["boundary", "center"])
        self.AddLabelAndControl(sizerLeft, "starting place", self.cmbStartingPlace)

        self.cmbEntryMove = wx.ComboBox(self, ID_DESCENT_STRATGEY, choices = ["Plunge", "Ramp", "Helical"])
        self.AddLabelAndControl(sizerLeft, "entry move", self.cmbEntryMove)

        self.tools_for_combo = HeeksCNC.program.tools.FindAllTools()

        tool_choices = []
        for tool in self.tools_for_combo:
            tool_choices.append(tool[1])
        self.cmbTool = wx.ComboBox(self, ID_TOOL, choices = tool_choices)
        self.AddLabelAndControl(sizerLeft, "Tool", self.cmbTool)

        self.chkUseZigZag = wx.CheckBox( self, ID_USE_ZIG_ZAG, "use zig zag" )
        sizerLeft.Add( self.chkUseZigZag, 0, wx.ALL, self.control_border )
        
        self.chkKeepToolDown = wx.CheckBox( self, ID_KEEP_TOOL_DOWN, "keep tool down" )
        sizerLeft.Add( self.chkKeepToolDown, 0, wx.ALL, self.control_border )
        
        self.dblZigAngle = DoubleCtrl(self, ID_ZIG_ANGLE)
        self.AddLabelAndControl(sizerLeft, "zig zag angle", self.dblZigAngle)

        self.lgthClearanceHeight = LengthCtrl(self, ID_CLEARANCE_HEIGHT)
        self.AddLabelAndControl(sizerLeft, "clearance height", self.lgthClearanceHeight)
        
        self.lgthRapidDownToHeight = LengthCtrl(self, ID_RAPID_DOWN_TO_HEIGHT)
        self.AddLabelAndControl(sizerLeft, "rapid safety space", self.lgthRapidDownToHeight)
        
        self.lgthStartDepth = LengthCtrl(self, ID_START_DEPTH)
        self.AddLabelAndControl(sizerLeft, "start depth", self.lgthStartDepth)
        
        self.lgthFinalDepth = LengthCtrl(self, ID_FINAL_DEPTH)
        self.AddLabelAndControl(sizerLeft, "final depth", self.lgthFinalDepth)
        
        self.lgthStepDown = LengthCtrl(self, ID_STEP_DOWN)
        self.AddLabelAndControl(sizerLeft, "step down", self.lgthStepDown)

        self.SetFromData()

        self.SetSizer( sizerMain )
        sizerMain.SetSizeHints(self)
        sizerMain.Fit(self)

        self.idsSketches.SetFocus()

        self.ignore_event_functions = False

        self.SetPicture()

        self.Bind(wx.EVT_CHILD_FOCUS, self.OnChildFocus)
        self.Bind(wx.EVT_COMBOBOX, self.OnComboStartingPlace, self.cmbStartingPlace)
        self.Bind(wx.EVT_CHECKBOX, self.OnCheckKeepToolDown, self.chkKeepToolDown)
        self.Bind(wx.EVT_CHECKBOX, self.OnCheckUseZigZag, self.chkUseZigZag)
        self.Bind(wx.EVT_COMBOBOX, self.OnComboTool, self.cmbTool)
        self.Bind(wx.EVT_BUTTON, self.OnPickSketches, btn_pick_sketches)
        
    def OnChildFocus(self, event):
        if self.ignore_event_functions: return
        
        if event.GetWindow():
            self.SetPicture()

    def OnComboStartingPlace(self, event):
        if self.ignore_event_functions: return
        self.SetPicture()

    def OnCheckKeepToolDown(self, event):
        if self.ignore_event_functions: return
        self.SetPicture()

    def OnCheckUseZigZag(self, event):
        if self.ignore_event_functions: return
        self.SetPicture()

    def OnComboTool(self, event):
        pass
        #if self.ignore_event_functions: return
        #self.SetPicture()
    
    def OnPickSketches(self, event):
        self.Show(False)
        sketches = HeeksCNC.cad.pick_sketches()
        self.Show()
        self.idsSketches.SetFromGeomList(sketches)

    def GetData(self):
        if self.ignore_event_functions: return
        self.ignore_event_functions = True
        self.pocket.sketches = self.idsSketches.GetGeomList()
        self.pocket.step_over = self.lgthStepOver.GetValue()
        self.pocket.material_allowance = self.lgthMaterialAllowance.GetValue()
        self.pocket.starting_place = (self.cmbStartingPlace.GetValue() == "center")
        if self.cmbEntryMove.GetValue() == "Plunge": self.pocket.entry_move = ENTRY_STYLE_PLUNGE
        elif self.cmbEntryMove.GetValue() == "Ramp": self.pocket.entry_move = ENTRY_STYLE_RAMP
        elif self.cmbEntryMove.GetValue() == "Helical": self.pocket.entry_move = ENTRY_STYLE_HELICAL
        self.pocket.keep_tool_down_if_poss = self.chkKeepToolDown.GetValue()
        self.pocket.use_zig_zag = self.chkUseZigZag.GetValue()
        if self.pocket.use_zig_zag: self.pocket.zig_angle = self.dblZigAngle.GetValue()
        if self.cmbAbsMode.GetValue() == "incremental": self.pocket.abs_mode = ABS_MODE_INCREMENTAL
        else: self.pocket.abs_mode = ABS_MODE_ABSOLUTE
        self.pocket.clearance_height = self.lgthClearanceHeight.GetValue()
        self.pocket.rapid_down_to_height = self.lgthRapidDownToHeight.GetValue()
        self.pocket.start_depth = self.lgthStartDepth.GetValue()
        self.pocket.final_depth = self.lgthFinalDepth.GetValue()
        self.pocket.step_down = self.lgthStepDown.GetValue()
        self.pocket.horizontal_feed_rate = self.lgthHFeed.GetValue()
        self.pocket.vertical_feed_rate = self.lgthVFeed.GetValue()
        self.pocket.spindle_speed = self.dblSpindleSpeed.GetValue()
        self.pocket.comment = self.txtComment.GetValue()
        self.pocket.active = self.chkActive.GetValue()
    
        # get the tool number
        self.pocket.tool_number = 0
        if self.cmbTool.GetSelection() >= 0:
            self.pocket.tool_number = self.tools_for_combo[self.cmbTool.GetSelection()][0]

        self.pocket.title = self.txtTitle.GetValue()
        self.ignore_event_functions = False

    def SetFromData(self):
        self.ignore_event_functions = True
        self.idsSketches.SetFromGeomList(self.pocket.sketches)
        self.lgthStepOver.SetValue(self.pocket.step_over)
        self.lgthMaterialAllowance.SetValue(self.pocket.material_allowance)
        self.cmbStartingPlace.SetValue("center" if self.pocket.starting_place else "boundary")
        if self.pocket.entry_move == ENTRY_STYLE_PLUNGE: self.cmbEntryMove.SetValue("Plunge")
        elif self.pocket.entry_move == ENTRY_STYLE_RAMP: self.cmbEntryMove.SetValue("Ramp")
        elif self.pocket.entry_move == ENTRY_STYLE_HELICAL: self.cmbEntryMove.SetValue("Helical")

        # set the tool combo to the correct tool
        for i in range(0, len(self.tools_for_combo)):
            if self.tools_for_combo[i][0] == self.pocket.tool_number:
                self.cmbTool.SetSelection(i)
                break

        self.chkKeepToolDown.SetValue(self.pocket.keep_tool_down_if_poss)
        self.chkUseZigZag.SetValue(self.pocket.use_zig_zag)
        if self.pocket.use_zig_zag: self.dblZigAngle.SetValue(self.pocket.zig_angle)
        if self.pocket.abs_mode == ABS_MODE_ABSOLUTE: self.cmbAbsMode.SetValue("absolute")
        else: self.cmbAbsMode.SetValue("incremental")
        self.lgthClearanceHeight.SetValue(self.pocket.clearance_height)
        self.lgthRapidDownToHeight.SetValue(self.pocket.rapid_down_to_height)
        self.lgthStartDepth.SetValue(self.pocket.start_depth)
        self.lgthFinalDepth.SetValue(self.pocket.final_depth)
        self.lgthStepDown.SetValue(self.pocket.step_down)
        self.lgthHFeed.SetValue(self.pocket.horizontal_feed_rate)
        self.lgthVFeed.SetValue(self.pocket.vertical_feed_rate)
        self.dblSpindleSpeed.SetValue(self.pocket.spindle_speed)
        self.txtComment.SetValue(self.pocket.comment)
        self.chkActive.SetValue(self.pocket.active)
        self.txtTitle.SetValue(self.pocket.title)
        self.ignore_event_functions = False

    def SetPictureBitmap(self, bitmap, name):
        self.picture.SetPictureBitmap(bitmap, HeeksCNC.heekscnc_path + "/bitmaps/pocket/" + name + ".png", wx.BITMAP_TYPE_PNG)

    def SetPicture(self):
        w = self.FindFocus()

        if w == self.lgthStepOver: self.SetPictureBitmap(self.step_over_bitmap, "step over")
        elif w == self.lgthMaterialAllowance: self.SetPictureBitmap(self.material_allowance_bitmap, "material allowance")
        elif w == self.cmbStartingPlace:
            if self.cmbStartingPlace.GetValue() == "boundary": self.SetPictureBitmap(self.starting_boundary_bitmap, "starting boundary")
            else: self.SetPictureBitmap(self.starting_center_bitmap, "starting center")
        elif w == self.chkKeepToolDown:
            if self.chkKeepToolDown.IsChecked(): self.SetPictureBitmap(self.tool_down_bitmap, "tool down")
            else: self.SetPictureBitmap(self.not_tool_down_bitmap, "not tool down")
        elif w == self.chkUseZigZag:
            if self.chkUseZigZag.IsChecked(): self.SetPictureBitmap(self.use_zig_zag_bitmap, "use zig zag")
            else: self.SetPictureBitmap(self.general_bitmap, "general")
        elif w == self.dblZigAngle: self.SetPictureBitmap(self.zig_angle_bitmap, "zig angle")
        elif w == self.lgthClearanceHeight: self.SetPictureBitmap(self.clearance_height_bitmap, "clearance height")
        elif w == self.lgthRapidDownToHeight: self.SetPictureBitmap(self.rapid_down_to_bitmap, "rapid down height")
        elif w == self.lgthStartDepth: self.SetPictureBitmap(self.start_depth_bitmap, "start depth")
        elif w == self.lgthFinalDepth: self.SetPictureBitmap(self.final_depth_bitmap, "final depth")
        elif w == self.lgthStepDown: self.SetPictureBitmap(self.step_down_bitmap, "step down")
        else: self.SetPictureBitmap(self.general_bitmap, "general")

