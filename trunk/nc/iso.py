################################################################################
# iso.py
#
# Simple ISO NC code creator
#
# Hirutso Enni, 2009-01-13

import iso_codes as iso
import nc
import math
import circular_pocket as circular

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

        self.fmt = iso.codes.FORMAT_MM()

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
        self.write(iso.codes.BLOCK() % self.n)
        self.write(iso.codes.SPACE())
        self.n += 10

    def write_spindle(self):
        self.write(self.s)
        self.s = ''

    ############################################################################
    ##  Programs

    def program_begin(self, id, name=''):
        self.write((iso.codes.PROGRAM() % id) + iso.codes.SPACE() + (iso.codes.COMMENT(name)))
        self.write('\n')

    def program_stop(self, optional=False):
        self.write_blocknum()
        if (optional) : self.write(iso.codes.STOP_OPTIONAL() + '\n')
        else : self.write(iso.codes.STOP() + '\n')

    def program_end(self):
        self.write_blocknum()
        self.write(iso.codes.PROGRAM_END() + '\n')

    def flush_nc(self):
        if len(self.g) == 0 and len(self.m) == 0: return
        self.write_blocknum()
        self.write_preps()
        self.write_misc()
        self.write('\n')

    ############################################################################
    ##  Subprograms
    
    def sub_begin(self, id, name=''):
        self.write((iso.codes.PROGRAM() % id) + iso.codes.SPACE() + (iso.codes.COMMENT(name)))
        self.write('\n')

    def sub_call(self, id):
        self.write_blocknum()
        self.write((iso.codes.SUBPROG_CALL() % id) + '\n')

    def sub_end(self):
        self.write_blocknum()
        self.write(iso.codes.SUBPROG_END() + '\n')

    ############################################################################
    ##  Settings
    
    def imperial(self):
        self.g += iso.codes.IMPERIAL()
        self.fmt = iso.codes.FORMAT_IN()

    def metric(self):
        self.g += iso.codes.METRIC()
        self.fmt = iso.codes.FORMAT_MM()

    def absolute(self):
        self.g += iso.codes.ABSOLUTE()

    def incremental(self):
        self.g += iso.codes.INCREMENTAL()

    def polar(self, on=True):
        if (on) : self.g += iso.codes.POLAR_ON()
        else : self.g += iso.codes.POLAR_OFF()

    def set_plane(self, plane):
        if (plane == 0) : self.g += iso.codes.PLANE_XY()
        elif (plane == 1) : self.g += iso.codes.PLANE_XZ()
        elif (plane == 2) : self.g += iso.codes.PLANE_YZ()

    def set_temporary_origin(self, x=None, y=None, z=None, a=None, b=None, c=None):
	self.write_blocknum()
	self.write((iso.codes.SET_TEMPORARY_COORDINATE_SYSTEM()))
	if (x != None): self.write( iso.codes.SPACE() + 'X ' + (self.fmt % x) )
	if (y != None): self.write( iso.codes.SPACE() + 'Y ' + (self.fmt % y) )
	if (z != None): self.write( iso.codes.SPACE() + 'Z ' + (self.fmt % z) )
	if (a != None): self.write( iso.codes.SPACE() + 'A ' + (self.fmt % a) )
	if (b != None): self.write( iso.codes.SPACE() + 'B ' + (self.fmt % b) )
	if (c != None): self.write( iso.codes.SPACE() + 'C ' + (self.fmt % c) )
	self.write('\n')

    def remove_temporary_origin(self):
	self.write_blocknum()
	self.write((iso.codes.REMOVE_TEMPORARY_COORDINATE_SYSTEM()))
	self.write('\n')

    ############################################################################
    ##  Tools

    def tool_change(self, id):
        self.write_blocknum()
        self.write((iso.codes.TOOL() % id) + '\n')

    def tool_defn(self, id, name='', radius=None, length=None):
        self.write_blocknum()
	self.write(iso.codes.TOOL_DEFINITION())
	self.write(('P%i' % id) + ' ')

	if (radius != None):
		self.write(('R%.3f' % radius) + ' ')

	if (length != None):
		self.write('Z%.3f' % length)

	self.write('\n')

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

    # This is the coordinate system we're using.  G54->G59, G59.1, G59.2, G59.3
    # These are selected by values from 1 to 9 inclusive.
    def workplane(self, id):
	if ((id >= 1) and (id <= 6)):
		self.g += iso.codes.WORKPLANE() % (id + iso.codes.WORKPLANE_BASE())
	if ((id >= 7) and (id <= 9)):
		self.g += ((iso.codes.WORKPLANE() % (6 + iso.codes.WORKPLANE_BASE())) + ('.%i' % (id - 6)))

    ############################################################################
    ##  Rates + Modes

    def feedrate(self, f):
        self.f = iso.codes.FEEDRATE() + (self.fmt % f)
        self.fhv = False

    def feedrate_hv(self, fh, fv):
        self.fh = fh
        self.fv = fv
        self.fhv = True

    def calc_feedrate_hv(self, h, v):
        l = math.sqrt(h*h+v*v)
        if (h == 0) : self.f = iso.codes.FEEDRATE() + (self.fmt % self.fv)
        elif (v == 0) : self.f = iso.codes.FEEDRATE() + (self.fmt % self.fh)
        else:
            self.f = iso.codes.FEEDRATE() + (self.fmt % (self.fh * l * min([1/h, 1/v])))

    def spindle(self, s, clockwise):
	if s < 0: clockwise = not clockwise
	s = abs(s)
        self.s = iso.codes.SPINDLE(iso.codes.FORMAT_ANG(), s)
	if clockwise:
		self.s = self.s + iso.codes.SPINDLE_CW()
	else:
		self.s = self.s + iso.codes.SPINDLE_CCW()

    def coolant(self, mode=0):
        if (mode <= 0) : self.m.append(iso.codes.COOLANT_OFF())
        elif (mode == 1) : self.m.append(iso.codes.COOLANT_MIST())
        elif (mode == 2) : self.m.append(iso.codes.COOLANT_FLOOD())

    def gearrange(self, gear=0):
        if (gear <= 0) : self.m.append(iso.codes.GEAR_OFF())
        elif (gear <= 4) : self.m.append(iso.codes.GEAR() % (gear + GEAR_BASE()))

    ############################################################################
    ##  Moves

    def rapid(self, x=None, y=None, z=None, a=None, b=None, c=None):
        self.write_blocknum()
        self.write(iso.codes.RAPID())
        self.write_preps()
        if (x != None):
            dx = x - self.x
            self.write(iso.codes.X() + (self.fmt % x))
            self.x = x
        if (y != None):
            dy = y - self.y
            self.write(iso.codes.Y() + (self.fmt % y))
            self.y = y
        if (z != None):
            dz = z - self.z
            self.write(iso.codes.Z() + (self.fmt % z))
            self.z = z
        if (a != None) : self.write(iso.codes.A() + (iso.codes.FORMAT_ANG() % a))
        if (b != None) : self.write(iso.codes.B() + (iso.codes.FORMAT_ANG() % b))
        if (c != None) : self.write(iso.codes.C() + (iso.codes.FORMAT_ANG() % c))
        self.write_spindle()
        self.write_misc()
        self.write('\n')

    def feed(self, x=None, y=None, z=None):
        if self.same_xyz(x, y, z): return
        self.write_blocknum()
        self.write(iso.codes.FEED())
        self.write_preps()
        dx = dy = dz = 0
        if (x != None):
            dx = x - self.x
            self.write(iso.codes.X() + (self.fmt % x))
            self.x = x
        if (y != None):
            dy = y - self.y
            self.write(iso.codes.Y() + (self.fmt % y))
            self.y = y
        if (z != None):
            dz = z - self.z
            self.write(iso.codes.Z() + (self.fmt % z))
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
        if cw: self.write(iso.codes.ARC_CW())
        else: self.write(iso.codes.ARC_CCW())
        self.write_preps()
        if (x != None):
            self.write(iso.codes.X() + (self.fmt % x))
            self.x = x
        if (y != None):
            self.write(iso.codes.Y() + (self.fmt % y))
            self.y = y
        if (z != None):
            self.write(iso.codes.Z() + (self.fmt % z))
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

    def dwell(self, t):
        self.write_blocknum()
        self.write_preps()
        self.write(iso.codes.DWELL() + (iso.codes.TIME() % t))
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

    def circular_pocket(self, x=None, y=None, ToolDiameter=None, HoleDiameter=None, ClearanceHeight=None, StartHeight=None, MaterialTop=None, FeedRate=None, SpindleRPM=None, HoleDepth=None, DepthOfCut=None, StepOver=None ):
	self.write_preps()
	circular.pocket.block_number = self.n
	(self.g_code, self.n) = circular.pocket.GeneratePath( x,y, ToolDiameter, HoleDiameter, ClearanceHeight, StartHeight, MaterialTop, FeedRate, SpindleRPM, HoleDepth, DepthOfCut, StepOver )
	self.write(self.g_code)

	# The drill routine supports drilling (G81), drilling with dwell (G82) and peck drilling (G83).
	# The x,y,z values are INITIAL locations (above the hole to be made.  This is in contrast to
	# the Z value used in the G8[1-3] cycles where the Z value is that of the BOTTOM of the hole.
	# Instead, this routine combines the Z value and the depth value to determine the bottom of
	# the hole.
	#
	# The standoff value is the distance up from the 'z' value (normally just above the surface) where the bit retracts
	# to in order to clear the swarf.  This combines with 'z' to form the 'R' value in the G8[1-3] cycles.
	#
	# The peck_depth value is the incremental depth (Q value) that tells the peck drilling
	# cycle how deep to go on each peck until the full depth is achieved.
	#
	# NOTE: This routine forces the mode to absolute mode so that the values  passed into
	# the G8[1-3] cycles make sense.  I don't know how to find the mode to revert it so I won't
	# revert it.  I must set the mode so that I can be sure the values I'm passing in make
	# sense to the end-machine.
	#
    def drill(self, x=None, y=None, z=None, depth=None, standoff=None, dwell=None, peck_depth=None):
	if (standoff == None):
		# This is a bad thing.  All the drilling cycles need a retraction (and starting) height.
		return

	if (z == None): return	# We need a Z value as well.  This input parameter represents the top of the hole

        self.write_blocknum()
        self.write_preps()

	if (peck_depth != 0):
		# We're pecking.  Let's find a tree.
		self.write(iso.codes.PECK_DRILL() + iso.codes.PECK_DEPTH(self.fmt, peck_depth))
	else:
		# We're either just drilling or drilling with dwell.
		if (dwell == 0):
			# We're just drilling.
			self.write(iso.codes.DRILL())
		else:
			# We're drilling with dwell.
			self.write(iso.codes.DRILL_WITH_DWELL(iso.codes.FORMAT_DWELL(),dwell))

	# Set the retraction point to the 'standoff' distance above the starting z height.
	retract_height = z + standoff
        if (x != None):
            dx = x - self.x
            self.write(iso.codes.X() + (self.fmt % x))
            self.x = x
        if (y != None):
            dy = y - self.y
            self.write(iso.codes.Y() + (self.fmt % y))
            self.y = y

	dz = (z + standoff) - self.z
			# In the end, we will be standoff distance above the z value passed in.
	self.write(iso.codes.Z() + (self.fmt % (z - depth)))	# This is the 'z' value for the bottom of the hole.
	self.z = (z + standoff)				# We want to remember where z is at the end (at the top of the hole)
	self.write(iso.codes.RETRACT(self.fmt, retract_height))

        if (self.fhv) : self.calc_feedrate_hv(math.sqrt(dx*dx+dy*dy), math.fabs(dz))
        self.write( iso.codes.FEEDRATE() + (self.fmt % self.fv) + iso.codes.SPACE() )

        self.write_spindle()

        self.write_misc()
        self.write('\n')


    def tap(self, x=None, y=None, z=None, zretract=None, depth=None, standoff=None, dwell_bottom=None, pitch=None, stoppos=None, spin_in=None, spin_out=None):
        pass

    def bore(self, x=None, y=None, z=None, zretract=None, depth=None, standoff=None, dwell_bottom=None, feed_in=None, feed_out=None, stoppos=None, shift_back=None, shift_right=None, backbore=False, stop=False):
        pass

    ############################################################################
    ##  Misc

    def comment(self, text):
        self.write((iso.codes.COMMENT(text) + '\n'))

    def variable(self, id):
        return (iso.codes.VARIABLE() % id)

    def variable_set(self, id, value):
        self.write_blocknum()
        self.write((iso.codes.VARIABLE() % id) + (iso.codes.VARIABLE_SET() % value) + '\n')

    # This routine uses the G92 coordinate system offsets to establish a temporary coordinate
    # system at the machine's current position.  It can then use absolute coordinates relative
    # to this position which makes coding easy.  It then moves to the 'point along edge' which
    # should be above the workpiece but still on one edge.  It then backs off from the edge
    # to the 'retracted point'.  It then plunges down by the depth value specified.  It then
    # probes back towards the 'destination point'.  The probed X,Y location are stored
    # into the 'intersection variable' variables.  Finally the machine moves back to the
    # original location.  This is important so that the results of multiple calls to this
    # routine may be compared meaningfully.
    def probe_single_point(self, point_along_edge_x=None, point_along_edge_y=None, depth=None, retracted_point_x=None, retracted_point_y=None, destination_point_x=None, destination_point_y=None, intersection_variable_x=None, intersection_variable_y=None, probe_radius_x_component=None, probe_radius_y_component=None ):
	self.write_blocknum()
	self.write((iso.codes.SET_TEMPORARY_COORDINATE_SYSTEM() + (' X 0 Y 0 Z 0') + ('\t(Temporarily make this the origin)\n')))
	if (self.fhv) : self.calc_feedrate_hv(1, 0)
	self.write_blocknum()
	self.write_feedrate()
	self.write('\t(Set the feed rate for probing)\n')

	self.rapid(point_along_edge_x,point_along_edge_y)
	self.rapid(retracted_point_x,retracted_point_y)
	self.rapid(z=depth)

	self.write_blocknum()
	self.write((iso.codes.PROBE_TOWARDS_WITH_SIGNAL() + (' X ' + (self.fmt % destination_point_x) + ' Y ' + (self.fmt % destination_point_y) ) + ('\t(Probe towards our destination point)\n')))

	self.comment('Back off the workpiece and re-probe more slowly')
	self.write_blocknum()
	self.write(('#' + intersection_variable_x + '= [#5061 - [ 2.0 * ' + probe_radius_x_component + ']]\n'))
	self.write_blocknum()
	self.write(('#' + intersection_variable_y + '= [#5062 - [ 2.0 * ' + probe_radius_y_component + ']]\n'))
    	self.write_blocknum();
	self.write(iso.codes.RAPID())
	self.write(' X #' + intersection_variable_x + ' Y #' + intersection_variable_y + '\n')

	self.write_blocknum()
	self.write(iso.codes.FEEDRATE() + (self.fmt % (self.fh / 2.0)) + '\n')

	self.write_blocknum()
	self.write((iso.codes.PROBE_TOWARDS_WITH_SIGNAL() + (' X ' + (self.fmt % destination_point_x) + ' Y ' + (self.fmt % destination_point_y) ) + ('\t(Probe towards our destination point)\n')))

	self.comment('Store the probed location somewhere we can get it again later')
	self.write_blocknum()
	self.write(('#' + intersection_variable_x + '=' + probe_radius_x_component + ' (Portion of probe radius that contributes to the X coordinate)\n'))
	self.write_blocknum()
	self.write(('#' + intersection_variable_x + '=[#' + intersection_variable_x + ' + #5061]\n'))
	self.write_blocknum()
	self.write(('#' + intersection_variable_y + '=' + probe_radius_y_component + ' (Portion of probe radius that contributes to the Y coordinate)\n'))
	self.write_blocknum()
	self.write(('#' + intersection_variable_y + '=[#' + intersection_variable_y + ' + #5062]\n'))

	self.comment('Now move back to the original location')
	self.rapid(retracted_point_x,retracted_point_y)
	self.rapid(z=0)
	self.rapid(point_along_edge_x,point_along_edge_y)
	self.rapid(x=0, y=0)

	self.write_blocknum()
	self.write((iso.codes.REMOVE_TEMPORARY_COORDINATE_SYSTEM() + ('\t(Restore the previous coordinate system)\n')))

    def probe_downward_point(self, x=None, y=None, depth=None, intersection_variable_z=None):
	self.write_blocknum()
	self.write((iso.codes.SET_TEMPORARY_COORDINATE_SYSTEM() + (' X 0 Y 0 Z 0') + ('\t(Temporarily make this the origin)\n')))
	if (self.fhv) : self.calc_feedrate_hv(1, 0)
	self.write_blocknum()
	self.write(iso.codes.FEEDRATE() + ' [' + (self.fmt % self.fh) + ' / 5.0 ]')
	self.write('\t(Set the feed rate for probing)\n')

    	self.write_blocknum();
	self.write(iso.codes.RAPID())
	self.write(' X ' + x + ' Y ' + y + '\n')

	self.write_blocknum()
	self.write((iso.codes.PROBE_TOWARDS_WITH_SIGNAL() + ' Z ' + (self.fmt % depth) + ('\t(Probe towards our destination point)\n')))

	self.comment('Store the probed location somewhere we can get it again later')
	self.write_blocknum()
	self.write(('#' + intersection_variable_z + '= #5063\n'))

	self.comment('Now move back to the original location')
	self.rapid(z=0)
	self.rapid(x=0, y=0)

	self.write_blocknum()
	self.write((iso.codes.REMOVE_TEMPORARY_COORDINATE_SYSTEM() + ('\t(Restore the previous coordinate system)\n')))


    def report_probe_results(self, x1=None, y1=None, z1=None, x2=None, y2=None, z2=None, x3=None, y3=None, z3=None, x4=None, y4=None, z4=None, x5=None, y5=None, z5=None, x6=None, y6=None, z6=None, xml_file_name=None ):
	pass

    # Rapid movement to the midpoint between the two points specified.
    # NOTE: The points are specified either as strings representing numbers or as strings
    # representing variable names.  This allows the HeeksCNC module to determine which
    # variable names are used in these various routines.
    def rapid_to_midpoint(self, x1, y1, z1, x2, y2, z2):
	self.write_blocknum()
	self.write(iso.codes.RAPID())
	self.write((' X ' + '[[[' + x1 + '-' + x2 + '] / 2.0] + ' + x2 + ']'))
	self.write((' Y ' + '[[[' + y1 + '-' + y2 + '] / 2.0] + ' + y2 + ']'))
	self.write((' Z ' + '[[[' + z1 + '-' + z2 + '] / 2.0] + ' + z2 + ']'))
	self.write('\n')

    # Rapid movement to the intersection of two lines (in the XY plane only). This routine
    # is based on information found in http://local.wasp.uwa.edu.au/~pbourke/geometry/lineline2d/
    # written by Paul Bourke.  The ua_numerator, ua_denominator, ua and ub parameters
    # represent variable names (with the preceding '#' included in them) for use as temporary
    # variables.  They're specified here simply so that HeeksCNC can manage which variables
    # are used in which GCode calculations.
    #
    # As per the notes on the web page, the ua_denominator and ub_denominator formulae are
    # the same so we don't repeat this.  If the two lines are coincident or parallel then
    # no movement occurs.
    #
    # NOTE: The points are specified either as strings representing numbers or as strings
    # representing variable names.  This allows the HeeksCNC module to determine which
    # variable names are used in these various routines.
    def rapid_to_intersection(self, x1, y1, x2, y2, x3, y3, x4, y4, intersection_x, intersection_y, ua_numerator, ua_denominator, ua, ub_numerator, ub):
	self.comment('Find the intersection of the two lines made up by the four probed points')
    	self.write_blocknum();
	self.write(ua_numerator + '=[[[' + x4 + '-' + x3 + '] * [' + y1 + '-' + y3 + ']] - [[' + y4 + '-' + y3 + '] * [' + x1 + '-' + x3 + ']]]\n')
    	self.write_blocknum();
	self.write(ua_denominator + '=[[[' + y4 + '-' + y3 + '] * [' + x2 + '-' + x1 + ']] - [[' + x4 + '-' + x3 + '] * [' + y2 + '-' + y1 + ']]]\n')
    	self.write_blocknum();
	self.write(ub_numerator + '=[[[' + x2 + '-' + x1 + '] * [' + y1 + '-' + y3 + ']] - [[' + y2 + '-' + y1 + '] * [' + x1 + '-' + x3 + ']]]\n')

	self.comment('If they are not parallel')
	self.write('O900 IF [' + ua_denominator + ' NE 0]\n')
	self.comment('And if they are not coincident')
	self.write('O901    IF [' + ua_numerator + ' NE 0 ]\n')

    	self.write_blocknum();
	self.write('       ' + ua + '=[' + ua_numerator + ' / ' + ua_denominator + ']\n')
    	self.write_blocknum();
	self.write('       ' + ub + '=[' + ub_numerator + ' / ' + ua_denominator + ']\n') # NOTE: ub denominator is the same as ua denominator
    	self.write_blocknum();
	self.write('       ' + intersection_x + '=[' + x1 + ' + [[' + ua + ' * [' + x2 + ' - ' + x1 + ']]]]\n')
    	self.write_blocknum();
	self.write('       ' + intersection_y + '=[' + y1 + ' + [[' + ua + ' * [' + y2 + ' - ' + y1 + ']]]]\n')
    	self.write_blocknum();
	self.write('       ' + iso.codes.RAPID())
	self.write(' X ' + intersection_x + ' Y ' + intersection_y + '\n')

	self.write('O901    ENDIF\n')
	self.write('O900 ENDIF\n')

	# We need to calculate the rotation angle based on the line formed by the
	# x1,y1 and x2,y2 coordinate pair.  With that angle, we need to move
	# x_offset and y_offset distance from the current (0,0,0) position.
	#
	# The x1,y1,x2 and y2 parameters are all variable names that contain the actual
	# values.
	# The x_offset and y_offset are both numeric (floating point) values
    def rapid_to_rotated_coordinate(self, x1, y1, x2, y2, ref_x, ref_y, x_current, y_current, x_final, y_final):
	self.comment('Rapid to rotated coordinate')
    	self.write_blocknum();
	self.write( '#1 = [atan[' + y2 + '-' + y1 + ']/[' + x2 +' - ' + x1 + ']] (nominal_angle)\n')
    	self.write_blocknum();
	self.write( '#2 = [atan[' + ref_y + ']/[' + ref_x + ']] (reference angle)\n')
    	self.write_blocknum();
	self.write( '#3 = [#1 - #2] (angle)\n' )
    	self.write_blocknum();
	self.write( '#4 = [[[' + (self.fmt % 0) + ' - ' + (self.fmt % x_current) + '] * COS[ #3 ]] - [[' + (self.fmt % 0) + ' - ' + (self.fmt % y_current) + '] * SIN[ #3 ]]]\n' )
    	self.write_blocknum();
	self.write( '#5 = [[[' + (self.fmt % 0) + ' - ' + (self.fmt % x_current) + '] * SIN[ #3 ]] + [[' + (self.fmt % 0) + ' - ' + (self.fmt % y_current) + '] * COS[ #3 ]]]\n' )

    	self.write_blocknum();
	self.write( '#6 = [[' + (self.fmt % x_final) + ' * COS[ #3 ]] - [' + (self.fmt % y_final) + ' * SIN[ #3 ]]]\n' )
    	self.write_blocknum();
	self.write( '#7 = [[' + (self.fmt % y_final) + ' * SIN[ #3 ]] + [' + (self.fmt % y_final) + ' * COS[ #3 ]]]\n' )

    	self.write_blocknum();
	self.write( iso.codes.RAPID() + ' X [ #4 + #6 ] Y [ #5 + #7 ]\n' )

################################################################################

nc.creator = CreatorIso()
