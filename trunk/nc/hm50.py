import nc
import emc2

class CreatorHM50(emc2.CreatorEMC2):
    def init(self): 
        iso.CreatorEMC2.init(self) 
	
    def program_begin(self, id, comment):
	self.write( ('(' + comment + ')' + '\n') )

nc.creator = CreatorHM50()
