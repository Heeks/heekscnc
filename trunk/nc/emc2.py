import nc
import iso

class CreatorEMC2(iso.CreatorIso):
    def init(self): 
        iso.CreatorIso.init(self) 

    def program_begin(self, id, comment):
	self.write( ('(' + comment + ')' + '\n') )

nc.creator = CreatorEMC2()
