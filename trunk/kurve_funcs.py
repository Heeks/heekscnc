import kurve
import math
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

def split_for_tags( k, radius, start_depth, depth, tag ):
    num_tags, tag_width, tag_angle, tag_at_start = tag
    tag_height = tag_width/2 * math.tan(tag_angle)
    perim = kurve.perim(k)
    sub_perim = perim / num_tags
    half_flat_top = radius
    if tag_height > (start_depth - depth):
        # cut the top off the tag
        chop_off_height = tag_height - (start_depth - depth)
        half_flat_top += chop_off_height / math.tan(tag_angle)

    for i in range(0, num_tags):
        d = sub_perim * i
        if tag_at_start == False: d += sub_perim / 2

        d0 = d - half_flat_top
        while d0 < 0: d0 += perim
        while d0 > perim: d0 -= perim
        px, py = kurve.perim_to_point(k, d0)
        kurve.kbreak(k, px, py)
        d1 = d + half_flat_top
        while d1 < 0: d1 += perim
        while d1 > perim: d1 -= perim
        px, py = kurve.perim_to_point(k, d1)
        kurve.kbreak(k, px, py)
        
        if tag_width + 2 * radius > sub_perim:
            px, py = kurve.perim_to_point(k, d + sub_perim/2)
            kurve.kbreak(k, px, py)
        else:
            d0 = d - tag_width/2 - radius
            while d0 < 0: d0 += perim
            while d0 > perim: d0 -= perim
            px, py = kurve.perim_to_point(k, d0)
            kurve.kbreak(k, px, py)
            d1 = d + tag_width/2 + radius
            while d1 < 0: d1 += perim
            while d1 > perim: d1 -= perim
            px, py = kurve.perim_to_point(k, d1)
            kurve.kbreak(k, px, py)

def get_tag_z_for_span(current_perim, k, radius, start_depth, depth, tag):
    num_tags, tag_width, tag_angle, tag_at_start = tag
    tag_height = tag_width/2 * math.tan(tag_angle)
    perim = kurve.perim(k)
    sub_perim = perim / num_tags
    max_z = depth
    for i in range(0, num_tags + 1):
        d = sub_perim * i
        if (tag_at_start == False) and (i != num_tags + 1): d += sub_perim / 2
        dist_from_d = math.fabs(current_perim - d) - radius
        if dist_from_d < 0: dist_from_d = 0
        z = (depth + tag_height) - (dist_from_d / ( tag_width / 2 )) * tag_height
        if z > max_z: max_z = z

    if max_z > start_depth:
        max_z = start_depth
        
    return max_z 
    
# profile command,
# direction should be 'left' or 'right' or 'on'
def profile(k, direction = "on", radius = 1.0, offset_extra = 0.0, rollstartx = None, rollstarty = None, rollfinishx = None, rollfinishy = None, start_depth = None, depth = None, tag = None):
    if kurve.exists(k) == False:
        raise "kurve doesn't exist, number %d" % (k)

    offset_k = k

    if direction != "on":
        if direction != "left" and direction != "right":
            raise "direction must be left or right", direction

        # get tool diameter
        offset = radius + offset_extra
        if direction == "right":
            offset = -offset
        offset_k = kurve.new()
        offset_success = kurve.offset(k, offset_k, offset)
        if offset_success == False:
            raise "couldn't offset kurve %d" % (k)

    if tag != None:
        num_tags, tag_width, tag_angle, tag_at_start = tag
        if tag_width < 0.00001:
            tag = None
        else:
            tag_height = tag_width/2 * math.tan(tag_angle)
            perim = kurve.perim(offset_k)
            sub_perim = perim / num_tags
            lowest_z = (depth + tag_height) - ((sub_perim / 2) / ( tag_width / 2 )) * tag_height
            if lowest_z > start_depth:
                return # the whole layer is tag
            split_for_tags(offset_k, radius, start_depth, depth, tag)
        
    num_spans = kurve.num_spans(offset_k)

    if num_spans == 0:
        raise "sketch has no spans!"

    current_perim = 0.0
    
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

        # height for tags
        current_perim += kurve.get_span_length(offset_k, span)
        ez = None
        if tag != None:
            ez = get_tag_z_for_span(current_perim, offset_k, radius, start_depth, depth, tag)
        
        if sp == 0:#line
            feed(ex, ey, ez)
        else:
            cx = cx - sx # make relative to the start position
            cy = cy - sy
            if sp == 1:# anti-clockwise arc
                arc_ccw(ex, ey, ez, i = cx, j = cy)
            else:
                arc_cw(ex, ey, ez, i = cx, j = cy)

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
