import area
from nc.nc import *

def cut_area(a, rapid_down_to_height, final_depth, clearance_height):
    for curve in range(0, area.num_curves(a)):
        px = 0.0
        py = 0.0
        
        first = True

        for vertex in range(0, area.num_vertices(a, curve)):
            sp, x, y, cx, cy = area.get_vertex(a, curve, vertex)
            
            if first:
                # rapid across
                rapid(x, y)
                
                ##rapid down
                rapid(z = rapid_down_to_height)
                
                #feed down
                feed(z = final_depth)

                first = False
            else:
                if sp == 1:
                    arc_ccw(x, y, i = cx - px, j = cy - py)
                elif sp == -1:
                    arc_cw(x, y, i = cx - px, j = cy - py)
                else:
                    feed(x, y)

            px = x
            py = y

        rapid(z = clearance_height)

def recur(arealist, a1, stepover, from_center):
    if area.num_curves(a1) == 0:
        return
    
    if from_center:
        arealist.insert(0, a1)
    else:
        arealist.append(a1)

    a_offset = area.new()
    area.copy(a1, a_offset)
    area.offset(a_offset, stepover)
    
    for curve in range(0, area.num_curves(a_offset)):
        a2 = area.new()
        area.add_curve(a2, a_offset, curve)
        recur(arealist, a2, stepover, from_center)

def pocket(a, first_offset, rapid_down_to_height, start_depth, final_depth, stepover, stepdown, round_corner_factor, clearance_height, from_center):
    
    if rapid_down_to_height > clearance_height:
        rapid_down_to_height = clearance_height
    
    area.set_round_corner_factor(round_corner_factor)

    arealist = list()

    a_firstoffset = area.new()
    area.copy(a, a_firstoffset)
    area.offset(a_firstoffset, first_offset)
    
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
        a_offset = area.new()
        area.copy(a, a_offset)
        area.set_round_corner_factor(round_corner_factor)
        area.offset(a_offset, offset_value)
        
        for a_offset in arealist:
            cut_area(a_offset, rapid_down_to_height, depth, clearance_height)


