import nc
import iso
import iso_codes
import math

class CodesEMC2(iso_codes.Codes):
	def SPACE(self): return(' ')
	def TAP(self): return('G33.1')
	def TAP_DEPTH(self, format, depth): return(self.SPACE() + 'K' + (format % depth))
	def BORE_FEED_OUT(self): return('G85')
	def BORE_SPINDLE_STOP_RAPID_OUT(self): return('G86')
	def BORE_DWELL_FEED_OUT(self, format, dwell): return('G89') + self.SPACE() + (format % dwell)
	
	
	# This version of COMMENT removes comments from the resultant GCode
	# def COMMENT(self,comment): return('')

iso_codes.codes = CodesEMC2()


class CreatorEMC2(iso.CreatorIso):
	def init(self): 
		iso.CreatorIso.init(self) 

	def program_begin(self, id, comment):
		self.write( ('(' + comment + ')' + '\n') )

 ############################################################################
    ##  Settings
    
	def imperial(self):
		self.write_blocknum()
		self.write( iso_codes.codes.IMPERIAL() + '\t (Imperial Values)\n')
		self.fmt = iso_codes.codes.FORMAT_IN()

	def metric(self):
		self.write_blocknum()
		self.write( iso_codes.codes.METRIC() + '\t (Metric Values)\n' )
		self.fmt = iso_codes.codes.FORMAT_MM()

	def absolute(self):
		self.write_blocknum()
		self.write( iso_codes.codes.ABSOLUTE() + '\t (Absolute Coordinates)\n')

	def incremental(self):
		self.write_blocknum()
		self.write( iso_codes.codes.INCREMENTAL() + '\t (Incremental Coordinates)\n' )

	def polar(self, on=True):
		if (on) :
			self.write_blocknum()
			self.write(iso_codes.codes.POLAR_ON() + '\t (Polar ON)\n' )
		else : 
			self.write_blocknum()
			self.write(iso_codes.codes.POLAR_OFF() + '\t (Polar OFF)\n' )

	def set_plane(self, plane):
		if (plane == 0) : 
			self.write_blocknum()
			self.write(iso_codes.codes.PLANE_XY() + '\t (Select XY Plane)\n')
		elif (plane == 1) :
			self.write_blocknum()
			self.write(iso_codes.codes.PLANE_XZ() + '\t (Select XZ Plane)\n')
		elif (plane == 2) : 
			self.write_blocknum()
			self.write(iso_codes.codes.PLANE_YZ() + '\t (Select YZ Plane)\n')

	def comment(self, text):
		self.write_blocknum()
		self.write((iso_codes.codes.COMMENT(text) + '\n'))

	# This is the coordinate system we're using.  G54->G59, G59.1, G59.2, G59.3
	# These are selected by values from 1 to 9 inclusive.
	def workplane(self, id):
		if ((id >= 1) and (id <= 6)):
			self.write_blocknum()
			self.write( (iso_codes.codes.WORKPLANE() % (id + iso_codes.codes.WORKPLANE_BASE())) + '\t (Select Relative Coordinate System)\n')
		if ((id >= 7) and (id <= 9)):
			self.write_blocknum()
			self.write( ((iso_codes.codes.WORKPLANE() % (6 + iso_codes.codes.WORKPLANE_BASE())) + ('.%i' % (id - 6))) + '\t (Select Relative Coordinate System)\n')

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
		
	def start_CRC(self, left = True, radius = 0.0):
		if self.t == None:
			raise "No tool specified for start_CRC()"
		self.write_blocknum()
		if left:
			self.write(('G41' + iso_codes.codes.SPACE() + 'D%i') % self.t  + '\t (start left cutter radius compensation)\n' )
		else:
			self.write(('G42' + iso_codes.codes.SPACE() + 'D%i') % self.t  + '\t (start right cutter radius compensation)\n' )

	def end_CRC(self):
		self.g = 'G40'
		self.write_blocknum()
		self.write_preps()
		self.write_misc()
		self.write('\t (end cutter radius compensation)\n')
		
	
	
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
    # extended argument list for EMC boring mah 30102001
    #    retract_mode : 0 - rapid retract, 1 - feed retract
    #   spindle_mode ;     if true, stop spindle at bottom, otherwise keep runnung

	def drill(self, x=None, y=None, z=None, depth=None, standoff=None, dwell=None, peck_depth=None, retract_mode=None, spindle_mode=None):

		if standoff == None:		
			# This is a bad thing.  All the drilling cycles need a retraction (and starting) height.
			return

		if (z == None): 
			return	# We need a Z value as well.  This input parameter represents the top of the hole
								 
		self.write_preps()
		self.write_blocknum()				
		
		if (peck_depth != 0):
			if spindle_mode == 1:
				raise "cannot stop spindle at bottom while peck drilling"				 
			if retract_mode == 1:
				raise "cannot feed retract while peck drilling" 
			
			# We're pecking.  Let's find a tree. 
			if self.drill_modal:	   
				if  iso_codes.codes.PECK_DRILL() + iso_codes.codes.PECK_DEPTH(self.fmt, peck_depth) != self.prev_drill:
					self.write(iso_codes.codes.PECK_DRILL() + iso_codes.codes.PECK_DEPTH(self.fmt, peck_depth))  

					self.prev_drill = iso_codes.codes.PECK_DRILL() + iso_codes.codes.PECK_DEPTH(self.fmt, peck_depth)
			else:	   
				self.write(iso_codes.codes.PECK_DRILL() + iso_codes.codes.PECK_DEPTH(self.fmt, peck_depth)) 
						   
		else:	  
			if (spindle_mode == 1) or (retract_mode == 1):
				  # this is a boring cycle.
			
				if (spindle_mode == 0): # keep spindle running, feed retract
					if (dwell == 0):
						self.write(iso_codes.codes.BORE_FEED_OUT())
					else:
						self.write(iso_codes.codes.BORE_DWELL_FEED_OUT(iso_codes.codes.FORMAT_DWELL(), dwell))
				else:
					# stop spindle at bottom 
					self.write(iso_codes.codes.BORE_SPINDLE_STOP_RAPID_OUT())
					if (dwell > 0):
						self.write( iso_codes.codes.SPACE() + iso_codes.codes.FORMAT_DWELL() % dwell)	   
			  
			# We're either just drilling or drilling with dwell.		
			else:
				if (dwell == 0):		
					# We're just drilling. 
					if self.drill_modal:	   
						if  iso_codes.codes.DRILL() != self.prev_drill:
							self.write(iso_codes.codes.DRILL())  
							self.prev_drill = iso_codes.codes.DRILL()
					else:
						self.write(iso_codes.codes.DRILL())
				else:		
					# We're drilling with dwell.
					if self.drill_modal:	   
						if  iso_codes.codes.DRILL_WITH_DWELL(iso_codes.codes.FORMAT_DWELL(), dwell) != self.prev_drill:
							self.write(iso_codes.codes.DRILL_WITH_DWELL(iso_codes.codes.FORMAT_DWELL(), dwell))  
							self.prev_drill = iso_codes.codes.DRILL_WITH_DWELL(iso_codes.codes.FORMAT_DWELL(), dwell)
					else:
						self.write(iso_codes.codes.DRILL_WITH_DWELL(iso_codes.codes.FORMAT_DWELL(), dwell))

		
				#self.write(iso_codes.codes.DRILL_WITH_DWELL(iso_codes.codes.FORMAT_DWELL(),dwell))				
	
		   # Set the retraction point to the 'standoff' distance above the starting z height.		
		retract_height = z + standoff		
		if (x != None):		
			dx = x - self.x		
			self.write(iso_codes.codes.X() + (self.fmt % x))		
			self.x = x 
	   
		if (y != None):		
			dy = y - self.y		
			self.write(iso_codes.codes.Y() + (self.fmt % y))		
			self.y = y
					  
		dz = (z + standoff) - self.z # In the end, we will be standoff distance above the z value passed in.

		if self.drill_modal:
			if z != self.prev_z:
				self.write(iso_codes.codes.Z() + (self.fmt % (z - depth)))
				self.prev_z = z
		else:			 
			 self.write(iso_codes.codes.Z() + (self.fmt % (z - depth)))	# This is the 'z' value for the bottom of the hole.
		self.z = (z + standoff)			# We want to remember where z is at the end (at the top of the hole)

		if self.drill_modal:
			if self.prev_retract != iso_codes.codes.RETRACT(self.fmt, retract_height) :
				self.write(iso_codes.codes.RETRACT(self.fmt, retract_height))			   
				self.prev_retract = iso_codes.codes.RETRACT(self.fmt, retract_height)
		else:			  
			self.write(iso_codes.codes.RETRACT(self.fmt, retract_height))
		   
		if (self.fhv) : 
			self.calc_feedrate_hv(math.sqrt(dx * dx + dy * dy), math.fabs(dz))

		if self.drill_modal:
			if (iso_codes.codes.FEEDRATE() + (self.ffmt % self.fv) + iso_codes.codes.SPACE()) != self.prev_f:
			   self.write(iso_codes.codes.FEEDRATE() + (self.ffmt % self.fv) + iso_codes.codes.SPACE())		
			   self.prev_f = iso_codes.codes.FEEDRATE() + (self.ffmt % self.fv) + iso_codes.codes.SPACE()
		else: 
			self.write(iso_codes.codes.FEEDRATE() + (self.ffmt % self.fv) + iso_codes.codes.SPACE())			
		self.write_spindle()			
		self.write_misc()	
		self.write('\n')
		
        
	# G33.1 tapping with EMC for now
	# unsynchronized (chuck) taps NIY (tap_mode = 1)
    
	def tap(self, x=None, y=None, z=None, zretract=None, depth=None, standoff=None, dwell_bottom=None, pitch=None, stoppos=None, spin_in=None, spin_out=None, tap_mode=None, direction=None):
		# mystery parameters: 
		# zretract=None, dwell_bottom=None,pitch=None, stoppos=None, spin_in=None, spin_out=None):
		# I dont see how to map these to EMC Gcode

		if (standoff == None):		
			# This is a bad thing.  All the drilling cycles need a retraction (and starting) height.		
			return
		if (z == None): 
			return	# We need a Z value as well.  This input parameter represents the top of the hole 
		if (pitch == None): 
			return	# We need a pitch value.
		if (direction == None): 
			return	# We need a direction value.

		if (tap_mode != 0):
			raise "only rigid tapping currently supported"

		self.write_preps()
		self.write_blocknum()				
		self.write_spindle()
		self.write('\n')

		# rapid to starting point; z first, then x,y iff given

		# Set the retraction point to the 'standoff' distance above the starting z height.		
		retract_height = z + standoff		

		# unsure if this is needed:
		if self.z != retract_height:
				self.rapid(z = retract_height)

		# then continue to x,y if given
		if (x != None) or (y != None):
				self.write_blocknum()				
				self.write(iso_codes.codes.RAPID() )		   

				if (x != None):		
						self.write(iso_codes.codes.X() + (self.fmt % x))		
						self.x = x 

				if (y != None):		
						self.write(iso_codes.codes.Y() + (self.fmt % y))		
						self.y = y
				self.write('\n')
       
		self.write_blocknum()				
		self.write( iso_codes.codes.TAP() )
		self.write( iso_codes.codes.TAP_DEPTH(self.ffmt,pitch) + iso_codes.codes.SPACE() )			
		self.write(iso_codes.codes.Z() + (self.fmt % (z - depth)))	# This is the 'z' value for the bottom of the tap.
		self.write_misc()	
		self.write('\n')

		self.z = retract_height	# this cycle returns to the start position, so remember that as z value

			
nc.creator = CreatorEMC2()

