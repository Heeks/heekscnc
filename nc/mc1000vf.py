import nc
import iso_modal
import math
import datetime
import time

now = datetime.datetime.now()

class Creator(iso_modal.Creator):
    def __init__(self):
        iso_modal.Creator.__init__(self)
        #self.output_block_numbers = False
        self.output_tool_definitions = False
        self.output_h_and_d_at_tool_change=True

    def SPACE(self):
        return ''
    def TOOL(self): return('G53G0Z0.0\nT%iM6')

    def PROGRAM_END(self): return( 'G53G0Z0.0\nG53G0X-600.0Y0.0\nM30')
    def ABSOLUTE(self): return('G90\n')
    
    def set_plane(self, plane):
        return
    
    def tool_change(self, id):
        if self.output_comment_before_tool_change:
            self.comment('tool change to ' + self.tool_defn_params[id]['name']);
            
        if self.output_cutviewer_comments:
            import cutviewer
            if id in self.tool_defn_params:
                cutviewer.tool_defn(self, id, self.tool_defn_params[id])
        if (self.t != None) and (self.z_for_g53 != None):
            self.write('G53 Z' + str(self.z_for_g53) + '\n')
        self.write(self.SPACE() + (self.TOOL() % id))
        if self.output_g43_on_tool_change_line == True:
            self.write(self.SPACE() + 'G43')
        self.write('\n')
        if self.output_h_and_d_at_tool_change == True:
            if self.output_g43_on_tool_change_line == False:
                self.write(self.SPACE() + 'G43')
            self.write(self.SPACE() + 'D' + str(id) + self.SPACE() + 'H' + str(id) + '\n')
        self.write('G54\n')
    
        self.t = id
        self.move_done_since_tool_change = False
    
    def metric(self):
        self.g_list.append(self.METRIC())
        self.fmt.number_of_decimal_places = 3
        self.fmt.add_trailing_zeros = True



nc.creator = Creator()

