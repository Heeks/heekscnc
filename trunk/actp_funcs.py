from nc.nc import *
import sys
sys.path.insert(0,'../../libactp/PythonLib')
import actp

def cut(stl_file):
    actp.makerough(stl_file)

    npaths = actp.getnumpaths()
    for path in range(0, npaths):
        npoints = actp.getnumpoints(path)
        nbreaks = actp.getnumbreaks(path)
        nlinkpaths = actp.getnumlinkpths(path)
        z = actp.getz(path)
        start_pos = 0
        for brk in range(0, nbreaks):
            brkpos = actp.getbreak(path, brk)
            for point in range(start_pos, brkpos):
                x, y = actp.getpoint(path, point)
                feed(x, y, z)
            start_pos = brkpos
            nlinkpoints = actp.getnumlinkpoints(path, brk)
            for linkpoint in range(0, nlinkpoints):
                x, y, z = actp.getlinkpoint(path, brk, linkpoint)
                rapid(x, y, z)            
