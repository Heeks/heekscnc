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

    def tool_defn(self, id, name='', radius=None, length=None):
        pass
            
################################################################################

nc.creator = CreatorSieg()
