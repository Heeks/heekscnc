// DepthOp.h
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

// base class for machining operations which can have multiple depths

#ifndef DEPTH_OP_HEADER
#define DEPTH_OP_HEADER

#include "SpeedOp.h"
#include <list>

class CDepthOp;

class CDepthOpParams{
public:
	double m_clearance_height;
	double m_start_depth;
	double m_step_down;
	double m_final_depth;
	double m_rapid_down_to_height;

	CDepthOpParams();

	void set_initial_values(const std::list<int> *sketches = NULL, const int cutting_tool_number = -1);
	void write_values_to_config();
	void GetProperties(CDepthOp* parent, std::list<Property *> *list);
	void WriteXMLAttributes(TiXmlNode* pElem);
	void ReadFromXMLElement(TiXmlElement* pElem);
};

class CDepthOp : public CSpeedOp
{
public:
	CDepthOpParams m_depth_op_params;

	CDepthOp(const wxString& title, const std::list<int> *sketches = NULL, const int cutting_tool_number = -1, const int operation_type = UnknownType )
		: CSpeedOp(title, cutting_tool_number, operation_type)
	{
		ReadDefaultValues();
		SetDepthsFromSketchesAndTool(sketches);
	}

	CDepthOp & operator= ( const CDepthOp & rhs );
	CDepthOp( const CDepthOp & rhs );

	// HeeksObj's virtual functions
	void GetProperties(std::list<Property *> *list);
	void WriteBaseXML(TiXmlElement *element);
	void ReadBaseXML(TiXmlElement* element);
	void ReloadPointers();

	// COp's virtual functions
	void WriteDefaultValues();
	void ReadDefaultValues();
	void AppendTextToProgram(const CFixture *pFixture);
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);
	void glCommands(bool select, bool marked, bool no_color);

	void SetDepthsFromSketchesAndTool(const std::list<int> *sketches);
	std::list<wxString> DesignRulesAdjustment(const bool apply_changes);
};

#endif
