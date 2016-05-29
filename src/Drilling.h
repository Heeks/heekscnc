
#ifndef DRILLING_CYCLE_CLASS_DEFINITION
#define DRILLING_CYCLE_CLASS_DEFINITION

// Drilling.h
/*
 * Copyright (c) 2009, Dan Heeks, Perttu Ahola
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "DepthOp.h"
#include "HeeksCNCTypes.h"
#include <list>
#include <vector>
#include "CNCPoint.h"

class CDrilling;

class CDrillingParams{

public:
	double m_dwell;			// If dwell_bottom is non-zero then we're using the G82 drill cycle rather than G83 peck drill cycle.  This is the 'P' word
	int    m_retract_mode;	// boring - 0 - rapid retract, 1 - feed retract
	int    m_spindle_mode;	// boring - if true, stop spindle at bottom
	bool   m_internal_coolant_on;
	bool   m_rapid_to_clearance;

	void set_initial_values( const double depth, const int tool_number );
	void write_values_to_config();
	void GetProperties(CDrilling* parent, std::list<Property *> *list);
	void WriteXMLAttributes(TiXmlNode* pElem);
	void ReadParametersFromXMLElement(TiXmlElement* pElem);

	bool operator== ( const CDrillingParams & rhs ) const;
	bool operator!= ( const CDrillingParams & rhs ) const { return(! (*this == rhs)); }
};

class CDrilling: public CDepthOp {
public:
	/**
		The following two methods are just to draw pretty lines on the screen to represent drilling
		cycle activity when the operator selects the Drilling Cycle operation in the data list.
	 */
	std::list< CNCPoint > PointsAround( const CNCPoint & origin, const double radius, const unsigned int numPoints ) const;
	std::list< CNCPoint > DrillBitVertices( const CNCPoint & origin, const double radius, const double length ) const;

public:
	std::list<int> m_points;
	CDrillingParams m_params;

	//	Constructors.
	CDrilling():CDepthOp(0){}
	CDrilling(	const std::list<int> &points,
			const int tool_number,
			const double depth );

	CDrilling( const CDrilling & rhs );
	CDrilling & operator= ( const CDrilling & rhs );

	// HeeksObj's virtual functions
	virtual int GetType() const {return DrillingType;}
	const wxChar* GetTypeString(void) const { return _("Drilling"); }
	void glCommands(bool select, bool marked, bool no_color);

	const wxBitmap &GetIcon();
	void GetProperties(std::list<Property *> *list);
	HeeksObj *MakeACopy(void)const;
	void CopyFrom(const HeeksObj* object);
	void WriteXML(TiXmlNode *root);
	bool CanAddTo(HeeksObj* owner);
	bool CanAdd(HeeksObj* object);
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);
	void GetOnEdit(bool(**callback)(HeeksObj*));

	// This is the method that gets called when the operator hits the 'Python' button.  It generates a Python
	// program whose job is to generate RS-274 GCode.
	Python AppendTextToProgram();

	void AddPoint(int i){m_points.push_back(i);}

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);

	bool operator==( const CDrilling & rhs ) const;
	bool operator!=( const CDrilling & rhs ) const { return(! (*this == rhs)); }
	bool IsDifferent( HeeksObj *other ) { return( *this != (*(CDrilling *)other) ); }
};




#endif // DRILLING_CYCLE_CLASS_DEFINITION
