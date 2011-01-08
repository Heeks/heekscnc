from Object import Object


# to do, if at all

class NCCode(Object):
    def __init__(self):
        Object.__init__(self)
        
    def icon(self):
        return "nccode"
    
    def TypeName(self):
        return "NC Code"
        
    def SetTextCtrl(self, textCtrl):
        pass
    
    def FormatBlocks(self, textCtrl, i0, i1):
        pass
    
    def HighlightBlock(self, pos):
        pass
