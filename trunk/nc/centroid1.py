################################################################################
# centroid1.py
#
# Post Processor for the centroid M40 machine
# 
#
# Dan Falck, 7th March 2010

import nc
import iso_modal
import math
import iso_codes as iso

import datetime

now = datetime.datetime.now()



################################################################################
class CreatorCentroid1(iso_modal.CreatorIsoModal):

    def __init__(self):
        iso_modal.CreatorIsoModal.__init__(self)
################################################################################
# general 

    def comment(self, text):
        self.write(';' + text +'\n')  
    def write_blocknum(self):
        pass 

################################################################################
# program begin and end

    def program_begin(self, id, name=''):
        self.write(';time:'+str(now)+'\n')
        #self.write('G17 G80 G40 G90\n')
        

    def program_end(self):
        self.write('M05\n')
        self.write('G49 M25\n')
        self.write('G00 X-1.0 Y1.0\n')
        self.write('G17 G80 G40 G90\n')
        self.write('M99\n')

    def program_stop(self, optional=False):
        self.write_blocknum()
        if (optional) : 
            self.write(iso.codes.STOP_OPTIONAL() + '\n')
        else : 
            self.write('M05\n')
            self.write('G49 M25\n')
            self.write(iso.codes.STOP() + '\n')
            self.prev_g0123 = ''
################################################################################
# coordinate system ie G54-G59

    def workplane(self, id):
        self.write('M25\n')
        if ((id >= 1) and (id <= 6)):
            self.g += iso.codes.WORKPLANE() % (id + iso.codes.WORKPLANE_BASE())
        if ((id >= 7) and (id <= 9)):
            self.g += ((iso.codes.WORKPLANE() % (6 + iso.codes.WORKPLANE_BASE())) + ('.%i' % (id - 6)))
        self.prev_g0123 = ''            


                     
################################################################################
# tool info
    def tool_change(self, id):
        self.write_blocknum()
        self.write((iso.codes.TOOL() % id) + '\n')
        self.t = id
        self.write('G49 M25\n')
        self.write('G43 H'+ str(id) + 'Z.75 \n')


    def tool_defn(self, id, name='', radius=None, length=None, gradient=None):
        #self.write('G43 \n')
        pass

    def write_spindle(self):
        pass


    def spindle(self, s, clockwise):
        if s < 0: 
            clockwise = not clockwise
            s = abs(s)
        
        self.s = iso.codes.SPINDLE(iso.codes.FORMAT_ANG(), s)
        if clockwise:
           #self.s =  iso.codes.SPINDLE_CW() + self.s
            self.s =  iso.codes.SPINDLE_CW()                
            self.write(self.s +  '\n')
            self.write('G04 P2.0 \n')
                
        else:
            self.s =  iso.codes.SPINDLE_CCW() + self.s







nc.creator = CreatorCentroid1()
