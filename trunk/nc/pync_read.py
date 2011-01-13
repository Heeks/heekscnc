################################################################################
# pync_read.py
#
# Base class for NC code parsing
#
# Dan Heeks 2011

################################################################################

import HeeksCNC

class Parser:

    def __init__(self):
        self.currentx = 0.0
        self.currenty = 0.0
        self.currentz = 0.0
        self.absolute_flag = True

    ############################################################################
    ##  Internals

    def files_open(self, name, oname):
        self.file_in = open(name, 'r')

    def files_close(self):
        self.file_in.close()

    def readline(self):
        self.line = self.file_in.readline().rstrip()
        if (len(self.line)) : return True
        else : return False

    ############################################################################
    ##  send to cad

    def begin_ncblock(self):
        self.block = ""

    def end_ncblock(self):
        HeeksCNC.program.nccode.blocks.append(self.block)

    def add_text(self, s, col=None, cdata=False):
        self.block += s

    def set_mode(self, units=None):
        pass # to do

    def set_tool(self, number=None):
        pass # to do

    def begin_path(self, col=None):
        pass # to do

    def end_path(self):
        pass # to do

    def add_line(self, x=None, y=None, z=None, a=None, b=None, c=None):
        pass # to do

    def add_arc(self, x=None, y=None, z=None, i=None, j=None, k=None, r=None, d=None):
        pass # to do
        
    def incremental(self):
        pass # to do
        
    def absolute(self):
        pass # to do
