from Object import Object
from Tool import Tool
from consts import *

class Tools(Object):
    def __init__(self):
        Object.__init__(self)
        
    def TypeName(self):
        return "Tools"
    
    def icon(self):
        # the name of the PNG file in the HeeksCNC icons folder
        return "tools"
    
    def load_default(self):
        # to do, read a defaults file
        
        # test, add 2 tools
        self.Add(Tool(diameter = 3.0, type = TOOL_TYPE_SLOTCUTTER, tool_number = 1))
        self.Add(Tool(diameter = 6.0, type = TOOL_TYPE_SLOTCUTTER, tool_number = 2))
        
    def FindAllTools(self):
        tools = []
        tools.append( (0, "No tool") )
        if self.children != None:
            for child in self.children:
                tools.append( (child.tool_number, child.name()) )
        return tools
    
    def FindFirstTool(self, type):
        if self.children != None:
            for child in self.children:
                if child.type == type:
                    return child
        return None                
        