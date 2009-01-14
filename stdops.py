import kurve
from nc.nc import *

# line above will be import machine, so these operations are machine independant

# profile command,
# direction should be 'left' or 'right' or 'on'
def profile2(k, startx, starty, direction, radius, finishx, finishy):
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
                    arc_ccw(sx, sy, 0, rcx, rcy)
                elif rdir == -1:# clockwise arc
                    arc_cw(sx, sy, 0, rcx, rcy)
                else:# line
                    feed(sx, sy)
        if sp == 0:#line
            feed(ex, ey)
        else:
            cx = cx - sx # make relative to the start position
            cy = cy - sy
            if sp == 1:# anti-clockwise arc
                arc_ccw(ex, ey, 0, cx, cy)
            else:
                arc_cw(ex, ey, 0, cx, cy)

        if span == num_spans - 1:# last span
            if (finishx != ex or finishy != ey) and direction != "on":
                # do a roll off arc
                vx, vy = kurve.get_span_dir(offset_k, span, 1) # get end direction
                rcx, rcy, rdir = kurve.tangential_arc(ex, ey, vx, vy, finishx, finishy)
                rcx = rcx - ex # make relative to the start position
                rcy = rcy - ey
                if rdir == 1:# anti-clockwise arc
                    arc_ccw(finishx, finishy, 0, rcx, rcy)
                elif rdir == -1:# clockwise arc
                    arc_cw(finishx, finishy, 0, rcx, rcy)
                else:# line
                    feed(finishx, finishy)
                
                
    if offset_k != k:
        kurve.delete(offset_k)

def get_roll_on_pos(k, direction):
    # to do
    return 0, 0

def get_roll_off_pos(k, direction):
    # to do
    return 0, 0

# direction should be 'left' or 'right' or 'on'
# auto_profile calculates suitable roll on and roll off positions and also does rapid moves
def profile(k, direction, radius, clearance, rapid_down_to_height, final_depth):
    # start - assume we are at a suitable clearance height

    # get roll on position
    sx, sy = get_roll_on_pos(k, direction)

    # rapid across to it
    rapid(sx, sy)

    # rapid down to just above the metal
    rapid(z = rapid_down_to_height)

    # profile the shape, with roll off
    ex, ey = get_roll_off_pos(k, direction)
    profile2(k, sx, sy, direction, radius, ex, ey)

    # rapid back up to clearance plane
    rapid(z = clearance)
