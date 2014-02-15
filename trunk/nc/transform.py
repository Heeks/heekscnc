################################################################################
# transform.py
#
# NC code creator for attaching Z coordinates to a surface
#

import nc
import area

transformed = False

class FeedXY:
    def __init__(self, x, y):
        self.x = x
        self.y = y
        
    def Do(self, original, matrix):
        x,y,z = matrix.TransformedPoint(self.x, self.y, 0)
        original.feed(x, y)

class FeedZ:
    def __init__(self, z):
        self.z = z
        
    def Do(self, original, matrix):
        original.feed(z = self.z)
        
class FeedXYZ:
    def __init__(self, x, y, z):
        self.x = x
        self.y = y
        self.z = z
        
    def Do(self, original, matrix):
        x,y,z = matrix.TransformedPoint(self.x, self.y, self.z)
        original.feed(x,y,z)

class RapidXY:
    def __init__(self, x, y):
        self.x = x
        self.y = y
        
    def Do(self, original, matrix):
        x,y,z = matrix.TransformedPoint(self.x, self.y, 0)
        original.rapid(x, y)

class RapidZ:
    def __init__(self, z):
        self.z = z
        
    def Do(self, original, matrix):
        original.rapid(z = self.z)
        
class RapidXYZ:
    def __init__(self, x, y, z):
        self.x = x
        self.y = y
        self.z = z
        
    def Do(self, original, matrix):
        x,y,z = matrix.TransformedPoint(self.x, self.y, self.z)
        original.rapid(x,y,z)
        
class Feedrate:
    def __init__(self, h, v):
        self.h = h
        self.v = v
        
    def Do(self, original, matrix):
        original.feedrate_hv(self.h, self.v)
        
class Arc:
    def __init__(self, x, y, z, i, j, ccw):
        self.x = x
        self.y = y
        self.z = z
        self.i = i
        self.j = j
        self.ccw = ccw
        
    def Do(self, original, matrix):
        x,y,z = matrix.TransformedPoint(self.x, self.y, self.z)
        cx,cy,cz = matrix.TransformedPoint(original.x + self.i, original.y + self.j, self.z)
        i = cx - original.x
        j = cy - original.y
        if self.ccw:
            original.arc_ccw(x, y, z, i, j)
        else:
            original.arc_cw(x, y, z, i, j)
            
class Drill:
    def __init__(self, x, y, z, depth, standoff, dwell, peck_depth, retract_mode, spindle_mode, internal_coolant_on):
        self.x = x
        self.y = y
        self.z = z
        self.depth = depth
        self.standoff = standoff
        self.dwell = dwell
        self.peck_depth = peck_depth
        self.retract_mode = retract_mode
        self.spindle_mode = spindle_mode
        self.internal_coolant_on = internal_coolant_on
        
    def Do(self, original, matrix):
        x,y,z = matrix.TransformedPoint(self.x, self.y, self.z)
        original.drill(x, y, z, self.depth, self.standoff, self.dwell, self.peck_depth, self.retract_mode, self.spindle_mode, self.clearance_height)
    

################################################################################
class Creator:
    def __init__(self, original, matrix_list):
        self.original = original
        self.matrix_list = matrix_list
        self.commands = []
        self.x = original.x
        self.y = original.y
        self.z = original.z

    ############################################################################

    def DoAllCommands(self):
        for matrix in self.matrix_list:
            for command in self.commands:
                command.Do(self.original, matrix)

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
        self.cut_path()
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
        if id != self.original.current_tool():
            self.DoAllCommands()
            self.commands = []
        self.original.tool_change(id)

    def tool_defn(self, id, name='', radius=None, length=None, gradient=None):
        self.original.tool_defn(id, name, radius, length, gradient)

    def offset_radius(self, id, radius=None):
        self.original.offset_radius(id, radius)

    def offset_length(self, id, length=None):
        self.original.offset_length(id, length)
        
    def current_tool(self):
        return self.original.current_tool()

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
        self.commands.append(Feedrate(f, f))

    def feedrate_hv(self, fh, fv):
        self.commands.append(Feedrate(fh, fv))
        
    def spindle(self, s, clockwise=True):
        self.original.spindle(s, clockwise)

    def coolant(self, mode=0):
        self.original.coolant(mode)

    def gearrange(self, gear=0):
        self.original.gearrange(gear)

    ############################################################################
    ##  Moves

    def rapid(self, x=None, y=None, z=None, a=None, b=None, c=None):
        if x != None: self.x = x
        if y != None: self.y = y
        if z != None: self.z = z
        if self.x == None or self.y == None or self.z == None:
            return
        if z == None:
            self.commands.append(RapidXY(self.x, self.y))
        elif x == None and y == None:
            self.commands.append(RapidZ(self.z))
        else:
            self.commands.append(RapidXYZ(self.x, self.y, self.z))
        
    def feed(self, x=None, y=None, z=None, a = None, b = None, c = None):
        if x != None: self.x = x
        if y != None: self.y = y
        if z != None: self.z = z
        if self.x == None or self.y == None or self.z == None:
            return
        if z == None:
            self.commands.append(FeedXY(self.x, self.y))
        elif x == None and y == None:
            self.commands.append(FeedZ(self.z))
        else:
            self.commands.append(FeedXYZ(self.x, self.y, self.z))
        
    def arc(self, x=None, y=None, z=None, i=None, j=None, k=None, r=None, ccw = True):
        if x != None: self.x = x
        if y != None: self.y = y
        if z != None: self.z = z
        if self.x == None or self.y == None or self.z == None:
            return
        self.commands.append(Arc(self.x, self.y, self.z, i, j, ccw))
        
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

    def drill(self, x=None, y=None, dwell=None, depthparams = None, retract_mode=None, spindle_mode=None, internal_coolant_on=None):
        self.commands.append(Drill(x, y, dwell, depthparams, spindle_mode, internal_coolant_on))

    # argument list adapted for compatibility with Tapping module
    # wild guess - I'm unsure about the purpose of this file and wether this works -haberlerm
    def tap(self, x=None, y=None, z=None, zretract=None, depth=None, standoff=None, dwell_bottom=None, pitch=None, stoppos=None, spin_in=None, spin_out=None, tap_mode=None, direction=None):
        self.original.tap( x, y, self.z2(z), self.z2(zretract), depth, standoff, dwell_bottom, pitch, stoppos, spin_in, spin_out, tap_mode, direction)


    def bore(self, x=None, y=None, z=None, zretract=None, depth=None, standoff=None, dwell_bottom=None, feed_in=None, feed_out=None, stoppos=None, shift_back=None, shift_right=None, backbore=False, stop=False):
        self.original.bore(x, y, self.z2(z), self.z2(zretract), depth, standoff, dwell_Bottom, feed_in, feed_out, stoppos, shift_back, shift_right, backbore, stop)

    def end_canned_cycle(self):
        self.original.end_canned_cycle()

    ############################################################################
    ##  Misc

    def comment(self, text):
        self.original.comment(text)

    def variable(self, id):
        self.original.variable(id)

    def variable_set(self, id, value):
        self.original.variable_set(id, value)
        

    def set_ocl_cutter(self, cutter):
        self.original.set_ocl_cutter(cutter)

################################################################################

def transform_begin(matrix_list):
    global transformed
    if transformed == True:
        transform_end()
    nc.creator = Creator(nc.creator, matrix_list)
    transformed = True

def transform_end():
    global transformed
    nc.creator.DoAllCommands()
    nc.creator = nc.creator.original
    transformed = False
