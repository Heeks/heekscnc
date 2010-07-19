import area
from nc.nc import *

def cut_area(a, need_rapid, p, rapid_down_to_height, final_depth, clearance_height, stepover):
    for curve in a.getCurves():
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
                    arc_cw(x, y, i = dc.x, j = dc.y)
                else:
                    feed(vertex.p.x, vertex.p.y)

            prev_p = vertex.p

        rapid(z = clearance_height)
        
def cut_arealist(arealist, rapid_down_to_height, depth, clearance_height, stepover):
    need_rapid = True
    p = area.Point(0, 0)
    for a in arealist:
        cut_area(a, need_rapid, p, rapid_down_to_height, depth, clearance_height, stepover)
    
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

def pocket(a, first_offset, rapid_down_to_height, start_depth, final_depth, stepover, stepdown, round_corner_factor, clearance_height, from_center):
    
    if rapid_down_to_height > clearance_height:
        rapid_down_to_height = clearance_height
    
    area.set_round_corner_factor(round_corner_factor)

    arealist = list()

    a_firstoffset = area.Area(a)
    a_firstoffset.Offset(first_offset)
    
    recur(arealist, a_firstoffset, stepover, from_center)
    
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
        
        cut_arealist(arealist, rapid_down_to_height, depth, clearance_height, stepover)


