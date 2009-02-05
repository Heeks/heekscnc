################################################################################
# nc_read.py
#
# Base class for NC code parsing
#
# Hirutso Enni, 2009-01-13

################################################################################
class Parser:

    def __init__(self):
        pass

    ############################################################################
    ##  Internals

    def files_open(self, name):
        self.file_in = open(name, 'r')
        self.file_out = open(name+'.nc.xml', 'w')

        self.file_out.write('<?xml version="1.0" ?>\n')
        self.file_out.write('<nccode>\n')

    def files_close(self):
        self.file_out.write('</nccode>\n')

        self.file_in.close()
        self.file_out.close()

    def readline(self):
        self.line = self.file_in.readline().rstrip()
        if (len(self.line)) : return True
        else : return False

    def write(self, s):
        self.file_out.write(s)

    ############################################################################
    ##  Xml

    def begin_ncblock(self):
        self.file_out.write('\t<ncblock>\n')

    def end_ncblock(self):
        self.file_out.write('\t</ncblock>\n')

    def add_text(self, s, col=None):
        s.replace('&', '&amp;')
        s.replace('"', '&quot;')
        s.replace('<', '&lt;')
        s.replace('>', '&gt;')
        if (col != None) : self.file_out.write('\t\t<text col="'+col+'">'+s+'</text>\n')
        else : self.file_out.write('\t\t<text>'+s+'</text>\n')

    def begin_path(self, col=None):
        if (col != None) : self.file_out.write('\t\t<path col="'+col+'">\n')
        else : self.file_out.write('\t\t<path>\n')

    def end_path(self):
        self.file_out.write('\t\t</path>\n')

    def add_line(self, x=None, y=None, z=None):
        if (x == None and y == None and z == None) : return
        self.file_out.write('\t\t\t<line')
        if (x != None) : self.file_out.write(' x="%.6f"' % x)
        if (y != None) : self.file_out.write(' y="%.6f"' % y)
        if (z != None) : self.file_out.write(' z="%.6f"' % z)
        self.file_out.write(' />\n')

    def add_arc(self, x=None, y=None, z=None, i=None, j=None, k=None, d=None):
        if (x == None and y == None and z == None and i == None and j == None and k == None) : return
        self.file_out.write('\t\t\t<arc')
        if (x != None) : self.file_out.write(' x="%.6f"' % x)
        if (y != None) : self.file_out.write(' y="%.6f"' % y)
        if (z != None) : self.file_out.write(' z="%.6f"' % z)
        if (i != None) : self.file_out.write(' i="%.6f"' % i)
        if (j != None) : self.file_out.write(' j="%.6f"' % j)
        if (k != None) : self.file_out.write(' k="%.6f"' % k)
        if (d != None) : self.file_out.write(' d="%i"' % d)
        self.file_out.write(' />\n')
