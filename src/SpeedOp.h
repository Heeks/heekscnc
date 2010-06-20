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
	bool operator== ( const CSpeedOpParams & rhs ) const;
	bool operator!= ( const CSpeedOpParams & rhs ) const { return(! (*this == rhs)); }

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

	CSpeedOp(const wxString& title, const int cutting_tool_number = -1, const int operation_type = UnknownType )
            :COp(title, cutting_tool_number, operation_type)
    {
        ReadDefaultValues();
    }

	CSpeedOp & operator= ( const CSpeedOp & rhs );
	CSpeedOp( const CSpeedOp & rhs );

	// HeeksObj's virtual functions
	void GetProperties(std::list<Property *> *list);
	void WriteBaseXML(TiXmlElement *element);
	void ReadBaseXML(TiXmlElement* element);

	// COp's virtual functions
	void WriteDefaultValues();
	void ReadDefaultValues();
	Python AppendTextToProgram(CMachineState *pMachineState);

	static void GetOptions(std::list<Property *> *list);
	static void ReadFromConfig();
	static void WriteToConfig();
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);
	void glCommands(bool select, bool marked, bool no_color);
	void ReloadPointers() { COp::ReloadPointers(); }

	static wxString ConfigScope() { return(_T("SpeedOp")); }

	bool operator==( const CSpeedOp & rhs ) const;
	bool operator!=( const CSpeedOp & rhs ) const { return(! (*this == rhs)); }
	bool IsDifferent(HeeksObj *other) { return( *this != (*((CSpeedOp *) other))); }
};

#endif
