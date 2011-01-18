################################################################################
# emc2b_crc.py
#
# a class derived from emc2b machine, with Cutter Radius Compensation turned on.
#
# Dan Heeks, 18th Jan 2011

import nc
import emc2b
import math

################################################################################
class CreatorEmc2bCrc(emc2b.CreatorEMC2):

    def __init__(self):
        emc2b.CreatorEMC2.__init__(self)
        self.useCrc = True

################################################################################

nc.creator = CreatorEmc2bCrc()
