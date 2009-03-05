################################################################################
# iso.py
#
# Simple ISO NC code creator
#
# Dan Heeks, 5th March 2009

import nc
import iso
import math

################################################################################
class CreatorSieg(iso.CreatorIso):

    def __init__(self):
        iso.CreatorIso.__init__(self)

    def write_spindle(self):
        if self.s != '':
            self.write(self.s)
            self.s = ''
            self.write('M3')
################################################################################

nc.creator = CreatorSieg()
