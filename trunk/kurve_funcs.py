import math
from nc.nc import *
import area

def set_good_start_point( curve, rev ):
    if curve.IsClosed():
        # find the longest span and use the middle as the new start point
        longest_length = None
        mid_point = None
        spans = curve.GetSpans()
        if rev:
            spans = reversed(spans)

        for span in spans:
            length = span.Length()
            if longest_length == None or length > longest_length:
                longest_length = length
                mid_point = span.MidParam(0.5)
        if mid_point != None:
            make_smaller(curve, mid_point)

def make_smaller( curve, start = None, finish = None, end_beyond = False ):
    if start != None:
        curve.ChangeStart(curve.NearestPoint(start))

    if finish != None:
        if end_beyond:
            curve2 = area.Curve(curve)
            curve2.ChangeEnd(curve2.NearestPoint(finish))
            first = True
            for vertex in curve2.getVertices():
                if first == False: curve.append(vertex)
                first = False
        else:
            curve.ChangeEnd(curve.NearestPoint(finish))
        
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
        if roll_radius < 0.0000000001:
            rollstart = first_span.p
        v = first_span.GetVector(0.0)
        if direction == 'right':
            off_v = area.Point(v.y, -v.x)
        else:
            off_v = area.Point(-v.y, v.x)
        rollstart = first_span.p + off_v * roll_radius
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
        if roll_radius < 0.0000000001: return
        v = last_span.GetVector(1.0) # get end direction
        if direction == 'right':
            off_v = area.Point(v.y, -v.x)
        else:
            off_v = area.Point(-v.y, v.x)

        rollend = last_span.v.p + off_v * roll_radius;
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
            if span.v.type == 1:# anti-clockwise arc
                arc_ccw(span.v.p.x, span.v.p.y, i = span.v.c.x, j = span.v.c.y)
            else:
                arc_cw(span.v.p.x, span.v.p.y, i = span.v.c.x, j = span.v.c.y)
    
def add_CRC_start_line(curve,roll_on_curve,roll_off_curve,radius,direction,crc_start_point,lead_in_line_len):
    first_span = curve.GetFirstSpan()
    v = first_span.GetVector(0.0)
    if direction == 'right':
        off_v = area.Point(v.y, -v.x)
    else:
        off_v = area.Point(-v.y, v.x)
    startpoint_roll_on = roll_on_curve.FirstVertex().p
    crc_start = startpoint_roll_on  + off_v * lead_in_line_len
    crc_start_point.x = crc_start.x 
    crc_start_point.y = crc_start.y 

def add_CRC_end_line(curve,roll_on_curve,roll_off_curve,radius,direction,crc_end_point,lead_out_line_len):
    last_span = curve.GetLastSpan()
    v = last_span.GetVector(1.0)
    if direction == 'right':
        off_v = area.Point(v.y, -v.x)
    else:
        off_v = area.Point(-v.y, v.x)
    endpoint_roll_off = roll_off_curve.LastVertex().p
    crc_end = endpoint_roll_off  + off_v * lead_out_line_len
    crc_end_point.x = crc_end.x 
    crc_end_point.y = crc_end.y 

using_area_for_offset = False

# profile command,
# direction should be 'left' or 'right' or 'on'
def profile(curve, direction = "on", radius = 1.0, offset_extra = 0.0, roll_radius = 2.0, roll_on = None, roll_off = None, depthparams = None, extend_at_start = 0.0, extend_at_end = 0.0, lead_in_line_len=0.0,lead_out_line_len= 0.0):
    global tags

    offset_curve = area.Curve(curve)
    if direction == "on":
        use_CRC() == False 
        
    if direction != "on":
        if direction != "left" and direction != "right":
            raise "direction must be left or right", direction

        # get tool diameter
        offset = radius + offset_extra
        if use_CRC() == False or (use_CRC()==True and CRC_nominal_path()==True):
            if math.fabs(offset) > 0.00005:
                if direction == "right":
                    offset = -offset
                offset_success = offset_curve.Offset(offset)
                if offset_success == False:
                    global using_area_for_offset
                    if curve.IsClosed() and (using_area_for_offset == False):
                        cw = curve.IsClockwise()
                        using_area_for_offset = True
                        a = area.Area()
                        a.append(curve)
                        a.Offset(-offset)
                        for curve in a.getCurves():
                            curve_cw = curve.IsClockwise()
                            if cw != curve_cw:
                                curve.Reverse()
                            set_good_start_point(curve, False)
                            profile(curve, direction, 0.0, 0.0, roll_radius, roll_on, roll_off, depthparams, extend_at_start, extend_at_end, lead_in_line_len, lead_out_line_len)
                        using_area_for_offset = False
                        return                    
                    else:
                        raise Exception, "couldn't offset kurve " + str(offset_curve)
            
    # extend curve
    if extend_at_start > 0.0:
        span = offset_curve.GetFirstSpan()
        new_start = span.p + span.GetVector(0.0) * ( -extend_at_start)
        new_curve = area.Curve()
        new_curve.append(new_start)
        for vertex in offset_curve.getVertices():
            new_curve.append(vertex)
        offset_curve = new_curve
        
    if extend_at_end > 0.0:
        span = offset_curve.GetLastSpan()
        new_end = span.v.p + span.GetVector(1.0) * extend_at_end
        offset_curve.append(new_end)
                
    # remove tags further than radius from the offset kurve
    new_tags = []
    for tag in tags:
        if tag.dist(offset_curve) <= radius + 0.001:
            new_tags.append(tag)
    tags = new_tags

    if offset_curve.getNumVertices() <= 1:
        raise "sketch has no spans!"

    # do multiple depths
    depths = depthparams.get_depths()

    current_start_depth = depthparams.start_depth

    # tags
    if len(tags) > 0:
        # make a copy to restore to after each level
        copy_of_offset_curve = area.Curve(offset_curve)
    
    prev_depth = depthparams.start_depth
    
    endpoint = None
    
    for depth in depths:
        mat_depth = prev_depth
        
        if len(tags) > 0:
            split_for_tags(offset_curve, radius, depthparams.start_depth, depth, depthparams.final_depth)

        # make the roll on and roll off kurves
        roll_on_curve = area.Curve()
        add_roll_on(offset_curve, roll_on_curve, direction, roll_radius, offset_extra, roll_on)
        roll_off_curve = area.Curve()
        add_roll_off(offset_curve, roll_off_curve, direction, roll_radius, offset_extra, roll_off)
        if use_CRC():
            crc_start_point = area.Point()
            add_CRC_start_line(offset_curve,roll_on_curve,roll_off_curve,radius,direction,crc_start_point,lead_in_line_len)
        
        # get the tag depth at the start
        start_z = get_tag_z_for_span(0, offset_curve, radius, depthparams.start_depth, depth, depthparams.final_depth)
        if start_z > mat_depth: mat_depth = start_z

        # rapid across to the start
        s = roll_on_curve.FirstVertex().p
        
        # start point 
        if (endpoint == None) or (endpoint != s):
            if use_CRC():
                rapid(crc_start_point.x,crc_start_point.y)
            else:
                rapid(s.x, s.y)
        
            # rapid down to just above the material
            if endpoint == None:
                rapid(z = mat_depth + depthparams.rapid_safety_space)
            else:
                rapid(z = mat_depth)

        # feed down to depth
        mat_depth = depth
        if start_z > mat_depth: mat_depth = start_z
        feed(z = mat_depth)

        if use_CRC():
            start_CRC(direction == "left", radius)
            # move to the startpoint
            feed(s.x, s.y)
        
        # cut the roll on arc
        cut_curve(roll_on_curve)
        
        # cut the main kurve
        current_perim = 0.0
        
        for span in offset_curve.GetSpans():
            # height for tags
            current_perim += span.Length()
            ez = get_tag_z_for_span(current_perim, offset_curve, radius, depthparams.start_depth, depth, depthparams.final_depth)
            
            if span.v.type == 0:#line
                feed(span.v.p.x, span.v.p.y, ez)
            else:
                if span.v.type == 1:# anti-clockwise arc
                    arc_ccw(span.v.p.x, span.v.p.y, ez, i = span.v.c.x, j = span.v.c.y)
                else:
                    arc_cw(span.v.p.x, span.v.p.y, ez, i = span.v.c.x, j = span.v.c.y)
                    
    
        # cut the roll off arc
        cut_curve(roll_off_curve)

        endpoint = offset_curve.LastVertex().p
        if roll_off_curve.getNumVertices() > 0:
            endpoint = roll_off_curve.LastVertex().p
        
        #add CRC end_line
        if use_CRC():
            crc_end_point = area.Point()
            add_CRC_end_line(offset_curve,roll_on_curve,roll_off_curve,radius,direction,crc_end_point,lead_out_line_len)
            if direction == "on":
                rapid(z = depthparams.clearance_height)
            else:
                feed(crc_end_point.x, crc_end_point.y)
            
              
        # restore the unsplit kurve
        if len(tags) > 0:
            offset_curve = area.Curve(copy_of_offset_curve)
        if use_CRC():
            end_CRC()            
        
        if endpoint != s:
            # rapid up to the clearance height
            rapid(z = depthparams.clearance_height)   
            
        prev_depth = depth

    rapid(z = depthparams.clearance_height)        

    del offset_curve
                
    if len(tags) > 0:
        del copy_of_offset_curve
