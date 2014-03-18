import wx
import HeeksCNC
from Program import Program
from wxHDialog import HDialog
from wxPictureWindow import PictureWindow
from wxNiceTextCtrl import LengthCtrl
from wxNiceTextCtrl import DoubleCtrl
from wxNiceTextCtrl import GeomCtrl
from consts import *

class ProgramDlg(HDialog):
    def __init__(self, program):
        HDialog.__init__(self, "Program")
        self.program = program
        
        self.ignore_event_functions = True
        sizerMain = wx.BoxSizer(wx.VERTICAL)
        
        # add the controls in one column
        self.machines = program.GetMachines()
        choices = []
        for machine in self.machines:
            choices.append(machine.description)
        self.cmbMachine = wx.ComboBox(self, choices = choices)
        self.AddLabelAndControl(sizerMain, "machine", self.cmbMachine)
        
        self.chkOutputSame = wx.CheckBox( self, label = "output file name follows data file name" )
        sizerMain.Add( self.chkOutputSame, 0, wx.ALL, self.control_border )

        self.txtOutputFile = wx.TextCtrl(self)
        self.lblOutputFile, self.btnOutputFile = self.AddFileNameControl(sizerMain, "output file", self.txtOutputFile)

        self.cmbUnits = wx.ComboBox(self, choices = ["mm", "inch"])
        self.AddLabelAndControl(sizerMain, "units", self.cmbUnits)
        
        # to do "Raw Material" and "Brinell Hardness of raw material"

        # add OK and Cancel
        sizerOKCancel = self.MakeOkAndCancel(wx.HORIZONTAL)
        sizerMain.Add( sizerOKCancel, 0, wx.ALL + wx.ALIGN_RIGHT + wx.ALIGN_BOTTOM, self.control_border )

        self.SetFromData()
        
        self.EnableControls()

        self.SetSizer( sizerMain )
        sizerMain.SetSizeHints(self)
        sizerMain.Fit(self)

        self.cmbMachine.SetFocus()

        self.ignore_event_functions = False

        self.Bind(wx.EVT_CHECKBOX, self.OnCheckOutputSame, self.chkOutputSame)
        
    def EnableControls(self):
        output_same = self.chkOutputSame.GetValue()
        self.txtOutputFile.Enable(output_same == False)
        self.btnOutputFile.Enable(output_same == False)
        self.lblOutputFile.Enable(output_same == False)
        
    def OnCheckOutputSame(self, event):
        if self.ignore_event_functions: return
        self.EnableControls()

    def GetData(self):
        if self.ignore_event_functions: return
        self.ignore_event_functions = True
        
        if self.cmbMachine.GetSelection() != wx.NOT_FOUND:
            self.program.machine = self.machines[self.cmbMachine.GetSelection()]
            
        self.program.output_file_name_follows_data_file_name = self.chkOutputSame.GetValue()
        self.program.output_file = self.txtOutputFile.GetValue()
            
        if self.cmbUnits.GetValue() == "inch":
            self.program.units = 25.4
        else:
            self.program.units = 1.0
        self.ignore_event_functions = False

    def SetFromData(self):
        self.ignore_event_functions = True
        
        self.cmbMachine.SetValue(self.program.machine.description)
        
        self.chkOutputSame.SetValue(self.program.output_file_name_follows_data_file_name)

        self.txtOutputFile.SetValue(self.program.output_file)
            
        if self.program.units == 25.4:
            self.cmbUnits.SetValue("inch")
        else:
            self.cmbUnits.SetValue("mm")

        self.ignore_event_functions = False
        
