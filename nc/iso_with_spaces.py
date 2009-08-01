import nc
import iso
import iso_codes

class CreatorIsoWithSpaces(iso.CreatorIso):
    def __init__(self): 
        iso.CreatorIso.__init__(self)
        iso_codes.SPACE = ' '

nc.creator = CreatorIsoWithSpaces()
