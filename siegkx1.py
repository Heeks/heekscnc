f = open('test.tap', 'wb')

ln = int(0)

def write_str(s):
    f.write(s)
    print s,
    
def get_ln_str():
    global ln
    s = 'N%04d' % (ln)
    ln = ln + 10
    return s
    
def tool(station, diam, corner):
    write_str('%sT%dM6\n' % (get_ln_str(), station))

def rapid(x, y, z):
    xs = ''
    ys = ''
    zs = ''
    if x != 'NOT_SET':
        xs = 'X%.3f' % (x)
    if y != 'NOT_SET':
        ys = 'Y%.3f' % (y)
    if z != 'NOT_SET':
        zs = 'Z%.3f' % (z)
    write_str('%sG0%s%s%s\n' % (get_ln_str(), xs, ys, zs))

def feed(x, y, z):
    xs = ''
    ys = ''
    zs = ''
    if x != 'NOT_SET':
        xs = 'X%.3f' % (x)
    if y != 'NOT_SET':
        ys = 'Y%.3f' % (y)
    if z != 'NOT_SET':
        zs = 'Z%.3f' % (z)
    write_str('%sG1%s%s%s\n' % (get_ln_str(), xs, ys, zs))

def arc(direction, x, y, i, j):
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

    write_str('%s%s%s%s%s%s\n' % (get_ln_str(), dir_str, xss, yss, iss, jss))

def end():
    write_str('%sM2\n' % (get_ln_str()))
    
def zig():
    pass
