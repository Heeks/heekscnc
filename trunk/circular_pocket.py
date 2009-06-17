#!/usr/local/bin/python

"""
    Circular Pocket G-Code Generator.
    This source is taken and modified from the
    Counterbore G-Code Generator
    Version 1.3
    Copyright (C) <2008>  <John Thornton>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    e-mail me any suggestions to "jet1024 at semo dot net"
    If you make money using this software
    you must donate $20 USD to a local food bank
    or the food police will get you! Think of others from time to time...
    
    1.2 Fix bug that incorrectly calculated the number of circles when the tool
    	was less than 1/2 the diameter of the hole
    1.3 Fix bug that did not calculate the depth of the hole if it was a single cut	

"""
from math import *
import os

class CircularPocket():
    def __init__(self):
    	self.g_code = ''
	self.block_number = int(10)

    # The write() method simply aggregates the resultant GCode string data into
    # one place.
    def write( self, s ):
	    self.g_code = self.g_code + ('N%d ' % self.block_number)
	    self.block_number = self.block_number + 10
	    self.g_code = self.g_code + s

    # The GeneratePath() method generates the GCode necessary to mill the circular
    # pocket with the parameters passed in.  The resultant GCode is returned as a
    # string return status. (from self.g_code)
    def GeneratePath(	self, 
			x = None,		# Mandatory
			y = None,		# Mandatory
		    	ToolDiameter = None,	# Mandatory
			HoleDiameter = None,	# Mandatory
			ClearanceHeight = None,	# Mandatory
			StartHeight = None,	# Mandatory
			MaterialTop = None,	# Mandatory
			FeedRate = None,	# Mandatory
			SpindleRPM = None,	# Optional. Adds 'M3' (spingle ON) if it's given
			HoleDepth = None,	# Mandatory
	    		DepthOfCut = None,	# Optional. Defaults to quarter of the tool diameter if not specified.
        		StepOver = None 	# Optional. Defaults to quarter of tool diameter if not specified.
			):


        if (ToolDiameter == None):
            raise ParameterError('Entry Missing', 'Please Enter a Tool Diameter!')
            return

        if (HoleDiameter == None):
            raise ParameterError('Entry Missing',\
                'Please Enter a Hole Diameter!\nOr select one from the list.')
            return

        if ToolDiameter >= HoleDiameter:
            raise ParameterError('Entry Error', \
            'Tool Diameter Larger than\n or Equal to  Hole Diameter!\
                \nPlease use a smaller tool.')
            return

    	if (x == None) or (y == None):
            raise ParameterError('Entry Missing',\
                'Please Press Enter from the Y Center\nto add an entry to the list.')
            return
                    
        self.HoleRadius = HoleDiameter/2
        self.FinishPathDiameter = HoleDiameter - ToolDiameter
        self.FinishPathRadius = self.FinishPathDiameter/2

        # Max Depth of Cut
        if (DepthOfCut == None):
            self.MaxCutDepth = ToolDiameter/4
        else:
            self.MaxCutDepth = float(DepthOfCut)

        # Depth of each cut
        if HoleDepth > self.MaxCutDepth:
            self.NumberOfCuts = int(ceil(HoleDepth/self.MaxCutDepth))
            self.CutDepth = HoleDepth / self.NumberOfCuts
        else:
            self.CutDepth = HoleDepth
            self.NumberOfCuts = 1

        # Spiral Depth of Cut
        self.SpiralDepth = self.CutDepth / 4
        self.NumberOfSpirals = 4

        # Stepover
        if (StepOver == None):
            StepOver = ToolDiameter * .75
            
        StepOver = self.FinishPathDiameter / \
            int(ceil(self.FinishPathDiameter / StepOver))
        self.ArcCenterOffset = StepOver / 8

        # Number of Circles
        self.NumberOfCircles = int(self.FinishPathDiameter/StepOver)-1
        
    
        # generate tool paths
        self.write( '(Socket Head Cap Screw Counterbore, Diameter = %.4f, Depth = %.4f )\n'\
            %(HoleDiameter, HoleDepth))
        self.write( '(Number of Cuts %d, Depth of Cut %.4f)\n' \
            %(self.NumberOfCuts, self.CutDepth))
        self.write( '(Tool Diameter = %.4f)\n' %ToolDiameter)
        if (SpindleRPM != None):
            self.write( 'F%.1f S%d\n' %(FeedRate, int(SpindleRPM)))
        else:
            self.write( 'F%.1f\n' %(FeedRate))
	self.XCenter = float(x)
	self.YCenter = float(y)
	self.write( '(Hole Center X%.4f Y%.4f)\n' \
                %(self.XCenter, self.YCenter))
                        
	# raise to clearance height
	self.write( 'G0 Z%.4f\n' %(ClearanceHeight))

	if ToolDiameter <= self.HoleRadius:
                
           # go to start position at 12 o'clock
           if (SpindleRPM != None):
                 self.write( 'G0 X%.4f Y%.4f M3\n'\
                    %(self.XCenter, self.YCenter+StepOver))
           else:
                self.write( 'G0 X%.4f Y%.4f\n'\
                     %(self.XCenter, self.YCenter+StepOver))
                
           # go to start height
           self.write( 'G1 Z%.4f\n' %(StartHeight))

           # spiral down to material top
           self.write( 'G3 X%.4f Y%.4f Z%.4f J%.4f\n' \
                        %(self.XCenter, self.YCenter+StepOver,\
                        MaterialTop, - StepOver))
           self.CurrentZ = MaterialTop
                        
           for n in range(0,self.NumberOfCuts):
               # spiral down to cut depth
               self.write( '(spiral down)\n')
               for n in range(0,self.NumberOfSpirals):
                   self.write( 'G3 X%.4f Y%.4f Z%.4f J%.4f\n' \
                       %(self.XCenter, self.YCenter+StepOver,\
                       self.CurrentZ-self.SpiralDepth, - StepOver))
                   self.CurrentZ = self.CurrentZ - self.SpiralDepth

               # spiral out to max cut diameter
               self.write( '(spiral out)\n')
               # cypher destination of each arc end point
               self.XMinus = self.XCenter - (StepOver + (self.ArcCenterOffset*2))
               self.YMinus = self.YCenter - (StepOver + (self.ArcCenterOffset*4))
               self.XPlus = self.XCenter + (StepOver + (self.ArcCenterOffset*6))
               self.YPlus = self.YCenter + (StepOver*2)
               for n in range(1,(self.NumberOfCircles-1)):
                   # 1st arc
                   self.write( 'G3 X%.4f Y%.4f I%.4f J%.4f\n' \
                       %(self.XMinus, (self.YCenter),
                       -self.ArcCenterOffset, \
                       -(self.ArcCenterOffset+(StepOver*n))))
                   self.XMinus = self.XMinus - StepOver
                   # 2nd arc
                   self.write( 'G3 X%.4f Y%.4f I%.4f J%.4f\n' \
                       %(self.XCenter, (self.YMinus),
                       (self.ArcCenterOffset*3)+(StepOver*n), \
                       -self.ArcCenterOffset))
                   self.YMinus = self.YMinus - StepOver
                   # 3rd arc
                   self.write( 'G3 X%.4f Y%.4f I%.4f J%.4f\n' \
                       %(self.XPlus, (self.YCenter),
                       self.ArcCenterOffset, \
                       (self.ArcCenterOffset*5)+(StepOver*n)))
                   self.XPlus = self.XPlus + StepOver
                   # 4th arc
                   self.write( 'G3 X%.4f Y%.4f I%.4f J%.4f\n' \
                       %(self.XCenter, (self.YPlus),
                       -((self.ArcCenterOffset*7)+(StepOver*n)),\
                           self.ArcCenterOffset))
                   self.YPlus = self.YPlus + StepOver

               # clean up circle
               self.write( 'G3 X%.4f Y%.4f J%.4f\n' \
                   %(self.XCenter, (self.YPlus-StepOver), \
                       -(StepOver+(StepOver*n))))
               # if n < self.NumberOfCuts:
               # go back to start positions
               self.write( 'G1 X%.4f Y%.4f\n' \
                   %(self.XCenter, self.YCenter+StepOver))
	elif ToolDiameter > self.HoleRadius:
           # start at 12 o'clock and spiral down to final depth
           self.Offset = HoleDiameter - ToolDiameter
           self.ArcRadius = self.Offset/2
           self.StartPositionY = self.YCenter - (self.Offset/2)
           self.ArcRadius = self.Offset/2
           # go to start position
           self.write( 'G0 X%.4f Y%.4f\n' \
           %(self.XCenter,self.StartPositionY))
           # go to start height
           self.write( 'G1 Z%.4f\n' %(StartHeight))
           # spiral down
           self.write( '(spiral down)\n')
           self.NextZPosition = self.CutDepth
           for n in range(0,self.NumberOfCuts):
               self.write( 'G3 X%.4f Y%.4f Z%.4f J%.4f\n' \
                   %(self.XCenter,self.StartPositionY \
                   ,-(self.NextZPosition), self.ArcRadius))
               self.NextZPosition = self.NextZPosition + self.CutDepth

           # clean up circle
           self.write( '(clean up hole)\n')
           self.write( 'G3 X%.4f Y%.4f J%.4f\n' \
               %(self.XCenter, self.StartPositionY, \
               self.FinishPathRadius))

	# return to center
	self.write( 'G1 X%.4f Y%.4f\n' %(self.XCenter, self.YCenter))
	# return to safe Z height
	if (SpindleRPM != None):
		self.write( 'G0 Z%.4f M5\n' %(ClearanceHeight))
	else:
           self.write( 'G0 Z%.4f M5\n' %(ClearanceHeight))
           self.write( '(end of Socket Head Cap Screw Counterbore)\n')

	return((self.g_code, self.block_number))

pocket = CircularPocket()

