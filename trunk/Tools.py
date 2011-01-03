from Object import Object
from Tool import Tool

class Tools(Object):
    def __init__(self):
        Object.__init__(self)
        
    def name(self):
        # the name of the item in the tree
        return "Tools"
    
    def icon(self):
        # the name of the PNG file in the HeeksCNC icons folder
        return "tools"
    
    def load_default(self):
        # to do, read a defaults file
        
        # test, add 2 tools
        self.children = []
        self.children.append(Tool())
        self.children.append(Tool())