// SpeedOp.h
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

// base class for machining operations which have feedrates and spindle speed

#ifndef SPEED_OP_HEADER
#define SPEED_OP_HEADER

#include "Op.h"

class CSpeedOp;

class CSpeedOpParams{
public:
	double m_horizontal_feed_rate;
	double m_vertical_feed_rate;
	double m_spindle_speed;

	CSpeedOpParams();

	void GetProperties(CSpeedOp* parent, std::list<Property *> *list);
	void WriteXMLAttributes(TiXmlNode* pElem);
	void ReadFromXMLElement(TiXmlElement* pElem);
	void ResetSpeeds(const int cutting_tool_number);
	void ResetFeeds(const int cutting_tool_number);
};

class CSpeedOp : public COp
{
public:
	CSpeedOpParams m_speed_op_params;

	static bool m_auto_set_speeds_feeds;

	CSpeedOp(const wxString& title, const int cutting_tool_number = -1 ):COp(title, cutting_tool_number){ReadDefaultValues();}

	// HeeksObj's virtual functions
	void GetProperties(std::list<Property *> *list);
	void WriteBaseXML(TiXmlElement *element);
	void ReadBaseXML(TiXmlElement* element);

	// COp's virtual functions
	void WriteDefaultValues();
	void ReadDefaultValues();
	void AppendTextToProgram(const CFixture *pFixture);

	static void GetOptions(std::list<Property *> *list);
	static void ReadFromConfig();
	static void WriteToConfig();
};

#endif
