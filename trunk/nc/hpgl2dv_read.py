import hpgl2d_read as hpgl
import sys

# same as hpgl2d, but with 0.25mm units, instead of 0.01mm

class Parser(hpgl.Parser):
    def __init__(self):
        hpgl.ParserHgpl2d.__init__(self)
        self.units_to_mm = 0.25
