import kurve
from nc.nc import *

def make_smaller( k, startx = None, starty = None, finishx = None, finishy = None ):
    if startx != None or starty != None:
        sp, sx, sy, ex, ey, cx, cy = kurve.get_span(k, 0)

        if startx == None: startx = sx
        if starty == None: starty = sy

        kurve.change_start(k, startx, starty)

    if finishx != None or finishy != None:
        num_spans = kurve.num_spans(k)
        sp, sx, sy, ex, ey, cx, cy = kurve.get_span(k, num_spans - 1)

        if finishx == None: finishx = ex
        if finishy == None: finishy = ey

        kurve.change_end(k, finishx, finishy)
    
# profile command,
# direction should be 'left' or 'right' or 'on'
def profile(k, direction = "on", radius = 1.0, rollstartx = None, rollstarty = None, rollfinishx = None, rollfinishy = None):
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
            if sx != rollstartx or sy != rollstarty:
                rdir = 0 # line
                if direction != "on":
                    # do a roll-on arc
                    vx, vy = kurve.get_span_dir(offset_k, span, 0) # get start direction
                    if rollstartx == 'NOT_SET' or rollstarty == 'NOT_SET':
                        raise "can not do an arc without a line move first"
                    rcx, rcy, rdir = kurve.tangential_arc(sx, sy, -vx, -vy, rollstartx, rollstarty)
                    rcx = rcx - rollstartx # make relative to the start position
                    rcy = rcy - rollstarty
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
            if (rollfinishx != ex or rollfinishy != ey) and direction != "on":
                # do a roll off arc
                vx, vy = kurve.get_span_dir(offset_k, span, 1) # get end direction
                rcx, rcy, rdir = kurve.tangential_arc(ex, ey, vx, vy, rollfinishx, rollfinishy)
                rcx = rcx - ex # make relative to the start position
                rcy = rcy - ey
                if rdir == 1:# anti-clockwise arc
                    arc_ccw(rollfinishx, rollfinishy, i = rcx, j = rcy)
                elif rdir == -1:# clockwise arc
                    arc_cw(rollfinishx, rollfinishy, i = rcx, j = rcy)
                else:# line
                    feed(rollfinishx, rollfinishy)
                
    if offset_k != k:
        kurve.delete(offset_k)

def roll_on_point(k, direction, radius, roll_radius):
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
        x = sx + off_vx * roll_radius - vx * roll_radius
        y = sy + off_vy * roll_radius - vy * roll_radius

    return x, y

def roll_off_point(k, direction, radius, roll_radius):
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
        x = ex + off_vx * roll_radius + vx * roll_radius
        y = ey + off_vy * roll_radius + vy * roll_radius

    return x, y
