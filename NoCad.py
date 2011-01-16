import sys
import getopt
from Cad import Cad
import HeeksCNC
import wx
import wx.aui

# this is an example of how to plugin HeeksCNC into a cad system
# here we make a wxWidgets application with a menu to represent the CAD system

class NoCad(Cad):
    def __init__(self):
        self.current_profile_dxf = []
        
        Cad.__init__(self)
        
        try:
            opts, args = getopt.getopt(sys.argv[1:], "h", ["help"])
        except getopt.error, msg:
            print msg
            print "for help use --help"
            sys.exit(2)
        # process options
        for o, a in opts:
            if o in ("-h", "--help"):
                print __doc__
                sys.exit(0)
        # process arguments
        for arg in args:
            self.current_profile_dxf.append('"')
            self.current_profile_dxf.append(arg)
            self.current_profile_dxf.append('" ')
            #self.current_profile_dxf = arg # process() is defined elsewhere
        
        # make a wxWidgets application
        self.frame= wx.Frame(None, -1, 'Dxf CAD, all you need to make gcode')
        self.menubar = wx.MenuBar()
        self.menu = wx.Menu("File")
        self.frame.Bind(wx.EVT_MENU_RANGE, self.OnMenu, id=100, id2=1000)
        self.menu_map = {}
        self.next_menu_id = 100
        #self.add_menu_item(self.menu, "Open", self.OnMenuOpen)
        self.aui_manager = wx.aui.AuiManager()
        self.aui_manager.SetManagedWindow(self.frame)
        
    def OnMenu(self, event):
        callback = self.menu_map[event.GetId()]
        callback()
    
    def OnMenuOpen(self):
        pass

    def add_menu_item(self, menu, label, callback, icon = None):
        item = wx.MenuItem(menu, self.next_menu_id, label)
        self.menu_map[self.next_menu_id] = callback
        self.next_menu_id = self.next_menu_id + 1
        menu.AppendItem(item)
        
    def addmenu(self, name):
        menu = wx.Menu()
        self.menubar.Append(menu, name)
        return menu
        
    def add_window(self, window):
        self.aui_manager.AddPane(window, wx.aui.AuiPaneInfo().Name(window.GetLabel()).Caption(window.GetLabel()).Center())
        
    def get_frame_hwnd(self):
        return self.frame.GetHandle()  
        
    def get_frame_id(self):
        return self.frame.GetId()
    
    def on_new_or_open(self, open, res):
        if open == 0:
            pass
        else:
            pass
            
    def register_callbacks(self):
        #heekscad.register_callbacks(self.on_new_or_open)
        pass
        # to do, on_save
        
    def get_view_units(self):
        return 1.0

    def get_selected_sketches(self):        
        return self.current_profile_dxf

    def pick_sketches(self):
        # returns a list of strings, one name for each sketch
        str_sketches = []

        # open dxf file
        dialog = wx.FileDialog(HeeksCNC.frame, "Choose sketch DXF file", wildcard = "DXF files" + " |*.dxf")
        dialog.CentreOnParent()
        
        if dialog.ShowModal() == wx.ID_OK:
            str_sketches.append(dialog.GetPath())
        return str_sketches
        
    def repaint(self):
        # repaints the CAD system
        #heekscad.redraw()
        pass
            
    def GetFileFullPath(self):
        return None
    
    def WriteAreaToProgram(self, sketches):
        HeeksCNC.program.python_program += "a = area.Area()\n"
        for sketch in sketches:
            HeeksCNC.program.python_program += 'sub_a = area.AreaFromDxf("' + sketch + '")\n'
            HeeksCNC.program.python_program += "for curve in sub_a.getCurves():\n"
            HeeksCNC.program.python_program += " a.append(curve)\n"
        HeeksCNC.program.python_program += "\n"

def main():
    import wx
    app = wx.App()
    nocad = NoCad()
    HeeksCNC.cad = nocad
    HeeksCNC.start()
    nocad.frame.SetMenuBar(nocad.menubar)
    nocad.frame.Center()
    nocad.aui_manager.Update()
    nocad.frame.Show()
    app.MainLoop()

if __name__ == '__main__':
    main()
