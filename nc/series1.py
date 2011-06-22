import nc
import iso_modal
import math

################################################################################
class Creator(iso_modal.Creator):

    def __init__(self):
        iso_modal.Creator.__init__(self)
        self.arc_centre_positive = True
        self.drillExpanded = True

    def tool_defn(self, id, name='', radius=None, length=None, gradient=None):
        pass
            
################################################################################

nc.creator = Creator()
