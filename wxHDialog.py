import wx
import HeeksCNC

class HDialog(wx.Dialog):
    def __init__(self, title):
        wx.Dialog.__init__(self, HeeksCNC.tree.window.frame, wx.ID_ANY, title)
        self.control_border = 3
        self.ignore_event_functions = False
        
    def AddLabelAndControl(self, sizer, label, control):
        sizer_horizontal = wx.BoxSizer(wx.HORIZONTAL)
        static_label = wx.StaticText(self, wx.ID_ANY, label)
        sizer_horizontal.Add( static_label, 0, wx.RIGHT + wx.ALIGN_LEFT + wx.ALIGN_CENTER_VERTICAL, self.control_border )
        sizer_horizontal.Add( control, 1, wx.LEFT + wx.ALIGN_RIGHT + wx.ALIGN_CENTER_VERTICAL, self.control_border )
        sizer.Add( sizer_horizontal, 0, wx.EXPAND + wx.ALL, self.control_border )
        
    def MakeOkAndCancel(self, orient):
        sizerOKCancel = wx.BoxSizer(orient)
        buttonOK = wx.Button(self, wx.ID_OK, "OK")
        sizerOKCancel.Add( buttonOK, 0, wx.ALL + wx.DOWN, self.control_border )
        buttonCancel = wx.Button(self, wx.ID_CANCEL, "Cancel")
        sizerOKCancel.Add( buttonCancel, 0, wx.ALL + wx.UP, self.control_border )
        buttonOK.SetDefault()
        return sizerOKCancel