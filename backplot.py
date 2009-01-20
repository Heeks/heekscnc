# just a test to demonstrate how backplotting works
# this writes an xml file HeeksCNC and paste into the program's operations list

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
pz = float(200) # tool start position
move_type = 0
cx = float(0)
cy = float(0)

while (True):
    line = f_in.readline();
    if (len(line) == 0) : break

    move_found = False
    
    f.write('       <ncblock>\n')
    f.write('           <text>' + line + '</text>\n')

    nx = px
    ny = py
    nz = pz

    c = re.compile("X([-+]?(\d+(\.\d*)?|\.\d+))")
    m = c.search(line)
    if m != None:
        nx = float(m.group(1))
        move_found = True
        
    c = re.compile("Y([-+]?(\d+(\.\d*)?|\.\d+))")
    m = c.search(line)
    if m != None:
        ny = float(m.group(1))
        move_found = True
    
    c = re.compile("Z([-+]?(\d+(\.\d*)?|\.\d+))")
    m = c.search(line)
    if m != None:
        nz = float(m.group(1))
        move_found = True
     
    c = re.compile("I([-+]?(\d+(\.\d*)?|\.\d+))")
    m = c.search(line)
    if m != None:
        cx = float(m.group(1))
     
    c = re.compile("J([-+]?(\d+(\.\d*)?|\.\d+))")
    m = c.search(line)
    if m != None:
        cy = float(m.group(1))
   
    c = re.compile("G([-+]?(\d+(\.\d*)?|\.\d+))")
    m = c.search(line)
    if m != None:
        g_code = int(m.group(1))
        if g_code >= 0 or g_code <= 3: move_type = g_code

    if move_found:
        if move_type == 0:
            f.write('           <path col="rapid">\n')
        else:
            f.write('           <path col="feed">\n')
        f.write('               <p x="' + str(px) + '" y="' + str(py) + '" z="' + str(pz) + '" />\n')
        if move_type == 2 or move_type == 3:
            arc_str = 'acw'
            if move_type == 2: arc_str = 'cw'
            f.write('               <' + arc_str + ' x="' + str(nx) + '" y="' + str(ny) + '" z="' + str(nz) + '" i="' + str(cx) + '" j="' + str(cy) + '" />\n')
        else:
            f.write('               <p x="' + str(nx) + '" y="' + str(ny) + '" z="' + str(nz) + '" />\n')
        f.write('           </path>\n')

    px = nx
    py = ny
    pz = nz
    
    f.write('       </ncblock>\n')

f.write('   </nccode>\n')
f.write('</HeeksCAD_Document>\n')
f.close();
