import math
from nc.nc import *
import area

def make_smaller( curve, start = None, finish = None ):
    if start != None:
        curve.ChangeStart(start)

    if finish != None:
        curve.ChangeEnd(finish)
        
class Tag:
    def __init__(self, p, width, angle, height):
        self.p = p
        self.width = width # measured at the top of the tag. In the toolpath, the tag width will be this with plus the tool diameter, so that the finished tag has this "width" at it's smallest
        self.angle = angle # the angle of the ramp in radians. Between 0 and Pi/2; 0 is horizontal, Pi/2 is vertical
        self.height = height # the height of the tag, always measured above "final_depth"
        self.ramp_width = self.height / math.tan(self.angle)
        
    def split_curve(self, curve, radius, start_depth, depth, final_depth):
        tag_top_depth = final_depth + self.height
        
        if depth > tag_top_depth - 0.0000001:
            return # kurve is above this tag, so doesn't need splitting
        
        height_above_depth = tag_top_depth - depth
        ramp_width_at_depth = height_above_depth / math.tan(self.angle)
        cut_depth = start_depth - depth
        half_flat_top = radius + self.width / 2

        d = curve.PointToPerim(self.p)
        d0 = d - half_flat_top
        perim = curve.Perim()
        if curve.IsClosed():
            while d0 < 0: d0 += perim
            while d0 > perim: d0 -= perim
        p = curve.PerimToPoint(d0)
        curve.Break(p)
        d1 = d + half_flat_top
        if curve.IsClosed():
            while d1 < 0: d1 += perim
            while d1 > perim: d1 -= perim
        p = curve.PerimToPoint(d1)
        curve.Break(p)
        
        d0 = d - half_flat_top - ramp_width_at_depth
        if curve.IsClosed():
            while d0 < 0: d0 += perim
            while d0 > perim: d0 -= perim
        p = curve.PerimToPoint(d0)
        curve.Break(p)
        d1 = d + half_flat_top + ramp_width_at_depth
        if curve.IsClosed():
            while d1 < 0: d1 += perim
            while d1 > perim: d1 -= perim
        p = curve.PerimToPoint(d1)
        curve.Break(p)
        
    def get_z_at_perim(self, current_perim, curve, radius, start_depth, depth, final_depth):
        # return the z for this position on the kurve ( specified by current_perim ), for this tag
        # if the position is not within the tag, then depth is returned
        cut_depth = start_depth - depth
        half_flat_top = radius + self.width / 2

        z = depth
        d = curve.PointToPerim(self.p)
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
            
    def dist(self, curve):
        # return the distance from the tag point to the given kurve
        d = curve.PointToPerim(self.p)
        p = curve.PerimToPoint(d)
        v = self.p - p
        return v.length()

tags = []

def clear_tags():
    global tags
    tags = []
    
def add_tag(p, width, angle, height):
    global tags
    tag = Tag(p, width, angle, height)
    tags.append(tag)

def split_for_tags( curve, radius, start_depth, depth, final_depth ):
    global tags
    for tag in tags:
        tag.split_curve(curve, radius, start_depth, depth, final_depth)

def get_tag_z_for_span(current_perim, curve, radius, start_depth, depth, final_depth):
    global tags
    max_z = None
    perim = curve.Perim()
    for tag in tags:
        z = tag.get_z_at_perim(current_perim, curve, radius, start_depth, depth, final_depth)
        if max_z == None or z > max_z:
            max_z = z
        if curve.IsClosed():
            # do the same test, wrapped around the closed kurve
            z = tag.get_z_at_perim(current_perim - perim, curve, radius, start_depth, depth, final_depth)
            if max_z == None or z > max_z:
                max_z = z
            z = tag.get_z_at_perim(current_perim + perim, curve, radius, start_depth, depth, final_depth)
            if max_z == None or z > max_z:
                max_z = z
        
    return max_z

def add_roll_on(curve, roll_on_curve, direction, roll_radius, offset_extra, roll_on):
    if direction == "on": roll_on = None
    if curve.getNumVertices() <= 1: return
    first_span = curve.GetFirstSpan()

    if roll_on == None:
        rollstart = first_span.p
    elif roll_on == 'auto':
        v = first_span.GetVector(0.0)
        if direction == 'right':
            off_v = area.Point(v.y, -v.x)
        else:
            off_v = area.Point(-v.y, v.x)
            
        rollstart = first_span.p + off_v * roll_radius - v * roll_radius
    else:
        rollstart = roll_on       

    rvertex = area.Vertex(first_span.p)
    
    if first_span.p == rollstart:
        rvertex.type = 0
    else:
        v = first_span.GetVector(0.0) # get start direction
        rvertex.c, rvertex.type = area.TangentialArc(first_span.p, rollstart, -v)
        rvertex.type = -rvertex.type # because TangentialArc was used in reverse
    
    # add a start roll on point
    roll_on_curve.append(rollstart)

    # add the roll on arc
    roll_on_curve.append(rvertex)
    
def add_roll_off(curve, roll_off_curve, direction, roll_radius, offset_extra, roll_off):
    if direction == "on": return
    if roll_off == None: return
    if curve.getNumVertices() <= 1: return

    last_span = curve.GetLastSpan()
    
    if roll_off == 'auto':
        v = last_span.GetVector(1.0) # get end direction
        if direction == 'right':
            off_v = area.Point(v.y, -v.x)
        else:
            off_v = area.Point(-v.y, v.x)

        rollend = last_span.v.p + off_v * roll_radius + v * roll_radius;
    else:
        rollend =  roll_off  
             
    # add the end of the original kurve
    roll_off_curve.append(last_span.v.p)
    if rollend == last_span.v.p: return
    rvertex = area.Vertex(rollend)
    v = last_span.GetVector(1.0) # get end direction
    rvertex.c, rvertex.type = area.TangentialArc(last_span.v.p, rollend, v)

    # add the roll off arc  
    roll_off_curve.append(rvertex)
   
def cut_curve(curve):
    for span in curve.GetSpans():
        if span.v.type == 0:#line
            feed(span.v.p.x, span.v.p.y)
        else:
            c = span.v.c - span.p # make relative to the start position
            if span.v.type == 1:# anti-clockwise arc
                arc_ccw(span.v.p.x, span.v.p.y, i = c.x, j = c.y)
            else:
                arc_cw(span.v.p.x, span.v.p.y, i = c.x, j = c.y)
    
def add_CRC_start_line(curve, radius):
    # to do
    pass

# profile command,
# direction should be 'left' or 'right' or 'on'
def profile(curve, direction = "on", radius = 1.0, offset_extra = 0.0, roll_radius = 2.0, roll_on = None, roll_off = None, rapid_down_to_height = None, clearance = None, start_depth = None, step_down = None, final_depth = None):
    global tags

    offset_curve = area.Curve(curve)
    
    if direction != "on":
        if direction != "left" and direction != "right":
            raise "direction must be left or right", direction

        # get tool diameter
        offset = radius + offset_extra
        if use_CRC() == False:
            if direction == "right":
                offset = -offset
            offset_success = offset_curve.Offset(offset)
            if offset_success == False:
                raise "couldn't offset kurve " + offset_curve
                
    # remove tags further than radius from the offset kurve
    new_tags = []
    for tag in tags:
        if tag.dist(offset_curve) <= radius + 0.001:
            new_tags.append(tag)
    tags = new_tags

    if offset_curve.getNumVertices() <= 1:
        raise "sketch has no spans!"

    # do multiple depths
    total_to_cut = start_depth - final_depth;
    num_step_downs = int(float(total_to_cut) / math.fabs(step_down) + 0.999999)

    # tags
    if len(tags) > 0:
        # make a copy to restore to after each level
        copy_of_offset_curve = area.Curve(offset_curve)
    
    prev_depth = start_depth
    for step in range(0, num_step_downs):
        depth_of_cut = ( start_depth - final_depth ) * ( step + 1 ) / num_step_downs
        depth = start_depth - depth_of_cut
        mat_depth = prev_depth
        
        if len(tags) > 0:
            split_for_tags(offset_curve, radius, start_depth, depth, final_depth)

        # make the roll on and roll off kurves
        roll_on_curve = area.Curve()
        add_roll_on(offset_curve, roll_on_curve, direction, roll_radius, offset_extra, roll_on)
        roll_off_curve = area.Curve()
        add_roll_off(offset_curve, roll_off_curve, direction, roll_radius, offset_extra, roll_off)
        
        # get the tag depth at the start
        start_z = get_tag_z_for_span(0, offset_curve, radius, start_depth, depth, final_depth)
        if start_z > mat_depth: mat_depth = start_z

        # rapid across to the start
        s = roll_on_curve.FirstVertex().p
        rapid(s.x, s.y)
        
        # rapid down to just above the material
        rapid(z = mat_depth + rapid_down_to_height)
        
        # feed down to depth
        mat_depth = depth
        if start_z > mat_depth: mat_depth = start_z
        feed(z = mat_depth)

        if use_CRC():
            start_CRC(direction == "left", radius)
        
        # cut the roll on arc
        cut_curve(roll_on_curve)
        
        # cut the main kurve
        current_perim = 0.0
        
        for span in offset_curve.GetSpans():
            # height for tags
            current_perim += span.Length()
            ez = get_tag_z_for_span(current_perim, offset_curve, radius, start_depth, depth, final_depth)
            
            if span.v.type == 0:#line
                feed(span.v.p.x, span.v.p.y, ez)
            else:
                c = span.v.c - span.p # make relative to the start position
                if span.v.type == 1:# anti-clockwise arc
                    arc_ccw(span.v.p.x, span.v.p.y, ez, i = c.x, j = c.y)
                else:
                    arc_cw(span.v.p.x, span.v.p.y, ez, i = c.x, j = c.y)
                    
        # cut the roll off arc
        cut_curve(roll_off_curve)

        if use_CRC():
            end_CRC()
                    
        # restore the unsplit kurve
        if len(tags) > 0:
            offset_curve = area.Curve(copy_of_offset_curve)
            
        # rapid up to the clearance height
        rapid(z = clearance)
        
    del offset_curve
                
    if len(tags) > 0:
        del copy_of_offset_curve
