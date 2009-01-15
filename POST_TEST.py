from nc.nc import *

import nc.iso

output('POST_TEST.txt')

program_begin(123, 'Test program')
absolute()
metric()
set_plane(0)
flush_nc()

feedrate(420)
rapid(100,120)
rapid(z=50)
feed(z=0)
rapid(z=50)

feedrate_hv(100, 200)
feed(z=0)
feed(x=110,z=10)

feedrate(700)
feed(x=100,z=0)
rapid(z=50)

rapid_home()

program_end()
