import nc
import iso

class CreatorGantryRouter(iso.CreatorIso):
    def init(self): 
        iso.CreatorIso.init(self) 

    def program_begin(self, id, comment):
	self.write( ('(' + comment + ')' + '\n') )

    def tool_defn(self, id, name='', radius=None, length=None):
        pass

    def spindle(self, s, clockwise):
	pass

nc.creator = CreatorGantryRouter()
