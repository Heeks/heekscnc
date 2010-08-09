import nc
import iso
import iso_codes

class CodesMach3(iso_codes.Codes):
	def SPACE(self): return(' ')

iso_codes.codes = CodesMach3()


class CreatorMach3(iso.CreatorIso):
	def init(self): 
		iso.CreatorIso.init(self) 

	def program_begin(self, id, comment):
		self.write( ('(' + 'GCode created using the HeeksCNC Mach3 post processor' + ')' + '\n') )
		self.write( ('(' + comment + ')' + '\n') )

	def tool_change(self, id):
		self.write_blocknum()
		self.write('G43H%i'% id +'\n')
		self.write_blocknum()
		self.write((iso_codes.codes.TOOL() % id) + '\n')
		self.t = id

nc.creator = CreatorMach3()

