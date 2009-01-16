# just a test to demonstrate how backplotting works
# this writes an xml file which HeeksCNC will open, and paste into the program

import re # regular expressions

f = open('nccode.xml', 'wb')
f.write('<?xml version="1.0" ?>\n')
f.write('<HeeksCAD_Document>\n')
f.write('   <nccode>\n')

# Process the nc file
filename_file = open('backplotfile.txt')
filename = filename_file.readline()
f_in = open(filename)

px = float(0)
py = float(0)
pz = float(0)

while (True):
    line = f_in.readline();
    if (len(line) == 0) : break

    move_found = False
    
    f.write('       <ncblock>\n')
    f.write('           <text>' + line + '</text>\n')

    nx = px
    ny = py
    nz = pz

    x = re.compile("X[-+]?(\d+(\.\d*)?|\.\d+)")
    m = x.search(line)
    if m != None:
        nx = float(m.group(1))
        move_found = True
        
    y = re.compile("Y[-+]?(\d+(\.\d*)?|\.\d+)")
    m = y.search(line)
    if m != None:
        ny = float(m.group(1))
        move_found = True
    
    z = re.compile("Z[-+]?(\d+(\.\d*)?|\.\d+)")
    m = z.search(line)
    if m != None:
        nz = float(m.group(1))
        move_found = True

    if move_found:
        f.write('           <lines col="feed">\n')
        f.write('               <p x="' + str(px) + '" y="' + str(py) + '" z="' + str(pz) + '0.000000" />\n')
        f.write('               <p x="' + str(nx) + '" y="' + str(ny) + '" z="' + str(nz) + '0.000000" />\n')
        f.write('           </lines>\n')

    px = nx
    py = ny
    pz = nz
    
    f.write('       </ncblock>\n')

f.write('   </nccode>\n')
f.write('</HeeksCAD_Document>\n')
f.close();
