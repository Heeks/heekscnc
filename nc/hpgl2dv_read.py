import hpgl2d_read as hpgl
import sys

# same as hpgl2d, but with 0.25mm units, instead of 0.01mm

class ParserHgpl2dv(hpgl.ParserHgpl2d):
    def __init__(self):
        hpgl.ParserHgpl2d.__init__(self)
        self.units_to_mm = 0.25

if __name__ == '__main__':
    parser = ParserHgpl2dv()
    if len(sys.argv)>2:
        parser.Parse(sys.argv[1],sys.argv[2])
    else:
        parser.Parse(sys.argv[1])

        
