
# heiden.py, just copied from iso.py, to start with, but needs to be modified to make this sort of output

#1 BEGIN PGM 0011 MM
#2 BLK FORM 0.1 Z X-262.532 Y-262.55 Z-75.95
#3 BLK FORM 0.2 X262.532 Y262.55 Z0.05
#4 TOOL CALL 3 Z S3263 DL+0.0 DR+0.0
#5 TOOL CALL 3 Z S3263 DL+0.0 DR+0.0
#6 L X-80.644 Y-95.2 Z+100.0 R0 F237 M3
#7 L Z-23.222 F333
#8 L X-80.627 Y-95.208 Z-23.5 F326
#49 L X-73.218 Y-88.104 Z-26.747 F229
#50 L X-73.529 Y-87.795 Z-26.769 F227
#51 L X-74.09 Y-87.326 Z-25.996 F279
#52 M30
#53 END PGM 0011 MM

import nc
import iso
import math
import datetime
import time
from format import *

now = datetime.datetime.now()

class Creator(iso.Creator):
    def __init__(self):
        iso.Creator.__init__(self)
        self.output_tool_definitions = False
        self.drillExpanded = True # to do: implement drill cycle, but for now just do linear moves
        self.output_block_numbers = True
        self.start_block_number = 1
        self.block_number_increment = 1
        self.output_spindle_speed_on_tool_change_line = True
        self.s = Address('S', fmt = Format(number_of_decimal_places = 2))
        self.spindle_dir_for_next_move = None
        self.output_arcs_as_lines = False
        
    def BLOCK(self): return('%i')
    def SPACE_STR(self): return ' '

    # ignore these ISO outputs
    def imperial(self): pass
    def metric(self): pass
    def absolute(self): pass
    def incremental(self): pass
    def polar(self, on=True): pass
    def set_plane(self, plane): pass

    def PROGRAM_END(self): return( 'M30\nEND PGM' + self.SPACE() + self.id_string + self.SPACE() + 'MM')
 
    def program_begin(self, id, name=''):
        if self.use_this_program_id:
            id = self.use_this_program_id
            
        self.id_string = Format(add_leading_zeros = 4).string(id)
        self.write('BEGIN PGM' + self.SPACE() + self.id_string + self.SPACE() + 'MM')
        self.write('\n')
        
        self.program_id = id
        self.program_name = name
        
    def RAPID(self): return('L')
    def FEED(self): return('L')
    
    # capture rapid call
    def rapid(self, x=None, y=None, z=None, a=None, b=None, c=None ):
        if self.same_xyz(x, y, z, a, b, c): return
        iso.Creator.rapid(self, x, y, z, a, b, c, False)
        self.write(' FMAX\n')
    
    def tool_defn(self, id, name='',params=None):
        self.tool_defn_params[id] = params
        if self.output_tool_definitions:
            self.write('TOOL DEF' + self.SPACE() + str(id) + ' L+0\n')
            
    def tool_change(self, id):
        # wait for spindle call
        self.saved_tool_id = id
        
    def on_move(self):
        iso.Creator.on_move(self)
        if self.spindle_dir_for_next_move:
            self.m.append(self.spindle_dir_for_next_move)
            self.spindle_dir_for_next_move = None

    def spindle(self, s, clockwise):
        self.spindle_dir_for_next_move = self.SPINDLE_CW() if clockwise else self.SPINDLE_CCW()
        self.s.set(s)
        iso.Creator.tool_change(self, self.saved_tool_id, False)
        self.write_spindle()
        self.write('\n')
            
    def TOOL(self): return('TOOL CALL %i')
    
    def comment(self, text):
        pass
    
    def heidenhain_arc(self, x, y, i, j, cw):
        if self.same_xyz(x, y, self.z): return
        
        if (self.fmt.string(i) == self.fmt.string(self.x) and self.fmt.string(j) == self.fmt.string(self.y)) or (self.fmt.string(i) == self.fmt.string(x if x != None else self.x) and self.fmt.string(j) == self.fmt.string(y if y != None else self.y)):
            # if arc has zero radius, output an line instead
            self.feed(x, y)
            return
 
        if (x != None):
            self.x = x
        if (y != None):
            self.y = y

        # write the centre point
        self.write('CC X' + self.fmt.string(i) + ' Y' + self.fmt.string(j) + '\n')
        
        # write the circle
        self.write('C X' + self.fmt.string(self.x) + ' Y' + self.fmt.string(self.y) + (' DR-' if cw else ' DR+') + '\n')

    def arc_cw(self, x=None, y=None, z=None, i=None, j=None, k=None, r=None):
        if self.output_arcs_as_lines:
            iso.Creator.arc_cw(self, x, y, z, i, j, k, r)
        else:
            self.heidenhain_arc(x, y, i, j, True)

    def arc_ccw(self, x=None, y=None, z=None, i=None, j=None, k=None, r=None):
        if self.output_arcs_as_lines:
            iso.Creator.arc_ccw(self, x, y, z, i, j, k, r)
        else:
            self.heidenhain_arc(x, y, i, j, False)


nc.creator = Creator()

