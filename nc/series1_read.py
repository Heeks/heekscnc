import iso_read as iso
import sys

# use the iso reader, but with i_and_j_always_positive

class Parser(iso.Parser):
    def __init__(self):
        iso.Parser.__init__(self)
        self.arc_centre_positive = True
