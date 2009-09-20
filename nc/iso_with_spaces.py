import nc
import iso
import iso_codes

# Define a class that overloads the SPACE() method of the iso_codes.SPACE() method.
class CodesWithSpaces(iso_codes.Codes):
	def SPACE(self): return(' ')

iso_codes.codes = CodesWithSpaces()

