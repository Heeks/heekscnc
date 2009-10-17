
#ifndef PROBING_CYCLE_CLASS_DEFINITION
#define PROBING_CYCLE_CLASS_DEFINITION

// Probing.h
/*
 * Copyright (c) 2009, Dan Heeks, Perttu Ahola
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "SpeedOp.h"
#include "HeeksCNCTypes.h"
#include <list>
#include <vector>
#include "CNCPoint.h"
#include "interface/Tool.h"

class CProbing;

/**
	The CProbing class represents a base class from which a variety of probing patterns/styles may be
	based.  For example a Probe_LinearCentre_Outside class might be based on the CProbing class whereby the resultant
	GCode might produce a program that runs from the current location up and outwards before dropping down
	and probing back towards the original position.  It would repeat this at one other point and finish by
	moving back to the original Z location but to the middle of the two probed XY locations.  The operator would
	then be able to set a relative coordinate system manually based on the machine's current location.

	The idea is not (yet) to have the Probing classes GCode be included in that for a normal data model.  Instead,
	this class is intended to be a helper class (wizard would be the next step) that helps to produce the
	necessary GCode programs appropriate to the various probing actions required.  It could almost be replaced
	by standalone GCode programs with editable parameters but I like the idea of having all the GCode functionality
	available in one place (HeeksCNC).  There are already enough individual utilities to cure cancer.  The problem
	is that they are all written with a sufficiently different user interface as to make them difficult to
	utilize.  The hope is that, by incorporating much of this functionality into HeeksCNC, the interaction
	with the user will be easier to work through.

	I can imagine a time when the results of some of these probing operations could generate an XML report of
	their results.  The user would need to retrieve this XML file and ask HeeksCNC to read it back in so that
	things like fixture rotations could be set accordingly.  That's just a wish-list thing though.  At the 
	moment this class will do what I need it to (today)

	It is based on CSpeedOp so that feed rates can be obtained.
 */

class CProbing: public CSpeedOp {
public:
	//	Constructors.
	CProbing( const wxString title, const int cutting_tool_number):CSpeedOp(title, cutting_tool_number)
	{
		m_speed_op_params.m_spindle_speed = 0;	// We don't want the spindle to move while we're probing.
		m_active = 0;	// We don't want the normal GCode generation routines to include us.
		m_depth = 10.0;	// mm

		// If the cutting tool number has been defined as a probe already, use half the probe's length
		// as the depth to plunge (by default)
		CCuttingTool *pCuttingTool = CCuttingTool::Find(cutting_tool_number);
		if ((pCuttingTool != NULL) && (pCuttingTool->m_params.m_type == CCuttingToolParams::eTouchProbe))
		{
			m_depth = pCuttingTool->m_params.m_tool_length_offset / 2.0;
		}
	}
	void GetProperties(std::list<Property *> *list);
	void WriteBaseXML(TiXmlElement *element);
	void ReadBaseXML(TiXmlElement* element);

	double m_depth;			// How far to drop down from the current position before starting to probe inwards.
};


/**
	This class probes from the current location to find the centre point between two points of the
	item currently beneath the probe's tip.  i.e. from the outside inwards.  The angle that defines
	whether the outer starting points are north/south or east/west is also defined in this class.
 */
class CProbe_LinearCentre_Outside: public CProbing {
public:
	//	Constructors.
	CProbe_LinearCentre_Outside(const int cutting_tool_number = 0) : CProbing(_("Probe Linear Centre Outside"), cutting_tool_number )
	{
		m_starting_angle = 0.0;	// degrees
		m_starting_distance = 50.0;	// mm
	}

	// HeeksObj's virtual functions
	int GetType()const{return ProbeLinearCentreOutsideType;}
	void WriteXML(TiXmlNode *root);
	const wxChar* GetTypeString(void)const{return _T("Probe Linear Centre Outside");}
	void glCommands(bool select, bool marked, bool no_color);

	wxString GetIcon(){if(m_active)return theApp.GetResFolder() + _T("/icons/probe"); else return COp::GetIcon();}
	void GetProperties(std::list<Property *> *list);
	HeeksObj *MakeACopy(void)const;
	void CopyFrom(const HeeksObj* object);
	bool CanAddTo(HeeksObj* owner);

	// This is the method that gets called when the operator hits the 'Python' button.  It generates a Python
	// program whose job is to generate RS-274 GCode.
	void AppendTextToProgram( const CFixture *pFixture );

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);

	wxString GetOutputFileName(const wxString extension, const bool filename_only);

public:
	double m_starting_angle;		// Direction to move before dropping down and probing back.  We should not
					// just assume north/south or east/west as the item we're probing may be
					// a rectangular object oriented differently to the axes.

	double m_starting_distance;	// Distance from starting point outwards before dropping down and probing in.
	// The probing feed rate will be taken from CSpeedOp::m_speed_op_params.m_horozontal_feed_rate


};


#endif // PROBING_CYCLE_CLASS_DEFINITION
