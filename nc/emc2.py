import nc
import iso
import iso_codes

class CodesEMC2(iso_codes.Codes):
	def SPACE(self): return(' ')

	# This version of COMMENT removes comments from the resultant GCode
	# def COMMENT(self,comment): return('')

iso_codes.codes = CodesEMC2()


class CreatorEMC2(iso.CreatorIso):
	def init(self): 
		iso.CreatorIso.init(self) 

	def program_begin(self, id, comment):
		self.write( ('(' + comment + ')' + '\n') )

 ############################################################################
    ##  Settings
    
	def imperial(self):
		self.write_blocknum()
		self.write( iso_codes.codes.IMPERIAL() + '\t (Imperial Values)\n')
		self.fmt = iso_codes.codes.FORMAT_IN()

	def metric(self):
		self.write_blocknum()
		self.write( iso_codes.codes.METRIC() + '\t (Metric Values)\n' )
		self.fmt = iso_codes.codes.FORMAT_MM()

	def absolute(self):
		self.write_blocknum()
		self.write( iso_codes.codes.ABSOLUTE() + '\t (Absolute Coordinates)\n')

	def incremental(self):
		self.write_blocknum()
		self.write( iso_codes.INCREMENTAL() + '\t (Incremental Coordinates)\n' )

	def polar(self, on=True):
		if (on) :
			self.write_blocknum()
			self.write(iso_codes.codes.POLAR_ON() + '\t (Polar ON)\n' )
		else : 
			self.write_blocknum()
			self.write(iso_codes.codes.POLAR_OFF() + '\t (Polar OFF)\n' )

	def set_plane(self, plane):
		if (plane == 0) : 
			self.write_blocknum()
			self.write(iso_codes.codes.PLANE_XY() + '\t (Select XY Plane)\n')
		elif (plane == 1) :
			self.write_blocknum()
			self.write(iso_codes.codes.PLANE_XZ() + '\t (Select XZ Plane)\n')
		elif (plane == 2) : 
			self.write_blocknum()
			self.write(iso_codes.codes.PLANE_YZ() + '\t (Select YZ Plane)\n')

	def comment(self, text):
		self.write_blocknum()
		self.write((iso_codes.codes.COMMENT(text) + '\n'))

	# This is the coordinate system we're using.  G54->G59, G59.1, G59.2, G59.3
	# These are selected by values from 1 to 9 inclusive.
	def workplane(self, id):
		if ((id >= 1) and (id <= 6)):
			self.write_blocknum()
			self.write( (iso_codes.codes.WORKPLANE() % (id + iso_codes.codes.WORKPLANE_BASE())) + '\t (Select Relative Coordinate System)\n')
		if ((id >= 7) and (id <= 9)):
			self.write_blocknum()
			self.write( ((iso_codes.codes.WORKPLANE() % (6 + iso_codes.codes.WORKPLANE_BASE())) + ('.%i' % (id - 6))) + '\t (Select Relative Coordinate System)\n')


nc.creator = CreatorEMC2()

