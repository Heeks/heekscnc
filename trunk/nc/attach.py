################################################################################
# attach.py
#
# NC code creator for attaching Z coordinates to a surface
#
# Hirutso Enni, 2009-01-13

import nc
import ocl
import ocl_funcs

units = 1.0

################################################################################
class CreatorAttach(nc.Creator):

    def __init__(self, original):
        nc.Creator.__init__(self)

        self.original = original
        self.x = original.x * units
        self.y = original.y * units
        self.z = original.z * units
        self.imperial = False
        self.stl = None
        self.cutter = None
        self.minz = None

    ############################################################################
    ##  Shift in Z

    def z2(self, z):
        path = ocl.Path()
        # use a line with no length
        path.append(ocl.Line(ocl.Point(self.x, self.y, self.z), ocl.Point(self.x, self.y, self.z)))
        pdcf = ocl.PathDropCutter()
        pdcf.setSTL(self.stl)
        pdcf.setCutter(self.cutter)
        pdcf.setSampling(0.1)
        pdcf.setZ(self.minz)
        pdcf.setPath(path)
        pdcf.run()
        plist = pdcf.getCLPoints()
        p = plist[0]
        return p.z

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
        self.imperial = True
        self.original.imperial()

    def metric(self):
        self.original.metric()

    def absolute(self):
        self.original.absolute()

    def incremental(self):
        self.original.incremental()

    def polar(self, on=True):
        self.original.polar(on)

    def set_plane(self, plane):
        self.original.set_plane(plane)

    def set_temporary_origin(self, x=None, y=None, z=None, a=None, b=None, c=None):
        self.original.set_temporary_origin(x,y,z,a,b,c)

    def remove_temporary_origin(self):
        self.original.remove_temporary_origin()

    ############################################################################
    ##  Tools

    def tool_change(self, id):
        self.original.tool_change(id)

    def tool_defn(self, id, name='', radius=None, length=None, gradient=None):
        self.original.tool_defn(id, name, radius, length, gradient)

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

    def spindle(self, s, clockwise=True):
        self.original.spindle(s, clockwise)

    def coolant(self, mode=0):
        self.original.coolant(mode)

    def gearrange(self, gear=0):
        self.original.gearrange(gear)

    ############################################################################
    ##  Moves

    def rapid(self, x=None, y=None, z=None, a=None, b=None, c=None):
        self.original.rapid(x, y, z, a, b, c)
        if x != None: self.x = x * units
        if y != None: self.y = y * units
        if z != None: self.z = z * units
        
    def cut_path(self, path):
        # get the points on the surface
        pdcf = ocl.PathDropCutter()
        pdcf.setSTL(self.stl)
        pdcf.setCutter(self.cutter)
        pdcf.setSampling(0.1)
        pdcf.setZ(self.minz)
        pdcf.setPath(path)
        pdcf.run()
        plist = pdcf.getCLPoints()
        
        #refine the points
        f = ocl.LineCLFilter()
        f.setTolerance(0.005)
        for p in plist:
            f.addCLPoint(p)
        f.run()
        plist = f.getCLPoints()
        
        i = 0
        for p in plist:
            if i > 0:
                self.original.feed(p.x/units, p.y/units, p.z/units)
            i = i + 1

    def feed(self, x=None, y=None, z=None):
        px = self.x
        py = self.y
        pz = self.z
        if x != None: self.x = x * units
        if y != None: self.y = y * units
        if z != None: self.z = z * units
        if self.x == None or self.y == None or self.z == None:
            self.original.feed(x, y, z)
            return
        if px == self.x and py == self.y:
            # z move only
            self.original.feed(self.x/units, self.y/units, self.z2(self.z)/units)
            return
            
        # make a path which is a line
        path = ocl.Path()
        path.append(ocl.Line(ocl.Point(px, py, pz), ocl.Point(self.x, self.y, self.z)))
        
        # cut along the path
        self.cut_path(path)
        
    def arc(self, x=None, y=None, z=None, i=None, j=None, k=None, r=None, ccw = True):
        if self.x == None or self.y == None or self.z == None:
            raise "first attached move can't be an arc"
        px = self.x
        py = self.y
        pz = self.z
        if x != None: self.x = x * units
        if y != None: self.y = y * units
        if z != None: self.z = z * units
        path = ocl.Path()
        path.append(ocl.Arc(ocl.Point(px, py, pz), ocl.Point(self.x, self.y, self.z), ocl.Point(px + i, py + j, pz), ccw))
        
        # cut along the path
        self.cut_path(path)

    def arc_cw(self, x=None, y=None, z=None, i=None, j=None, k=None, r=None):
        self.arc(x, y, z, i, j, k, r, False)

    def arc_ccw(self, x=None, y=None, z=None, i=None, j=None, k=None, r=None):
        self.arc(x, y, z, i, j, k, r, True)

    def dwell(self, t):
        self.original.dwell(t)

    def rapid_home(self, x=None, y=None, z=None, a=None, b=None, c=None):
        self.original.rapid_home(x, y, z, a, b, c)

    def rapid_unhome(self):
        self.original.rapid_unhome()

    ############################################################################
    ##  Cutter radius compensation

    def use_CRC(self):
        return self.original.use_CRC()

    def start_CRC(self, left = True, radius = 0.0):
        self.original.start_CRC(left, radius)

    def end_CRC(self):
        self.original.end_CRC()

    ############################################################################
    ##  Cycles

    def pattern(self):
        self.original.pattern()

    def pocket(self):
        self.original.pocket()

    def profile(self):
        self.original.profile()

    def circular_pocket(self, x=None, y=None, ToolDiameter=None, HoleDiameter=None, ClearanceHeight=None, StartHeight=None, MaterialTop=None, FeedRate=None, SpindleRPM=None, HoleDepth=None, DepthOfCut=None, StepOver=None ):
		self.circular_pocket(x, y, ToolDiameter, HoleDiameter, ClearanceHeight, StartHeight, MaterialTop, FeedRate, SpindleRPM, HoleDepth, DepthOfCut, StepOver)

    def drill(self, x=None, y=None, z=None, depth=None, standoff=None, dwell=None, peck_depth=None):
        self.original.drill(x, y, z, depth, standoff, dwell, peck_depth)

#    def tap(self, x=None, y=None, z=None, zretract=None, depth=None, standoff=None, dwell_bottom=None, pitch=None, stoppos=None, spin_in=None, spin_out=None):
#        self.original.tap(x, y, self.z2(z), self.z2(zretract), depth, standoff, dwell_bottom, pitch, stoppos, spin_in, spin_out)

    # argument list adapted for compatibility with Tapping module
    # wild guess - I'm unsure about the purpose of this file and wether this works -haberlerm
    def tap(self, x=None, y=None, z=None, zretract=None, depth=None, standoff=None, dwell_bottom=None, pitch=None, stoppos=None, spin_in=None, spin_out=None, tap_mode=None, direction=None):
        self.original.tap( x, y, self.z2(z), self.z2(zretract), depth, standoff, dwell_bottom, pitch, stoppos, spin_in, spin_out, tap_mode, direction)


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
