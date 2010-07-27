import area
from nc.nc import *

# some globals, to save passing variables as parameters too much
area_for_pocket = None
first_offset_for_pocket = None

def curve_start(curve):
    vlist = curve.getVertices()
    return vlist[0].p

def cut_curve(curve, need_rapid, p, rapid_down_to_height, final_depth, stepover):
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

            first = False
        else:
            dc = vertex.c - prev_p
            if vertex.type == 1:
                arc_ccw(vertex.p.x, vertex.p.y, i = dc.x, j = dc.y)
            elif vertex.type == -1:
                arc_cw(vertex.p.x, vertex.p.y, i = dc.x, j = dc.y)
            else:
                feed(vertex.p.x, vertex.p.y)

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
    obround = make_obround(p0, p1, first_offset_for_pocket)
    a = area.Area(area_for_pocket)
    obround.Subtract(a)
    if obround.num_curves() > 0:
        return False
    return True

def cut_curvelist(curve_list, rapid_down_to_height, depth, clearance_height, stepover):
    p = area.Point(0, 0)
    first = True
    for curve in curve_list:
        need_rapid = True
        if(first == False):
            # see if we can feed across
            s = curve_start(curve)
            if feed_possible(p, s):
                need_rapid = False
            if need_rapid:
                rapid(z = clearance_height)
        p = cut_curve(curve, need_rapid, p, rapid_down_to_height, depth, stepover)
        first = False
    rapid(z = clearance_height)
    
def recur(arealist, a1, stepover, from_center):
    # this makes arealist by recursively offsetting a1 inwards
    
    if a1.num_curves() == 0:
        return
    
    if from_center:
        arealist.insert(0, a1)
    else:
        arealist.append(a1)

    a_offset = area.Area(a1)
    a_offset.Offset(stepover)
    
    # split curves into new areas
    for curve in a_offset.getCurves():
        a2 = area.Area()
        a2.append(curve)
        recur(arealist, a2, stepover, from_center)
        
def get_curve_list(arealist):
    curve_list = list()
    for a in arealist:
        for curve in a.getCurves():
            curve_list.append(curve)
    return curve_list

def pocket(a, first_offset, rapid_down_to_height, start_depth, final_depth, stepover, stepdown, round_corner_factor, clearance_height, from_center):
    global area_for_pocket
    global first_offset_for_pocket
    area_for_pocket = a
    first_offset_for_pocket = first_offset
        
    if rapid_down_to_height > clearance_height:
        rapid_down_to_height = clearance_height
    
    area.set_round_corner_factor(round_corner_factor)

    arealist = list()

    a_firstoffset = area.Area(a)
    a_firstoffset.Offset(first_offset)
    
    recur(arealist, a_firstoffset, stepover, from_center)
    
    curve_list = get_curve_list(arealist)
    
    layer_count = int((start_depth - final_depth) / stepdown)

    if layer_count * stepdown + 0.00001 < start_depth - final_depth:
        layer_count += 1

    for i in range(1, layer_count+1):
        if i == layer_count:
            depth = final_depth
        else:
            depth = start_depth - i * stepdown

        offset_value = first_offset
        a_offset = area.Area(a)
        area.set_round_corner_factor(round_corner_factor)
        a_offset.Offset(offset_value)
        
        cut_curvelist(curve_list, rapid_down_to_height, depth, clearance_height, stepover)


