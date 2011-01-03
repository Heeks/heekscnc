CAD_SYSTEM_HEEKS = 1

WIDGETS_WX = 1
WIDGETS_QT = 2

tree = None
program = None
cad_system = CAD_SYSTEM_HEEKS
widgets = WIDGETS_WX
heekscnc_path = None

if cad_system == CAD_SYSTEM_HEEKS:
    import HeeksPython as heekscad
    
import platform
from Program import Program
from Operations import AddOperationMenuItems

def on_post_process():
    import wx
    wx.MessageBox("post process")

def on_polar_array():
    import polar_array
    frame_1 = polar_array.MyFrame(None, -1, "")
    frame_1.Show()

def add_menus():
    CAM_menu = addmenu('CAM')
    global heekscnc_path
    AddOperationMenuItems(CAM_menu)
    add_menu_item(CAM_menu, 'Post Process', on_post_process, heekscnc_path + '/bitmaps/postprocess.png')
    if platform.system() != "Windows":
        add_menu_item(CAM_menu, 'Bolt Circle', on_polar_array, heekscnc_path + '/polar_array/polar_array.png')
    
def add_windows():
    from ui.Tree import Tree
    global tree
    tree = Tree()
    tree.add(program)
    tree.Refresh()

def start():
    global heekscnc_path
    
    import os
    full_path_here = os.path.abspath( __file__ )
    bslash = full_path_here.rfind('\\')
    fslash = full_path_here.rfind('/')
    slash = bslash if bslash > fslash else fslash
    heekscnc_path = full_path_here[0:slash]
    
    add_program_with_children()
    add_menus()
    add_windows()
    register_callbacks() 
    
def add_menu_item(menu, label, callback, icon):
    if cad_system == CAD_SYSTEM_HEEKS:
        heekscad.add_menu_item(menu, label, callback, icon)
        
def addmenu(name):
    if cad_system == CAD_SYSTEM_HEEKS:
        return heekscad.addmenu(name)
    
def add_window(hwnd):
    if cad_system == CAD_SYSTEM_HEEKS:
        return heekscad.add_window(hwnd)
    
def get_frame_hwnd():
    if cad_system == CAD_SYSTEM_HEEKS:
        return heekscad.get_frame_hwnd()    
    
def get_frame_id():
    if cad_system == CAD_SYSTEM_HEEKS:
        return heekscad.get_frame_id()
        
def register_callbacks():
    if cad_system == CAD_SYSTEM_HEEKS:
        heekscad.register_callbacks(on_new_or_open)
    
def on_new_or_open(open, res):
    from PyQt4 import QtGui

    app = QtGui.QApplication([])
    w = QtGui.QWidget()
    w.show()

    if open == 0:
        # new file
        add_program_with_children()
        tree.Recreate()
    #else: to do, load the program

def add_program_with_children():
    global program
    program = Program()
    program.add_initial_children()
