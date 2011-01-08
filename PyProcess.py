import wx
import HeeksCNC

class PyProcess(wx.Process):
    def __init__(self):
        wx.Process.__init__(self, HeeksCNC.frame)
        self.pid = 0
        self.redirect = False
        self.Bind(wx.EVT_TIMER, self.OnTimer)
        self.Timer = wx.Timer(self)        
    
    def HandleInput(self):
        input_stream = self.GetInputStream()
        error_stream = self.GetErrorStream()
        
        if input_stream:
            s = ""
            while input_stream.CanRead():
                s += input_stream.readline()
            if len(s)>0:
                wx.LogMessage("> " + s)
                
        if error_stream:
            s = ""
            while error_stream.CanRead():
                s += error_stream.readline()
            if len(s)>0:
                wx.LogMessage("! " + s)
            
    def Execute(self, cmd):
        if self.redirect:
            Redirect()
            
        self.pid = wx.Execute(cmd, wx.EXEC_ASYNC + wx.EXEC_MAKE_GROUP_LEADER, self)
        
        if self.pid == 0:
            wx.LogMessage("could not execute '" + cmd + "'")
        else:
            wx.LogMessage("starting '" + cmd + "'")
            
        if self.redirect:
            self.timer.Start(100) # msec
    
    def Cancel(self):
        if self.pid == 0:
            return
        
        if self.Exists(self.pid):
            kerror = wx.Kill(self.pid, flags = wx.KILL_CHILDREN)
            if kerror == wx.KILL_OK:
                wx.LogMessage("sent kill signal to process " + str(self.pid))
            elif kerror == wx.KILL_NO_PROCESS:
                wx.LogMessage("process " + str(self.pid) + " already exited")
            elif kerror == wx.KILL_ACCESS_DENIED:
                wx.LogMessage("sending kill signal to process " + str(self.pid) + " - access denied")
            elif kerror == wx.KILL_BAD_SIGNAL:
                wx.LogMessage("no such signal")
            elif kerror == wx.KILL_ERROR:
                wx.LogMessage("unspecified error sending kill signal")
        else:
            wx.LogMessage("process " + str(self.pid) + " has gone away")
        self.pid = 0
    
    def OnTerminate(self, pid, status):
        if pid == self.pid:
            if self.redirect:
                self.timer.Stop()
                self.HandleInput() # anything left?
            if status:
                wx.LogMessage("process " + pid + " exit(" + status + ")")
            else:
                wx.LogDebug("process " + pid + " exit(0)")
    
    def OnTimer(self, event):
        self.HandleInput()
    
    def ThenDo(self):
        pass

def HeeksPyPostProcess(include_backplot_processing):
#    try:
    HeeksCNC.output_window.Clear()

    # write the python file
    standard_paths = wx.StandardPaths.Get()
    file_str = (standard_paths.GetTempDir() + "/post.py").replace('\\', '/')
    
    if write_python_file(file_str):
        # call the python file
        post_process = PyPostProcess(include_backplot_processing)
        post_process.Do()
        return True
    else:
        wx.MessageBox("couldn't write " + file_str)
#    except:
#        wx.MessageBox("Error while post-processing the program!")
    return False

current_backplot = None
current_post_process = None

def HeeksPyBackplot(filepath):
    HeeksCNC.output_window.Clear()

    # call the python file
    backplot = PyBackPlot(filepath)
    backplot.Do()
    del backplot

def HeeksPyCancel():
    global current_backplot
    if current_backplot: current_backplot.Cancel()
    global current_post_process
    if current_post_process: current_post_process.Cancel()
    
class PyBackPlot(PyProcess):
    def __init__(self, filename):
        PyProcess.__init__(self)
        self.filename = filename
        global current_backplot
        current_backplot = self
        
    def __del__(self):
        current_backplot = None
        
    def Do(self):
        busy_cursor = wx.BusyCursor()
        if HeeksCNC.program.machine.file_name == "not found":
            wx.MessageBox("Machine name not set")
        else:
            import platform
            if platform.system() == "Windows":
                wx.Execute('"' + HeeksCNC.heekscnc_path + '\\nc_read.bat" "' + HeeksCNC.program.machine.file_name + '" "' + self.filename + '"')
            else:
                wx.Execute('python "' + HeeksCNC.heekscnc_path + '/' + HeeksCNC.program.machine.file_name + '_read.py"  "' + self.filename + '"')
        del busy_cursor

class PyPostProcess(PyProcess):
    def __init__(self, include_backplot_processing):
        PyProcess.__init__(self)
        self.include_backplot_processing = include_backplot_processing
        global current_backplot
        current_post_process = self
        
    def __del__(self):
        current_post_process = None
        
    def Do(self):
        self.busy_cursor = wx.BusyCursor()
        standard_paths = wx.StandardPaths.Get()
        path = (standard_paths.GetTempDir() + "/post.py").replace('\\', '/')
        import platform
        if platform.system() == "Windows":
            command = '"' + HeeksCNC.heekscnc_path + '/post.bat" "' + path + '"'
            print command
            wx.Execute(command)
        else:
            wx.Execute('python "' + path + '"')
        
    def ThenDo(self):
        if self.include_backplot_processing:
            backplot = PyBackPlot(HeeksCNC.program.GetOutputFileName())
            backplot.Do()
            del backplot
            
def write_python_file(python_file_path):
    f = open(python_file_path, "w")
    f.write(HeeksCNC.program.python_program)
    return True

