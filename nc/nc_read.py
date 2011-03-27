################################################################################
# nc_read.py
#
# Base class for NC code parsing
#
# Hirutso Enni, 2009-01-13

################################################################################
class Parser:

    def __init__(self):
        self.writer = None
        self.currentx = None
        self.currenty = None
        self.currentz = None
        self.absolute_flag = True

    ############################################################################
    ##  Internals

    def files_open(self, name):
        self.file_in = open(name, 'r')
        self.writer.files_open(name)
        
    def files_close(self):
        self.writer.files_close()
        self.file_in.close()

    def readline(self):
        self.line = self.file_in.readline().rstrip()
        if (len(self.line)) : return True
        else : return False
        
    def set_current_pos(self, x, y, z):
        if (x != None) :
            if self.absolute_flag or self.currentx == None: self.currentx = x
            else: self.currentx = self.currentx + x
        if (y != None) :
            if self.absolute_flag or self.currenty == None: self.currenty = y
            else: self.currenty = self.currenty + y
        if (z != None) :
            if self.absolute_flag or self.currentz == None: self.currentz = z
            else: self.currentz = self.currentz + z

    def add_line(self, x, y, z, a, b, c):
        if (x == None and y == None and z == None and a == None and b == None and c == None) : return
        set_current_pos(x, y, z)
        self.writer.add_line(self.currentx, self.currenty, self.currentz, a, b, c)
        
    def add_arc(self, x, y, z, i, j, k, r, d):
        if (x == None and y == None and z == None and i == None and j == None and k == None and r == None and d == None) : return
        set_current_pos(x, y, z)
        self.writer.add_arc(self.currentx, self.currenty, self.currentz, i, j, k, r, d)
        
    def incremental(self):
        self.absolute_flag = False
        
    def absolute(self):
        self.absolute_flag = True