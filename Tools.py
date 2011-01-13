from Object import Object
from Tool import Tool
from consts import *
import HeeksCNC

class Tools(Object):
    def __init__(self):
        Object.__init__(self)
        
    def TypeName(self):
        return "Tools"
    
    def icon(self):
        # the name of the PNG file in the HeeksCNC icons folder
        return "tools"
    
    def CanBeDeleted(self):
        return False
    
    def AddToPopupMenu(self, menu):
        menu.AddItem("save as default tools", self.OnSaveDefault)
        
    def OnSaveDefault(self):
        self.save_default()
        
    def save_default(self):
        import cPickle
        f = open(HeeksCNC.heekscnc_path + "/default_tools.txt", "w")
        for tool in self.children:
            cPickle.dump(tool, f)
        f.close()        

    def load_default(self):
        self.ClearChildren()
        
        try:
            f = open(HeeksCNC.heekscnc_path + "/default_tools.txt")
        except:
            # no default file found, add 2 tools
            self.Add(Tool(diameter = 3.0, type = TOOL_TYPE_SLOTCUTTER, tool_number = 1))
            self.Add(Tool(diameter = 6.0, type = TOOL_TYPE_SLOTCUTTER, tool_number = 2))
            return
    
        import cPickle
        from Object import next_object_index
        while True:
            try:
                tool = cPickle.load(f)
            except:
                break # end of file
            tool.index = next_object_index
            next_object_index = next_object_index + 1
            self.Add(tool)
        f.close()
        
    def FindAllTools(self):
        tools = []
        tools.append( (0, "No tool") )
        for child in self.children:
            tools.append( (child.tool_number, child.name()) )
        return tools
    
    def FindFirstTool(self, type):
        for child in self.children:
            if child.type == type:
                return child
        return None                
        
    def FindTool(self, tool_number):
        for child in self.children:
            if child.tool_number == tool_number:
                return child
        return None
        