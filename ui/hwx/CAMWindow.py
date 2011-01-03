import wx
import HeeksCNC

class CAMWindow(wx.ScrolledWindow):
    def __init__(self, parent):
        wx.ScrolledWindow.__init__(self, parent, name = 'CAM')
        self.image_list = wx.ImageList(16, 16)
        self.image_map = dict()
        self.tree = wx.TreeCtrl(self, style=wx.TR_HAS_BUTTONS + wx.TR_LINES_AT_ROOT + wx.TR_HIDE_ROOT)
        
        self.tree.SetImageList(self.image_list)
        self.tree.SetSize(wx.Size(200, 300))
        self.root = self.tree.AddRoot("Root")

        # Use some sizers to see layout options
        self.sizer = wx.BoxSizer(wx.VERTICAL)
        self.sizer.Add(self.tree, 1, wx.EXPAND)
        
        #Layout sizers
        self.SetSizer(self.sizer)
        self.SetAutoLayout(1)
        self.sizer.Fit(self)
        self.Show()
        
    def add(self, object, parent):
        #add a tree object to the tree control
        
        icon_name = object.icon()
        if icon_name in self.image_map:
            icon = self.image_map[icon_name]
        else:
            icon = self.image_list.AddIcon(wx.Icon(HeeksCNC.heekscnc_path + "/icons/" + object.icon() + ".png", wx.BITMAP_TYPE_PNG))
            self.image_map[icon_name] = icon
 
        parent_tree_item = self.root
        if parent != None:
            parent_tree_item = parent.tree_item

        object.tree_item = self.tree.AppendItem(parent_tree_item, object.name(), icon)
