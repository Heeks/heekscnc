import kurve
import area
from nc.nc import *
import math
# roughing_funcs.py- intended to be used for lathe roughing 
# adapted from area_funcs.py and turning.py
# and possibly roughing a profile-approaching the part from the side
# some globals, to save passing variables as parameters too much
area_for_feed_possible = None
tool_radius_for_pocket = None

def make_area_for_roughing(k):
    num_spans = kurve.num_spans(k)

    if num_spans == 0:
        raise "sketch has no spans!"

    d, startx, starty, ex, ey, cx, cy = kurve.get_span(k, 0)
    d, sx, sy, endx, endy, cx, cy = kurve.get_span(k, num_spans - 1)
    a = area.Area()
    c = area.Curve()
    largey = 7
    
    for span in range(0, num_spans):
        d, sx, sy, ex, ey, cx, cy = kurve.get_span(k, span)
        if span == 0:# first span
            c.append(area.Vertex(0, area.Point(startx, largey), area.Point(0, 0)))

        c.append(area.Vertex(d, area.Point(ex, ey), area.Point(cx, cy)))
    # close the area

    c.append(area.Vertex(0, area.Point(endx, largey), area.Point(0, 0)))
    c.append(area.Vertex(0, area.Point(startx, largey), area.Point(0, 0)))
    a.append(c)
    return a



def cut_curve(curve, need_rapid, p, rapid_down_to_height, final_depth):
    prev_p = p
    first = True

    for vertex in curve.getVertices():
        if need_rapid and first:
            # rapid across
            rapid(vertex.p.x, vertex.p.y)
            
            ##rapid down
            rapid(z = rapid_down_to_height)
            
            #feed down
            feed(z = final_depth)
            #x_first=vertex.p.x;y_first=vertex.p.y
            first = False
        else:
            dc = vertex.c - prev_p
            if vertex.type == 1:
                arc_ccw(vertex.p.x, vertex.p.y, i = dc.x, j = dc.y)
            elif vertex.type == -1:
                arc_cw(vertex.p.x, vertex.p.y, i = dc.x, j = dc.y)
            else:
                feed(vertex.p.x, vertex.p.y)
            #rapid(x_first,y_first)
                #rapid(x_first)
                #rapid(vertex.p.y)
            #x_first=vertex.p.x;y_first=vertex.p.y
        #rapid(x=(vertex.p.x+1))
        prev_p = vertex.p

    return prev_p

def cut_curve_lathe(curve, need_rapid, p, rapid_down_to_height, final_depth):
    prev_p = p
    first = True
    l = []
    feed(z=0)
    for vertex in curve.getVertices():
        if need_rapid and first:
            # rapid across
            rapid(vertex.p.x, vertex.p.y)
            
            first = False        
        l.append((vertex.p.x,vertex.p.y))
    feed(x=l[0][0])
    feed(y=l[0][1])
    feed(x=l[1][0])
#pull tool away from profile at 45 degree angle- back towards Y+ and X start point

    rapid(x=(l[1][0]+(l[2][1]-l[0][1])),y=l[2][1])   
    
        
    rapid(x=l[3][0])
    rapid(y=l[0][1])
        
    prev_p = vertex.p

    return prev_p





        
def area_distance(a, old_area):
    best_dist = None
    
    for curve in a.getCurves():
        for vertex in curve.getVertices():
            c = old_area.NearestPoint(vertex.p)
            d = c.dist(vertex.p)
            if best_dist == None or d < best_dist:
                best_dist = d
    
    for curve in old_area.getCurves():
        for vertex in curve.getVertices():
            c = a.NearestPoint(vertex.p)
            d = c.dist(vertex.p)
            if best_dist == None or d < best_dist:
                best_dist = d
                
    return best_dist
    
def make_obround(p0, p1, radius):
    dir = p1 - p0
    d = dir.length()
    dir.normalize()
    right = area.Point(dir.y, -dir.x)
    obround = area.Area()
    c = area.Curve()
    vt0 = p0 + right * radius
    vt1 = p1 + right * radius
    vt2 = p1 - right * radius
    vt3 = p0 - right * radius
    c.append(area.Vertex(0, vt0, area.Point(0, 0)))
    c.append(area.Vertex(0, vt1, area.Point(0, 0)))
    c.append(area.Vertex(1, vt2, p1))
    c.append(area.Vertex(0, vt3, area.Point(0, 0)))
    c.append(area.Vertex(1, vt0, p0))
    obround.append(c)
    return obround
    
def feed_possible(p0, p1):    
    obround = make_obround(p0, p1, tool_radius_for_pocket)
    a = area.Area(area_for_feed_possible)
    obround.Subtract(a)
    if obround.num_curves() > 0:
        return False
    return True

def cut_curvelist(curve_list, rapid_down_to_height, depth, clearance_height, keep_tool_down_if_poss):
    p = area.Point(0, 0)
    first = True
    for curve in curve_list:
        need_rapid = True
        if first == False:
            s = curve.FirstVertex().p
            if keep_tool_down_if_poss == True:
                # see if we can feed across
                if feed_possible(p, s):
                    need_rapid = False
            elif s.x == p.x and s.y == p.y:
                need_rapid = False
                #rapid(p.x,p.y)
        if need_rapid:
            rapid(z = clearance_height)
        p = cut_curve_lathe(curve, need_rapid, p, rapid_down_to_height, depth)
        
        first = False
    rapid(z = clearance_height)
    

        
def get_curve_list(arealist):
    curve_list = list()
    for a in arealist:
        for curve in a.getCurves():
            curve_list.append(curve)
    return curve_list

curve_list_for_zigs = []
rightward_for_zigs = True
sin_angle_for_zigs = 0.0
cos_angle_for_zigs = 1.0
sin_minus_angle_for_zigs = 0.0
cos_minus_angle_for_zigs = 1.0
test_count = 0

def make_zig_curve(curve, y0, y):
    global test_count
    
    if rightward_for_zigs:
        curve.Reverse()
        
    zig = area.Curve()
    
    zig_started = False
    zag_found = False
    
    prev_p = None
    
    for vertex in curve.getVertices():
        if prev_p != None:
            if math.fabs(vertex.p.y - y0) < 0.002:
                if zig_started:
                    zig.append(unrotated_vertex(vertex))
                elif math.fabs(prev_p.y - y0) < 0.002 and vertex.type == 0:
                    zig.append(area.Vertex(0, unrotated_point(prev_p), area.Point(0, 0)))
                    zig.append(unrotated_vertex(vertex))
                    zig_started = True
            elif zig_started:
                zig.append(unrotated_vertex(vertex))
                if math.fabs(vertex.p.y - y) < 0.002:
                    zag_found = True
                    break
        prev_p = vertex.p
        
    if zig_started:
        curve_list_for_zigs.append(zig)        

def make_zig(a, y0, y):
    for curve in a.getCurves():
        make_zig_curve(curve, y0, y)
        
reorder_zig_list_list = []
        
def add_reorder_zig(curve):
    global reorder_zig_list_list
    
    # look in existing lists
    s = curve.FirstVertex().p
    for curve_list in reorder_zig_list_list:
        last_curve = curve_list[len(curve_list) - 1]
        e = last_curve.LastVertex().p
        if math.fabs(s.x - e.x) < 0.002 and math.fabs(s.y - e.y) < 0.002:
            curve_list.append(curve)
            return
        
    # else add a new list
    curve_list = []
    curve_list.append(curve)
    reorder_zig_list_list.append(curve_list)

def reorder_zigs():
    global curve_list_for_zigs
    global reorder_zig_list_list
    reorder_zig_list_list = []
    for curve in curve_list_for_zigs:
        add_reorder_zig(curve)
        
    curve_list_for_zigs = []
    for curve_list in reorder_zig_list_list:
        for curve in curve_list:
            curve_list_for_zigs.append(curve)
            
def rotated_point(p):
    return area.Point(p.x * cos_angle_for_zigs - p.y * sin_angle_for_zigs, p.x * sin_angle_for_zigs + p.y * cos_angle_for_zigs)
    
def unrotated_point(p):
    return area.Point(p.x * cos_minus_angle_for_zigs - p.y * sin_minus_angle_for_zigs, p.x * sin_minus_angle_for_zigs + p.y * cos_minus_angle_for_zigs)

def rotated_vertex(v):
    if v.type:
        return area.Vertex(v.type, rotated_point(v.p), rotated_point(v.c))
    return area.Vertex(v.type, rotated_point(v.p), area.Point(0, 0))

def unrotated_vertex(v):
    if v.type:
        return area.Vertex(v.type, unrotated_point(v.p), unrotated_point(v.c))
    return area.Vertex(v.type, unrotated_point(v.p), area.Point(0, 0))

def rotated_area(a):
    an = area.Area()
    for curve in a.getCurves():
        curve_new = area.Curve()
        for v in curve.getVertices():
            curve_new.append(rotated_vertex(v))
        an.append(curve_new)
    return an

def zigzag(a, a_firstoffset, stepover):
    if a.num_curves() == 0:
        return
    
    global rightward_for_zigs
    global curve_list_for_zigs
    global test_count
    global sin_angle_for_zigs
    global cos_angle_for_zigs
    global sin_minus_angle_for_zigs
    global cos_minus_angle_for_zigs
    
    a = rotated_area(a)
    
    b = area.Box()
    a.GetBox(b)
    
    #x0 = b.MinX() - 1.0
    #x1 = b.MaxX() + 1.0
    x1 = b.MinX() - 1.0
    x0 = b.MaxX() + 1.0

    height = b.MaxY() - b.MinY()
    num_steps = int(height / stepover + 1)
    #y = b.MinY() + 0.1
    y = b.MaxY() - 0.1
    null_point = area.Point(0, 0)
    rightward_for_zigs = True
    curve_list_for_zigs = []
    test_count = 0
    
    for i in range(0, num_steps):
#collect vertices for a  box shape from X+,Y+ toward the curve
#then move the tool Y+ and then back toward the X start position
#   ------->
#  |
#   -------<
        test_count = test_count + 1
        y0 = y
        #y = y + stepover
        y = y - stepover
        p0 = area.Point(x0, y0)
        p1 = area.Point(x0, y)
        p2 = area.Point(x1, y)
        p3 = area.Point(x1, y0)
        c = area.Curve()
        c.append(area.Vertex(0, p0, null_point, 0))
        c.append(area.Vertex(0, p1, null_point, 0))
        c.append(area.Vertex(0, p2, null_point, 1))
        c.append(area.Vertex(0, p3, null_point, 0))
        c.append(area.Vertex(0, p0, null_point, 1))

        a2 = area.Area()
        a2.append(c)
        a2.Intersect(a)
        
        rightward_for_zigs = (rightward_for_zigs == False)
        

        y10 = y + stepover
        #y = y + stepover
        y2 = y + stepover*2
        p10 = area.Point(x0, y10)
        p11 = area.Point(x0, y2)
        p12 = area.Point(x1, y2)
        p13 = area.Point(x1, y10)
        c2 = area.Curve()
        c2.append(area.Vertex(0, p10, null_point, 0))
        c2.append(area.Vertex(0, p11, null_point, 0))
        c2.append(area.Vertex(0, p12, null_point, 1))
        c2.append(area.Vertex(0, p13, null_point, 0))
        c2.append(area.Vertex(0, p10, null_point, 1))
        a3 = area.Area()
        a3.append(c2)
        a3.Intersect(a)
        make_zig(a3, y0, y)
        rightward_for_zigs = (rightward_for_zigs == False)
        
    reorder_zigs()

def pocket(a, tool_radius, extra_offset, rapid_down_to_height, start_depth, final_depth, stepover, stepdown, round_corner_factor, clearance_height, from_center, keep_tool_down_if_poss, use_zig_zag, zig_angle):
    global area_for_feed_possible
    global tool_radius_for_pocket
    global sin_angle_for_zigs
    global cos_angle_for_zigs
    global sin_minus_angle_for_zigs
    global cos_minus_angle_for_zigs
    tool_radius_for_pocket = tool_radius
    radians_angle = zig_angle * math.pi / 180
    sin_angle_for_zigs = math.sin(-radians_angle)
    cos_angle_for_zigs = math.cos(-radians_angle)
    sin_minus_angle_for_zigs = math.sin(radians_angle)
    cos_minus_angle_for_zigs = math.cos(radians_angle)
        
    if rapid_down_to_height > clearance_height:
        rapid_down_to_height = clearance_height
    
    area.set_round_corner_factor(round_corner_factor)

    arealist = list()
    
    area_for_feed_possible = area.Area(a)
    area_for_feed_possible.Offset(extra_offset - 0.01)

    a_firstoffset = area.Area(a)
    a_firstoffset.Offset(tool_radius + extra_offset)
    
    if use_zig_zag:
        zigzag(a_firstoffset, a_firstoffset, stepover)
        curve_list = curve_list_for_zigs
    else:
        pass #we're just using zig_zag for roughing

        
    layer_count = int((start_depth - final_depth) / stepdown)

    if layer_count * stepdown + 0.00001 < start_depth - final_depth:
        layer_count += 1

    for i in range(1, layer_count+1):
        if i == layer_count:
            depth = final_depth
        else:
            depth = start_depth - i * stepdown
        
        cut_curvelist(curve_list, rapid_down_to_height, depth, clearance_height, keep_tool_down_if_poss)

def rough_open_prof(k,tool_diameter, extra_offset, rapid_down_to_height, start_depth, final_depth, stepover, stepdown, round_corner_factor, clearance_height):
    pass
    a = make_area_for_roughing(k)
    pocket(a, tool_diameter/2, extra_offset, rapid_down_to_height, start_depth, final_depth, stepover, stepdown, round_corner_factor, clearance_height, 1, True, True, 0)
    #pocket(a7, tool_diameter/2, 0.05, rapid_down_to_height, start_depth, final_depth, 0.075, step_down, 1, clearance, 1, True, True, 0)












