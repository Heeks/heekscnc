# -*- coding: utf-8 -*-
WIDGETS_WX = 1
WIDGETS_QT = 2

tree = None
program = None
cad = None
widgets = WIDGETS_WX
heekscnc_path = None
program_window = None
output_window = None
machine_state = None
    
import platform
from Program import Program
from Operations import AddOperationMenuItems


def on_post_process():
    import wx
    wx.MessageBox("post process")
    
def RunPythonScript():
    global program
    # clear the output file
    f = open(program.GetOutputFileName(), "w")
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
    dialog = wx.FileDialog(cad.frame, "Open NC file", wildcard = "NC files" + " |*.*")
    dialog.CentreOnParent()
    
    if dialog.ShowModal() == wx.ID_OK:
        from PyProcess import HeeksPyBackplot
        HeeksPyBackplot(dialog.GetPath())
    
def on_save_nc_file():
    import wx
    dialog = wx.FileDialog(cad.frame, "Save NC file", wildcard = "NC files" + " |*.*", style = wx.FD_SAVE + wx.FD_OVERWRITE_PROMPT)
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
    global cad
    machining_menu = cad.addmenu('Machining')
    global heekscnc_path
    print "heekscnc_path = ", heekscnc_path    
    AddOperationMenuItems(machining_menu)
    cad.add_menu_item(machining_menu, 'Make Program Script', on_make_python_script, heekscnc_path + '/bitmaps/python.png')
    cad.add_menu_item(machining_menu, 'Run Program Script', on_run_program_script, heekscnc_path + '/bitmaps/runpython.png')
    cad.add_menu_item(machining_menu, 'Post Process', on_post_process, heekscnc_path + '/bitmaps/postprocess.png')
    cad.add_menu_item(machining_menu, 'Open NC File', on_open_nc_file, heekscnc_path + '/bitmaps/opennc.png')
    cad.add_menu_item(machining_menu, 'Save NC File', on_save_nc_file, heekscnc_path + '/bitmaps/savenc.png')
    cad.add_menu_item(machining_menu, 'Cancel Python Script', on_cancel_script, heekscnc_path + '/bitmaps/cancel.png')
    
def add_windows():
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
        program_window = ProgramWindow(cad.frame)
        output_window = OutputWindow(cad.frame)
        cad.add_window(program_window)
        cad.add_window(output_window)
    
def on_new(self):
    add_program_with_children()
    tree.Recreate()

def on_open(self):
    # to do, load the program
    pass

def on_save(self):
    # to do, save the program
    pass

def start():
    global heekscnc_path
    
    import wx
    import os
    import sys

    full_path_here = os.path.abspath( __file__ )
    full_path_here = full_path_here.replace("\\", "/")
    heekscnc_path = full_path_here
    slash = heekscnc_path.rfind("/")
    print 'heekscnc_path = ', heekscnc_path
    
    if slash != -1:
        heekscnc_path = heekscnc_path[0:slash]
        slash = heekscnc_path.rfind("/")
        if slash != -1: heekscnc_path = heekscnc_path[0:slash]
    print 'heekscnc_path = ', heekscnc_path
    
    add_program_with_children()
    add_menus()
    add_windows()
    cad.on_start() 

def add_program_with_children():
    global program
    program = Program()
    program.add_initial_children()
