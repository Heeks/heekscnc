from Object import Object
from Tools import Tools
from Operations import Operations
from RawMaterial import RawMaterial
from Machine import Machine
from NCCode import NCCode
from CNCConfig import CNCConfig
from consts import *
import HeeksCNC
from MachineState import MachineState

class Program(Object):
    def __init__(self):
        Object.__init__(self)
        config = CNCConfig()
        self.units = config.ReadFloat("ProgramUnits", 1.0) # set to 25.4 for inches
        self.alternative_machines_file = config.Read("ProgramAlternativeMachinesFile", "")
        self.raw_material = RawMaterial()    #// for material hardness - to determine feeds and speeds.
        machine_name = config.Read("ProgramMachine", "emc2b")
        self.machine = self.GetMachine(machine_name)
        import wx
        default_output_file = (wx.StandardPaths.Get().GetTempDir() + "/test.tap").replace('\\', '/')
        self.output_file = config.Read("ProgramOutputFile", default_output_file)  #  // NOTE: Only relevant if the filename does NOT follow the data file's name.
        self.output_file_name_follows_data_file_name = config.ReadBool("OutputFileNameFollowsDataFileName", True) #    // Just change the extension to determine the NC file name
        self.python_program = ""
        self.path_control_mode = config.ReadInt("ProgramPathControlMode", PATH_CONTROL_UNDEFINED)
        self.motion_blending_tolerance = config.ReadFloat("ProgramMotionBlendingTolerance", 0.0)    # Only valid if m_path_control_mode == eBestPossibleSpeed
        self.naive_cam_tolerance = config.ReadFloat("ProgramNaiveCamTolerance", 0.0)        # Only valid if m_path_control_mode == eBestPossibleSpeed
        
    def TypeName(self):
        return "Program"
    
    def icon(self):
        # the name of the PNG file in the HeeksCNC icons folder
        return "program"
    
    def CanBeDeleted(self):
        return False
    
    def add_initial_children(self):
        # add tools, operations, etc.
        self.children = []
        self.tools = Tools()
        self.tools.load_default()
        self.Add(self.tools)
        self.operations = Operations()
        self.Add(self.operations)
        self.nccode = NCCode()
        self.Add(self.nccode)
        
    def LanguageCorrection(self):
        '''
        // Language and Windows codepage detection and correction
        #ifndef WIN32
            python << _T("# coding=UTF8\n");
            python << _T("# No troubled Microsoft Windows detected\n");
        #else
            switch((wxLocale::GetSystemLanguage()))
            {
                case wxLANGUAGE_SLOVAK :
                    python << _T("# coding=CP1250\n");
                    python << _T("# Slovak language detected in Microsoft Windows\n");
                    break;
                case wxLANGUAGE_GERMAN:
                case wxLANGUAGE_GERMAN_AUSTRIAN:
                case wxLANGUAGE_GERMAN_BELGIUM:
                case wxLANGUAGE_GERMAN_LIECHTENSTEIN:
                case wxLANGUAGE_GERMAN_LUXEMBOURG:
                case wxLANGUAGE_GERMAN_SWISS  :
                    python << _T("# coding=CP1252\n");
                    python << _T("# German language or it's variant detected in Microsoft Windows\n");
                    break;
                case wxLANGUAGE_FRENCH:
                case wxLANGUAGE_FRENCH_BELGIAN:
                case wxLANGUAGE_FRENCH_CANADIAN:
                case wxLANGUAGE_FRENCH_LUXEMBOURG:
                case wxLANGUAGE_FRENCH_MONACO:
                case wxLANGUAGE_FRENCH_SWISS:
                    python << _T("# coding=CP1252\n");
                    python << _T("# French language or it's variant detected in Microsoft Windows\n");
                    break;
                case wxLANGUAGE_ITALIAN:
                case wxLANGUAGE_ITALIAN_SWISS :
                    python << _T("# coding=CP1252\n");
                    python << _T("#Italian language or it's variant detected in Microsoft Windows\n");
                    break;
                case wxLANGUAGE_ENGLISH:
                case wxLANGUAGE_ENGLISH_UK:
                case wxLANGUAGE_ENGLISH_US:
                case wxLANGUAGE_ENGLISH_AUSTRALIA:
                case wxLANGUAGE_ENGLISH_BELIZE:
                case wxLANGUAGE_ENGLISH_BOTSWANA:
                case wxLANGUAGE_ENGLISH_CANADA:
                case wxLANGUAGE_ENGLISH_CARIBBEAN:
                case wxLANGUAGE_ENGLISH_DENMARK:
                case wxLANGUAGE_ENGLISH_EIRE:
                case wxLANGUAGE_ENGLISH_JAMAICA:
                case wxLANGUAGE_ENGLISH_NEW_ZEALAND:
                case wxLANGUAGE_ENGLISH_PHILIPPINES:
                case wxLANGUAGE_ENGLISH_SOUTH_AFRICA:
                case wxLANGUAGE_ENGLISH_TRINIDAD:
                case wxLANGUAGE_ENGLISH_ZIMBABWE:
                    python << _T("# coding=CP1252\n");
                    python << _T("#English language or it's variant detected in Microsoft Windows\n");
                    break;
                default:
                    python << _T("# coding=CP1252\n");
                    python << _T("#Not supported language detected in Microsoft Windows. Assuming English alphabet\n");
                    break;
            }
        #endif
        '''
        pass
    
    def RewritePythonProgram(self):
        HeeksCNC.program_window.Clear()
        self.python_program = ""
        del HeeksCNC.machine_state
        HeeksCNC.machine_state = MachineState()

        kurve_module_needed = False
        kurve_funcs_needed = False
        area_module_needed = False
        area_funcs_needed = False

        active_operations = []
        
        for operation in self.operations.children:
            if operation.active:
                active_operations.append(operation)

                if operation.__class__.__name__ == "Profile":
                    kurve_module_needed = True
                    kurve_funcs_needed = True
                elif operation.__class__.__name__ == "Pocket":
                    area_module_needed = True
                    area_funcs_needed = True
                    
        self.LanguageCorrection()

        # add standard stuff at the top
        self.python_program += "import sys\n"
        
        self.python_program += "sys.path.insert(0,'" + HeeksCNC.heekscnc_path + "')\n"
        self.python_program += "import math\n"

        if kurve_module_needed: self.python_program += "import kurve\n"
        if kurve_funcs_needed: self.python_program += "import kurve_funcs\n"
        if area_module_needed:
            self.python_program += "import area\n"
            self.python_program += "area.set_units(" + str(self.units) + ")\n"
        if area_funcs_needed: self.python_program += "import area_funcs\n"

        # machine general stuff
        self.python_program += "from nc.nc import *\n"

        # specific machine
        if self.machine.file_name == "not found":
            import wx
            wx.MessageBox("Machine name not set")
        else :
            self.python_program += "import nc." + self.machine.file_name + "\n"
            self.python_program += "\n"

        # output file
        self.python_program += "output('" + self.GetOutputFileName() + "')\n"

        # begin program
        self.python_program += "program_begin(123, 'Test program')\n"
        self.python_program += "absolute()\n"
        if self.units > 25.0:
            self.python_program += "imperial()\n"
        else:
            self.python_program += "metric()\n"
        self.python_program += "set_plane(0)\n"
        self.python_program += "\n"

        #self.python_program += self.raw_material.AppendTextToProgram()

        # write the tools setup code.
        for tool in self.tools.children:
            tool.AppendTextToProgram()

        for operation in active_operations:
            operation.AppendTextToProgram()

        self.python_program += "program_end()\n"
        self.python_program += "from nc.nc import creator\n"
        self.python_program += "creator.file_close()\n"
        
        HeeksCNC.program_window.AppendText(self.python_program)
        if len(self.python_program) > len(HeeksCNC.program_window.textCtrl.GetValue()):
            # The python program is longer than the text control object can handle.  The maximum
            # length of the text control objects changes depending on the operating system (and its
            # implementation of wxWidgets).  Rather than showing the truncated program, tell the
            # user that it has been truncated and where to find it.

            import wx
            standard_paths = wx.StandardPaths.Get()
            file_str = (standard_paths.GetTempDir() + "/post.py").replace('\\', '/')

            HeeksCNC.program_window.Clear();
            HeeksCNC.program_window.AppendText("The Python program is too long \n")
            HeeksCNC.program_window.AppendText("to display in this window.\n")
            HeeksCNC.program_window.AppendText("Please edit the python program directly at \n")
            HeeksCNC.program_window.AppendText(file_str)
    
    def GetOutputFileName(self):
        if self.output_file_name_follows_data_file_name == False:
            return self.output_file
        
        filepath = HeeksCNC.cad.GetFileFullPath()
        if filepath == None:
            # The user hasn't assigned a filename yet.  Use the default.
            return self.output_file

        pos = filepath.rfind('.')
        if pos == -1:
            return self.output_file
        
        filepath = filepath[0:pos] + ".tap"
        return filepath
    
    def GetMachines(self):
        machines_file = self.alternative_machines_file
        if machines_file == "":
            machines_file = HeeksCNC.heekscnc_path + "/nc/machines2.txt"
            
        f = open(machines_file)

        machines = []
        
        while (True):
            line = f.readline()
            if (len(line) == 0) : break
            line = line.rstrip('\n')
            
            machine = Machine()
            space_pos = line.find(' ')
            if space_pos == -1:
                machine.file_name = line
                machine.description = line
            else:
                machine.file_name = str(line)[0:space_pos]
                machine.description = line[space_pos:]
            machines.append(machine)

        return machines
    
    def GetMachine(self, file_name):
        machines = self.GetMachines()
        for machine in machines:
            if machine.file_name == file_name:
                return machine
        if len(machines):
            return machines[0]
        return None
        
    def Edit(self):
        if HeeksCNC.widgets == HeeksCNC.WIDGETS_WX:
            from wxProgramDlg import ProgramDlg
            import wx
            dlg = ProgramDlg(self)
            return dlg.ShowModal() == wx.ID_OK
        return False
        