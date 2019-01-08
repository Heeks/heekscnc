################################################################################
# heiden_read.py
#
# Simple ISO NC code parsing
#

import nc_read as nc
import sys
import math

################################################################################
class Parser(nc.Parser):

    def __init__(self, writer):
        nc.Parser.__init__(self, writer)
        self.arc_centre_absolute = True
        self.circle_i = None
        self.circle_j = None
        
        
    def lineToWords(self):
        # default implementation uses regular expressions, which are a bit hard to understand
        self.tool_call = False
        self.circle_centre = False
        self.circle = False
        self.d_found = False
        words = self.pattern_main.findall(self.line)
        new_words = []
        alpha_word = None
        for word in words:
            if len(word) == 1 and word[0].isalpha():
                if alpha_word:
                    alpha_word += word
                else:
                    alpha_word = word
            else:
                if alpha_word:
                    new_words.append(alpha_word)
                    alpha_word = None
                new_words.append(word)

        if alpha_word:
            new_words.append(alpha_word)
            alpha_word = None
            
        words = []
        alpha_words = []
        for word in new_words:
            if word.isalpha() or word == ' ':
                alpha_words.append(word)
            else:
                add_alpha_words_as_one(words, alpha_words)
                alpha_words = []
                words.append(word)

        add_alpha_words_as_one(words, alpha_words)

        return words
       
    def ParseWord(self, word):
        word = word.upper()
        
        if (word[0] == 'F'):
            self.col = "axis"
            if word == 'FMAX':
                self.rapid = True
                self.path_col = "rapid"
                self.col = "rapid"
            else:
                self.writer.feedrate(word[1:])
                self.rapid = False
                self.path_col = "feed"
                self.col = "feed"
        elif (word == 'L'):
            self.arc = 0
            self.move = True
        elif (word == 'C'):
            self.arc = 1
            self.circle = True
            self.i = self.circle_i
            self.j = self.circle_j
            self.move = True
        elif (word[0] == 'S'):
            self.col = "axis"
            self.writer.spindle(word[1:], (float(word[1:]) >= 0.0))
        elif (word == 'TOOL CALL'):
            self.col = "tool"
            self.tool_call = True
        elif (word == 'CC'):
            self.circle_centre = True
            self.col = "axis"
        elif (word == 'D'):
            self.d_found = True
        elif (word[0] == 'R'):
            if self.d_found:
                if word[1] == '-':
                    self.arc = -1
                else:
                    self.arc = 1
        elif (word == 'TOOL CALL'):
            self.col = "tool"
            self.tool_call = True
        elif (word[0] == 'X'):
            self.col = "axis"
            if self.circle_centre:
                self.circle_i = eval(word[1:])
            else:
                self.x = eval(word[1:])
            self.move = True
        elif (word[0] == 'Y'):
            self.col = "axis"
            if self.circle_centre:
                self.circle_j = eval(word[1:])
            else:
                self.y = eval(word[1:])
            self.move = True
        elif (word[0] == 'Z'):
            self.col = "axis"
            self.z = eval(word[1:])
            self.move = True
        elif (word[0] == '(') : (self.col, self.cdata) = ("comment", True)
        elif (word[0] == '!') : (self.col, self.cdata) = ("comment", True)
        elif (word[0] == ';') : (self.col, self.cdata) = ("comment", True)
        elif (word[0] == '#') : self.col = "variable"
        elif (word[0] == ':') : self.col = "blocknum"
        elif (ord(word[0]) >= 48) and (ord(word[0]) <= 57):
            if self.tool_call:
                self.writer.tool_change( eval(word) )
                self.tool_call = False
                self.col = "tool"                
        elif (ord(word[0]) <= 32) :
            self.cdata = True



def add_alpha_words_as_one(words, alpha_words):
    if len(alpha_words) > 0:
        # add start spaces
        while (len(alpha_words) > 0) and (alpha_words[0] == ' '):
            words.append(' ')
            alpha_words.pop(0)

        # collect and remove spaces from end, to add later
        spaces = []
        while (len(alpha_words) > 0) and (alpha_words[-1] == ' '):
            spaces.append(' ')
            alpha_words.pop()

        # add remaingin words as one word
        if len(alpha_words) > 0:
            one_word = ''
            for alpha_word in alpha_words:
                one_word += alpha_word
            words.append(one_word)
            
        # add spaces individually at the end
        for space in spaces:
            words.append(space)
        
