// CuttingTool.h
/*
 * Copyright (c) 2009, Dan Heeks, Perttu Ahola
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "Op.h"
#include "HeeksCNCTypes.h"

class CCuttingTool;

class CCuttingToolParams{
	
public:

	// The G10 command can be used (within EMC2) to add a tool to the tool
	// table from within a program.
	// G10 L1 P[tool number] R[radius] X[offset] Z[offset] Q[orientation]

	int m_pocket_number;
	int m_tool_number;
	double m_diameter;
	double m_x_offset;
	double m_tool_length_offset;
	int m_orientation;

	void set_initial_values();
	void write_values_to_config();
	void GetProperties(CCuttingTool* parent, std::list<Property *> *list);
	void WriteXMLAttributes(TiXmlNode* pElem);
	void ReadParametersFromXMLElement(TiXmlElement* pElem);
};

/**
	The CCuttingTool class stores a list of symbols (by type/id pairs) of elements that represent the starting point
	of a drilling cycle.  In the first instance, we use PointType objects as starting points.  Rather than copy
	the PointType elements into this class, we just refer to them by ID.  In the case of PointType objects,
	the class assumes that the drilling will occur in the negative Z direction.

	One day, when I get clever, I intend supporting the reference of line elements whose length defines the
	drill's depth and whose orientation describes the drill's orientation at machining time (i.e. rotate A, B and/or C axes)
 */

class CCuttingTool: public COp {
private:
	/**
		There are all types of 3d point classes around but most of them seem to be in the HeeksCAD code
		rather than in cod that's accessible by the plugin.  I suspect I'm missing something on this
		but, just in case I'm not, here is a special one (just for this class)
	 */
	typedef struct Point3d {
		double x;
		double y;
		double z;
		Point3d( double a, double b, double c ) : x(a), y(b), z(c) { }
	} Point3d;

	/**
		The following two methods are just to draw pretty lines on the screen to represent drilling
		cycle activity when the operator selects the CuttingTool Cycle operation in the data list.
	 */
	std::list< Point3d > PointsAround( const Point3d & origin, const double radius, const unsigned int numPoints ) const;
	std::list< Point3d > DrillBitVertices( const Point3d & origin, const double radius, const double length ) const;

public:
	//	These are references to the CAD elements whose position indicate where the CuttingTool Cycle begins.
	CCuttingToolParams m_params;

	//	Constructors.
	CCuttingTool():COp(GetTypeString()) { m_params.set_initial_values();  }

	// HeeksObj's virtual functions
	int GetType()const{return CuttingToolType;}
	const wxChar* GetTypeString(void)const{return _T("CuttingTool");}
	void glCommands(bool select, bool marked, bool no_color);

	// TODO Draw a drill cycle icon and refer to it here.
	wxString GetIcon(){if(m_active)return theApp.GetResFolder() + _T("/icons/adapt"); else return COp::GetIcon();}
	void GetProperties(std::list<Property *> *list);
	HeeksObj *MakeACopy(void)const;
	void CopyFrom(const HeeksObj* object);
	void WriteXML(TiXmlNode *root);
	bool CanAddTo(HeeksObj* owner);

	// This is the method that gets called when the operator hits the 'Python' button.  It generates a Python
	// program whose job is to generate RS-274 GCode.
	void AppendTextToProgram();

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);


};




