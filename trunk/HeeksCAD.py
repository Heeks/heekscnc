import HeeksPython as heekscad
from Cad import Cad
import HeeksCNC
import re
pattern_main = re.compile('([(!;].*|\s+|[a-zA-Z0-9_:](?:[+-])?\d*(?:\.\d*)?|\w\#\d+|\(.*?\)|\#\d+\=(?:[+-])?\d*(?:\.\d*)?)')

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
        heekscad.add_window(hwnd_or_id)
        
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
    
    def WriteAreaToProgram(self, sketches):
        HeeksCNC.program.python_program += "a = area.Area()\n"
        for sketch in sketches:
            sketch_shape = heekscad.GetSketchShape(int(sketch))
            if sketch_shape:
                length = len(sketch_shape)
                i = 0
                s = ""
                HeeksCNC.program.python_program += "c = area.Curve()\n"
                while i < length:
                    if sketch_shape[i] == '\n':
                        WriteSpan(s)
                        s = ""
                    else:
                        s += sketch_shape[i]
                    i = i + 1
                HeeksCNC.program.python_program += "a.append(c)\n"
        HeeksCNC.program.python_program += "\n"

def WriteLine(words):
    if words[0][0] != "x":return
    x = words[0][1:]
    if words[1][0] != "y":return
    y = words[1][1:]
    HeeksCNC.program.python_program += "c.append(area.Point(" + x + ", " + y + "))\n"

def WriteArc(direction, words):
    type_str = "-1"
    if direction: type_str = "1"
    if words[1][0] != "x":return
    x = words[1][1:]
    if words[2][0] != "y":return
    y = words[2][1:]
    if words[3][0] != "i":return
    i = words[3][1:]
    if words[4][0] != "j":return
    j = words[4][1:]
    HeeksCNC.program.python_program += "c.append(area.Vertex(" + type_str + ", area.Point(" + x + ", " + y + "), area.Point(" + i + ", " + j + ")))\n"

def WriteSpan(span_str):
    global pattern_main
    words = pattern_main.findall(span_str)
    length = len(words)
    if length < 1:return
    print "words[0] = ", words[0]
    if words[0][0] == 'a':
        if length != 5:return
        WriteArc(True, words)
    elif words[0][0] == 't':
        if length != 5:return
        WriteArc(False, words)
    else:
        if length != 2:return
        WriteLine(words)
        
