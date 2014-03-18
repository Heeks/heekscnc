import wx
import HeeksCNC

class HDialog(wx.Dialog):
    def __init__(self, title):
        wx.Dialog.__init__(self, HeeksCNC.cad.frame, wx.ID_ANY, title)
        self.control_border = 3
        self.ignore_event_functions = False
        self.button_id_txt_map = {}
        
    def ShowModal(self):
        result = wx.Dialog.ShowModal(self)
        if result == wx.ID_OK:
            self.GetData()
        return result
        
    def AddLabelAndControl(self, sizer, label, control):
        sizer_horizontal = wx.BoxSizer(wx.HORIZONTAL)
        static_label = wx.StaticText(self, wx.ID_ANY, label)
        sizer_horizontal.Add( static_label, 0, wx.RIGHT + wx.ALIGN_LEFT + wx.ALIGN_CENTER_VERTICAL, self.control_border )
        sizer_horizontal.Add( control, 1, wx.LEFT + wx.ALIGN_RIGHT + wx.ALIGN_CENTER_VERTICAL, self.control_border )
        sizer.Add( sizer_horizontal, 0, wx.EXPAND + wx.ALL, self.control_border )
        return static_label
        
    def AddFileNameControl(self, sizer, label, text_control):
        sizer_horizontal = wx.BoxSizer(wx.HORIZONTAL)
        static_label = wx.StaticText(self, wx.ID_ANY, label)
        sizer_horizontal.Add( static_label, 0, wx.RIGHT + wx.ALIGN_LEFT + wx.ALIGN_CENTER_VERTICAL, self.control_border )
        sizer_horizontal.Add( text_control, 1, wx.LEFT + wx.RIGHT + wx.ALIGN_CENTER_VERTICAL, self.control_border )
        button_control = wx.Button(self, label = "...")
        sizer_horizontal.Add( button_control, 0, wx.LEFT + wx.ALIGN_RIGHT + wx.ALIGN_CENTER_VERTICAL, self.control_border )
        sizer.Add( sizer_horizontal, 0, wx.EXPAND + wx.ALL, self.control_border )
        self.button_id_txt_map[button_control.GetId()] = text_control
        self.Bind(wx.EVT_BUTTON, self.OnFileBrowseButton, button_control)
        return static_label, button_control
        
    def MakeOkAndCancel(self, orient):
        sizerOKCancel = wx.BoxSizer(orient)
        buttonOK = wx.Button(self, wx.ID_OK, "OK")
        if orient == wx.RIGHT:
            ok_flag = wx.LEFT
            cancel_flag = wx.UP
        else:
            ok_flag = wx.DOWN
            cancel_flag = wx.UP
        ok_flag = 0
        cancel_flag = 0
        sizerOKCancel.Add( buttonOK, 0, wx.ALL + ok_flag, self.control_border )
        buttonCancel = wx.Button(self, wx.ID_CANCEL, "Cancel")
        sizerOKCancel.Add( buttonCancel, 0, wx.ALL + cancel_flag, self.control_border )
        buttonOK.SetDefault()
        return sizerOKCancel
    
    def OnFileBrowseButton(self, event):
        dialog = wx.FileDialog(HeeksCNC.cad.frame, "Choose File", wildcard = "All files" + " |*.*")
        dialog.CentreOnParent()
        
        if dialog.ShowModal() == wx.ID_OK:
            text_control = self.button_id_txt_map[event.GetId()]
            text_control.SetValue(dialog.GetPath())
            