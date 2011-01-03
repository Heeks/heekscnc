from Operation import Operation

class Pocket(Operation):
    def __init__(self):
        Operation.__init__(self)
        
    def name(self):
        # the name of the item in the tree
        return "Pocket"
    
    def op_icon(self):
        # the name of the PNG file in the HeeksCNC icons folder
        return "pocket"