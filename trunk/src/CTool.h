
// CTool.h
/*
 * Copyright (c) 2009, Dan Heeks, Perttu Ahola
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#pragma once

#include "Op.h"
#include "HeeksCNCTypes.h"

#include <vector>
#include <algorithm>

class CTool;
class CAttachOp;

class CToolParams{

public:

	typedef enum {
		eDrill = 0,
		eCentreDrill,
		eEndmill,
		eSlotCutter,
		eBallEndMill,
		eChamfer,
		eTurningTool,
		eTouchProbe,
		eToolLengthSwitch,
		eExtrusion,
		eTapTool,
		eUndefinedToolType
	} eToolType;

	typedef std::pair< eToolType, wxString > ToolTypeDescription_t;
	typedef std::vector<ToolTypeDescription_t > ToolTypesList_t;

	static ToolTypesList_t GetToolTypesList()
	{
		ToolTypesList_t types_list;

		types_list.push_back( ToolTypeDescription_t( eDrill, wxString(_("Drill Bit")) ));
		types_list.push_back( ToolTypeDescription_t( eCentreDrill, wxString(_("Centre Drill Bit")) ));
		types_list.push_back( ToolTypeDescription_t( eEndmill, wxString(_("End Mill")) ));
		types_list.push_back( ToolTypeDescription_t( eSlotCutter, wxString(_("Slot Cutter")) ));
		types_list.push_back( ToolTypeDescription_t( eBallEndMill, wxString(_("Ball End Mill")) ));
		types_list.push_back( ToolTypeDescription_t( eChamfer, wxString(_("Chamfer")) ));
#ifndef STABLE_OPS_ONLY
		types_list.push_back( ToolTypeDescription_t( eTurningTool, wxString(_("Turning Tool")) ));
#endif
		types_list.push_back( ToolTypeDescription_t( eTouchProbe, wxString(_("Touch Probe")) ));
		types_list.push_back( ToolTypeDescription_t( eToolLengthSwitch, wxString(_("Tool Length Switch")) ));
#ifndef STABLE_OPS_ONLY
		types_list.push_back( ToolTypeDescription_t( eExtrusion, wxString(_("Extrusion")) ));
#endif
#ifndef STABLE_OPS_ONLY
		types_list.push_back( ToolTypeDescription_t( eTapTool, wxString(_("Tapping Tool")) ));
#endif
		return(types_list);
	} // End GetToolTypesList() method



	typedef enum {
		eHighSpeedSteel = 0,
		eCarbide,
		eUndefinedMaterialType
	} eMaterial_t;



	typedef std::pair< eMaterial_t, wxString > MaterialDescription_t;
	typedef std::vector<MaterialDescription_t > MaterialsList_t;

	static MaterialsList_t GetMaterialsList()
	{
		MaterialsList_t materials_list;

		materials_list.push_back( MaterialDescription_t( eHighSpeedSteel, wxString(_("High Speed Steel")) ));
		materials_list.push_back( MaterialDescription_t( eCarbide, wxString(_("Carbide")) ));

		return(materials_list);
	} // End Get() method

	// The G10 command can be used (within EMC2) to add a tool to the tool
	// table from within a program.
	// G10 L1 P[tool number] R[radius] X[offset] Z[offset] Q[orientation]

	int m_material;	// eMaterial_t - describes the cutting surface type.




	double m_diameter;
	double m_tool_length_offset;

	// The following are all for lathe tools.  They become relevant when the m_type = eTurningTool
	double m_x_offset;
	double m_front_angle;
	double m_tool_angle;
	double m_back_angle;
	int m_orientation;
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

		For a ball-nose milling bit we would have;
			- m_corner_radius = m_diameter / 2
			- m_flat_radius = 0;	// No middle bit at the bottom of the cutter that remains flat
						// before the corner radius starts.
			- m_vertical_cutting_edge_angle = 0

		For an end-mill we would have;
			- m_corner_radius = 0;
			- m_flat_radius = m_diameter / 2
			- m_vertical_cutting_edge_angle = 0

		For a chamfering bit we would have;
			- m_corner_radius = 0;
			- m_flat_radius = 0;	// sharp pointed end.  This may be larger if we can't use the centre point.
			- m_vertical_cutting_edge_angle = 45	// degrees from centre line of tool
	 */
	double m_corner_radius;
	double m_flat_radius;
	double m_cutting_edge_angle;
	double m_cutting_edge_height;	// How far, from the bottom of the cutter, do the flutes extend?

	eToolType	m_type;
	double m_max_advance_per_revolution;	// This is the maximum distance a tool should advance during a single
						// revolution.  This value is often defined by the manufacturer in
						// terms of an advance no a per-tooth basis.  This value, however,
						// must be expressed on a per-revolution basis.  i.e. we don't want
						// to maintain the number of cutting teeth so a per-revolution
						// value is easier to use.

	int m_automatically_generate_title;	// Set to true by default but reset to false when the user edits the title.

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

	double m_probe_offset_x;
	double m_probe_offset_y;

	// The following  properties relate to the extrusions created by a reprap style 3D printer.
	// using temperature, speed, and the height of the nozzle, and the nozzle size it's possible to create
	// many different sizes and shapes of extrusion.

	typedef enum {
		eABS = 0,
		ePLA,
		eHDPE,
		eUndefinedExtrusionMaterialType
	} eExtrusionMaterial_t;
	typedef std::pair< eExtrusionMaterial_t, wxString > ExtrusionMaterialDescription_t;
	typedef std::vector<ExtrusionMaterialDescription_t > ExtrusionMaterialsList_t;

	static ExtrusionMaterialsList_t GetExtrusionMaterialsList()
	{
		ExtrusionMaterialsList_t ExtrusionMaterials_list;

		ExtrusionMaterials_list.push_back( ExtrusionMaterialDescription_t( eABS, wxString(_("ABS Plastic")) ));
		ExtrusionMaterials_list.push_back( ExtrusionMaterialDescription_t( ePLA, wxString(_("PLA Plastic")) ));
		ExtrusionMaterials_list.push_back( ExtrusionMaterialDescription_t( eHDPE, wxString(_("HDPE Plastic")) ));

		return(ExtrusionMaterials_list);
	}
	int m_extrusion_material;
	double m_feedrate;
	double m_layer_height;
	double m_width_over_thickness;
	double m_temperature;
	double m_flowrate;
	double m_filament_diameter;



	// The gradient is the steepest angle at which this tool can plunge into the material.  Many
	// tools behave better if they are slowly ramped down into the material.  This gradient
	// specifies the steepest angle of decsent.  This is expected to be a negative number indicating
	// the 'rise / run' ratio.  Since the 'rise' will be downward, it will be negative.
	// By this measurement, a drill bit's straight plunge would have an infinite gradient (all rise, no run).
	// To cater for this, a value of zero will indicate a straight plunge.

	double m_gradient;

	// properties for tapping tools
	int m_direction;    // 0.. right hand tapping, 1..left hand tapping
        double m_pitch;     // in units/rev

	void set_initial_values();
	void write_values_to_config();
	void GetProperties(CTool* parent, std::list<Property *> *list);
	void WriteXMLAttributes(TiXmlNode* pElem);
	void ReadParametersFromXMLElement(TiXmlElement* pElem);

	const wxString ConfigScope(void)const{return _T("ToolParam_");}
	double ReasonableGradient( const eToolType type ) const;

	bool operator== ( const CToolParams & rhs ) const;
	bool operator!= ( const CToolParams & rhs ) const { return(! (*this == rhs)); }
};

class CTool: public HeeksObj {
public:
	//	These are references to the CAD elements whose position indicate where the Tool Cycle begins.
	CToolParams m_params;
	wxString m_title;

	typedef int ToolNumber_t;
	ToolNumber_t m_tool_number;
	HeeksObj *m_pToolSolid;

	//	Constructors.
	CTool(const wxChar *title, CToolParams::eToolType type, const int tool_number) : m_tool_number(tool_number), m_pToolSolid(NULL)
	{
		m_params.set_initial_values();
		m_params.m_type = type;
		if (title != NULL)
		{
			m_title = title;
		} // End if - then
		else
		{
			m_title = GenerateMeaningfulName();
		} // End if - else

		ResetParametersToReasonableValues();
	} // End constructor

    CTool( const CTool & rhs );
    CTool & operator= ( const CTool & rhs );

	~CTool();

	bool operator== ( const CTool & rhs ) const;
	bool operator!= ( const CTool & rhs ) const { return(! (*this == rhs)); }

	bool IsDifferent( HeeksObj *other ) { return(*this != (*(CTool *)other)); }

	 // HeeksObj's virtual functions
        int GetType()const{return ToolType;}
	const wxChar* GetTypeString(void) const{ return _T("Tool"); }
        HeeksObj *MakeACopy(void)const;

        void WriteXML(TiXmlNode *root);
        static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);

	// program whose job is to generate RS-274 GCode.
	Python AppendTextToProgram();
	Python OCLDefinition(CAttachOp* attach_op)const;

	void GetProperties(std::list<Property *> *list);
	void CopyFrom(const HeeksObj* object);
	bool CanAddTo(HeeksObj* owner);
	const wxBitmap &GetIcon();
    const wxChar* GetShortString(void)const{return m_title.c_str();}
	void glCommands(bool select, bool marked, bool no_color);
	void KillGLLists(void);
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);

        bool CanEditString(void)const{return true;}
        void OnEditString(const wxChar* str);

	static CTool *Find( const int tool_number );
	static int FindTool( const int tool_number );
	static ToolNumber_t FindFirstByType( const CToolParams::eToolType type );
	static std::vector< std::pair< int, wxString > > FindAllTools();
	wxString GenerateMeaningfulName() const;
	wxString ResetTitle();
	static wxString FractionalRepresentation( const double original_value, const int max_denominator = 64 );
	static wxString GuageNumberRepresentation( const double size, const double units );

	TopoDS_Shape GetShape() const;
	TopoDS_Face  GetSideProfile() const;

	double CuttingRadius(const bool express_in_drawing_units = false, const double depth = -1) const;
	static CToolParams::eToolType CutterType( const int tool_number );
	static CToolParams::eMaterial_t CutterMaterial( const int tool_number );

	void SetDiameter( const double diameter );
	void ResetParametersToReasonableValues();
	void ImportProbeCalibrationData( const wxString & probed_points_xml_file_name );
	double Gradient() const { return(m_params.m_gradient); }

	Python OpenCamLibDefinition(const unsigned int indent = 0);

	void GetOnEdit(bool(**callback)(HeeksObj*));

private:
	void DeleteSolid();

public:
    typedef struct
    {
        wxString description;
        double  diameter;
        double  pitch;
    } tap_sizes_t;

    void SelectTapFromStandardSizes(const tap_sizes_t *tap_sizes);
    std::list<wxString> DesignRulesAdjustment(const bool apply_changes);

}; // End CTool class definition.

