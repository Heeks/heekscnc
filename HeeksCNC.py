CAD_SYSTEM_HEEKS = 1

WIDGETS_WX = 1
WIDGETS_QT = 2

tree = None
program = None
cad_system = CAD_SYSTEM_HEEKS
widgets = WIDGETS_WX
heekscnc_path = None
program_window = None
output_window = None
frame = None

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
    
def RunPythonScript():
    global program
    # clear the output file
    f = open(program.GetOutputFileName(), "w")
    f.write("\n")
    f.close()
    
    # clear the backplot file
    backplot_path = program.GetOutputFileName() + ".nc.xml"
    f = open(backplot_path, "w")
    f.write("\n")
    f.close()
    
    # Check to see if someone has modified the contents of the
    # program canvas manually.  If so, replace the m_python_program
    # with the edited program.  We don't want to do this without
    # this check since the maximum size of m_textCtrl is sometimes
    # a limitation to the size of the python program.  If the first 'n' characters
    # of m_python_program matches the full contents of the m_textCtrl then
    # it's likely that the text control holds as much of the python program
    # as it can hold but more may still exist in m_python_program.
    text_control_length = program_window.textCtrl.GetLastPosition()
    
    if program.python_program[0:text_control_length] != program_window.textCtrl.GetValue():
        # copy the contents of the program canvas to the string
        program.python_program = program_window.textCtrl.GetValue()

    from PyProcess import HeeksPyPostProcess
    HeeksPyPostProcess(True)
    
def on_make_python_script():
    global program
    program.RewritePythonProgram()
    
def on_run_program_script():
    RunPythonScript()
    
def on_post_process():
    global program
    program.RewritePythonProgram()
    RunPythonScript()

def on_open_nc_file():
    import wx
    global frame
    dialog = wx.FileDialog(frame, "Open NC file", wildcard = "NC files" + " |*.*")
    dialog.CentreOnParent()
    
    if dialog.ShowModal() == wx.ID_OK:
        from PyProcess import HeeksPyBackplot
        HeeksPyBackplot(dialog.GetPath())
    
def on_save_nc_file():
    import wx
    global frame
    dialog = wx.FileDialog(frame, "Save NC file", wildcard = "NC files" + " |*.*", style = wx.FD_SAVE + wx.FD_OVERWRITE_PROMPT)
    dialog.CentreOnParent()
    dialog.SetFilterIndex(1)
    
    if dialog.ShowModal() == wx.ID_OK:
        nc_file_str = dialog.GetPath()
        f = open(nc_file_str, "w")
        if f.errors:
            wx.MessageBox("Couldn't open file" + " - " + nc_file_str)
            return
        global output_window
        f.write(ouput_window.textCtrl.GetValue())
        
        from PyProcess import HeeksPyPostProcess
        HeeksPyBackplot(dialog.GetPath())
    
def on_cancel_script():
    from PyProcess import HeeksPyCancel
    HeeksPyCancel()
    
def add_menus():
    CAM_menu = addmenu('CAM')
    global heekscnc_path
    AddOperationMenuItems(CAM_menu)
    add_menu_item(CAM_menu, 'Make Program Script', on_make_python_script, heekscnc_path + '/bitmaps/python.png')
    add_menu_item(CAM_menu, 'Run Program Script', on_run_program_script, heekscnc_path + '/bitmaps/runpython.png')
    add_menu_item(CAM_menu, 'Post Process', on_post_process, heekscnc_path + '/bitmaps/postprocess.png')
    add_menu_item(CAM_menu, 'Open NC File', on_open_nc_file, heekscnc_path + '/bitmaps/opennc.png')
    add_menu_item(CAM_menu, 'Save NC File', on_save_nc_file, heekscnc_path + '/bitmaps/savenc.png')
    add_menu_item(CAM_menu, 'Cancel Python Script', on_cancel_script, heekscnc_path + '/bitmaps/cancel.png')
    
def add_windows():
    if widgets == WIDGETS_WX:
        import wx
        global frame
        if platform.system() == "Windows":
            hwnd = get_frame_hwnd()
            frame = wx.Window_FromHWND(None, hwnd)
        else:
            ID = get_frame_id()
            frame = wx.FindWindowById(ID)
            
    elif widgets == WIDGETS_QT:
        from PyQt4 import QtGui
        app = QtGui.QApplication([])
        
        
    from Tree import Tree
    global tree
    tree = Tree()
    tree.Add(program)
    tree.Refresh()
    if widgets == WIDGETS_WX:
        from wxProgramWindow import ProgramWindow
        from wxOutputWindow import OutputWindow
        global program_window
        global output_window
        program_window = ProgramWindow(frame)
        output_window = OutputWindow(frame)
        add_window(program_window)
        add_window(output_window)

def start():
    global heekscnc_path
    
    import os
    full_path_here = os.path.abspath( __file__ )
    bslash = full_path_here.rfind('\\')
    fslash = full_path_here.rfind('/')
    slash = bslash if bslash > fslash else fslash
    heekscnc_path = full_path_here[0:slash].replace('\\', '/')
    
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
    
def add_window(window):
    if platform.system() == "Windows":
        hwnd_or_id = window.GetHandle()
    else:
        hwnd_or_id = window.GetId()
    
    if cad_system == CAD_SYSTEM_HEEKS:
        return heekscad.add_window(hwnd_or_id)
    
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
    if open == 0:
        # new file
        add_program_with_children()
        tree.Recreate()
    #else: to do, load the program

def add_program_with_children():
    global program
    program = Program()
    program.add_initial_children()
    
def get_view_units():
    if cad_system == CAD_SYSTEM_HEEKS:
        return heekscad.get_view_units()
    return 1.0

def get_selected_sketches():
    if cad_system == CAD_SYSTEM_HEEKS:
        sketches = heekscad.get_selected_sketches()
        str_sketches = []
        for sketch in sketches:
            str_sketches.append(str(sketch))
        return str_sketches
    return []

def pick_sketches():
    # returns a list of strings, one name for each sketch
    if cad_system == CAD_SYSTEM_HEEKS:
        sketches = heekscad.getsketches()
        str_sketches = []
        for sketch in sketches:
            str_sketches.append(str(sketch))
        return str_sketches
    
def repaint():
    # repaints the CAD system
    if cad_system == CAD_SYSTEM_HEEKS:
        heekscad.redraw()
        
def GetFileFullPath():
    if cad_system == CAD_SYSTEM_HEEKS:
        s = heekscad.GetFileFullPath()
        if s == None: return None
        return s.replace('\\', '/')
