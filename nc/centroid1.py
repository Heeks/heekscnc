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
################################################################################
# general 

    def comment(self, text):
        self.write(';' + text +'\n')  
    def write_blocknum(self):
        pass 

################################################################################
# program begin and end

    def program_begin(self, id, name=''):
        self.write(';time:'+str(now)+'\n')
        #self.write('G17 G80 G40 G90\n')
        

    def program_end(self):
        self.write('M05\n')
        self.write('M25\n')
        self.write('G00 X-1.0 Y1.0\n')
        self.write('G17 G80 G40 G90\n')
        self.write('M99\n')
                     
################################################################################
# tool info
    def tool_defn(self, id, name='', radius=None, length=None):
        pass

    def write_spindle(self):
        pass


    def spindle(self, s, clockwise):
	if s < 0: clockwise = not clockwise
	s = abs(s)
        self.s = iso.codes.SPINDLE(iso.codes.FORMAT_ANG(), s)
	if clockwise:
                #self.s =  iso.codes.SPINDLE_CW() + self.s
		self.s =  iso.codes.SPINDLE_CW()                
                self.write(self.s +  '\n')
                self.write('G04 P2.0 \n')
	else:
		self.s =  iso.codes.SPINDLE_CCW() + self.s

################################################################################
# feedrate

    def imperial(self):
        self.g += iso.codes.IMPERIAL()
        self.fmt = iso.codes.FORMAT_IN()
        self.ffmt = iso.codes.FORMAT_FEEDRATE() 

    def feedrate(self, f):
               
        self.f = iso.codes.FEEDRATE() + (self.ffmt % f)
        self.fhv = False

    def feedrate_hv(self, fh, fv):
        self.fh = fh
        self.fv = fv
        self.fhv = True

    def calc_feedrate_hv(self, h, v):
        if math.fabs(v) > math.fabs(h * 2):
            # some horizontal, so it should be fine to use the horizontal feed rate
            self.f = iso.codes.FEEDRATE() + (self.ffmt % self.fv)
        else:
            # not much, if any horizontal component, so use the vertical feed rate
            self.f = iso.codes.FEEDRATE() + (self.ffmt % self.fh)

################################################################################
# drill cycle

    def drill(self, x=None, y=None, z=None, depth=None, standoff=None, dwell=None, peck_depth=None):
        if (standoff == None):        
        # This is a bad thing.  All the drilling cycles need a retraction (and starting) height.        
            return
           
        if (z == None): 
            return	# We need a Z value as well.  This input parameter represents the top of the hole    	          
        self.write_preps()
        self.write_blocknum()                
        
        if (peck_depth != 0):        
            # We're pecking.  Let's find a tree. 
                if self.drill_modal:       
                    if  iso.codes.PECK_DRILL() + iso.codes.PECK_DEPTH(self.ffmt, peck_depth) != self.prev_drill:
                        self.write(iso.codes.PECK_DRILL() + iso.codes.PECK_DEPTH(self.ffmt, peck_depth))  
                        self.prev_drill = iso.codes.PECK_DRILL() + iso.codes.PECK_DEPTH(self.ffmt, peck_depth)
                else:       
                    self.write(iso.codes.PECK_DRILL() + iso.codes.PECK_DEPTH(self.ffmt, peck_depth)) 
                           
        else:        
            # We're either just drilling or drilling with dwell.        
            if (dwell == 0):        
                # We're just drilling. 
                if self.drill_modal:       
                    if  iso.codes.DRILL() != self.prev_drill:
                        self.write(iso.codes.DRILL())  
                        self.prev_drill = iso.codes.DRILL()
                else:
                    self.write(iso.codes.DRILL())
      
            else:        
                # We're drilling with dwell.

                if self.drill_modal:       
                    if  iso.codes.DRILL_WITH_DWELL(iso.codes.FORMAT_DWELL(),dwell) != self.prev_drill:
                        self.write(iso.codes.DRILL_WITH_DWELL(iso.codes.FORMAT_DWELL(),dwell))  
                        self.prev_drill = iso.codes.DRILL_WITH_DWELL(iso.codes.FORMAT_DWELL(),dwell)
                else:
                    self.write(iso.codes.DRILL_WITH_DWELL(iso.codes.FORMAT_DWELL(),dwell))

        
                #self.write(iso.codes.DRILL_WITH_DWELL(iso.codes.FORMAT_DWELL(),dwell))                
    
    # Set the retraction point to the 'standoff' distance above the starting z height.        
        retract_height = z + standoff        
        if (x != None):        
            dx = x - self.x        
            self.write(iso.codes.X() + (self.ffmt % x))        
            self.x = x 
       
        if (y != None):        
            dy = y - self.y        
            self.write(iso.codes.Y() + (self.ffmt % y))        
            self.y = y
                      
        dz = (z + standoff) - self.z # In the end, we will be standoff distance above the z value passed in.

        if self.drill_modal:
            if z != self.prev_z:
                self.write(iso.codes.Z() + (self.ffmt % (z - depth)))
                self.prev_z=z
        else: 			
	    self.write(iso.codes.Z() + (self.ffmt % (z - depth)))	# This is the 'z' value for the bottom of the hole.
	self.z = (z + standoff)			# We want to remember where z is at the end (at the top of the hole)

        if self.drill_modal:
            if iso.codes.RETRACT(self.ffmt, retract_height) != self.prev_retract:
                self.write(iso.codes.RETRACT(self.ffmt, retract_height))               
                self.prev_retract = iso.codes.RETRACT(self.ffmt, retract_height)
        else:              
            self.write(iso.codes.RETRACT(self.ffmt, retract_height)) 
           
        if (self.fhv) : 
            self.calc_feedrate_hv(math.sqrt(dx*dx+dy*dy), math.fabs(dz))

        if self.drill_modal:
            if ( iso.codes.FEEDRATE() + (self.ffmt % self.fv) + iso.codes.SPACE() )!= self.prev_f:
               self.write(iso.codes.FEEDRATE() + (self.ffmt % self.fv) + iso.codes.SPACE() )        
               self.prev_f = iso.codes.FEEDRATE() + (self.ffmt % self.fv) + iso.codes.SPACE()
        else: 
            self.write( iso.codes.FEEDRATE() + (self.ffmt % self.fv) + iso.codes.SPACE() )            
        self.write_spindle()            
        #self.write_misc()    
        self.write('\n')
        




nc.creator = CreatorCentroid1()
