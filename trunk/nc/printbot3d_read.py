import iso_read as iso
import sys

# based on the iso reader

class ParserPrintBot(iso.ParserIso):
    def __init__(self):
        iso.ParserIso.__init__(self)

    def ParseWord(self, word):
        iso.ParserIso.ParseWord(self, word)
        if (word == 'M103'):
            self.path_col = "rapid"
            self.col = "rapid"
        elif (word == 'M101'):
            self.path_col = "feed"
            self.col = "feed"

if __name__ == '__main__':
    parser = ParserPrintBot()
    if len(sys.argv)>2:
        parser.Parse(sys.argv[1],sys.argv[2])
    else:
        parser.Parse(sys.argv[1])
