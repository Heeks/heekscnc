################################################################################
# iso.py
#
# Simple ISO NC code creator
#
# Hirutso Enni, 2009-01-13

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

        self.fmt_in = '%.5f'
        self.fmt_mm = '%.3f'
        self.fmt = self.fmt_mm

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
        self.write('N' + str(self.n) + ' ')
        self.n += 10

    def write_spindle(self):
        self.write(self.s)
        self.s = ''

    ############################################################################
    ##  Programs

    def program_begin(self, id, name=''):
        self.write('O' + str(id) + ' ' + self.comment(name))
        self.write('\n')

    def program_stop(self, optional=False):
        self.write_blocknum()
        if (optional) : self.write('M01\n')
        else : self.write('M00\n')

    def program_end(self):
        self.write_blocknum()
        self.write('M02\n')

    def flush_nc(self):
        self.write_blocknum()
        self.write_preps()
        self.write_misc()
        self.write('\n')

    ############################################################################
    ##  Subprograms
    
    def sub_begin(self, id, name=''):
        self.write('O' + str(id) + ' ' + self.comment(name))
        self.write('\n')

    def sub_call(self, id):
        self.write_blocknum()
        self.write('M98 P' + str(id) + '\n')

    def sub_end(self):
        self.write_blocknum()
        self.write('M99\n')

    ############################################################################
    ##  Settings
    
    def imperial(self):
        self.g += ' G20'
        self.fmt = self.fmt_in

    def metric(self):
        self.g += ' G21'
        self.fmt = self.fmt_mm

    def absolute(self):
        self.g += ' G90'

    def incremental(self):
        self.g += ' G91'

    def polar(self, on=True):
        if (on) : self.g += ' G16'
        else : self.g += ' G15'

    def set_plane(self, plane):
        if (plane == 0) : self.g += ' G17'
        elif (plane == 1) : self.g += ' G18'
        elif (plane == 2) : self.g += ' G19'

    ############################################################################
    ##  Tools

    def tool_change(self, id):
        self.write_blocknum()
        self.write('T' + str(id) + ' M06\n')

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
        self.g += 'G' + str(id+53)

    ############################################################################
    ##  Rates + Modes

    def feedrate(self, f):
        self.f = ' F' + (self.fmt % f)
        self.fhv = False

    def feedrate_hv(self, fh, fv):
        self.fh = fh
        self.fv = fv
        self.fhv = True

    def calc_feedrate_hv(self, h, v):
        l = math.sqrt(h*h+v*v)
        if (h == 0) : self.f = ' F' + (self.fmt % self.fv)
        elif (v == 0) : self.f = ' F' + (self.fmt % self.fh)
        else:
            self.f = ' F' + (self.fmt % (self.fh * l * min([1/h, 1/v])))

    def spindle(self, s):
        self.s = ' S%.3f' % s

    def coolant(self, mode=0):
        if (mode == 0) : self.m.append(' M09')
        elif (mode == 1) : self.m.append(' M07')
        elif (mode == 2) : self.m.append(' M08')

    def gearrange(self, gear=0):
        if (gear == 0) : self.m.append(' ?')
        elif (gear == 1) : self.m.append(' M38')
        elif (gear == 2) : self.m.append(' M39')
        elif (gear == 3) : self.m.append(' M40')
        elif (gear == 4) : self.m.append(' M41')

    ############################################################################
    ##  Moves

    def rapid(self, x=None, y=None, z=None, a=None, b=None, c=None):
        self.write_blocknum()
        self.write('G00')
        self.write_preps()
        if (x != None):
            dx = x - self.x
            self.write(' X' + (self.fmt % x))
            self.x = x
        if (y != None):
            dy = y - self.y
            self.write(' Y' + (self.fmt % y))
            self.y = y
        if (z != None):
            dz = z - self.z
            self.write(' Z' + (self.fmt % z))
            self.z = z
        if (a != None) : self.write(' A%.1f' % a)
        if (b != None) : self.write(' B%.1f' % b)
        if (c != None) : self.write(' C%.1f' % c)
        self.write_spindle()
        self.write_misc()
        self.write('\n')

    def feed(self, x=None, y=None, z=None):
        self.write_blocknum()
        self.write('G01')
        self.write_preps()
        dx = dy = dz = 0
        if (x != None):
            dx = x - self.x
            self.write(' X' + (self.fmt % x))
            self.x = x
        if (y != None):
            dy = y - self.y
            self.write(' Y' + (self.fmt % y))
            self.y = y
        if (z != None):
            dz = z - self.z
            self.write(' Z' + (self.fmt % z))
            self.z = z
        if (self.fhv) : self.calc_feedrate_hv(math.sqrt(dx*dx+dy*dy), math.fabs(dz))
        self.write_feedrate()
        self.write_spindle()
        self.write_misc()
        self.write('\n')

    def arc_cw(self, x=None, y=None, z=None, i=None, j=None, k=None, r=None):
        self.write_blocknum()
        self.write('G02')
        self.write_preps()
        if (x != None):
            dx = x - self.x
            self.write(' X' + (self.fmt % x))
            self.x = x
        if (y != None):
            dy = y - self.y
            self.write(' Y' + (self.fmt % y))
            self.y = y
        if (z != None):
            dz = z - self.z
            self.write(' Z' + (self.fmt % z))
            self.z = z
        if (i != None) : self.write(' I' + (self.fmt % i))
        if (j != None) : self.write(' J' + (self.fmt % j))
        if (k != None) : self.write(' K' + (self.fmt % k))
        if (r != None) : self.write(' R' + (self.fmt % r))
#        if (self.fhv) : self.calc_feedrate_hv(ARC_LENGTH, math.fabs(dz))
        self.write_feedrate()
        self.write_spindle()
        self.write_misc()
        self.write('\n')

    def arc_ccw(self, x=None, y=None, z=None, i=None, j=None, k=None, r=None):
        self.write_blocknum()
        self.write('G03')
        self.write_preps()
        if (x != None):
            dx = x - self.x
            self.write(' X' + (self.fmt % x))
            self.x = x
        if (y != None):
            dy = y - self.y
            self.write(' Y' + (self.fmt % y))
            self.y = y
        if (z != None):
            dz = z - self.z
            self.write(' Z' + (self.fmt % z))
            self.z = z
        if (i != None) : self.write(' I' + (self.fmt % i))
        if (j != None) : self.write(' J' + (self.fmt % j))
        if (k != None) : self.write(' K' + (self.fmt % k))
        if (r != None) : self.write(' R' + (self.fmt % r))
#        if (self.fhv) : self.calc_feedrate_hv(ARC_LENGTH, math.fabs(dz))
        self.write_feedrate()
        self.write_spindle()
        self.write_misc()
        self.write('\n')

    def dwell(self, t):
        self.write_blocknum()
        self.write_preps()
        self.write('G04 P%.2f' % t)
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
        return '(' + text + ')'

    def variable(self, id):
        return '#' + str(id)

    def variable_set(self, id, value):
        self.write_blocknum()
        self.write('#' + str(id) + ('=%.3f' % value) + '\n')

################################################################################

nc.creator = CreatorIso()
