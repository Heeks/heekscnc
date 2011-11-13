# -*- coding: utf-8 -*-
WIDGETS_WX = 1
WIDGETS_QT = 2

heekscnc_path = None
heekscnc = None
    
import platform
from Program import Program
from Operations import AddOperationMenuItems

class HeeksCNC:
    def __init__(self):
        self.tree = None
        self.program = None
        self.cad = None
        self.widgets = WIDGETS_WX
        self.program_window = None
        self.output_window = None
        self.machine_state = None

    def on_post_process(self):
        import wx
        wx.MessageBox("post process")
        
    def RunPythonScript(self):
        # clear the output file
        f = open(self.program.GetOutputFileName(), "w")
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
        text_control_length = self.program_window.textCtrl.GetLastPosition()
        
        if self.program.python_program[0:text_control_length] != self.program_window.textCtrl.GetValue():
            # copy the contents of the program canvas to the string
            self.program.python_program = self.program_window.textCtrl.GetValue()

        from PyProcess import HeeksPyPostProcess
        HeeksPyPostProcess(True)
        
    def on_make_python_script(self):
        self.program.RewritePythonProgram()
        
    def on_run_program_script(self):
        self.RunPythonScript()
        
    def on_post_process(self):
        self.program.RewritePythonProgram(self)
        self.RunPythonScript()

    def on_open_nc_file(self):
        import wx
        dialog = wx.FileDialog(self.cad.frame, "Open NC file", wildcard = "NC files" + " |*.*")
        dialog.CentreOnParent()
        
        if dialog.ShowModal() == wx.ID_OK:
            from PyProcess import HeeksPyBackplot
            HeeksPyBackplot(dialog.GetPath())
        
    def on_save_nc_file(self):
        import wx
        dialog = wx.FileDialog(self.cad.frame, "Save NC file", wildcard = "NC files" + " |*.*", style = wx.FD_SAVE + wx.FD_OVERWRITE_PROMPT)
        dialog.CentreOnParent()
        dialog.SetFilterIndex(1)
        
        if dialog.ShowModal() == wx.ID_OK:
            nc_file_str = dialog.GetPath()
            f = open(nc_file_str, "w")
            if f.errors:
                wx.MessageBox("Couldn't open file" + " - " + nc_file_str)
                return
            f.write(self.ouput_window.textCtrl.GetValue())
            
            from PyProcess import HeeksPyPostProcess
            HeeksPyBackplot(dialog.GetPath())
        
    def on_cancel_script(self):
        from PyProcess import HeeksPyCancel
        HeeksPyCancel()
        
    def add_menus(self):
        machining_menu = self.cad.addmenu('Machining')
        global heekscnc_path
        print "heekscnc_path = ", heekscnc_path    
        AddOperationMenuItems(machining_menu)
        self.cad.add_menu_item(machining_menu, 'Make Program Script', self.on_make_python_script, heekscnc_path + '/bitmaps/python.png')
        self.cad.add_menu_item(machining_menu, 'Run Program Script', self.on_run_program_script, heekscnc_path + '/bitmaps/runpython.png')
        self.cad.add_menu_item(machining_menu, 'Post Process', self.on_post_process, heekscnc_path + '/bitmaps/postprocess.png')
        self.cad.add_menu_item(machining_menu, 'Open NC File', self.on_open_nc_file, heekscnc_path + '/bitmaps/opennc.png')
        self.cad.add_menu_item(machining_menu, 'Save NC File', self.on_save_nc_file, heekscnc_path + '/bitmaps/savenc.png')
        self.cad.add_menu_item(machining_menu, 'Cancel Python Script', self.on_cancel_script, heekscnc_path + '/bitmaps/cancel.png')
        
    def add_windows(self):
        from Tree import Tree
        self.tree = Tree()
        self.tree.Add(self.program)
        self.tree.Refresh()
        if self.widgets == WIDGETS_WX:
            from wxProgramWindow import ProgramWindow
            from wxOutputWindow import OutputWindow
            self.program_window = ProgramWindow(self.cad.frame)
            self.output_window = OutputWindow(self.cad.frame)
            self.cad.add_window(self.program_window)
            self.cad.add_window(self.output_window)
        
    def on_new(self):
        self.add_program_with_children()
        self.tree.Recreate()

    def on_open(self):
        # to do, load the program
        pass

    def on_save(self):
        # to do, save the program
        pass

    def start(self):
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
        
        self.add_program_with_children()
        self.add_menus()
        self.add_windows()
        self.cad.on_start() 

    def add_program_with_children(self):
        self.program = Program()
        self.program.add_initial_children()
