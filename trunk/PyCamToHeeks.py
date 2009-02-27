from pycam.Exporters.gcode import gcode
from nc.nc import *

# simplistic GCode exporter
# does each run, and moves the tool to the safetyheight in between

class HeeksCNCExporter:

    def __init__(self, safetyz):
        self.safetyz = safetyz
        #self.file.write(gc.begin()+"\n")
        #self.file.write("F"+str(feedrate)+"\n")
        #self.file.write("S"+str(speed)+"\n")
        #self.file.write(gc.safety()+"\n")
        pass

    def AddPath(self, path):
        point = path.points[0]
        rapid(point.x,point.y, self.safetyz) # to do, use clearance height
#        self.file.write(gc.rapid(point.x,point.y,gc.safetyheight)+"\n")
        for point in path.points:
            feed(point.x,point.y,point.z)
            #self.file.write(gc.cut(point.x,point.y,point.z)+"\n")
        rapid(z=self.safetyz)
#        self.file.write(gc.rapid(point.x,point.y,gc.safetyheight)+"\n")

    def AddPathList(self, pathlist):
        for path in pathlist:
            self.AddPath(path)
