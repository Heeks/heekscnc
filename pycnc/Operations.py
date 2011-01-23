from Object import Object
import HeeksCNC
from Profile import Profile
from Pocket import Pocket

class Operations(Object):
    def __init__(self):
        Object.__init__(self)
        
    def TypeName(self):
        return "Operations"
    
    def icon(self):
        # the name of the PNG file in the HeeksCNC icons folder
        return "operations"
    
    def CanBeDeleted(self):
        return False
    
def AddOperationMenuItems(CAM_menu):
    HeeksCNC.cad.add_menu_item(CAM_menu, 'Profile Operation', on_profile_operation, HeeksCNC.heekscnc_path + '/bitmaps/opprofile.png')
    HeeksCNC.cad.add_menu_item(CAM_menu, 'Pocket Operation', on_pocket_operation, HeeksCNC.heekscnc_path + '/bitmaps/pocket.png')

def on_profile_operation():
    op = Profile()
    op.ReadDefaultValues()
    op.sketches = HeeksCNC.get_selected_sketches()
    if op.Edit():
        HeeksCNC.program.operations.Add(op)
        HeeksCNC.tree.Add(op)
        HeeksCNC.tree.Refresh()

def on_pocket_operation():
    op = Pocket()
    op.ReadDefaultValues()
    op.sketches = HeeksCNC.cad.get_selected_sketches()
    if op.Edit():
        HeeksCNC.program.operations.Add(op)
        HeeksCNC.tree.Add(op)
        HeeksCNC.tree.Refresh()
