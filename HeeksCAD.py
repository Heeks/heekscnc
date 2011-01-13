import HeeksPython as heekscad
from Cad import Cad
import HeeksCNC

class HeeksCAD(Cad):
    def __init__(self):
        Cad.__init__(self)
    
    def add_menu_item(self, menu, label, callback, icon):
        heekscad.add_menu_item(menu, label, callback, icon)
        
    def addmenu(self, name):
        return heekscad.addmenu(name)
        
    def add_window(self, window):
        import platform
        if platform.system() == "Windows":
            hwnd_or_id = window.GetHandle()
        else:
            hwnd_or_id = window.GetId()
        return heekscad.add_window(hwnd_or_id)
        
    def get_frame_hwnd(self):
        return heekscad.get_frame_hwnd()    
        
    def get_frame_id(self):
        return heekscad.get_frame_id()
    
    def on_new_or_open(self, open, res):
        if open == 0:
            HeeksCNC.on_new()
        else:
            HeeksCNC.on_open()
            
    def register_callbacks(self):
        heekscad.register_callbacks(self.on_new_or_open)
        # to do, on_save
        
    def get_view_units(self):
        return heekscad.get_view_units()

    def get_selected_sketches(self):
        sketches = heekscad.get_selected_sketches()
        str_sketches = []
        for sketch in sketches:
            str_sketches.append(str(sketch))
        return str_sketches

    def pick_sketches(self):
        # returns a list of strings, one name for each sketch
        sketches = heekscad.getsketches()
        str_sketches = []
        for sketch in sketches:
            str_sketches.append(str(sketch))
        return str_sketches
        
    def repaint(self):
        # repaints the CAD system
        heekscad.redraw()
            
    def GetFileFullPath(self):
        s = heekscad.GetFileFullPath()
        if s == None: return None
        return s.replace('\\', '/')
