import nc
import emc2

class CreatorGantryRouter(emc2.CreatorEMC2):
    def init(self): 
        emc2.CreatorEMC2.init(self) 

    def program_begin(self, id, comment):
	self.write( ('(' + comment + ')' + '\n') )

    def tool_defn(self, id, name='', radius=None, length=None, gradient=None):
        pass

    def spindle(self, s, clockwise):
	pass

nc.creator = CreatorGantryRouter()

