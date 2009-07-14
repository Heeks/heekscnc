import kurve
import area
from nc.nc import *

def make_area_for_roughing(k):
    num_spans = kurve.num_spans(k)

    if num_spans == 0:
        raise "sketch has no spans!"

    d, startx, starty, ex, ey, cx, cy = kurve.get_span(k, 0)
    d, sx, sy, endx, endy, cx, cy = kurve.get_span(k, num_spans - 1)
    a = area.new()

    largey = 500
    
    for span in range(0, num_spans):
        d, sx, sy, ex, ey, cx, cy = kurve.get_span(k, span)
        if span == 0:# first span
            area.add_point(a, 0, startx, largey, 0, 0)
            area.add_point(a, 0, startx, starty, 0, 0)
        area.add_point(a, d, ex, ey, cx, cy)

    # close the area
    area.add_point(a, 0, endx, largey, 0, 0)
    area.add_point(a, 0, startx, largey, 0, 0)

    return a

def rough(k, tool_radius, tool_angle, front_angle, back_angle, clearance):
    # make a closed area from the kurve
    a = make_area_for_roughing(k)

    # adjust the shape to add clearance for front angle and back angle
    # to do

    # offset it by the tool radius
    area.offset(a, tool_radius)

    # move it to take into account where the driven point is
    # to do

    # do some clever things with intersecting rectangular area with this one to make some tool path

    # for now do some sample tool path to indicate the kind of thing this will do when it's finished
    rapid(20, 20)
    rapid(2, 8)
    feed(-30, 8)
    rapid(-29, 9)
    rapid(2, 9)
    rapid(2, 6)
    feed(-30, 6)
    rapid(-29, 7)
    rapid(2, 7)
    rapid(2, 4)
    feed(-30, 4)
    rapid(-29, 5)
    rapid(2, 5)

    # rapid away maybe
    rapid(20, 20)
    
    
