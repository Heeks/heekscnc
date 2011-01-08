import HeeksCNC

class Object:
    def __init__(self):
        self.parent = None
        self.children = []
            
    def name(self):
        # the name of the item in the tree
        return self.TypeName()
    
    def TypeName(self):
        return "unknown"
    
    def icon(self):
        # the name of the PNG file in the HeeksCNC icons folder
        return "unknown"
    
    def CanEdit(self):
        return True # assume all objects will have an edit dialog
    
    def CanBeDeleted(self):
        return False
    
    def Edit(self):
        import wx
        wx.MessageBox(self.name() + " edit dialog not done yet")
        return False
        
    def AddToPopupMenu(self, menu):
        if self.CanBeDeleted(): menu.AddItem("Delete", self.OnDelete)
    
    def Add(self, child):
        child.parent = self
        self.children.append(child)

    def OnDelete(self):
        if self.parent != None:
            self.parent.children.remove(self)
        HeeksCNC.tree.Remove(self)
        HeeksCNC.tree.Refresh()
