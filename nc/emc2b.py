import nc
import iso_modal
import iso_codes
import math
import datetime
import time

now = datetime.datetime.now()


class CodesEMC2(iso_codes.Codes):
    def SPACE(self): return(' ')

    # This version of COMMENT removes comments from the resultant GCode
    # def COMMENT(self,comment): return('')

iso_codes.codes = CodesEMC2()



class CreatorEMC2(iso_modal.CreatorIsoModal):
    def __init__(self):
        iso_modal.CreatorIsoModal.__init__(self)
        self.absolute_flag = True
        self.prev_g91 = ''



############################################################################
## Begin Program 


    def program_begin(self, id, comment):
        self.write( ('(Created with emc2b post processor ' + str(now.strftime("%Y/%m/%d %H:%M")) + ')' + '\n') )


############################################################################
##  Settings

    def tool_defn(self, id, name='', radius=None, length=None, gradient=None):
        #self.write('G43 \n')
        pass

    def comment(self, text):
        self.write_blocknum()
        self.write((iso_codes.codes.COMMENT(text) + '\n'))

    def write_blocknum(self):
        pass 


# This is the coordinate system we're using.  G54->G59, G59.1, G59.2, G59.3
# These are selected by values from 1 to 9 inclusive.
    def workplane(self, id):
        if ((id >= 1) and (id <= 6)):
            self.write_blocknum()
            self.write( (iso_codes.codes.WORKPLANE() % (id + iso_codes.codes.WORKPLANE_BASE())) + '\t (Select Relative Coordinate System)\n')
        if ((id >= 7) and (id <= 9)):
            self.write_blocknum()
            self.write( ((iso_codes.codes.WORKPLANE() % (6 + iso_codes.codes.WORKPLANE_BASE())) + ('.%i' % (id - 6))) + '\t (Select Relative Coordinate System)\n')

############################################################################
##  Spindle

    def spindle(self, s, clockwise):
        if s < 0: 
            clockwise = not clockwise
            s = abs(s)
        
        self.s = iso_codes.codes.SPINDLE(iso_codes.codes.FORMAT_ANG(), s)
        if clockwise:

            self.s =  '\n'+ iso_codes.codes.SPINDLE_CW() + self.s
            #self.s =  iso.codes.SPINDLE_CW()                
            #self.write(self.s +  '\n')
            #self.write('G04 P2.0 \n')
                
        else:
            self.s =  '\n'+ iso_codes.codes.SPINDLE_CCW() + self.s



############################################################################
##  Moves

    def rapid(self, x=None, y=None, z=None, a=None, b=None, c=None, machine_coordinates=False ):
        self.write_blocknum()
        if (machine_coordinates != False):
            self.write(iso_codes.codes.MACHINE_COORDINATES())
            self.prev_g0123 != iso_codes.codes.RAPID()
        if self.g0123_modal:
            if self.prev_g0123 != iso_codes.codes.RAPID():
                self.write(iso_codes.codes.RAPID())
                self.prev_g0123 = iso_codes.codes.RAPID()
        else:
            self.write(iso_codes.codes.RAPID())
        self.write_preps()
        if (x != None):
            dx = x - self.x
            if (self.absolute_flag ):
                self.write(iso_codes.codes.X() + (self.fmt % x))
            else:
                self.write(iso_codes.codes.X() + (self.fmt % dx))
            self.x = x
        if (y != None):
            dy = y - self.y
            if (self.absolute_flag ):
                self.write(iso_codes.codes.Y() + (self.fmt % y))
            else:
                self.write(iso_codes.codes.Y() + (self.fmt % dy))

            self.y = y
        if (z != None):
            dz = z - self.z
            if (self.absolute_flag ):
                self.write(iso_codes.codes.Z() + (self.fmt % z))
            else:
                self.write(iso_codes.codes.Z() + (self.fmt % dz))

            self.z = z

        if (a != None):
            da = a - self.a
            if (self.absolute_flag ):
                self.write(iso_codes.codes.A() + (self.fmt % a))
            else:
                self.write(iso_codes.codes.A() + (self.fmt % da))
            self.a = a

        if (b != None):
            db = b - self.b
            if (self.absolute_flag ):
                self.write(iso_codes.codes.B() + (self.fmt % b))
            else:
                self.write(iso_codes.codes.B() + (self.fmt % db))
            self.b = b

        if (c != None):
            dc = c - self.c
            if (self.absolute_flag ):
                self.write(iso_codes.codes.C() + (self.fmt % c))
            else:
                self.write(iso_codes.codes.C() + (self.fmt % dc))
            self.c = c

        self.write_spindle()
        self.write_misc()
        self.write('\n')

    def feed(self, x=None, y=None, z=None, machine_coordinates=False):
        if self.same_xyz(x, y, z): return
        self.write_blocknum()
        if (machine_coordinates != False):
            self.write(iso_codes.codes.MACHINE_COORDINATES())
            self.prev_g0123 = ''
        if self.g0123_modal:
            if self.prev_g0123 != iso_codes.codes.FEED():
                self.write(iso_codes.codes.FEED())
                self.prev_g0123 = iso_codes.codes.FEED()
        else:
            self.write(iso_codes.codes.FEED())
        self.write_preps()
        dx = dy = dz = 0
        if (x != None):
            dx = x - self.x
            if (self.absolute_flag ):
                self.write(iso_codes.codes.X() + (self.fmt % x))
            else:
                self.write(iso_codes.codes.X() + (self.fmt % dx))
            self.x = x
        if (y != None):
            dy = y - self.y
            if (self.absolute_flag ):
                self.write(iso_codes.codes.Y() + (self.fmt % y))
            else:
                self.write(iso_codes.codes.Y() + (self.fmt % dy))
            self.y = y
        if (z != None):
            dz = z - self.z
            if (self.absolute_flag ):
                self.write(iso_codes.codes.Z() + (self.fmt % z))
            else:
                self.write(iso_codes.codes.Z() + (self.fmt % dz))
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
        if cw: arc_g_code = iso_codes.codes.ARC_CW()
        else: arc_g_code = iso_codes.codes.ARC_CCW()
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
                self.write(iso_codes.codes.X() + (self.fmt % x))
            else:
                self.write(iso_codes.codes.X() + (self.fmt % dx))
            self.x = x
        if (y != None):
            dy = y - self.y
            if (self.absolute_flag ):
                self.write(iso_codes.codes.Y() + (self.fmt % y))
            else:
                self.write(iso_codes.codes.Y() + (self.fmt % dy))
            self.y = y
        if (z != None):
            dz = z - self.z
            if (self.absolute_flag ):
                self.write(iso_codes.codes.Z() + (self.fmt % z))
            else:
                self.write(iso_codes.codes.Z() + (self.fmt % dz))
            self.z = z
        if (i != None) : self.write(iso_codes.codes.CENTRE_X() + (self.fmt % i))
        if (j != None) : self.write(iso_codes.codes.CENTRE_Y() + (self.fmt % j))
        if (k != None) : self.write(iso_codes.codes.CENTRE_Z() + (self.fmt % k))
        if (r != None) : self.write(iso_codes.codes.RADIUS() + (self.fmt % r))
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



























############################################################################
## Probe routines
    def report_probe_results(self, x1=None, y1=None, z1=None, x2=None, y2=None, z2=None, x3=None, y3=None, z3=None, x4=None, y4=None, z4=None, x5=None, y5=None, z5=None, x6=None, y6=None, z6=None, xml_file_name=None ):
        if (xml_file_name != None):
            self.comment('Generate an XML document describing the probed coordinates found');
            self.write_blocknum()
            self.write('(LOGOPEN,')
            self.write(xml_file_name)
            self.write(')\n')

        self.write_blocknum()
        self.write('(LOG,<POINTS>)\n')

        if ((x1 != None) or (y1 != None) or (z1 != None)):
            self.write_blocknum()
            self.write('(LOG,<POINT>)\n')

        if (x1 != None):
            self.write_blocknum()
            self.write('#<_value>=[' + x1 + ']\n')
            self.write_blocknum()
            self.write('(LOG,<X>#<_value></X>)\n')

        if (y1 != None):
            self.write_blocknum()
            self.write('#<_value>=[' + y1 + ']\n')
            self.write_blocknum()
            self.write('(LOG,<Y>#<_value></Y>)\n')

        if (z1 != None):
            self.write_blocknum()
            self.write('#<_value>=[' + z1 + ']\n')
            self.write_blocknum()
            self.write('(LOG,<Z>#<_value></Z>)\n')

        if ((x1 != None) or (y1 != None) or (z1 != None)):
            self.write_blocknum()
            self.write('(LOG,</POINT>)\n')

        if ((x2 != None) or (y2 != None) or (z2 != None)):
            self.write_blocknum()
            self.write('(LOG,<POINT>)\n')

        if (x2 != None):
            self.write_blocknum()
            self.write('#<_value>=[' + x2 + ']\n')
            self.write_blocknum()
            self.write('(LOG,<X>#<_value></X>)\n')

        if (y2 != None):
            self.write_blocknum()
            self.write('#<_value>=[' + y2 + ']\n')
            self.write_blocknum()
            self.write('(LOG,<Y>#<_value></Y>)\n')

        if (z2 != None):
            self.write_blocknum()
            self.write('#<_value>=[' + z2 + ']\n')
            self.write_blocknum()
            self.write('(LOG,<Z>#<_value></Z>)\n')

        if ((x2 != None) or (y2 != None) or (z2 != None)):
            self.write_blocknum()
            self.write('(LOG,</POINT>)\n')

        if ((x3 != None) or (y3 != None) or (z3 != None)):
            self.write_blocknum()
            self.write('(LOG,<POINT>)\n')

        if (x3 != None):
            self.write_blocknum()
            self.write('#<_value>=[' + x3 + ']\n')
            self.write_blocknum()
            self.write('(LOG,<X>#<_value></X>)\n')

        if (y3 != None):
            self.write_blocknum()
            self.write('#<_value>=[' + y3 + ']\n')
            self.write_blocknum()
            self.write('(LOG,<Y>#<_value></Y>)\n')

        if (z3 != None):
            self.write_blocknum()
            self.write('#<_value>=[' + z3 + ']\n')
            self.write_blocknum()
            self.write('(LOG,<Z>#<_value></Z>)\n')

        if ((x3 != None) or (y3 != None) or (z3 != None)):
            self.write_blocknum()
            self.write('(LOG,</POINT>)\n')

        if ((x4 != None) or (y4 != None) or (z4 != None)):
            self.write_blocknum()
            self.write('(LOG,<POINT>)\n')

        if (x4 != None):
            self.write_blocknum()
            self.write('#<_value>=[' + x4 + ']\n')
            self.write_blocknum()
            self.write('(LOG,<X>#<_value></X>)\n')

        if (y4 != None):
            self.write_blocknum()
            self.write('#<_value>=[' + y4 + ']\n')
            self.write_blocknum()
            self.write('(LOG,<Y>#<_value></Y>)\n')

        if (z4 != None):
            self.write_blocknum()
            self.write('#<_value>=[' + z4 + ']\n')
            self.write_blocknum()
            self.write('(LOG,<Z>#<_value></Z>)\n')

        if ((x4 != None) or (y4 != None) or (z4 != None)):
            self.write_blocknum()
            self.write('(LOG,</POINT>)\n')

        if ((x5 != None) or (y5 != None) or (z5 != None)):
            self.write_blocknum()
            self.write('(LOG,<POINT>)\n')

        if (x5 != None):
            self.write_blocknum()
            self.write('#<_value>=[' + x5 + ']\n')
            self.write_blocknum()
            self.write('(LOG,<X>#<_value></X>)\n')

        if (y5 != None):
            self.write_blocknum()
            self.write('#<_value>=[' + y5 + ']\n')
            self.write_blocknum()
            self.write('(LOG,<Y>#<_value></Y>)\n')

        if (z5 != None):
            self.write_blocknum()
            self.write('#<_value>=[' + z5 + ']\n')
            self.write_blocknum()
            self.write('(LOG,<Z>#<_value></Z>)\n')

        if ((x5 != None) or (y5 != None) or (z5 != None)):
            self.write_blocknum()
            self.write('(LOG,</POINT>)\n')

        if ((x6 != None) or (y6 != None) or (z6 != None)):
            self.write_blocknum()
            self.write('(LOG,<POINT>)\n')

        if (x6 != None):
            self.write_blocknum()
            self.write('#<_value>=[' + x6 + ']\n')
            self.write_blocknum()
            self.write('(LOG,<X>#<_value></X>)\n')

        if (y6 != None):
            self.write_blocknum()
            self.write('#<_value>=[' + y6 + ']\n')
            self.write_blocknum()
            self.write('(LOG,<Y>#<_value></Y>)\n')

        if (z6 != None):
            self.write_blocknum()
            self.write('#<_value>=[' + z6 + ']\n')
            self.write_blocknum()
            self.write('(LOG,<Z>#<_value></Z>)\n')

        if ((x6 != None) or (y6 != None) or (z6 != None)):
            self.write_blocknum()
            self.write('(LOG,</POINT>)\n')

            self.write_blocknum()
            self.write('(LOG,</POINTS>)\n')

        if (xml_file_name != None):
            self.write_blocknum()
            self.write('(LOGCLOSE)\n')

    def open_log_file(self, xml_file_name=None ):
        self.write_blocknum()
        self.write('(LOGOPEN,')
        self.write(xml_file_name)
        self.write(')\n')

    def close_log_file(self):
        self.write_blocknum()
        self.write('(LOGCLOSE)\n')

    def log_coordinate(self, x=None, y=None, z=None):
        if ((x != None) or (y != None) or (z != None)):
            self.write_blocknum()
            self.write('(LOG,<POINT>)\n')

        if (x != None):
            self.write_blocknum()
            self.write('#<_value>=[' + x + ']\n')
            self.write_blocknum()
            self.write('(LOG,<X>#<_value></X>)\n')

        if (y != None):
            self.write_blocknum()
            self.write('#<_value>=[' + y + ']\n')
            self.write_blocknum()
            self.write('(LOG,<Y>#<_value></Y>)\n')

        if (z != None):
            self.write_blocknum()
            self.write('#<_value>=[' + z + ']\n')
            self.write_blocknum()
            self.write('(LOG,<Z>#<_value></Z>)\n')

        if ((x != None) or (y != None) or (z != None)):
            self.write_blocknum()
            self.write('(LOG,</POINT>)\n')

    def log_message(self, message=None ):
        self.write_blocknum()
        self.write('(LOG,' + message + ')\n')

nc.creator = CreatorEMC2()

