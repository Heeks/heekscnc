# Preliminary postprocessor support for Anilam Crusader M CNC controller
# This code modified from iso.py and emc2.py distriuted with HeeksCAD as of Sep 2010
# Kurt Jensen 6 Sep 2010
# Use at your own risk.
import nc
import iso
import iso_codes

class CodesAnilamCM(iso_codes.Codes):
    def SPACE(self): return(' ')

    # This version of COMMENT removes comments from the resultant GCode
    # Note: The Anilam hates comments when importing code.
    def COMMENT(self,comment): return('')

iso_codes.codes = CodesAnilamCM()
#anilam_codes = CodesAnilamCM()
#print 'Foo bar'

class CreatorAnilamCM(iso.CreatorIso):
    def init(self): 
        iso.CreatorIso.init(self)

    def program_begin(self, id, comment):
        self.write('%\n');  # Start of file token that Anilam Crusader M likes
        # No Comments for the Anilam crusaher M, please......
        #self.write( ('(' + comment + ')' + '\n') )
        
    def program_end(self):
        self.write_blocknum()
        self.write('G29E\n')  # End of code signal for Anilam Crusader M
        self.write('%\n')     # EOF signal for Anilam Crusader M

    ############################################################################
    ##  Settings
    
    def imperial(self):
        self.write_blocknum()
        #self.write( iso_codes.codes.IMPERIAL() + '\t (Imperial Values)\n')
        self.write( iso_codes.codes.IMPERIAL() + '\n')
        self.fmt = iso_codes.codes.FORMAT_IN()

    def metric(self):
        self.write_blocknum()
        #self.write( iso_codes.codes.METRIC() + '\t (Metric Values)\n' )
        self.write( iso_codes.codes.METRIC() + '\n' )
        self.fmt = iso_codes.codes.FORMAT_MM()

    def absolute(self):
        self.write_blocknum()
        #self.write( iso_codes.codes.ABSOLUTE() + '\t (Absolute Coordinates)\n')
        self.write( iso_codes.codes.ABSOLUTE() + '\n')

    def incremental(self):
        self.write_blocknum()
        #self.write( iso_codes.codes.INCREMENTAL() + '\t (Incremental Coordinates)\n' )
        self.write( iso_codes.codes.INCREMENTAL() + '\n' )

    def polar(self, on=True):
        if (on) :
            self.write_blocknum()
            #self.write(iso_codes.codes.POLAR_ON() + '\t (Polar ON)\n' )
            self.write(iso_codes.codes.POLAR_ON() + '\n' )
        else : 
            self.write_blocknum()
            #self.write(iso_codes.codes.POLAR_OFF() + '\t (Polar OFF)\n' )
            self.write(iso_codes.codes.POLAR_OFF() + '\n' )

    def set_plane(self, plane):
        if (plane == 0) : 
            self.write_blocknum()
            #self.write(iso_codes.codes.PLANE_XY() + '\t (Select XY Plane)\n')
            self.write(iso_codes.codes.PLANE_XY() + '\n')
        elif (plane == 1) :
            self.write_blocknum()
            #self.write(iso_codes.codes.PLANE_XZ() + '\t (Select XZ Plane)\n')
            self.write(iso_codes.codes.PLANE_XZ() + '\n')
        elif (plane == 2) : 
            self.write_blocknum()
            #self.write(iso_codes.codes.PLANE_YZ() + '\t (Select YZ Plane)\n')
            self.write(iso_codes.codes.PLANE_YZ() + '\n')

    def comment(self, text):
       self.write_blocknum()
       #self.write((iso_codes.codes.COMMENT(text) + '\n'))
       
    ############################################################################
    ##  Tools

    def tool_change(self, id):
        self.write_blocknum()
        #self.write((iso.codes.TOOL() % id) + '\n')
        self.write(('T%i' % id) + '\n')
        self.t = id

    def tool_defn(self, id, name='', radius=None, length=None, gradient=None):
        self.write_blocknum()
        #self.write(iso.codes.TOOL_DEFINITION())
        self.write(('T10%.2d' % id) + ' ')

        if (radius != None):
            self.write(('X%.3f' % radius) + ' ')

        if (length != None):
            self.write('Z%.3f' % length)

        self.write('\n')   

    # This is the coordinate system we're using.  G54->G59, G59.1, G59.2, G59.3
    # These are selected by values from 1 to 9 inclusive.
    def workplane(self, id):
        if ((id >= 1) and (id <= 6)):
            self.write_blocknum()
            #self.write( (iso_codes.codes.WORKPLANE() % (id + iso_codes.codes.WORKPLANE_BASE())) + '\t (Select Relative Coordinate System)\n')
            self.write( (iso_codes.codes.WORKPLANE() % (id + iso_codes.codes.WORKPLANE_BASE())) + '\n')
        if ((id >= 7) and (id <= 9)):
            self.write_blocknum()
            #self.write( ((iso_codes.codes.WORKPLANE() % (6 + iso_codes.codes.WORKPLANE_BASE())) + ('.%i' % (id - 6))) + '\t (Select Relative Coordinate System)\n')
            self.write( ((iso_codes.codes.WORKPLANE() % (6 + iso_codes.codes.WORKPLANE_BASE())) + ('.%i' % (id - 6))) + '\n')
    
    # inhibit N codes being generated for line numbers:
    def write_blocknum(self): 
        pass
        
    # The Anilam crusader wants absolute arc centers, not relative arc centers.  So, add the arc start point to the I,J,K parameter output value        
    def arc(self, cw, x=None, y=None, z=None, i=None, j=None, k=None, r=None):
        if self.same_xyz(x, y, z): return
        #Note: The tool position state of the last block processed is stored
        # in self.x, self.y, and self.z.  We make a local copy of these before we overwrite them
        # them to make the conversion from relative to absolute arc centers.
        # IJK in G2 and G3 are relative to the current tool position, and *not*
        # relative to the target tool position passed in as parameters to this method.
        oldx = self.x
        oldy = self.y
        oldz = self.z
        self.write_blocknum()
        arc_g_code = ''
        if cw:
            arc_g_code = iso_codes.codes.ARC_CW()
        else:
            arc_g_code = iso_codes.codes.ARC_CCW()
        if self.g0123_modal:
            if self.prev_g0123 != arc_g_code:
                self.write(arc_g_code)
                self.prev_g0123 = arc_g_code
        else:
            self.write(arc_g_code)
        self.write_preps()
        if (x != None):
            self.write(iso_codes.codes.X() + (self.fmt % x))
            self.x = x
        if (y != None):
            self.write(iso_codes.codes.Y() + (self.fmt % y))
            self.y = y
        if (z != None):
            self.write(iso_codes.codes.Z() + (self.fmt % z))
            self.z = z
        if (i != None) : self.write(iso_codes.codes.CENTRE_X() + (self.fmt % (i+oldx)))
        if (j != None) : self.write(iso_codes.codes.CENTRE_Y() + (self.fmt % (j+oldy)))
        if (k != None) : self.write(iso_codes.codes.CENTRE_Z() + (self.fmt % (k+oldz)))
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

    def drill(self, x=None, y=None, z=None, depth=None, standoff=None, dwell=None, peck_depth=None,retract_mode=None, spindle_mode=None):
        self.write('(Canned drill cycle ops are not yet supported here on this Anilam Crusader M postprocessor)')

nc.creator = CreatorAnilamCM()

