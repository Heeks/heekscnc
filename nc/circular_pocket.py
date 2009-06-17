#!/usr/local/bin/python

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
        
	return((self.g_code, self.block_number))

pocket = CircularPocket()

