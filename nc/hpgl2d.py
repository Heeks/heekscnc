# hpgl2d.py
#
# Copyright (c) 2009, Dan Heeks
# This program is released under the BSD license. See the file COPYING for details.
#

import nc

class CreatorHpgl2d(nc.Creator):
    def __init__(self):
        nc.Creator.__init__(self) 
        self.x = int(0)
        self.y = int(0) # these are in machine units, like 0.01mm or maybe 0.25mm
        self.metric() # set self.units_to_mc_units

    def imperial(self):
        self.units_to_mc_units = 2540 # multiplier from inches to machine units

    def metric(self):
        self.units_to_mc_units = 100 # multiplier from mm to machine units

    def program_begin(self, id, name=''):
        self.write('IN;\n')
        self.write('VS32,1;\n')
        self.write('VS32,2;\n')
        self.write('VS32,3;\n')
        self.write('VS32,4;\n')
        self.write('VS32,5;\n')
        self.write('VS32,6;\n')
        self.write('VS32,7;\n')
        self.write('VS32,8;\n')

    def get_machine_x_y(self, x=None, y=None):
        machine_x = None
        machine_y = None
        if x != None:
            machine_x = x * self.units_to_mc_units
            if machine_x > 0: machine_x += 0.5
            else: machine_x -= 0.5
            machine_x = int(machine_x)
        if y != None:
            machine_y = y * self.units_to_mc_units
            if machine_y > 0: machine_y += 0.5
            else: machine_y -= 0.5
            machine_y = int(machine_y)
        return machine_x, machine_y
        
    def rapid(self, x=None, y=None, z=None, a=None, b=None, c=None):
        # ignore the z, any rapid will be assumed to be done with the pen up
        mx, my = self.get_machine_x_y(x, y)
        if mx != None and mx != self.x or my != None and my != self.y:
            self.write(('PU%i' % mx) + (' %i;\n' % my))
            self.x = mx
            self.y = my
            
    def feed(self, x=None, y=None, z=None):
        # ignore the z, any feed will be assumed to be done with the pen down
        mx, my = self.get_machine_x_y(x, y)
        if mx != None and mx != self.x or my != None and my != self.y:
            self.write(('PD%i' % mx) + (' %i;\n' % my))
            self.x = mx
            self.y = my

nc.creator = CreatorHpgl2d()
