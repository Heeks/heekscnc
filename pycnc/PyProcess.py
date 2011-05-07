import wx
import HeeksCNC

class NcWriter:
    def __init__(self):
        self.currentx = 0.0
        self.currenty = 0.0
        self.currentz = 0.0
        self.absolute_flag = True

    ############################################################################
    ##  Internals

    def on_parse_start(self, name):
        pass

    def on_parse_end(self):
        pass

    ############################################################################
    ##  send to cad

    def begin_ncblock(self):
        self.block = ""

    def end_ncblock(self):
        HeeksCNC.program.nccode.blocks.append(self.block)

    def add_text(self, s, col=None, cdata=False):
        self.block += s

    def set_mode(self, units=None):
        pass # to do

    def set_tool(self, number=None):
        pass # to do

    def begin_path(self, col=None):
        pass # to do

    def end_path(self):
        pass # to do

    def add_line(self, x=None, y=None, z=None, a=None, b=None, c=None):
        pass # to do

    def add_arc(self, x=None, y=None, z=None, i=None, j=None, k=None, r=None, d=None):
        pass # to do
        
    def incremental(self):
        pass # to do
        
    def absolute(self):
        pass # to do

def HeeksPyPostProcess(include_backplot_processing):
    HeeksCNC.output_window.Clear()

    # write the python file
    standard_paths = wx.StandardPaths.Get()
    file_str = (standard_paths.GetTempDir() + "/post.py").replace('\\', '/')
    
    if write_python_file(file_str):
        # call the python file
        execfile(file_str)
        
        # backplot the file
        HeeksPyBackplot(HeeksCNC.program.GetOutputFileName())        
        return True
    else:
        wx.MessageBox("couldn't write " + file_str)
    return False

def HeeksPyBackplot(filepath):
    HeeksCNC.output_window.Clear()
    HeeksCNC.program.nccode.blocks = []    
    
    machine_module = __import__('nc.' + HeeksCNC.program.machine.file_name + '_read', fromlist = ['dummy'])
        
    parser = machine_module.Parser()
    
    parser.writer = NcWriter()
    parser.Parse(filepath)

    HeeksCNC.program.nccode.SetTextCtrl(HeeksCNC.output_window.textCtrl)
            
def write_python_file(python_file_path):
    f = open(python_file_path, "w")
    f.write(HeeksCNC.program.python_program)
    f.close()
    return True

