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


def pocket(a, first_offset, rapid_down_to_height, start_depth, final_depth, stepover, stepdown, round_corner_factor, clearance_height):
    
    if rapid_down_to_height > clearance_height:
        rapid_down_to_height = clearance_height
    
    areas = list()

    offset_value = first_offset
    a_offset = area.new()
    area.copy(a, a_offset)
    area.set_round_corner_factor(round_corner_factor)
    area.offset(a_offset, offset_value)
    
    while area.num_curves(a_offset):
        a_1 = area.new()
        area.copy(a_offset, a_1)
        areas.append(a_1)
        area.copy(a, a_offset)
        offset_value = offset_value + stepover
        area.offset(a_offset, offset_value)

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
        
        b = 0
        for a_offset in areas:
            cut_area(a_offset, rapid_down_to_height, depth, clearance_height)


