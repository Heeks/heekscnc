import nc_read as nc
import sys

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

    def get_number(self, line, length, n):
        number = ''

        # skip spaces at start of number
        while(n < length):
            c = line[n]
            if c == ' ':
                number += c
            else:
                break
            n = n + 1

        while(n < length):
            c = line[n]
            if c == '0' or c == '1' or c == '2' or c == '3' or c == '4' or c == '5' or c == '6' or c == '7' or c == '8' or c == '9' or c == '-':
                number += c
            else:
                break
            n = n + 1

        found = False
        x = 0
        if len(number) > 0:
            x = int(number)
            
        return found, x, n, number
    
    def Parse(self, name, oname=None):
        self.files_open(name,oname)

        while self.readline():
            self.begin_ncblock()

            word = ""
            length = len(self.line)

            n = 0
            while n < length:
                c = self.line[n]
                word += c

                if c == 'P':
                    n = n + 1
                    if n < length:
                        c1 = self.line[n]
                        word += c1
                        if c1 == 'U': # PU
                            old_x = self.x
                            old_y = self.y
                            n = n + 1
                            found, self.x, n, number = self.get_number(self.line, length, n)
                            word += number
                            found, self.y, n, number = self.get_number(self.line, length, n)
                            word += number
                            self.add_text(word, "rapid")
                            word = ""
                            self.begin_path("rapid")
                            if self.up == False:
                                self.add_line(old_x, old_y, self.up_z)
                            self.add_line(self.x, self.y, self.up_z)
                            self.end_path()
                            self.up = True
                        if c1 == 'D': # PD
                            old_x = self.x
                            old_y = self.y
                            n = n + 1
                            found, self.x, n, number = self.get_number(self.line, length, n)
                            word += number
                            found, self.y, n, number = self.get_number(self.line, length, n)
                            word += number
                            self.add_text(word, "feed")
                            word = ""
                            self.begin_path("feed")
                            if self.up == True:
                                self.add_line(old_x, old_y, self.down_z)
                            self.add_line(self.x, self.y, self.down_z)
                            self.end_path()
                            self.up = False
                n = n + 1
 
            self.add_text(word, None)

            self.end_ncblock()

        self.files_close()

################################################################################

if __name__ == '__main__':
    parser = ParserHgpl2d()
    if len(sys.argv)>2:
        parser.Parse(sys.argv[1],sys.argv[2])
    else:
        parser.Parse(sys.argv[1])
