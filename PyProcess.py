import wx
import HeeksCNC

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
    from nc.pyemc2b_read import ParserEMC2
    parser = ParserEMC2()
    parser.Parse(filepath)
    HeeksCNC.program.nccode.SetTextCtrl(HeeksCNC.output_window.textCtrl)
            
def write_python_file(python_file_path):
    f = open(python_file_path, "w")
    f.write(HeeksCNC.program.python_program)
    return True

