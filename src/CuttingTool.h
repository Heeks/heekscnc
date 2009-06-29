
#ifndef CUTTING_TOOL_CLASS_DEFINTION
#define CUTTING_TOOL_CLASS_DEFINTION

// CuttingTool.h
/*
 * Copyright (c) 2009, Dan Heeks, Perttu Ahola
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "Program.h"
#include "Op.h"
#include "HeeksCNCTypes.h"

class CCuttingTool;

class CCuttingToolParams{
	
public:

	typedef enum {
		eDrill = 0,
		eEndmill,
		eSlotCutter,
		eBallEndMill,
		eChamfer
	} eCuttingToolType;

	// The G10 command can be used (within EMC2) to add a tool to the tool
	// table from within a program.
	// G10 L1 P[tool number] R[radius] X[offset] Z[offset] Q[orientation]

	double m_diameter;
	double m_x_offset;
	double m_tool_length_offset;
	int m_orientation;

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

	eCuttingToolType	m_type;

	void set_initial_values();
	void write_values_to_config();
	void GetProperties(CCuttingTool* parent, std::list<Property *> *list);
	void WriteXMLAttributes(TiXmlNode* pElem);
	void ReadParametersFromXMLElement(TiXmlElement* pElem);
};

class CCuttingTool: public HeeksObj {
public:
	//	These are references to the CAD elements whose position indicate where the CuttingTool Cycle begins.
	CCuttingToolParams m_params;
        wxString m_title;
	int m_tool_number;

	//	Constructors.
        CCuttingTool(const wxChar *title, const int tool_number) : m_tool_number(tool_number)
	{
		m_params.set_initial_values(); 
		if (title != NULL) 
		{
			m_title = title;
		} // End if - then
		else
		{
			m_title = GenerateMeaningfulName();
		} // End if - else
	} // End constructor

	 // HeeksObj's virtual functions
        int GetType()const{return CuttingToolType;}
	const wxChar* GetTypeString(void) const{ return _T("CuttingTool"); }
        HeeksObj *MakeACopy(void)const;

        void WriteXML(TiXmlNode *root);
        static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);

	// program whose job is to generate RS-274 GCode.
	void AppendTextToProgram();

	void GetProperties(std::list<Property *> *list);
	void CopyFrom(const HeeksObj* object);
	bool CanAddTo(HeeksObj* owner);
	wxString GetIcon() { return theApp.GetResFolder() + _T("/icons/tool"); }
        const wxChar* GetShortString(void)const{return m_title.c_str();}
	void glCommands(bool select, bool marked, bool no_color);

        bool CanEditString(void)const{return true;}
        void OnEditString(const wxChar* str);

	static int FindCuttingTool( const int tool_number );
	wxString GenerateMeaningfulName() const;
	wxString ResetTitle();

}; // End CCuttingTool class definition.




#endif // CUTTING_TOOL_CLASS_DEFINTION
