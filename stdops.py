import kurve
import area
from nc.nc import *

# profile command,
# direction should be 'left' or 'right' or 'on'
def profile(k, startx, starty, direction, radius, finishx, finishy):
    if kurve.exists(k) == False:
        raise "kurve doesn't exist, number %d" % (k)
    
    offset_k = k

    if direction != "on":
        if direction != "left" and direction != "right":
            raise "direction must be left or right", direction

        # get tool diameter
        offset = radius
        if direction == "right":
            offset = -offset
        offset_k = kurve.new()
        offset_success = kurve.offset(k, offset_k, offset)
        if offset_success == False:
            raise "couldn't offset kurve %d" % (k)
        
    num_spans = kurve.num_spans(offset_k)

    if num_spans == 0:
        raise "sketch has no spans!"
    
    for span in range(0, num_spans):
        sp, sx, sy, ex, ey, cx, cy = kurve.get_span(offset_k, span)
        if span == 0:#first span
            if sx != startx or sy != starty:
                rdir = 0 # line
                if direction != "on":
                    # do a roll-on arc
                    vx, vy = kurve.get_span_dir(offset_k, span, 0) # get start direction
                    if startx == 'NOT_SET' or starty == 'NOT_SET':
                        raise "can not do an arc without a line move first"
                    rcx, rcy, rdir = kurve.tangential_arc(sx, sy, -vx, -vy, startx, starty)
                    rcx = rcx - startx # make relative to the start position
                    rcy = rcy - starty
                    rdir = -rdir # because the tangential_arc was used in reverse
                    
                if rdir == 1:# anti-clockwise arc
                    arc_ccw(sx, sy, i = rcx, j = rcy)
                elif rdir == -1:# clockwise arc
                    arc_cw(sx, sy, i = rcx, j = rcy)
                else:# line
                    feed(sx, sy)
        if sp == 0:#line
            feed(ex, ey)
        else:
            cx = cx - sx # make relative to the start position
            cy = cy - sy
            if sp == 1:# anti-clockwise arc
                arc_ccw(ex, ey, i = cx, j = cy)
            else:
                arc_cw(ex, ey, i = cx, j = cy)

        if span == num_spans - 1:# last span
            if (finishx != ex or finishy != ey) and direction != "on":
                # do a roll off arc
                vx, vy = kurve.get_span_dir(offset_k, span, 1) # get end direction
                rcx, rcy, rdir = kurve.tangential_arc(ex, ey, vx, vy, finishx, finishy)
                rcx = rcx - ex # make relative to the start position
                rcy = rcy - ey
                if rdir == 1:# anti-clockwise arc
                    arc_ccw(finishx, finishy, i = rcx, j = rcy)
                elif rdir == -1:# clockwise arc
                    arc_cw(finishx, finishy, i = rcx, j = rcy)
                else:# line
                    feed(finishx, finishy)
                
    if offset_k != k:
        kurve.delete(offset_k)

def roll_on_point(k, direction, radius):
    x = float(0)
    y = float(0)

    offset = radius
    if direction == "right":
        offset = -offset
    offset_k = kurve.new()
    offset_success = kurve.offset(k, offset_k, offset)
    if offset_success == False:
        raise "couldn't offset kurve %d" % (k)

    if kurve.num_spans(offset_k) > 0:
        sp, sx, sy, ex, ey, cx, cy = kurve.get_span(offset_k, 0)
        vx, vy = kurve.get_span_dir(offset_k, 0, 0) # get start direction
        off_vx = -vy
        off_vy = vx
        if direction == 'right':
            off_vx = -off_vx
            off_vy = -off_vy
        x = sx + off_vx * 2 - vx * 2
        y = sy + off_vy * 2 - vy * 2

    return x, y

def roll_off_point(k, direction, radius):
    x = float(0)
    y = float(0)

    offset = radius
    if direction == "right":
        offset = -offset
    offset_k = kurve.new()
    offset_success = kurve.offset(k, offset_k, offset)
    if offset_success == False:
        raise "couldn't offset kurve %d" % (k)

    n = kurve.num_spans(offset_k)
    
    if n > 0:
        sp, sx, sy, ex, ey, cx, cy = kurve.get_span(offset_k, n - 1)
        vx, vy = kurve.get_span_dir(offset_k, n - 1, 1) # get end direction
        off_vx = -vy
        off_vy = vx
        if direction == 'right':
            off_vx = -off_vx
            off_vy = -off_vy
        x = ex + off_vx * 2 + vx * 2
        y = ey + off_vy * 2 + vy * 2

    return x, y

def cut_area(a, do_rapid):
    for curve in range(0, area.num_curves(a)):
        for vertex in range(0, area.num_vertices(a, curve)):
            sp, x, y, cx, cy = area.get_vertex(a, curve, vertex)
            if do_rapid:
                # rapid across
                rapid(x, y)

                ##rapid down , to do, correct height
                rapid(z = 2)
                
                #feed down, to do, use correct height
                feed(z = 0)

                do_rapid = False
            else:
                feed(x, y)
    
def pocket(a, radius, stepover):
    offset_value = radius
    a_offset = area.new()
    area.copy(a, a_offset)
    area.offset(a_offset, offset_value)

    first = True

    while area.num_curves(a_offset):
        cut_area(a_offset, first)
        first = False
        area.copy(a, a_offset)
        offset_value = offset_value + stepover
        area.offset(a_offset, offset_value)

    # rapid up, to do use clearance height
    rapid(z = 50)
    
