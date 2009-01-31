from nc.nc import *

import nc.iso

output('POST_TEST.txt')

#def_billet('billet.stl')
#def_workplane(54)
#def_tool(1, "Fraser", 100, 10)

program_begin(123, 'Test program')
absolute()
metric()
set_plane(0)
flush_nc()

feedrate(420)
rapid(100,120)
rapid(z=50)
feed(z=0)
arc_cw(x=120,y=100,i=100,j=100)
rapid(z=50)

feedrate_hv(100, 200)
feed(z=0)
feed(x=110,z=10)

feedrate(700)
feed(x=100,z=0)
rapid(z=50)

rapid_home()

program_end()
