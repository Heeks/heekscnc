import wx
import HeeksCNC
from PopupMenu import PopupMenu

class CAMWindow(wx.ScrolledWindow):
    def __init__(self):
        hwnd = HeeksCNC.get_frame_hwnd()
        self.frame = wx.Window_FromHWND(None, hwnd)
        wx.ScrolledWindow.__init__(self, self.frame, name = 'CAM')
        self.image_list = wx.ImageList(16, 16)
        self.image_map = dict()
        self.tree = wx.TreeCtrl(self, style=wx.TR_HAS_BUTTONS + wx.TR_LINES_AT_ROOT + wx.TR_HIDE_ROOT)

        self.tree.Bind(wx.EVT_TREE_ITEM_RIGHT_CLICK, self.OnItemRightClick, self.tree)
        self.tree.Bind(wx.EVT_LEFT_DCLICK, self.OnTreeDoubleClick)
        
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
        
    def OnItemRightClick(self, evt):
        item = evt.GetItem()
        object = self.tree.GetPyData(item)
        
        menu = PopupMenu()
        object.AddToPopupMenu(menu)

        self.PopupMenu( menu.menu, evt.GetPoint() )
        menu.menu.Destroy()
        
    def OnTreeDoubleClick(self, evt):
        item, flags = self.tree.HitTest(evt.GetPosition())
        if item and flags & (wx.TREE_HITTEST_ONITEMLABEL + wx.TREE_HITTEST_ONITEMICON): 
            object = self.tree.GetPyData(item)
            if object.CanEdit():
                object.Edit()
        
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
        self.tree.SetPyData(object.tree_item, object)
        
    def remove(self, object):
        #add a tree object to the tree control
        self.tree.Delete(object.tree_item)
