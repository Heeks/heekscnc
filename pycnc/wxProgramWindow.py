import wx

class ProgramWindow(wx.ScrolledWindow):
    def __init__(self, parent):
        wx.ScrolledWindow.__init__(self, parent, name = 'Program', style = wx.HSCROLL + wx.VSCROLL + wx.NO_FULL_REPAINT_ON_RESIZE)
        self.textCtrl = wx.TextCtrl(self, 100, "", style = wx.TE_MULTILINE + wx.TE_DONTWRAP)
        self.textCtrl.SetMaxLength(0) # Ensure the length is as long as this operating system supports.  (It may be only 32kb or 64kb)
        self.Bind(wx.EVT_SIZE, self.OnSize)
        self.Resize()
        
    def OnSize(self, event):
        self.Resize()
        event.Skip()
        
    def Resize(self):
        self.textCtrl.SetSize(self.GetClientSize())
        
    def Clear(self):
        self.textCtrl.Clear()
        
    def AppendText(self, value):
        self.textCtrl.AppendText(str(value))