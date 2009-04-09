################################################################################
# iso.py
#
# Simple ISO NC code creator
#
# Hirutso Enni, 2009-01-13

import iso_codes as iso
import nc
import math

################################################################################
class CreatorIso(nc.Creator):

    def __init__(self):
        nc.Creator.__init__(self)

        self.a = 0
        self.b = 0
        self.c = 0
        self.f = ''
        self.fh = None
        self.fv = None
        self.fhv = False
        self.g = ''
        self.i = 0
        self.j = 0
        self.k = 0
        self.m = []
        self.n = 10
        self.r = 0
        self.s = ''
        self.t = None
        self.x = 0
        self.y = 0
        self.z = 500

        self.fmt = iso.FORMAT_MM

    ############################################################################
    ##  Internals

    def write_feedrate(self):
        self.write(self.f)
        self.f = ''

    def write_preps(self):
        self.write(self.g)
        self.g = ''

    def write_misc(self):
        if (len(self.m)) : self.write(self.m.pop())

    def write_blocknum(self):
        pass
        #self.write(iso.BLOCK % self.n)
        #self.n += 10

    def write_spindle(self):
        self.write(self.s)
        self.s = ''

    ############################################################################
    ##  Programs

    def program_begin(self, id, name=''):
        self.write((iso.PROGRAM % id) + iso.SPACE + (iso.COMMENT % name))
        self.write('\n')

    def program_stop(self, optional=False):
        self.write_blocknum()
        if (optional) : self.write(iso.STOP_OPTIONAL + '\n')
        else : self.write(iso.STOP + '\n')

    def program_end(self):
        self.write_blocknum()
        self.write(iso.PROGRAM_END + '\n')

    def flush_nc(self):
        self.write_blocknum()
        self.write_preps()
        self.write_misc()
        self.write('\n')

    ############################################################################
    ##  Subprograms
    
    def sub_begin(self, id, name=''):
        self.write((iso.PROGRAM % id) + iso.SPACE + (iso.COMMENT % name))
        self.write('\n')

    def sub_call(self, id):
        self.write_blocknum()
        self.write((iso.SUBPROG_CALL % id) + '\n')

    def sub_end(self):
        self.write_blocknum()
        self.write(iso.SUBPROG_END + '\n')

    ############################################################################
    ##  Settings
    
    def imperial(self):
        self.g += iso.IMPERIAL
        self.fmt = iso.FORMAT_IN

    def metric(self):
        self.g += iso.METRIC
        self.fmt = iso.FORMAT_MM

    def absolute(self):
        self.g += iso.ABSOLUTE

    def incremental(self):
        self.g += iso.INCREMENTAL

    def polar(self, on=True):
        if (on) : self.g += iso.POLAR_ON
        else : self.g += iso.POLAR_OFF

    def set_plane(self, plane):
        if (plane == 0) : self.g += iso.PLANE_XY
        elif (plane == 1) : self.g += iso.PLANE_XZ
        elif (plane == 2) : self.g += iso.PLANE_YZ

    ############################################################################
    ##  Tools

    def tool_change(self, id):
        self.write_blocknum()
        self.write((iso.TOOL % id) + '\n')

    def tool_defn(self, id, name='', radius=None, length=None):
        pass

    def offset_radius(self, id, radius=None):
        pass

    def offset_length(self, id, length=None):
        pass

    ############################################################################
    ##  Datums
    
    def datum_shift(self, x=None, y=None, z=None, a=None, b=None, c=None):
        pass

    def datum_set(self, x=None, y=None, z=None, a=None, b=None, c=None):
        pass

    def workplane(self, id):
        self.g += iso.WORKPLANE % (id + iso.WORKPLANE_BASE)

    ############################################################################
    ##  Rates + Modes

    def feedrate(self, f):
        self.f = iso.FEEDRATE + (self.fmt % f)
        self.fhv = False

    def feedrate_hv(self, fh, fv):
        self.fh = fh
        self.fv = fv
        self.fhv = True

    def calc_feedrate_hv(self, h, v):
        l = math.sqrt(h*h+v*v)
        if (h == 0) : self.f = iso.FEEDRATE + (self.fmt % self.fv)
        elif (v == 0) : self.f = iso.FEEDRATE + (self.fmt % self.fh)
        else:
            self.f = iso.FEEDRATE + (self.fmt % (self.fh * l * min([1/h, 1/v])))

    def spindle(self, s):
        self.s = iso.SPINDLE % s

    def coolant(self, mode=0):
        if (mode <= 0) : self.m.append(iso.COOLANT_OFF)
        elif (mode == 1) : self.m.append(iso.COOLANT_MIST)
        elif (mode == 2) : self.m.append(iso.COOLANT_FLOOD)

    def gearrange(self, gear=0):
        if (gear <= 0) : self.m.append(iso.GEAR_OFF)
        elif (gear <= 4) : self.m.append(iso.GEAR % (gear + GEAR_BASE))

    ############################################################################
    ##  Moves

    def rapid(self, x=None, y=None, z=None, a=None, b=None, c=None):
        self.write_blocknum()
        self.write(iso.RAPID)
        self.write_preps()
        if (x != None):
            dx = x - self.x
            self.write(iso.X + (self.fmt % x))
            self.x = x
        if (y != None):
            dy = y - self.y
            self.write(iso.Y + (self.fmt % y))
            self.y = y
        if (z != None):
            dz = z - self.z
            self.write(iso.Z + (self.fmt % z))
            self.z = z
        if (a != None) : self.write(iso.A + (iso.FORMAT_ANG % a))
        if (b != None) : self.write(iso.B + (iso.FORMAT_ANG % b))
        if (c != None) : self.write(iso.C + (iso.FORMAT_ANG % c))
        self.write_spindle()
        self.write_misc()
        self.write('\n')

    def feed(self, x=None, y=None, z=None):
        if self.same_xyz(x, y, z): return
        self.write_blocknum()
        self.write(iso.FEED)
        self.write_preps()
        dx = dy = dz = 0
        if (x != None):
            dx = x - self.x
            self.write(iso.X + (self.fmt % x))
            self.x = x
        if (y != None):
            dy = y - self.y
            self.write(iso.Y + (self.fmt % y))
            self.y = y
        if (z != None):
            dz = z - self.z
            self.write(iso.Z + (self.fmt % z))
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

    def arc_cw(self, x=None, y=None, z=None, i=None, j=None, k=None, r=None):
        if self.same_xyz(x, y, z): return
        self.write_blocknum()
        self.write(iso.ARC_CW)
        self.write_preps()
        if (x != None):
            self.write(iso.X + (self.fmt % x))
            self.x = x
        if (y != None):
            self.write(iso.Y + (self.fmt % y))
            self.y = y
        if (z != None):
            self.write(iso.Z + (self.fmt % z))
            self.z = z
        if (i != None) : self.write(iso.CENTRE_X + (self.fmt % i))
        if (j != None) : self.write(iso.CENTRE_Y + (self.fmt % j))
        if (k != None) : self.write(iso.CENTRE_Z + (self.fmt % k))
        if (r != None) : self.write(iso.RADIUS + (self.fmt % r))
#        if (self.fhv) : self.calc_feedrate_hv(ARC_LENGTH, math.fabs(dz))
        self.write_feedrate()
        self.write_spindle()
        self.write_misc()
        self.write('\n')

    def arc_ccw(self, x=None, y=None, z=None, i=None, j=None, k=None, r=None):
        if self.same_xyz(x, y, z): return
        self.write_blocknum()
        self.write(iso.ARC_CCW)
        self.write_preps()
        if (x != None):
            self.write(iso.X + (self.fmt % x))
            self.x = x
        if (y != None):
            self.write(iso.Y + (self.fmt % y))
            self.y = y
        if (z != None):
            self.write(iso.Z + (self.fmt % z))
            self.z = z
        if (i != None) : self.write(iso.CENTRE_X + (self.fmt % i))
        if (j != None) : self.write(iso.CENTRE_Y + (self.fmt % j))
        if (k != None) : self.write(iso.CENTRE_Z + (self.fmt % k))
        if (r != None) : self.write(iso.RADIUS + (self.fmt % r))
#        if (self.fhv) : self.calc_feedrate_hv(ARC_LENGTH, math.fabs(dz))
        self.write_feedrate()
        self.write_spindle()
        self.write_misc()
        self.write('\n')

    def dwell(self, t):
        self.write_blocknum()
        self.write_preps()
        self.write(iso.DWELL + (iso.TIME % t))
        self.write_misc()
        self.write('\n')

    def rapid_home(self, x=None, y=None, z=None, a=None, b=None, c=None):
        pass

    def rapid_unhome(self):
        pass

    ############################################################################
    ##  Cycles

    def pattern(self):
        pass

    def pocket(self):
        pass

    def profile(self):
        pass

    def drill(self, x=None, y=None, z=None, zretract=None, depth=None, standoff=None, dwell_bottom=None, dwell_top=None, pecks=[], peck_to_top=True):
        pass

    def tap(self, x=None, y=None, z=None, zretract=None, depth=None, standoff=None, dwell_bottom=None, pitch=None, stoppos=None, spin_in=None, spin_out=None):
        pass

    def bore(self, x=None, y=None, z=None, zretract=None, depth=None, standoff=None, dwell_bottom=None, feed_in=None, feed_out=None, stoppos=None, shift_back=None, shift_right=None, backbore=False, stop=False):
        pass

    ############################################################################
    ##  Misc

    def comment(self, text):
        self.write((iso.COMMENT % text) + '\n')

    def variable(self, id):
        return (iso.VARIABLE % id)

    def variable_set(self, id, value):
        self.write_blocknum()
        self.write((iso.VARIABLE % id) + (iso.VARIABLE_SET % value) + '\n')

################################################################################

nc.creator = CreatorIso()
