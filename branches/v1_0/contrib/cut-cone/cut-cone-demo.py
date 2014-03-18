'''
    Von:     Lai <wuling@gmx.at>
    Betreff:     [heekscad-users] don't know where to put ScriptOp scripts
    Datum:     20. Oktober 2010 19:14:41 MESZ
'''

tool_id = 1
tool_diameter = 4
spindle_speed = 3000
horizontal_feedrate = 300
vertical_feedrate = 300
rapid_down_to_height = 5

x_position = 20
y_position = 20
z_position = 0

top_diameter = 20
angle = -30
final_depth = 9
step_down = 4
step_over = 0.5

cone(x_position, y_position, z_position, tool_id, tool_diameter, spindle_speed,
     horizontal_feedrate, vertical_feedrate,
     final_depth, top_diameter, angle, rapid_down_to_height, step_over, step_down)

x_position = 0
y_position = 0
z_position = 0

top_diameter = 20
angle = 30
final_depth = 9
step_down = 4
step_over = 0.5

cone(x_position, y_position, z_position, tool_id, tool_diameter, spindle_speed,
     horizontal_feedrate, vertical_feedrate,
     final_depth, top_diameter, angle, rapid_down_to_height, step_over, step_down)

