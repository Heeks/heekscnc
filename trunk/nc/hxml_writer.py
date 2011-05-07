class HxmlWriter:
    def __init__(self):
        pass
    
    def on_parse_start(self, name):
        self.file_out = open(name+'.nc.xml', 'w')
        self.file_out.write('<?xml version="1.0" ?>\n')
        self.file_out.write('<nccode>\n')

    def on_parse_end(self):
        self.file_out.write('</nccode>\n')
        self.file_out.close()

    def write(self, s):
        self.file_out.write(s)

############################################

    def begin_ncblock(self):
        self.file_out.write('\t<ncblock>\n')

    def end_ncblock(self):
        self.file_out.write('\t</ncblock>\n')

    def add_text(self, s, col, cdata):
        s.replace('&', '&amp;')
        s.replace('"', '&quot;')
        s.replace('<', '&lt;')
        s.replace('>', '&gt;')
        if (cdata) : (cd1, cd2) = ('<![CDATA[', ']]>')
        else : (cd1, cd2) = ('', '')
        if (col != None) : self.file_out.write('\t\t<text col="'+col+'">'+cd1+s+cd2+'</text>\n')
        else : self.file_out.write('\t\t<text>'+cd1+s+cd2+'</text>\n')

    def set_mode(self, units):
        self.file_out.write('\t\t<mode')
        if (units != None) : self.file_out.write(' units="'+str(units)+'"')
        self.file_out.write(' />\n')

    def set_tool(self, number):
        self.file_out.write('\t\t<tool')
        if (number != None) : 
            self.file_out.write(' number="'+str(number)+'"')
            self.file_out.write(' />\n')

    def begin_path(self, col):
        if (col != None) : self.file_out.write('\t\t<path col="'+col+'">\n')
        else : self.file_out.write('\t\t<path>\n')

    def end_path(self):
        self.file_out.write('\t\t</path>\n')

    def add_line(self, x, y, z, a = None, b = None, c = None):
        self.file_out.write('\t\t\t<line')
        if (x != None) :
            self.file_out.write(' x="%.6f"' % x)
        if (y != None) :
            self.file_out.write(' y="%.6f"' % y)
        if (z != None) :
            self.file_out.write(' z="%.6f"' % z)
        if (a != None) : self.file_out.write(' a="%.6f"' % a)
        if (b != None) : self.file_out.write(' b="%.6f"' % b)
        if (c != None) : self.file_out.write(' c="%.6f"' % c)
        self.file_out.write(' />\n')
        
    def add_arc(self, x, y, z, i, j, k, r = None, d = None):
        self.file_out.write('\t\t\t<arc')
        if (x != None) :
            self.file_out.write(' x="%.6f"' % x)
        if (y != None) :
            self.file_out.write(' y="%.6f"' % y)
        if (z != None) :
            self.file_out.write(' z="%.6f"' % z)
        if (i != None) : self.file_out.write(' i="%.6f"' % i)
        if (j != None) : self.file_out.write(' j="%.6f"' % j)
        if (k != None) : self.file_out.write(' k="%.6f"' % k)
        if (r != None) : self.file_out.write(' r="%.6f"' % r)
        if (d != None) : self.file_out.write(' d="%i"' % d)
        self.file_out.write(' />\n')
