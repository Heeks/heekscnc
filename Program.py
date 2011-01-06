from Object import Object
from Tools import Tools
from Operations import Operations

class Program(Object):
    def __init__(self):
        Object.__init__(self)
        self.units = 1.0 # set to 25.4 for inches
        
    def TypeName(self):
        return "Program"
    
    def icon(self):
        # the name of the PNG file in the HeeksCNC icons folder
        return "program"
    
    def add_initial_children(self):
        # add tools, operations, etc.
        self.children = []
        self.tools = Tools()
        self.tools.load_default()
        self.Add(self.tools)
        self.operations = Operations()
        self.Add(self.operations)