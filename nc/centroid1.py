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


        self.absolute_flag = True


################################################################################
# general 

    def comment(self, text):
        self.write(';' + text +'\n')  
    def write_blocknum(self):
        pass 

################################################################################
# settings for absolute or incremental mode
    def absolute(self):
        self.write(iso.codes.ABSOLUTE()+'\n')        
        self.absolute_flag = True

    def incremental(self):
        self.write(iso.codes.INCREMENTAL()+'\n')
        self.absolute_flag = False

################################################################################
# APT style INSERT- insert anything into program

    def insert(self, text):
        self.write((text + '\n'))

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
# return to home


    def rapid_home(self, x=None, y=None, z=None, a=None, b=None, c=None):
        """Rapid relative to home position"""
        self.write('M05\n')              
        self.write('M25\n')
	self.write(iso.codes.RAPID())
        self.write(iso.codes.X() + (self.fmt % x))
        self.write(iso.codes.Y() + (self.fmt % y))
        self.write('\n')                     
                     
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

############################################################################
##  Moves

    def rapid(self, x=None, y=None, z=None, a=None, b=None, c=None, machine_coordinates=False ):
        self.write_blocknum()
        if (machine_coordinates != False):
            self.write(iso.codes.MACHINE_COORDINATES())
            self.prev_g0123 != iso.codes.RAPID()
        if self.g0123_modal:
            if self.prev_g0123 != iso.codes.RAPID():
                self.write(iso.codes.RAPID())
                self.prev_g0123 = iso.codes.RAPID()
        else:
            self.write(iso.codes.RAPID())
        self.write_preps()
        if (x != None):
            dx = x - self.x
            if (self.absolute_flag ):
                self.write(iso.codes.X() + (self.fmt % x))
            else:
                self.write(iso.codes.X() + (self.fmt % dx))
            self.x = x
        if (y != None):
            dy = y - self.y
            if (self.absolute_flag ):
                self.write(iso.codes.Y() + (self.fmt % y))
            else:
                self.write(iso.codes.Y() + (self.fmt % dy))

            self.y = y
        if (z != None):
            dz = z - self.z
            if (self.absolute_flag ):
                self.write(iso.codes.Z() + (self.fmt % z))
            else:
                self.write(iso.codes.Z() + (self.fmt % dz))

            self.z = z
        if (a != None) : self.write(iso.codes.A() + (iso.codes.FORMAT_ANG() % a))
        if (b != None) : self.write(iso.codes.B() + (iso.codes.FORMAT_ANG() % b))
        if (c != None) : self.write(iso.codes.C() + (iso.codes.FORMAT_ANG() % c))
        self.write_spindle()
        self.write_misc()
        self.write('\n')

    def feed(self, x=None, y=None, z=None, machine_coordinates=False):
        if self.same_xyz(x, y, z): return
        self.write_blocknum()
        if (machine_coordinates != False):
            self.write(iso.codes.MACHINE_COORDINATES())
            self.prev_g0123 = ''
        if self.g0123_modal:
            if self.prev_g0123 != iso.codes.FEED():
                self.write(iso.codes.FEED())
                self.prev_g0123 = iso.codes.FEED()
        else:
            self.write(iso.codes.FEED())
        self.write_preps()
        dx = dy = dz = 0
        if (x != None):
            dx = x - self.x
            if (self.absolute_flag ):
                self.write(iso.codes.X() + (self.fmt % x))
            else:
                self.write(iso.codes.X() + (self.fmt % dx))
            self.x = x
        if (y != None):
            dy = y - self.y
            if (self.absolute_flag ):
                self.write(iso.codes.Y() + (self.fmt % y))
            else:
                self.write(iso.codes.Y() + (self.fmt % dy))
            self.y = y
        if (z != None):
            dz = z - self.z
            if (self.absolute_flag ):
                self.write(iso.codes.Z() + (self.fmt % z))
            else:
                self.write(iso.codes.Z() + (self.fmt % dz))
            self.z = z
        if (self.fhv) : self.calc_feedrate_hv(math.sqrt(dx*dx+dy*dy), math.fabs(dz))
        self.write_feedrate()
        self.write_spindle()
        self.write_misc()
        self.write('\n')

    def same_xyz(self, x=None, y=None, z=None):
        if (x != None):
            if (self.fmt % x) != (self.fmt % self.x):
                return False
        if (y != None):
            if (self.fmt % y) != (self.fmt % self.y):
                return False
        if (z != None):
            if (self.fmt % z) != (self.fmt % self.z):
                return False
            
        return True

    def arc(self, cw, x=None, y=None, z=None, i=None, j=None, k=None, r=None):
        if self.same_xyz(x, y, z): return
        self.write_blocknum()
        arc_g_code = ''
        if cw: arc_g_code = iso.codes.ARC_CW()
        else: arc_g_code = iso.codes.ARC_CCW()
        if self.g0123_modal:
            if self.prev_g0123 != arc_g_code:
                self.write(arc_g_code)
                self.prev_g0123 = arc_g_code
        else:
            self.write(arc_g_code)
        self.write_preps()
        if (x != None):
            dx = x - self.x
            if (self.absolute_flag ):
                self.write(iso.codes.X() + (self.fmt % x))
            else:
                self.write(iso.codes.X() + (self.fmt % dx))
            self.x = x
        if (y != None):
            dy = y - self.y
            if (self.absolute_flag ):
                self.write(iso.codes.Y() + (self.fmt % y))
            else:
                self.write(iso.codes.Y() + (self.fmt % dy))
            self.y = y
        if (z != None):
            dz = z - self.z
            if (self.absolute_flag ):
                self.write(iso.codes.Z() + (self.fmt % z))
            else:
                self.write(iso.codes.Z() + (self.fmt % dz))
            self.z = z
        if (i != None) : self.write(iso.codes.CENTRE_X() + (self.fmt % i))
        if (j != None) : self.write(iso.codes.CENTRE_Y() + (self.fmt % j))
        if (k != None) : self.write(iso.codes.CENTRE_Z() + (self.fmt % k))
        if (r != None) : self.write(iso.codes.RADIUS() + (self.fmt % r))
#       use horizontal feed rate
        if (self.fhv) : self.calc_feedrate_hv(1, 0)
        self.write_feedrate()
        self.write_spindle()
        self.write_misc()
        self.write('\n')

    def arc_cw(self, x=None, y=None, z=None, i=None, j=None, k=None, r=None):
        self.arc(True, x, y, z, i, j, k, r)

    def arc_ccw(self, x=None, y=None, z=None, i=None, j=None, k=None, r=None):
        self.arc(False, x, y, z, i, j, k, r)








nc.creator = CreatorCentroid1()
