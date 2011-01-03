from Object import Object

class Operation(Object):
    def __init__(self):
        Object.__init__(self)
        self.active = True
        
    def name(self):
        # the name of the item in the tree
        return "Operation"
    
    def icon(self):
        # the name of the PNG file in the HeeksCNC icons folder
        if active:
            return self.op_icon()
        else:
            return "noentry"