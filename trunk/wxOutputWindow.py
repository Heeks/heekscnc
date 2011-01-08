import wx
import HeeksCNC

class OutputTextCtrl(wx.TextCtrl):
    def __init__(self, parent):
        wx.TextCtrl.__init__(self, parent, style = wx.TE_MULTILINE + wx.TE_DONTWRAP + wx.TE_RICH + wx.TE_RICH2)
        self.painting = False
        self.Bind(wx.EVT_MOUSE_EVENTS, self.OnMouse)
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        
    def OnMouse(self, event):
        if event.LeftUp():
            pos = self.GetInsertionPoint()
            HeeksCNC.program.nccode.HighlightBlock(pos)
            HeeksCNC.repaint()
        event.Skip()
    
    def OnPaint(self, event):
        dc = wx.PaintDC(self)
        
        if self.painting == False:
            self.painting = True
            size = self.GetClientSize()
            scrollpos = self.GetScrollPos(wx.VERTICAL)
            result0, col0, row0 = self.HitTest(wx.Point(0, 0))
            result1, col1, row1 = self.HitTest(wx.Point(size.x, size.y))
            
            pos0 = self.XYToPosition(0, row0)
            pos1 = self.XYToPosition(1, row1)
            
            HeeksCNC.program.nccode.FormatBlocks(self, pos0, pos1)
            
            self.SetScrollPos(wx.VERTICAL, scrollpos)
            
            self.painting = False
        
        event.Skip()
    
class OutputWindow(wx.ScrolledWindow):
    def __init__(self, parent):
        wx.ScrolledWindow.__init__(self, parent, name = 'Output', style = wx.HSCROLL + wx.VSCROLL + wx.NO_FULL_REPAINT_ON_RESIZE)
        self.textCtrl = OutputTextCtrl(self)
        self.textCtrl.SetMaxLength(0)
        self.Bind(wx.EVT_SIZE, self.OnSize)
        self.Resize()
        
    def Resize(self):
        self.textCtrl.SetSize(self.GetClientSize())
        
    def Clear(self):
        self.textCtrl.Clear()
    
    def OnSize(self, event):
        self.Resize()
        event.Skip()
    
