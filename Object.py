import HeeksCNC

index_map = {}  # maps object_index to object
next_object_index = 1

class Object:
    def __init__(self):
        self.parent_index = None
        self.children = []
        
        # set the index
        global next_object_index
        self.index = next_object_index
        next_object_index = next_object_index + 1
        
        # add to the index map, to find it again later
        global index_map
        index_map[self.index] = self
            
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
        return True
    
    def Edit(self):
        import wx
        wx.MessageBox(self.name() + " edit dialog not done yet")
        return False
        
    def AddToPopupMenu(self, menu):
        if self.CanBeDeleted(): menu.AddItem("Delete", self.OnDelete)
    
    def Add(self, child):
        child.parent_index = self.index
        self.children.append(child)
        
    def ClearChildren(self):
        for child in self.children:
            del child
        self.children = []

    def OnDelete(self):
        parent = self.GetParent()
        if parent != None:
            parent.children.remove(self)
        HeeksCNC.tree.Remove(self)
        HeeksCNC.tree.Refresh()
    
    def GetParent(self):
        if self.parent_index == None:
            return None
        global index_map
        return index_map[self.parent_index]
