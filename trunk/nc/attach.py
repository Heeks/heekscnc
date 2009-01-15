################################################################################
# attach.py
#
# NC code creator for attaching Z coordinates to a surface
#
# Hirutso Enni, 2009-01-13

import nc

################################################################################
class CreatorAttach(nc.Creator):

    def __init__(self, original):
        nc.Creator.__init__(self)

        self.original = original

    ############################################################################
    ##  Shift in Z

    def z2(self, z):
        if (z == None) : return None
        return z + 100

    ############################################################################
    ##  Programs

    def program_begin(self, id, name=''):
        self.original.program_begin(id, name)

    def program_stop(self, optional=False):
        self.original.program_stop(optional)

    def program_end(self):
        self.original.program_end()

    def flush_nc(self):
        self.original.flush_nc()

    ############################################################################
    ##  Subprograms
    
    def sub_begin(self, id, name=''):
        self.original.sub_begin(id, name)

    def sub_call(self, id):
        self.original.sub_call(id)

    def sub_end(self):
        self.original.sub_end()

    ############################################################################
    ##  Settings
    
    def imperial(self):
        self.original.imperial()

    def metric(self):
        self.original.metric()

    def absolute(self):
        self.original.absolute()

    def incremental(self):
        self.original.incremental()

    ############################################################################
    ##  Tools

    def tool_change(self, id):
        self.original.tool_change(id)

    def tool_defn(self, id, name='', radius=None, length=None):
        self.original.tool_defn(id, name, radius, length)

    def offset_radius(self, id, radius=None):
        self.original.offset_radius(id, radius)

    def offset_length(self, id, length=None):
        self.original.offset_length(id, length)

    ############################################################################
    ##  Datums

    def datum_shift(self, x=None, y=None, z=None, a=None, b=None, c=None):
        self.original.datum_shift(x, y, z, a, b, c)

    def datum_set(self, x=None, y=None, z=None, a=None, b=None, c=None):
        self.original.datum_set(x, y, z, a, b, c)

    def workplane(self, id):
        self.original.workplane(id)

    ############################################################################
    ##  Rates + Modes

    def feedrate(self, f):
        self.original.feedrate(f)

    def feedrate_hv(self, fh, fv):
        self.original.feedrate_hv(fh, fv)

    def spindle(self, s):
        self.original.spindle(s)

    def coolant(self, mode=0):
        self.original.coolant(mode)

    def gearrange(self, gear=0):
        self.original.gearrange(gear)

    ############################################################################
    ##  Moves

    def rapid(self, x=None, y=None, z=None, a=None, b=None, c=None):
        self.original.rapid(x, y, self.z2(z), a, b, c)

    def feed(self, x=None, y=None, z=None):
        self.original.feed(x, y, self.z2(z))

    def arc_cw(self, x=None, y=None, z=None, i=None, j=None, k=None, r=None):
        self.original.arc_cw(x, y, self.z2(z), i, j, self.z2(k), r)

    def arc_ccw(self, x=None, y=None, z=None, i=None, j=None, k=None, r=None):
        self.original.arc_ccw(x, y, self.z2(z), i, j, self.z2(k), r)

    def dwell(self, t):
        self.original.dwell(t)

    def rapid_home(self, x=None, y=None, z=None, a=None, b=None, c=None):
        self.original.rapid_home(x, y, z, a, b, c)

    def rapid_unhome(self):
        self.original.rapid_unhome()

    ############################################################################
    ##  Cycles

    def pattern(self):
        self.original.pattern()

    def pocket(self):
        self.original.pocket()

    def profile(self):
        self.original.profile()

    def drill(self, x=None, y=None, z=None, zretract=None, depth=None, standoff=None, dwell_bottom=None, dwell_top=None, pecks=[], peck_to_top=True):
        self.original.drill(x, y, self.z2(z), self.z2(zretract), depth, standoff, dwell_bottom, dwell_top, pecks, peck_to_top)

    def tap(self, x=None, y=None, z=None, zretract=None, depth=None, standoff=None, dwell_bottom=None, pitch=None, stoppos=None, spin_in=None, spin_out=None):
        self.original.tap(x, y, self.z2(z), self.z2(zretract), depth, standoff, dwell_bottom, pitch, stoppos, spin_in, spin_out)

    def bore(self, x=None, y=None, z=None, zretract=None, depth=None, standoff=None, dwell_bottom=None, feed_in=None, feed_out=None, stoppos=None, shift_back=None, shift_right=None, backbore=False, stop=False):
        self.original.bore(x, y, self.z2(z), self.z2(zretract), depth, standoff, dwell_Bottom, feed_in, feed_out, stoppos, shift_back, shift_right, backbore, stop)

    ############################################################################
    ##  Misc

    def comment(self, text):
        self.original.comment(text)

    def variable(self, id):
        self.original.variable(id)

    def variable_set(self, id, value):
        self.original.variable_set(id, value)

################################################################################

def attach_begin():
    nc.creator = CreatorAttach(nc.creator)

def attach_end():
    nc.creator = nc.creator.original
