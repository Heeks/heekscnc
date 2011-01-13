# Dan Heeks 2011

# don't work on this file, it is just a copy of the original with "py" at the front
# this file will be deleted when we change over to python HeeksCNC

import pyiso_read as iso
import sys

# just use the iso reader

class ParserEMC2(iso.ParserIso):
    def __init__(self):
        iso.ParserIso.__init__(self)

if __name__ == '__main__':
    parser = ParserEMC2()
    if len(sys.argv)>2:
        parser.Parse(sys.argv[1],sys.argv[2])
    else:
        parser.Parse(sys.argv[1])
