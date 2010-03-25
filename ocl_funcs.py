import ocl
import math
from nc.nc import *

def zigzag( filepath, tool_diameter = 3.0, step_over = 1.0, x0= -10.0, x1 = 10.0, y0 = -10.0, y1 = 10.0, direction = 'X', mat_allowance = 0.0, clearance = 5.0, rapid_down_to_height = 2.0, start_depth = 0.0, step_down = 2.0, final_depth = -10.0):
   s = ocl.STLSurf(filepath)
   dcf = ocl.PathDropCutterFinish(s)
   cutter = ocl.CylCutter(tool_diameter + mat_allowance)
   dcf.setCutter(cutter)
   if final_depth > start_depth:
      raise 'final_depth > start_depth'
   height = start_depth - final_depth
   zsteps = int( height / math.fabs(step_down) + 0.999999 )
   zstep_down = height / zsteps
   incremental_rapid_to = rapid_down_to_height - start_depth
   if incremental_rapid_to < 0: incremental_rapid_to = 0.1
   for k in range(0, zsteps):
      z1 = start_depth - k * zstep_down
      z0 = start_depth - (k + 1) * zstep_down
      dcf.minimumZ = z0
      steps = int((y1 - y0)/step_over) + 1
      if direction == 'Y': steps = int((x1 - x0)/step_over) + 1
      sub_step_over = (y1 - y0)/ steps
      if direction == 'Y': sub_step_over = (x1 - x0)/ steps
      rapid_to = z1 + incremental_rapid_to
      for i in range(0, steps + 1):
         u = y0 + float(i) * step_over
         if direction == 'Y': u = x0 + float(i) * sub_step_over
         path = ocl.Path()
         if direction == 'Y': path.append(ocl.Line(ocl.Point(u, y0, 0), ocl.Point(u, y1, 0)))
         else: path.append(ocl.Line(ocl.Point(x0, u, 0), ocl.Point(x1, u, 0)))
         dcf.setPath(path)
         dcf.run()
         plist = dcf.getCLPoints()
         n = 0
         for p in plist:
            p.z = p.z + mat_allowance
            if n == 0:
               rapid(p.x, p.y)
               rz = rapid_to
               if p.z > z1: rz = p.z + incremental_rapid_to
               rapid(z = rz)
               feed(z = p.z)
            else:
               feed(p.x, p.y, p.z)
            n = n + 1
         rapid(z = clearance)
