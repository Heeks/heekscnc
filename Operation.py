from Object import Object
from consts import *
import HeeksCNC
from CNCConfig import CNCConfig

class Operation(Object):
    def __init__(self):
        Object.__init__(self)
        self.active = True
        self.comment = ''
        self.title = self.TypeName()
        self.tool_number = 0
        
    def TypeName(self):
        return "Operation"
    
    def icon(self):
        # the name of the PNG file in the HeeksCNC icons folder
        if self.active:
            return self.op_icon()
        else:
            return "noentry"
            
    def CanBeDeleted(self):
        return True
    
    def UsesTool(self): # some operations don't use the tool number
        return True
    
    def ReadDefaultValues(self):
        config = CNCConfig()
        
        self.tool_number = config.ReadInt("OpTool", 0)
        
        if self.tool_number != 0:
            default_tool = HeeksCNC.program.tools.FindTool(self.tool_number)
            if default_tool == None:
                self.tool_number = 0
            else:
                self.tool_number = default_tool.tool_number
        
        if self.tool_number == 0:
            first_tool = HeeksCNC.program.tools.FindFirstTool(TOOL_TYPE_SLOTCUTTER)
            if first_tool:
                self.tool_number = first_tool.tool_number
            else:
                first_tool = HeeksCNC.program.tools.FindFirstTool(TOOL_TYPE_ENDMILL)
                if first_tool:
                    self.tool_number = first_tool.tool_number
                else:
                    first_tool = HeeksCNC.program.tools.FindFirstTool(TOOL_TYPE_BALLENDMILL)
                    if first_tool:
                        self.tool_number = first_tool.tool_number

    def WriteDefaultValues(self):
        config = CNCConfig()
        if self.tool_number != 0:
            config.WriteInt("OpTool", self.tool_number)
        
    def AppendTextToProgram(self):
        if len(self.comment) > 0:
            HeeksCNC.program.python_program += "comment(" + self.comment + ")\n"        

        if self.UsesTool():
            HeeksCNC.machine_state.AppendToolChangeText(self.tool_number) # Select the correct  tool.
            