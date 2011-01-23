class Cad:
    def __init__(self):
        pass
    
    def add_menu_item(self, menu, label, callback, icon = None):
        pass
        
    def addmenu(self, name):
        return None
        
    def add_window(self, window):
        return None
        
    def get_frame_hwnd(self):
        return None
        
    def get_frame_id(self):
        return None
            
    def register_callbacks(self):
        pass
        
    def get_view_units(self):
        return 1.0

    def get_selected_sketches(self):
        return []

    def hide_window_on_pick_sketches(self):
        return True

    def pick_sketches(self):
        return []
        
    def repaint(self):
        pass
            
    def GetFileFullPath(self):
        return ""
    
    def WriteAreaToProgram(self, sketches):
        pass