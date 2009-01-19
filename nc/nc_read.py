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
        self.file_out = open(name+'.xml', 'w')

        self.file_out.write('<?xml version="1.0" ?>\n')
        self.file_out.write('<HeeksCAD_Document>\n')
        self.file_out.write('\t<nccode>\n')

    def files_close(self):
        self.file_out.write('\t</nccode>\n')
        self.file_out.write('</HeeksCAD_Document>\n')

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
        self.file_out.write('\t\t<ncblock>\n')

    def end_ncblock(self):
        self.file_out.write('\t\t</ncblock>\n')

    def add_text(self, s, col=None):
        s.replace('&', '&amp;')
        s.replace('"', '&quot;')
        s.replace('<', '&lt;')
        s.replace('>', '&gt;')
        if (col != None) : self.file_out.write('\t\t\t<text col="'+col+'">'+s+'</text>\n')
        else : self.file_out.write('\t\t\t<text>'+s+'</text>\n')

    def begin_lines(self, col=None):
        if (col != None) : self.file_out.write('\t\t\t<lines col="'+col+'">\n')
        else : self.file_out.write('\t\t\t<lines>\n')

    def end_lines(self):
        self.file_out.write('\t\t\t</lines>\n')

    def add_point(self, x=None, y=None, z=None):
        if (x == None and y == None and z == None) : return
        self.file_out.write('\t\t\t\t<p')
        if (x != None) : self.file_out.write(' x="%.6f"' % x)
        if (y != None) : self.file_out.write(' y="%.6f"' % y)
        if (z != None) : self.file_out.write(' z="%.6f"' % z)
        self.file_out.write(' />\n')
