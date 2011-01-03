from Object import Object

class Tool(Object):
    def __init__(self):
        Object.__init__(self)
        type = None # to do add types and variables
        
    def name(self):
        # the name of the item in the tree
        return "Tool"
    
    def icon(self):
        # the name of the PNG file in the HeeksCNC icons folder
        return "tool"