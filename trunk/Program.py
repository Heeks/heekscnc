from Object import Object
from Tools import Tools
from Operations import Operations

class Program(Object):
    def __init__(self):
        Object.__init__(self)
        
    def name(self):
        # the name of the item in the tree
        return "Program"
    
    def icon(self):
        # the name of the PNG file in the HeeksCNC icons folder
        return "program"
    
    def add_initial_children(self):
        # add tools, operations, etc.
        self.children = []
        tools = Tools()
        tools.load_default()
        self.children.append(tools)
        operations = Operations()
        self.children.append(operations)