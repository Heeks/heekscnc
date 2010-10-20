'''
    Von:     Lai <wuling@gmx.at>
    Betreff:     [heekscad-users] don't know where to put ScriptOp scripts
    Datum:     20. Oktober 2010 19:14:41 MESZ
'''

def cutcone(x_cen, y_cen, z_cen, top_r, bottom_r, depth, step_over):
    if top_r >= bottom_r:
        step_count = math.pi * top_r * 2 / step_over
    else:
        step_count = math.pi * bottom_r * 2 / step_over
    loop_count = 0
    while (loop_count < 360):
        top_x    = math.sin(loop_count * math.pi / 180) * top_r
        top_y    = math.cos(loop_count * math.pi / 180) * top_r
        bottom_x = math.sin(loop_count * math.pi / 180) * bottom_r
     bottom_y = math.cos(loop_count * math.pi / 180) * bottom_r

     feed(x=(x_cen + top_x), y=(y_cen + top_y), z=(z_cen))
     feed(x=(x_cen + bottom_x), y=(y_cen + bottom_y), z=(z_cen - depth))
     feed(z=(z_cen))
     loop_count = loop_count + (360 / step_count)



def cone(x_cen, y_cen, z_cen, tool_id, tooldiameter, spindle_speed,
         horizontal_feedrate, vertical_feedrate,
         depth, diameter, angle, z_safe, step_over, step_down):
    
   tool_r = tooldiameter / 2
   top_r = diameter / 2

   comment('tool change')
   tool_change(id=tool_id)
   spindle(spindle_speed)
   feedrate_hv(horizontal_feedrate, vertical_feedrate)

   bottom_r = top_r - (math.tan(angle * math.pi / 180) * depth)

   #toolradius correction
   if top_r >= bottom_r:
      top_r = top_r - tool_r
      bottom_r = bottom_r - tool_r
   if top_r < bottom_r:
       top_r = top_r + tool_r
       bottom_r = bottom_r + tool_r

   #correction if angle is too big or depth is to high
   if bottom_r < 0:
       bottom_r = bottom_r * -1
       depth = depth - (bottom_r / math.tan(angle * math.pi / 180))
       bottom_r = 0

   #calculate vertical step_down rate to a horizontal step_rate (have
no better idea)
   cone_feed = (step_down / math.tan(angle * math.pi / 180))
   if angle < 0 :
     cone_feed = cone_feed * -1
   flush_nc()

   rapid(x=(x_cen + bottom_r), y=y_cen)
   rapid(z=z_safe)

   #cutout to the bottom
   loop_feed = 0
   while(loop_feed < depth):
     loop_feed = loop_feed + step_down
     if loop_feed >= depth:
       feed(z=(z_cen - depth))
     else:
       feed(z=(z_cen - loop_feed))
     arc_ccw(x=(x_cen - bottom_r), y=y_cen, i= -bottom_r, j=0)
     arc_ccw(x=(x_cen + bottom_r), y=y_cen, i=bottom_r, j=0)
   feed(z=z_cen)

   #cut the cone
   loop_feed = 0
   while(loop_feed < depth):
     loop_feed = loop_feed + cone_feed
     if loop_feed >= depth:
       temp_depth = depth
     else:
       temp_depth = loop_feed
     temp_top_r = bottom_r + (math.tan(angle * math.pi / 180) * 
temp_depth)

cutcone(x_cen, y_cen, z_cen, temp_top_r, bottom_r, temp_depth, step_over)

rapid(z=z_safe)

