from Object import Object
import HeeksCNC

class Operations(Object):
    def __init__(self):
        Object.__init__(self)
        
    def name(self):
        # the name of the item in the tree
        return "Operations"
    
    def icon(self):
        # the name of the PNG file in the HeeksCNC icons folder
        return "operations"
    
def AddOperationMenuItems(CAM_menu):
    HeeksCNC.add_menu_item(CAM_menu, 'Profile Operation', on_profile_operation, HeeksCNC.heekscnc_path + '/bitmaps/opprofile.png')
    HeeksCNC.add_menu_item(CAM_menu, 'Pocket Operation', on_pocket_operation, HeeksCNC.heekscnc_path + '/bitmaps/pocket.png')

def on_profile_operation():
    import wx
    wx.MessageBox("profile operation")

def on_pocket_operation():
    import wx
    wx.MessageBox("pocket operation")
