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

class CCuttingTool: public CTools {
public:
	//	These are references to the CAD elements whose position indicate where the CuttingTool Cycle begins.
	CCuttingToolParams m_params;

	//	Constructors.
	CCuttingTool():CTools() { m_params.set_initial_values();  }

	// HeeksObj's virtual functions
	int GetType()const{return CuttingToolType;}
	const wxChar* GetTypeString(void)const{return _T("CuttingTool");}
	void glCommands(bool select, bool marked, bool no_color);

	// TODO Draw a drill cycle icon and refer to it here.
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




