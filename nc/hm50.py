import nc
import iso_codes
import emc2

iso_codes.codes = emc2.CodesEMC2()

class CreatorHM50(emc2.CreatorEMC2):
	def init(self): 
		iso.CreatorEMC2.init(self) 
	
	def program_begin(self, id, comment):
		self.write( ('(' + comment + ')' + '\n') )

	def tool_change(self, id):
		self.write_blocknum()
		self.write('G53 G00 Z350\n')
		self.write_blocknum()
		self.write((iso_codes.codes.TOOL() % id) + '\n')
		self.write_blocknum()
		self.write('G01 Z100.000 F800.000\n')
		self.write_blocknum()
		self.write('M0\n')
		self.write_blocknum()
		self.write('G01 Z10.000 F300.000\n')


nc.creator = CreatorHM50()
