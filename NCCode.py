from Object import Object


# to do, if at all

class NCCode(Object):
    def __init__(self):
        Object.__init__(self)
        self.blocks = [] # for now, just strings, but later to be NCCodeBlock objects
        
    def icon(self):
        return "nccode"
    
    def TypeName(self):
        return "NC Code"
    
    def CanBeDeleted(self):
        return False
        
    def SetTextCtrl(self, textCtrl):
        textCtrl.Clear()
        textCtrl.Freeze()

        import wx
        font = wx.Font(10, wx.FONTFAMILY_DEFAULT, wx.FONTSTYLE_NORMAL, wx.FONTWEIGHT_NORMAL, False, "Lucida Console", wx.FONTENCODING_SYSTEM)
        ta = wx.TextAttr()
        ta.SetFont(font)
        textCtrl.SetDefaultStyle(ta)

        str = ""
        for block in self.blocks:
            #str += block.Text()
            str += block + "\n"
        
        textCtrl.SetValue(str)

        '''
        import platform
        if platform.system() != "Windows":
        # for Windows, this is done in OutputTextCtrl.OnPaint
            for block in self.blocks:
                block.FormatText(textCtrl)
        '''
        
        textCtrl.Thaw()
    
    def FormatBlocks(self, textCtrl, i0, i1):
        '''
        textCtrl.Freeze()
        
        for block in self.blocks:
            if i0 <= block.from_pos and block.from_pos <= i1:
                block.FormatText(textCtrl)
        textCtrl.Thaw()
        '''
    
    def HighlightBlock(self, pos):
        # to do
        '''
        self.highlighted_block = None

        for block in self.blocks:
            if pos < block.to_pos:
                self.highlighted_block = block;
                break
        DestroyGLLists()
        '''