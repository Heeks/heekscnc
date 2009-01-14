from nc.nc import *
from nc.attach import *

import nc.iso

output('POST_TEST.txt')

program_begin(123, 'Test program')
absolute()
metric()
set_plane(0)

feedrate(420)
rapid(100,120)
rapid(z=50)
feed(z=0)
rapid(z=50)

rapid_home()

program_end()

# ATTACHED VERSION:

attach_begin()

program_begin(123, 'Test program, attached')
absolute()
metric()
set_plane(0)

feedrate(420)
rapid(100,120)
rapid(z=50)
feed(z=0)
rapid(z=50)

rapid_home()

program_end()

attach_end()
