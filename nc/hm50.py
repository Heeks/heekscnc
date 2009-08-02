import nc
import iso
import iso_codes

class CreatorHM50(iso.CreatorIso):
    def init(self): 
        iso.CreatorIso.init(self) 
        iso_codes.SPACE = ' '
	
    def program_begin(self, id, comment):
	self.write( ('(' + comment + ')' + '\n') )

nc.creator = CreatorHM50()
