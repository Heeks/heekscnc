from Object import Object
from consts import *
import HeeksCNC

class Tool(Object):
    def __init__(self, diameter = 3.0, title = None, tool_number = 0, type = TOOL_TYPE_SLOTCUTTER):
        Object.__init__(self)
        self.tool_number = tool_number
        self.type = type
        self.diameter = diameter
        self.material = TOOL_MATERIAL_UNDEFINED
        self.tool_length_offset = 0.0
        self.x_offset = 0.0
        self.front_angle = 0.0
        self.tool_angle = 0.0
        self.back_angle = 0.0
        self.orientation = 0
        
        '''
        // also m_corner_radius, see below, is used for turning tools and milling tools


        /**
                The next three parameters describe the cutting surfaces of the bit.

                The two radii go from the centre of the bit -> flat radius -> corner radius.
                The vertical_cutting_edge_angle is the angle between the centre line of the
                milling bit and the angle of the outside cutting edges.  For an end-mill, this
                would be zero.  i.e. the cutting edges are parallel to the centre line
                of the milling bit.  For a chamfering bit, it may be something like 45 degrees.
                i.e. 45 degrees from the centre line which has both cutting edges at 2 * 45 = 90
                degrees to each other

                For a ball-nose milling bit we would have
                        - m_corner_radius = m_diameter / 2
                        - m_flat_radius = 0    // No middle bit at the bottom of the cutter that remains flat
                                                // before the corner radius starts.
                        - m_vertical_cutting_edge_angle = 0

                For an end-mill we would have
                        - m_corner_radius = 0
                        - m_flat_radius = m_diameter / 2
                        - m_vertical_cutting_edge_angle = 0

                For a chamfering bit we would have
                        - m_corner_radius = 0
                        - m_flat_radius = 0    // sharp pointed end.  This may be larger if we can't use the centre point.
                        - m_vertical_cutting_edge_angle = 45    // degrees from centre line of tool
         */
        '''
        self.corner_radius = 0.0
        self.flat_radius = 0.0
        self.cutting_edge_angle = 0.0
        self.cutting_edge_height = 0.0    # How far, from the bottom of the cutter, do the flutes extend?
        self.max_advance_per_revolution = 0.0
        '''    // This is the maximum distance a tool should advance during a single
                                                // revolution.  This value is often defined by the manufacturer in
                                                // terms of an advance no a per-tooth basis.  This value, however,
                                                // must be expressed on a per-revolution basis.  i.e. we don't want
                                                // to maintain the number of cutting teeth so a per-revolution
                                                // value is easier to use.
        '''
        self.automatically_generate_title = True    #// Set to true by default but reset to false when the user edits the title.
        '''
        // The following coordinates relate ONLY to touch probe tools.  They describe
        // the error the probe tool has in locating an X,Y point.  These values are
        // added to a probed point's location to find the actual point.  The values
        // should come from calibrating the touch probe.  i.e. set machine position
        // to (0,0,0), drill a hole and then probe for the centre of the hole.  The
        // coordinates found by the centre finding operation should be entered into
        // these values verbatim.  These will represent how far off concentric the
        // touch probe's tip is with respect to the quil.  Of course, these only
        // make sense if the probe's body is aligned consistently each time.  I will
        // ASSUME this is correct.
        '''
        self.probe_offset_x = 0.0
        self.probe_offset_y = 0.0
        '''
        // The following  properties relate to the extrusions created by a reprap style 3D printer.
        // using temperature, speed, and the height of the nozzle, and the nozzle size it's possible to create
        // many different sizes and shapes of extrusion.
        typedef std::pair< eExtrusionMaterial_t, wxString > ExtrusionMaterialDescription_t
        typedef std::vector<ExtrusionMaterialDescription_t > ExtrusionMaterialsList_t

        static ExtrusionMaterialsList_t GetExtrusionMaterialsList()
        {
                ExtrusionMaterialsList_t ExtrusionMaterials_list

                ExtrusionMaterials_list.push_back( ExtrusionMaterialDescription_t( eABS, wxString(_("ABS Plastic")) ))
                ExtrusionMaterials_list.push_back( ExtrusionMaterialDescription_t( ePLA, wxString(_("PLA Plastic")) ))
                ExtrusionMaterials_list.push_back( ExtrusionMaterialDescription_t( eHDPE, wxString(_("HDPE Plastic")) ))

                return(ExtrusionMaterials_list)
        }
        '''
        self.extrusion_material = EXTRUSION_MATERIAL_ABS
        self.feedrate = 0.0
        self.layer_height = 0.1
        self.width_over_thickness = 1.0
        self.temperature = 200
        self.flowrate = 10
        self.filament_diameter = 0.2
        '''
        // The gradient is the steepest angle at which this tool can plunge into the material.  Many
        // tools behave better if they are slowly ramped down into the material.  This gradient
        // specifies the steepest angle of decsent.  This is expected to be a negative number indicating
        // the 'rise / run' ratio.  Since the 'rise' will be downward, it will be negative.
        // By this measurement, a drill bit's straight plunge would have an infinite gradient (all rise, no run).
        // To cater for this, a value of zero will indicate a straight plunge.
        '''
        self.gradient = 0.0
        '''
        // properties for tapping tools
        int m_direction    // 0.. right hand tapping, 1..left hand tapping
        double m_pitch     // in units/rev
        '''
        
        if title != None:
            self.title = title
        else:
            self.title = self.GenerateMeaningfulName()
            
        self.ResetParametersToReasonableValues()
        
    def TypeName(self):
        return "Tool"
    
    def name(self):
        return self.title
    
    def icon(self):
        # the name of the PNG file in the HeeksCNC icons folder
        return "tool"
    
    def ResetParametersToReasonableValues(self):
        if self.type != TOOL_TYPE_TURNINGTOOL:
            self.tool_length_offset = (5 * self.diameter)

            self.gradient = self.ReasonableGradient(self.type)

            if self.type == TOOL_TYPE_DRILL:
                self.corner_radius = 0.0
                self.flat_radius = 0.0
                self.cutting_edge_angle = 59.0
                self.cutting_edge_height = self.diameter * 3.0
                self.ResetTitle()

            elif self.type == TOOL_TYPE_CENTREDRILL:
                self.corner_radius = 0.0
                self.flat_radius = 0.0
                self.cutting_edge_angle = 59.0
                self.cutting_edge_height = self.diameter * 1.0
                self.ResetTitle()

            elif self.type == TOOL_TYPE_ENDMILL:
                self.corner_radius = 0.0
                self.flat_radius = self.diameter / 2
                self.cutting_edge_angle = 0.0
                self.cutting_edge_height = self.diameter * 3.0
                self.ResetTitle()

            elif self.type == TOOL_TYPE_SLOTCUTTER:
                self.corner_radius = 0.0
                self.flat_radius = self.diameter / 2
                self.cutting_edge_angle = 0.0
                self.cutting_edge_height = self.diameter * 3.0
                self.ResetTitle()

            elif self.type == TOOL_TYPE_BALLENDMILL:
                self.corner_radius = (self.diameter / 2)
                self.flat_radius = 0.0
                self.cutting_edge_angle = 0.0
                self.cutting_edge_height = self.diameter * 3.0
                self.ResetTitle()
            '''
                case CToolParams::eTouchProbe:
                self.corner_radius = (self.diameter / 2)
                self.flat_radius = 0
                                ResetTitle()
                                break

                case CToolParams::eExtrusion:
                self.corner_radius = (self.diameter / 2)
                self.flat_radius = 0
                                ResetTitle()
                                break

                case CToolParams::eToolLengthSwitch:
                self.corner_radius = (self.diameter / 2)
                                ResetTitle()
                                break

                case CToolParams::eChamfer:
                self.corner_radius = 0
                self.flat_radius = 0
                self.cutting_edge_angle = 45
                                height = (self.diameter / 2.0) * tan( degrees_to_radians(90.0 - self.cutting_edge_angle))
                self.cutting_edge_height = height
                                ResetTitle()
                                break

                case CToolParams::eTurningTool:
                                // No special constraints for this.
                                ResetTitle()
                                break

                case CToolParams::eTapTool:
                                self.tool_length_offset = (5 * self.diameter)
                self.automatically_generate_title = 1
                self.diameter = 6.0
                self.direction = 0
                self.pitch = 1.0
                self.cutting_edge_height = self.diameter * 3.0
                                ResetTitle()
                                break

                default:
                                wxMessageBox(_T("That is not a valid tool type. Aborting value change."))
                                return
            '''

    def ReasonableGradient(self, type):
        if self.type == TOOL_TYPE_SLOTCUTTER or self.type == TOOL_TYPE_ENDMILL or self.type == TOOL_TYPE_BALLENDMILL:
            return -0.1
        return 0.0

    def GenerateMeaningfulName(self):
        name_str = ""
        if self.type != TOOL_TYPE_TURNINGTOOL and self.type != TOOL_TYPE_TOUCHPROBE and self.type != TOOL_TYPE_TOOLLENGTHSWITCH:
            if HeeksCNC.program.units == 1.0:
                # We're using metric.  Leave the diameter as a floating point number.  It just looks more natural.
                name_str = name_str + str(self.diameter) + " mm "
            else:
                # We're using inches.
                # to do, Find a fractional representation if one matches.
                name_str = name_str + str(self.diameter/HeeksCNC.program.units) + " inch "
                
        if self.type != TOOL_TYPE_EXTRUSION and self.type != TOOL_TYPE_TOUCHPROBE and self.type != TOOL_TYPE_TOOLLENGTHSWITCH:
            if self.material == TOOL_MATERIAL_HSS:
                name_str = name_str + "HSS "
            elif self.material == TOOL_MATERIAL_CARBIDE:
                name_str = name_str + "Carbide "
        
        if self.type == TOOL_TYPE_EXTRUSION:
            if self.extrusion_material == EXTRUSION_MATERIAL_ABS:
                name_str = name_str + "ABS "
            elif self.extrusion_material == EXTRUSION_MATERIAL_PLA:
                name_str = name_str + "PLA "
            elif self.extrusion_material == EXTRUSION_MATERIAL_HDPE:
                name_str = name_str + "HDPE "
                
        if self.type == TOOL_TYPE_DRILL:
            name_str = name_str + "Drill Bit"
        elif self.type == TOOL_TYPE_CENTREDRILL:
            name_str = name_str + "Centre Drill Bit"
        elif self.type == TOOL_TYPE_ENDMILL:
            name_str = name_str + "End Mill"
        elif self.type == TOOL_TYPE_SLOTCUTTER:
            name_str = name_str + "Slot Cutter"
        elif self.type == TOOL_TYPE_BALLENDMILL:
            name_str = name_str + "Ball End Mill"
        elif self.type == TOOL_TYPE_CHAMFER:
            # Remove all that we've already prepared.
            name_str = str(self.cutting_edge_angle) + " degree" + "Chamfering Bit"
        elif self.type == TOOL_TYPE_TURNINGTOOL:
            name_str = name_str + "Turning Tool"
        elif self.type == TOOL_TYPE_TOUCHPROBE:
            name_str = name_str + "Touch Probe"
        elif self.type == TOOL_TYPE_EXTRUSION:
            name_str = name_str + "Extrusion"
        elif self.type == TOOL_TYPE_TOOLLENGTHSWITCH:
            name_str = name_str + "Tool Length Switch"
        elif self.type == TOOL_TYPE_TAPTOOL:
            # to do, copy code from CTool.cpp
            name_str = name_str + "Tap Tool"
        
        return name_str
    
    def ResetTitle(self):
        if self.automatically_generate_title:
            self.title = self.GenerateMeaningfulName()
