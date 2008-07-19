from math import *

f = open('test.tap', 'wb')

ln = int(0)
hfeed = float(0)
vfeed = float(0)
feedrate = float(-1)
spindle_speed = float(0)
movex = 'NOT_SET'
movey = 'NOT_SET'
movez = 'NOT_SET'
tool_station = int(0)
tool_diameter = float(0)
tool_corner_rad = float(0)

def write_str(s):
    f.write(s)
    print s,
    
def get_ln_str():
    global ln
    s = 'N%04d' % (ln)
    ln = ln + 10
    return s

def spindle(speed):
    global spindle_speed
    if speed != spindle_speed:
        spindle_speed = speed
        write_str('%sS%.3f\n' % (get_ln_str(), speed))

def rate(hf, vf):
    global hfeed
    global vfeed
    hfeed = hf
    vfeed = vf
    
def tool(station, diam, corner):
    global tool_station
    global tool_diameter
    global tool_corner_rad
    tool_station = station
    tool_diameter = diam
    tool_corner_rad = corner
    write_str('%sT%dM6\n' % (get_ln_str(), station))

def current_tool_pos():
    global movex
    global movey
    global movez
    return (movex, movey, movez)
    
def current_tool_data():
    global tool_station
    global tool_diameter
    global tool_corner_rad
    return (tool_station, tool_diameter, tool_corner_rad)

def calculate_feed(x, y, z):
    global movex
    global movey
    global movez
    global hfeed
    global vfeed
    
    vx = 0
    vy = 0
    vz = 0
    if x != 'NOT_SET':
        if movex == 'NOT_SET':
            raise "cant calculate feedrate, because exisiting x is unknown; move to "
        vx = x - movex
    if y != 'NOT_SET':
        if movey == 'NOT_SET':
            raise "cant calculate feedrate, because exisiting y is unknown; move to "
        vy = y - movey
    if z != 'NOT_SET':
        if movez == 'NOT_SET':
            raise "cant calculate feedrate, because exisiting z is unknown; move to "
        vz = z - movez

    if fabs(vz) < 0.000001:
        # horizontal only
        return hfeed

    if fabs(vx) < 0.000001 and fabs(vy) < 0.000001:
        # vertical only
        return vfeed

    vh = sqrt(vx * vx + vy * vy)
    hratio = fabs(vh / vz)

    # if the move is mostly flat, or it's upwards, use the horizontal feed rate
    if hratio > 4 or vz > 0:
        return hfeed

    # if it's got more than a quarter of downward vertical movement, use the vertical feed rate
    return vfeed

def move(rapid, x, y, z):
    global movex
    global movey
    global movez
    global feedrate

    xs = ''
    ys = ''
    zs = ''
    if x != 'NOT_SET':
        if movex != x:
            xs = 'X%.3f' % (x)
            movex = x
    if y != 'NOT_SET':
        if movey != y:
            ys = 'Y%.3f' % (y)
            movey = y
    if z != 'NOT_SET':
        if movez != z:
            zs = 'Z%.3f' % (z)
            movez = z

    if xs == '' and ys == '' and zs == '':
        return
           
    gs = 'G1'
    fs = ''
    if rapid == 1:
        gs = 'G0'
        
    else:
        f = calculate_feed(x, y, z)
        if f != feedrate:
            fs = 'F%.3f' % (f)
            feedrate = f
        
    write_str('%s%s%s%s%s%s\n' % (get_ln_str(), gs, xs, ys, zs, fs))

def rapid(x, y, z):
    move(1, x, y, z)

def rapidxy(x, y):
    rapid(x, y, 'NOT_SET')

def rapidz(z):
    rapid('NOT_SET', 'NOT_SET', z)
    
def feed(x, y, z):
    move(0, x, y, z)

def feedxy(x, y):
    feed(x, y, 'NOT_SET')

def feedz(z):
    feed('NOT_SET', 'NOT_SET', z)

def arc(direction, x, y, i, j):
    global movex
    global movey
    global feedrate
    
    dir_str = ''
    if direction == 'cw':
        dir_str = 'G2'
    elif direction == 'acw':
        dir_str = 'G3'
    else:
        raise direction, 'invalid direction given, only cw or acw allowed, but %s was found' % (direction)

    xss = 'X%.3f' % (x)
    yss = 'Y%.3f' % (y)
    iss = 'I%.3f' % (i)
    jss = 'J%.3f' % (j)
    movex = x
    movey = y

    fs = ''
    f = calculate_feed(x, y, 'NOT_SET')
    if f != feedrate:
        fs = 'F%.3f' % (f)
        feedrate = f

    write_str('%s%s%s%s%s%s%s\n' % (get_ln_str(), dir_str, xss, yss, iss, jss, fs))

def end():
    write_str('%sM2\n' % (get_ln_str()))
    
def zig():
    pass
