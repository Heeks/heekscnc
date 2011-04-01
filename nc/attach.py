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
attached = False

################################################################################
class Creator(nc.Creator):

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
        self.path = None
        self.pdcf = None
        self.material_allowance = 0.0

    ############################################################################
    ##  Shift in Z
    
    def setPdcfIfNotSet(self):
        if self.pdcf == None:
            self.pdcf = ocl.PathDropCutter()
            self.pdcf.setSTL(self.stl)
            self.pdcf.setCutter(self.cutter)
            self.pdcf.setSampling(0.1)
            self.pdcf.setZ(self.minz)
                    
    def z2(self, z):
        path = ocl.Path()
        # use a line with no length
        path.append(ocl.Line(ocl.Point(self.x, self.y, self.z), ocl.Point(self.x, self.y, self.z)))
        self.setPdcfIfNotSet()
        if (self.z>self.minz):
            self.pdcf.setZ(self.z)  # Adjust Z if we have gotten a higher limit (Fix pocketing loosing steps when using attach?)
        else:
            self.pdcf.setZ(self.minz) # Else use minz
        self.pdcf.setPath(path)
        self.pdcf.run()
        plist = self.pdcf.getCLPoints()
        p = plist[0]
        return p.z + self.material_allowance

    ############################################################################
    ##  Programs

    def program_begin(self, id, name=''):
        self.cut_path()
        self.original.program_begin(id, name)

    def program_stop(self, optional=False):
        self.cut_path()
        self.original.program_stop(optional)

    def program_end(self):
        self.cut_path()
        self.original.program_end()

    def flush_nc(self):
        self.cut_path()
        self.original.flush_nc()

    ############################################################################
    ##  Subprograms
    
    def sub_begin(self, id, name=''):
        self.cut_path()
        self.original.sub_begin(id, name)

    def sub_call(self, id):
        self.cut_path()
        self.original.sub_call(id)

    def sub_end(self):
        self.cut_path()
        self.original.sub_end()

    ############################################################################
    ##  Settings
    
    def imperial(self):
        self.cut_path()
        self.imperial = True
        self.original.imperial()

    def metric(self):
        self.cut_path()
        self.original.metric()

    def absolute(self):
        self.cut_path()
        self.original.absolute()

    def incremental(self):
        self.cut_path()
        self.original.incremental()

    def polar(self, on=True):
        self.cut_path()
        self.original.polar(on)

    def set_plane(self, plane):
        self.cut_path()
        self.original.set_plane(plane)

    def set_temporary_origin(self, x=None, y=None, z=None, a=None, b=None, c=None):
        self.cut_path()
        self.original.set_temporary_origin(x,y,z,a,b,c)

    def remove_temporary_origin(self):
        self.cut_path()
        self.original.remove_temporary_origin()

    ############################################################################
    ##  Tools

    def tool_change(self, id):
        self.cut_path()
        self.original.tool_change(id)

    def tool_defn(self, id, name='', radius=None, length=None, gradient=None):
        self.cut_path()
        self.original.tool_defn(id, name, radius, length, gradient)

    def offset_radius(self, id, radius=None):
        self.cut_path()
        self.original.offset_radius(id, radius)

    def offset_length(self, id, length=None):
        self.cut_path()
        self.original.offset_length(id, length)

    ############################################################################
    ##  Datums

    def datum_shift(self, x=None, y=None, z=None, a=None, b=None, c=None):
        self.cut_path()
        self.original.datum_shift(x, y, z, a, b, c)

    def datum_set(self, x=None, y=None, z=None, a=None, b=None, c=None):
        self.cut_path()
        self.original.datum_set(x, y, z, a, b, c)

    def workplane(self, id):
        self.cut_path()
        self.original.workplane(id)

    ############################################################################
    ##  Rates + Modes

    def feedrate(self, f):
        self.cut_path()
        self.original.feedrate(f)

    def feedrate_hv(self, fh, fv):
        self.cut_path()
        self.original.feedrate_hv(fh, fv)

    def spindle(self, s, clockwise=True):
        self.cut_path()
        self.original.spindle(s, clockwise)

    def coolant(self, mode=0):
        self.cut_path()
        self.original.coolant(mode)

    def gearrange(self, gear=0):
        self.cut_path()
        self.original.gearrange(gear)

    ############################################################################
    ##  Moves

    def rapid(self, x=None, y=None, z=None, a=None, b=None, c=None):
        self.cut_path()
        self.original.rapid(x, y, z, a, b, c)
        if x != None: self.x = x * units
        if y != None: self.y = y * units
        if z != None: self.z = z * units
        
    def cut_path(self):
        if self.path == None: return
        self.setPdcfIfNotSet()
        
        if (self.z>self.minz):
            self.pdcf.setZ(self.z)  # Adjust Z if we have gotten a higher limit (Fix pocketing loosing steps when using attach?)
        else:
            self.pdcf.setZ(self.minz) # Else use minz
            
       # get the points on the surface
        self.pdcf.setPath(self.path)
        self.pdcf.run()
        plist = self.pdcf.getCLPoints()
        
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
                self.original.feed(p.x/units, p.y/units, p.z/units + self.material_allowance)
            i = i + 1
            
        self.path = ocl.Path()

    def feed(self, x=None, y=None, z=None):
        px = self.x
        py = self.y
        pz = self.z
        if x != None: self.x = x * units
        if y != None: self.y = y * units
        if z != None: self.z = z * units
        if self.x == None or self.y == None or self.z == None:
            self.cut_path()
            self.original.feed(x, y, z)
            return
        if px == self.x and py == self.y:
            # z move only
            self.cut_path()
            self.original.feed(self.x/units, self.y/units, self.z2(self.z)/units)
            return
            
        # add a line to the path
        if self.path == None: self.path = ocl.Path()
        self.path.append(ocl.Line(ocl.Point(px, py, pz), ocl.Point(self.x, self.y, self.z)))
        
    def arc(self, x=None, y=None, z=None, i=None, j=None, k=None, r=None, ccw = True):
        if self.x == None or self.y == None or self.z == None:
            raise "first attached move can't be an arc"
        px = self.x
        py = self.y
        pz = self.z
        if x != None: self.x = x * units
        if y != None: self.y = y * units
        if z != None: self.z = z * units
        
        # add an arc to the path
        if self.path == None: self.path = ocl.Path()
        self.path.append(ocl.Arc(ocl.Point(px, py, pz), ocl.Point(self.x, self.y, self.z), ocl.Point(i, j, pz), ccw))

    def arc_cw(self, x=None, y=None, z=None, i=None, j=None, k=None, r=None):
        self.arc(x, y, z, i, j, k, r, False)

    def arc_ccw(self, x=None, y=None, z=None, i=None, j=None, k=None, r=None):
        self.arc(x, y, z, i, j, k, r, True)

    def dwell(self, t):
        self.cut_path()
        self.original.dwell(t)

    def rapid_home(self, x=None, y=None, z=None, a=None, b=None, c=None):
        self.cut_path()
        self.original.rapid_home(x, y, z, a, b, c)

    def rapid_unhome(self):
        self.cut_path()
        self.original.rapid_unhome()

    ############################################################################
    ##  Cutter radius compensation

    def use_CRC(self):
        return self.original.use_CRC()

    def start_CRC(self, left = True, radius = 0.0):
        self.cut_path()
        self.original.start_CRC(left, radius)

    def end_CRC(self):
        self.cut_path()
        self.original.end_CRC()

    ############################################################################
    ##  Cycles

    def pattern(self):
        self.cut_path()
        self.original.pattern()

    def pocket(self):
        self.cut_path()
        self.original.pocket()

    def profile(self):
        self.cut_path()
        self.original.profile()

    def circular_pocket(self, x=None, y=None, ToolDiameter=None, HoleDiameter=None, ClearanceHeight=None, StartHeight=None, MaterialTop=None, FeedRate=None, SpindleRPM=None, HoleDepth=None, DepthOfCut=None, StepOver=None ):
        self.cut_path()
        self.circular_pocket(x, y, ToolDiameter, HoleDiameter, ClearanceHeight, StartHeight, MaterialTop, FeedRate, SpindleRPM, HoleDepth, DepthOfCut, StepOver)

    def drill(self, x=None, y=None, z=None, depth=None, standoff=None, dwell=None, peck_depth=None,retract_mode=None, spindle_mode=None):
        self.cut_path()
        self.original.drill(x, y, z, depth, standoff, dwell, peck_depth, retract_mode, spindle_mode)

    # argument list adapted for compatibility with Tapping module
    # wild guess - I'm unsure about the purpose of this file and wether this works -haberlerm
    def tap(self, x=None, y=None, z=None, zretract=None, depth=None, standoff=None, dwell_bottom=None, pitch=None, stoppos=None, spin_in=None, spin_out=None, tap_mode=None, direction=None):
        self.cut_path()
        self.original.tap( x, y, self.z2(z), self.z2(zretract), depth, standoff, dwell_bottom, pitch, stoppos, spin_in, spin_out, tap_mode, direction)


    def bore(self, x=None, y=None, z=None, zretract=None, depth=None, standoff=None, dwell_bottom=None, feed_in=None, feed_out=None, stoppos=None, shift_back=None, shift_right=None, backbore=False, stop=False):
        self.cut_path()
        self.original.bore(x, y, self.z2(z), self.z2(zretract), depth, standoff, dwell_Bottom, feed_in, feed_out, stoppos, shift_back, shift_right, backbore, stop)

    ############################################################################
    ##  Misc

    def comment(self, text):
        self.cut_path()
        self.original.comment(text)

    def variable(self, id):
        self.cut_path()
        self.original.variable(id)

    def variable_set(self, id, value):
        self.cut_path()
        self.original.variable_set(id, value)

################################################################################

def attach_begin():
    global attached
    if attached == True:
        attach_end()
    nc.creator = Creator(nc.creator)
    attached = True
    nc.creator.pdcf = None
    nc.creator.path = None

def attach_end():
    global attached
    nc.creator.cut_path()
    nc.creator = nc.creator.original
    attached = False
