from Object import Object
from consts import *
import HeeksCNC

class Operation(Object):
    def __init__(self):
        Object.__init__(self)
        self.active = True
        self.comment = ''
        self.title = self.TypeName()
        first_tool = HeeksCNC.program.tools.FindFirstTool(TOOL_TYPE_SLOTCUTTER)
        if first_tool == None: self.tool_number = 0
        else: self.tool_number = first_tool.tool_number
        if self.tool_number == 0: HeeksCNC.program.tools.FindFirstTool(TOOL_TYPE_ENDMILL)
        if self.tool_number == 0: HeeksCNC.program.tools.FindFirstTool(TOOL_TYPE_BALLENDMILL)
        
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
