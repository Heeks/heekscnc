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
        
class Tag:
    def __init__(self, x, y, width, angle, height):
        self.x = x
        self.y = y
        self.width = width # measured at the top of the tag. In the toolpath, the tag width will be this with plus the tool diameter, so that the finished tag has this "width" at it's smallest
        self.angle = angle # the angle of the ramp in radians. Between 0 and Pi/2; 0 is horizontal, Pi/2 is vertical
        self.height = height # the height of the tag, always measured above "final_depth"
        self.ramp_width = self.height / math.tan(self.angle)
        
    def split_kurve(self, k, radius, start_depth, depth, final_depth):
        tag_top_depth = final_depth + self.height
        if depth > tag_top_depth - 0.0000001:
            return # kurve is above this tag, so doesn't need splitting
        
        height_above_depth = tag_top_depth - depth
        ramp_width_at_depth = height_above_depth / math.tan(self.angle)
        cut_depth = start_depth - depth
        half_flat_top = radius + self.width / 2

        d = kurve.point_to_perim(k, self.x, self.y)
        d0 = d - half_flat_top
        perim = kurve.perim(k)
        if kurve.is_closed(k):
            while d0 < 0: d0 += perim
            while d0 > perim: d0 -= perim
        px, py = kurve.perim_to_point(k, d0)
        kurve.kbreak(k, px, py)
        d1 = d + half_flat_top
        if kurve.is_closed(k):
            while d1 < 0: d1 += perim
            while d1 > perim: d1 -= perim
        px, py = kurve.perim_to_point(k, d1)
        kurve.kbreak(k, px, py)
        
        d0 = d - half_flat_top - ramp_width_at_depth
        if kurve.is_closed(k):
            while d0 < 0: d0 += perim
            while d0 > perim: d0 -= perim
        px, py = kurve.perim_to_point(k, d0)
        kurve.kbreak(k, px, py)
        d1 = d + half_flat_top + ramp_width_at_depth
        if kurve.is_closed(k):
            while d1 < 0: d1 += perim
            while d1 > perim: d1 -= perim
        px, py = kurve.perim_to_point(k, d1)
        kurve.kbreak(k, px, py)
        
    def get_z_at_perim(self, current_perim, k, radius, start_depth, depth, final_depth):
        # return the z for this position on the kurve ( specified by current_perim ), for this tag
        # if the position is not within the tag, then depth is returned
        cut_depth = start_depth - depth
        half_flat_top = radius + self.width / 2

        z = depth
        d = kurve.point_to_perim(k, self.x, self.y)
        dist_from_d = math.fabs(current_perim - d)
        if dist_from_d < half_flat_top:
            # on flat top of tag
            z = final_depth + self.height
        elif dist_from_d < half_flat_top + self.ramp_width:
            # on ramp
            dist_up_ramp = (half_flat_top + self.ramp_width) - dist_from_d
            z = final_depth + dist_up_ramp * math.tan(self.angle)
        if z < depth: z = depth
        return z
            
    def dist(self, k):
        # return the distance from the tag point to the given kurve
        d = kurve.point_to_perim(k, self.x, self.y)
        px, py = kurve.perim_to_point(k, d)
        dx = math.fabs(self.x - px)
        dy = math.fabs(self.y - py)
        d = math.sqrt(dx*dx + dy*dy)
        return d

tags = []

def clear_tags():
    global tags
    tags = []
    
def add_tag(x, y, width, angle, height):
    global tags
    tag = Tag(x, y, width, angle, height)
    tags.append(tag)

def split_for_tags( k, radius, start_depth, depth, final_depth ):
    global tags
    for tag in tags:
        tag.split_kurve(k, radius, start_depth, depth, final_depth)

def get_tag_z_for_span(current_perim, k, radius, start_depth, depth, final_depth):
    global tags
    max_z = None
    perim = kurve.perim(k)
    for tag in tags:
        z = tag.get_z_at_perim(current_perim, k, radius, start_depth, depth, final_depth)
        if max_z == None or z > max_z:
            max_z = z
        if kurve.is_closed(k):
            # do the same test, wrapped around the closed kurve
            z = tag.get_z_at_perim(current_perim - perim, k, radius, start_depth, depth, final_depth)
            if max_z == None or z > max_z:
                max_z = z
            z = tag.get_z_at_perim(current_perim + perim, k, radius, start_depth, depth, final_depth)
            if max_z == None or z > max_z:
                max_z = z
        
    return max_z

def add_roll_on(k, roll_on_k, direction, roll_radius, offset_extra, roll_on):
    if direction == "on": roll_on = None
    num_spans = kurve.num_spans(k)
    if num_spans == 0: return
    
    sp, sx, sy, ex, ey, cx, cy = kurve.get_span(k, 0)

    if roll_on == None:
        rollstartx = sx
        rollstarty = sy
    elif roll_on == 'auto':
        vx, vy = kurve.get_span_dir(k, 0, 0) # get start direction
        if direction == 'right':
            off_vx = vy
            off_vy = -vx
        else:
            off_vx = -vy
            off_vy = vx

        rollstartx = sx + off_vx * roll_radius - vx * roll_radius
        rollstarty = sy + off_vy * roll_radius - vy * roll_radius
    else:
        rollstartx, rollstarty = roll_on       

    if sx == rollstartx and sy == rollstarty:
        rdir = 0
        rcx = 0
        rcy = 0
    else:
        vx, vy = kurve.get_span_dir(k, 0, 0) # get start direction
        rcx, rcy, rdir = kurve.tangential_arc(sx, sy, -vx, -vy, rollstartx, rollstarty)
        rdir = -rdir # because the tangential_arc was used in reverse
    
    # add a start roll on point
    kurve.add_point(roll_on_k, 0, rollstartx, rollstarty, 0, 0)

    # add the roll on arc
    kurve.add_point(roll_on_k, rdir, sx, sy, rcx, rcy)
    
def add_roll_off(k, roll_off_k, direction, roll_radius, offset_extra, roll_off):
    if direction == "on": return
    if roll_off == None: return
    num_spans = kurve.num_spans(k)
    if num_spans == 0: return

    if roll_off == 'auto':
        sp, sx, sy, ex, ey, cx, cy = kurve.get_span(k, num_spans - 1)
        vx, vy = kurve.get_span_dir(k, num_spans - 1, 1) # get end direction
        if direction == 'right':
            off_vx = vy
            off_vy = -vx
        else:
            off_vx = -vy
            off_vy = vx

        rollendx = ex + off_vx * roll_radius + vx * roll_radius;
        rollendy = ey + off_vy * roll_radius + vy * roll_radius;
    else:
        rollendx, rollendy =  roll_off  
             
    # add the end of the original kurve
    sp, sx, sy, ex, ey, cx, cy = kurve.get_span(k, num_spans - 1)
    kurve.add_point(roll_off_k, 0, ex, ey, 0, 0)
    if ex == rollendx and ey == rollendy: return
    vx, vy = kurve.get_span_dir(k, num_spans - 1, 1) # get end direction
    rcx, rcy, rdir = kurve.tangential_arc(ex, ey, vx, vy, rollendx, rollendy)

    # add the roll off arc  
    kurve.add_point(roll_off_k, rdir, rollendx, rollendy, rcx, rcy)
   
def cut_kurve(k):
    for span in range(0, kurve.num_spans(k)):
        sp, sx, sy, ex, ey, cx, cy = kurve.get_span(k, span)
        if sp == 0:#line
            feed(ex, ey)
        else:
            cx = cx - sx # make relative to the start position
            cy = cy - sy
            if sp == 1:# anti-clockwise arc
                arc_ccw(ex, ey, i = cx, j = cy)
            else:
                arc_cw(ex, ey, i = cx, j = cy)
    
# profile command,
# direction should be 'left' or 'right' or 'on'
def profile(k, direction = "on", radius = 1.0, offset_extra = 0.0, roll_radius = 2.0, roll_on = None, roll_off = None, rapid_down_to_height = None, clearance = None, start_depth = None, step_down = None, final_depth = None):
    global tags
    if kurve.exists(k) == False:
        raise "kurve doesn't exist, number %d" % (k)

    offset_k = k

    if direction != "on":
        if direction != "left" and direction != "right":
            raise "direction must be left or right", direction

        # get tool diameter
        offset = radius + offset_extra
        if use_CRC():
            add_CRC_start_line(offset_k, radius)
            start_CRC(direction == "left", radius)
        else:
            if direction == "right":
                offset = -offset
            offset_k = kurve.new()
            offset_success = kurve.offset(k, offset_k, offset)
            if offset_success == False:
                raise "couldn't offset kurve %d" % (k)
                
    # remove tags further than radius from the offset kurve
    new_tags = []
    for tag in tags:
        if tag.dist(offset_k) <= radius + 0.001:
            new_tags.append(tag)
    tags = new_tags

    if kurve.num_spans(offset_k) == 0:
        raise "sketch has no spans!"

    # do multiple depths
    total_to_cut = start_depth - final_depth;
    num_step_downs = int(float(total_to_cut) / math.fabs(step_down) + 0.999999)

    # tags
    if len(tags) > 0:
        # make a copy to restore to after each level
        copy_of_offset_k = kurve.new()
        kurve.copy(offset_k, copy_of_offset_k)
    
    prev_depth = start_depth
    for step in range(0, num_step_downs):
        depth_of_cut = ( start_depth - final_depth ) * ( step + 1 ) / num_step_downs
        depth = start_depth - depth_of_cut
        mat_depth = prev_depth
        
        if len(tags) > 0:
            split_for_tags(offset_k, radius, start_depth, depth, final_depth)

        # make the roll on and roll off kurves
        roll_on_k = kurve.new()
        add_roll_on(offset_k, roll_on_k, direction, roll_radius, offset_extra, roll_on)
        roll_off_k = kurve.new()
        add_roll_off(offset_k, roll_off_k, direction, roll_radius, offset_extra, roll_on)
        
        # get the tag depth at the start
        start_z = get_tag_z_for_span(0, offset_k, radius, start_depth, depth, final_depth)
        if start_z > mat_depth: mat_depth = start_z

        # rapid across to the start
        sp, sx, sy, ex, ey, cx, cy = kurve.get_span(roll_on_k, 0)
        rapid(sx, sy)
        
        # rapid down to just above the material
        rapid(z = mat_depth + rapid_down_to_height)
        
        # feed down to depth
        mat_depth = depth
        if start_z > mat_depth: mat_depth = start_z
        feed(z = mat_depth)
        
        # cut the roll on arc
        cut_kurve(roll_on_k)
        
        # cut the main kurve
        current_perim = 0.0
        
        for span in range(0, kurve.num_spans(offset_k)):
            sp, sx, sy, ex, ey, cx, cy = kurve.get_span(offset_k, span)

            # height for tags
            current_perim += kurve.get_span_length(offset_k, span)
            ez = get_tag_z_for_span(current_perim, offset_k, radius, start_depth, depth, final_depth)
            
            if sp == 0:#line
                if len(tags) > 0: feed(ex, ey, ez)
                else: feed(ex, ey)
            else:
                cx = cx - sx # make relative to the start position
                cy = cy - sy
                if sp == 1:# anti-clockwise arc
                    if len(tags) > 0: arc_ccw(ex, ey, ez, i = cx, j = cy)
                    else: arc_ccw(ex, ey, i = cx, j = cy)
                else:
                    if len(tags) > 0: arc_cw(ex, ey, ez, i = cx, j = cy)
                    else: arc_cw(ex, ey, i = cx, j = cy)
                    
        # cut the roll off arc
        cut_kurve(roll_off_k)
                    
        # restore the unsplit kurve
        if len(tags) > 0:
            kurve.copy(copy_of_offset_k, offset_k)
            
        # rapid up to the clearance height
        rapid(z = clearance)
                
    if offset_k != k:
        kurve.delete(offset_k)

    if len(tags) > 0 != None:
        kurve.delete(copy_of_offset_k)

    if use_CRC():
        end_CRC()

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
