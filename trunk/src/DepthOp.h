// DepthOp.h
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

// base class for machining operations which can have multiple depths

#ifndef DEPTH_OP_HEADER
#define DEPTH_OP_HEADER

#include "Op.h"

class CDepthOp;

class CDepthOpParams{
public:
	int m_workplane;
	int m_tool_number;    
	double m_tool_diameter;
	double m_clearance_height;
	double m_start_depth;
	double m_step_down;
	double m_final_depth;
	double m_rapid_down_to_height;
	double m_horizontal_feed_rate;
	double m_vertical_feed_rate;
	double m_spindle_speed;

	CDepthOpParams();

	void set_initial_values();
	void write_values_to_config();
	void GetProperties(CDepthOp* parent, std::list<Property *> *list);
	void WriteXMLAttributes(TiXmlNode* pElem);
	void ReadFromXMLElement(TiXmlElement* pElem);
};

class CDepthOp : public COp
{
public:
	CDepthOpParams m_depth_op_params;

	CDepthOp(const wxString& title):COp(title){m_depth_op_params.set_initial_values();}

	// HeeksObj's virtual functions
	void GetProperties(std::list<Property *> *list);
	void WriteBaseXML(TiXmlElement *element);
	void ReadBaseXML(TiXmlElement* element);

	virtual void AppendTextToProgram();
};

#endif
