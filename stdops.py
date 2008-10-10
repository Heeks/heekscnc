from hc import *

# profile command,
# offset should be positive in mm
# direction should be 'left' or 'right'
def profile(kurve_id, direction, finishx, finishy):
    if kurve_exists(kurve_id) == 0:
        error("Line Drawing doesn't exist, number %d" % (kurve_id))
    
    off_kurve_id = kurve_id

    if direction != "on":
        if direction != "left" and direction != "right":
            raise "direction must be left or right", direction

        # get tool diameter
        station, diameter, corner_rad = current_tool_data()
        offset = diameter/2
        if direction == "right":
            offset = -offset
        off_kurve_id = kurve_offset(kurve_id, offset)
        
    px, py, pz = current_tool_pos()

    if off_kurve_id == 0:
        error("couldn't offset Line Drawing number %d" % (off_kurve_id))
    num_spans = kurve_num_spans(off_kurve_id)

    if num_spans == 0:
        raise "kurve has no spans!"
    
    for span in range(0, num_spans):
        sp, sx, sy, ex, ey, cx, cy = kurve_span_data(off_kurve_id, span)
        if span == 0:#first span
            if sx != px or sy != py:
                rdir = 0 # line
                if direction != "on":
                    # do a roll-on arc
                    vx, vy = kurve_span_dir(off_kurve_id, span, 0) # get start direction
                    if px == 'NOT_SET' or py == 'NOT_SET':
                        raise "can not do an arc without a line move first"
                    rcx, rcy, rdir = tangential_arc(sx, sy, -vx, -vy, px, py)
                    rcx = rcx - px # make relative to the start position
                    rcy = rcy - py
                    rdir = -rdir # because the tangential_arc was used in reverse
                    
                if rdir == 1:# anti-clockwise arc
                    arc("acw", sx, sy, rcx, rcy)
                elif rdir == -1:# clockwise arc
                    arc("cw", sx, sy, rcx, rcy)
                else:# line
                    feedxy(sx, sy)
        if sp == 0:#line
            feedxy(ex, ey)
        else:
            cx = cx - sx # make relative to the start position
            cy = cy - sy
            if sp == 1:# anti-clockwise arc
                arc("acw", ex, ey, cx, cy)
            else:
                arc("cw", ex, ey, cx, cy)

        if span == num_spans - 1:# last span
            if (finishx != ex or finishy != ey) and direction != "on":
                # do a roll off arc
                vx, vy = kurve_span_dir(off_kurve_id, span, 1) # get end direction
                rcx, rcy, rdir = tangential_arc(ex, ey, vx, vy, finishx, finishy)
                rcx = rcx - ex # make relative to the start position
                rcy = rcy - ey
                if rdir == 1:# anti-clockwise arc
                    arc("acw", finishx, finishy, rcx, rcy)
                elif rdir == -1:# clockwise arc
                    arc("cw", finishx, finishy, rcx, rcy)
                else:# line
                    feedxy(finishx, finishy)
                
                
    if off_kurve_id != kurve_id:
        kurve_delete(off_kurve_id)

def facemill(xmax, ymax, stepover):
    px, py, pz = current_tool_pos()
    xtotal = xmax - px
    xsteps = int(xtotal/stepover + 0.5)
    dx = xtotal / xsteps / 2
    starty = py
    for x in range(0, xsteps):
        x1 = px + dx * x * 2
        x2 = px + dx * (x * 2 + 1)
        x3 = px + dx * (x * 2 + 2)
        feedxy(x1, ymax)
        feedxy(x2, ymax)
        feedxy(x2, starty)
        feedxy(x3, starty)
    feedxy(xmax, ymax)
        
def remove_radii(kurve_id, minrad):
    off_kurve_id = kurve_offset(kurve_id, minrad, "left")
    off_kurve_id2 = kurve_offset(off_kurve_id, -minrad, "right")
    kurve_add(off_kurve_id2)

def print_kurve(kurve_id):
    num_spans = kurve_num_spans(kurve_id)

    if num_spans == 0:
        print "no_spans"
        return
    
    for span in range(0, num_spans):
        sp, sx, sy, ex, ey, cx, cy = kurve_span_data(kurve_id, span)
        if sp == 0:#first span
            print "LINE start X", sx, " Y", sy, " end X", ex, " Y", ey
        else:
            if sp == 1:# anti-clockwise arc
                print "CCW start X", sx, " Y", sy, " end X", ex, " Y", ey, " centre X", cx, " Y", cy
            else:
                print "CW start X", sx, " Y", sy, " end X", ex, " Y", ey, " centre X", cx, " Y", cy
    
