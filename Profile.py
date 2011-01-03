from Operation import Operation

class Profile(Operation):
    def __init__(self):
        Operation.__init__(self)
        
    def name(self):
        # the name of the item in the tree
        return "Profile"
    
    def op_icon(self):
        # the name of the PNG file in the HeeksCNC icons folder
        return "profile"