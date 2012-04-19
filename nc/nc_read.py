################################################################################
# nc_read.py
#
# Base class for NC code parsing

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
        self.writer.on_parse_start(name)

    def files_close(self):
        self.writer.on_parse_end()
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
        self.set_current_pos(x, y, z)
        self.writer.add_line(self.currentx, self.currenty, self.currentz, a, b, c)

    def add_arc(self, x, y, z, i, j, k, r, d):
        if (x == None and y == None and z == None and i == None and j == None and k == None and r == None and d == None) : return
        self.set_current_pos(x, y, z)
        self.writer.add_arc(self.currentx, self.currenty, self.currentz, i, j, k, r, d)

    def incremental(self):
        self.absolute_flag = False

    def absolute(self):
        self.absolute_flag = True
        
    def Parse(self, name):
        self.files_open(name)
        
        self.path_col = None
        self.f = None
        self.arc = 0

        while (self.readline()):
            
            self.a = None
            self.b = None
            self.c = None
            self.h = None
            self.i = None
            self.j = None
            self.k = None
            self.p = None
            self.q = None
            self.r = None
            self.s = None
            self.x = None
            self.y = None
            self.z = None

            self.writer.begin_ncblock()

            self.move = False
            self.height_offset = False
            self.drill = False
            self.no_move = False

            words = self.pattern_main.findall(self.line)
            for word in words:
                self.col = None
                self.cdata = False
                self.ParseWord(word)
                self.writer.add_text(word, self.col, self.cdata)


            if (self.drill):
                self.writer.begin_path("rapid")
                self.writer.add_line(self.x, self.y, self.r)
                self.writer.end_path()

                self.writer.begin_path("feed")
                self.writer.add_line(self.x, self.y, self.z)
                self.writer.end_path()

                self.writer.begin_path("feed")
                self.writer.add_line(self.x, self.y, self.r)
                self.writer.end_path()

            elif(self.height_offset):
                self.writer.begin_path("rapid")
                self.writer.add_line(self.x, self.y, self.z)
                self.writer.end_path()


            else:
                if (self.move and not self.no_move):
                    self.writer.begin_path(self.path_col)
                    if (self.arc==0): 
                        self.writer.add_line(self.x, self.y, self.z, self.a, self.b, self.c)
                    else:
                        i = self.i
                        j = self.j
                        k = self.k
                        if self.arc_centre_absolute == True:
                            i = i - self.oldx
                            j = j - self.oldy
                        else:
                            if (self.arc_centre_positive == True) and (self.oldx != None) and (self.oldy != None):
                                x = self.oldx
                                if self.x: x = self.x
                                if (self.x > self.oldx) != (self.arc > 0):
                                    j = -j
                                y = self.oldy
                                if self.y: y = self.y
                                if (self.y > self.oldy) != (self.arc < 0):
                                    i = -i
                        self.writer.add_arc(self.x, self.y, self.z, i, j, k, self.r, self.arc)
                    self.writer.end_path()
                    if self.x != None: self.oldx = self.x
                    if self.y != None: self.oldy = self.y
                    if self.z != None: self.oldz = self.z

            self.writer.end_ncblock()

        self.files_close()
        