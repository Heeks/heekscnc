import iso_read as iso
import sys

# just use the iso reader

class Parser(iso.ParserIso):
    def __init__(self):
        iso.ParserIso.__init__(self)

if __name__ == '__main__':
    parser = Parser()
    if len(sys.argv)>2:
        parser.Parse(sys.argv[1],sys.argv[2])
    else:
        parser.Parse(sys.argv[1])
