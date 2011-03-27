import iso_read as iso
import sys

# based on the iso reader

class Parser(iso.Parser):
    def __init__(self):
        iso.Parser.__init__(self)

    def ParseWord(self, word):
        iso.Parser.ParseWord(self, word)
        if (word == 'M103'):
            self.path_col = "rapid"
            self.col = "rapid"
        elif (word == 'M101'):
            self.path_col = "feed"
            self.col = "feed"
