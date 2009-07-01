import nc_read as nc
import sys
import math

class ParserHgpl2d(nc.Parser):

    def __init__(self):
        nc.Parser.__init__(self)
        self.i = 0
        self.j = 0
        self.x = 0
        self.y = 0
        self.down_z = 0
        self.up_z = 20
        self.up = True
        self.units_to_mm = 0.01

    def get_number(self):
        number = ''

        # skip spaces and commas at start of number
        while(self.line_index < self.line_length):
            c = self.line[self.line_index]
            if c == ' ' or c == ',':
                self.parse_word += c
            else:
                break
            self.line_index = self.line_index + 1

        while(self.line_index < self.line_length):
            c = self.line[self.line_index]
            if c == '0' or c == '1' or c == '2' or c == '3' or c == '4' or c == '5' or c == '6' or c == '7' or c == '8' or c == '9' or c == '-':
                number += c
            else:
                break
            self.parse_word += c
            self.line_index = self.line_index + 1

        return number

    def add_word(self, color):
        self.add_text(self.parse_word, color)
        self.parse_word = ""

    def ParsePuOrPd(self, up):
        self.line_index = self.line_index + 1
        x = self.get_number()
        if len(x) > 0:
            y = self.get_number()
            if len(y) > 0:
                if up: color = "rapid"
                else: color = "feed"
                self.add_word(color)
                self.begin_path(color)
                if up: z = self.up_z
                else: z = self.down_z
                if self.up != up:
                    self.add_line(self.x * self.units_to_mm, self.y * self.units_to_mm, z)
                self.add_line(int(x) * self.units_to_mm, int(y) * self.units_to_mm, z)
                self.end_path()
                self.up = up
                self.x = int(x)
                self.y = int(y)
    
    def ParseAA(self):
        self.line_index = self.line_index + 1
        cx = self.get_number()
        if len(cx) > 0:
            cy = self.get_number()
            if len(cy) > 0:
                a = self.get_number()
                if len(a) > 0:
                    self.add_word("feed")
                    self.begin_path("feed")
                    z = self.down_z
                    if self.up:
                        self.add_line(self.x * self.units_to_mm, self.y * self.units_to_mm, z)

                    sdx = self.x - int(cx)
                    sdy = self.y - int(cy)

                    print "cx, cy", cx, cy

                    print "a", a

                    print "sdx, sdy", sdx, sdy
                    
                    start_angle = math.atan2(sdy, sdx)

                    print "start_angle", start_angle
                    end_angle = start_angle + int(a) * math.pi/180

                    print "end_angle", end_angle
                    radius = math.sqrt(sdx*sdx + sdy*sdy)

                    print "radius", radius
                    ex = int(cx) + radius * math.cos(end_angle)
                    ey = int(cy) + radius * math.sin(end_angle)

                    print "ex, ey", ex, ey
                    print
                    
                    if int(a) > 0: d = 1
                    else: d = -1

                    print "d", d
                    self.add_arc(ex * self.units_to_mm, ey * self.units_to_mm, i = int(-sdx) * self.units_to_mm, j = int(-sdy) * self.units_to_mm, d = d)
                    self.end_path()
                    self.up = False
                    self.x = int(ex)
                    self.y = int(ey)
    
    def Parse(self, name, oname=None):
        self.files_open(name,oname)

        while self.readline():
            self.begin_ncblock()

            self.parse_word = ""
            self.line_index = 0
            self.line_length = len(self.line)
                    
            while self.line_index < self.line_length:
                c = self.line[self.line_index]
                self.parse_word += c

                if c == 'P':
                    self.line_index = self.line_index + 1
                    if self.line_index < self.line_length:
                        c1 = self.line[self.line_index]
                        self.parse_word += c1
                        if c1 == 'U': # PU
                            self.ParsePuOrPd(True)
                        if c1 == 'D': # PD
                            self.ParsePuOrPd(False)
                elif c == 'A':
                    self.line_index = self.line_index + 1
                    if self.line_index < self.line_length:
                        c1 = self.line[self.line_index]
                        self.parse_word += c1
                        if c1 == 'A': # AA, arc absolute
                            self.ParseAA()
                    
                self.line_index = self.line_index + 1
 
            self.add_text(self.parse_word, None)

            self.end_ncblock()

        self.files_close()

################################################################################

if __name__ == '__main__':
    parser = ParserHgpl2d()
    if len(sys.argv)>2:
        parser.Parse(sys.argv[1],sys.argv[2])
    else:
        parser.Parse(sys.argv[1])
