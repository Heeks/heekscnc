// Drilling.cpp
/*
 * Copyright (c) 2009, Dan Heeks, Perttu Ahola
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "stdafx.h"
#include "Drilling.h"
#include "CNCConfig.h"
#include "ProgramCanvas.h"
#include "../../interface/HeeksObj.h"
#include "../../interface/HeeksColor.h"
#include "../../interface/PropertyInt.h"
#include "../../interface/PropertyDouble.h"
#include "../../interface/PropertyLength.h"
#include "../../interface/PropertyChoice.h"
#include "../../interface/PropertyString.h"
#include "../../interface/PropertyCheck.h"
#include "../../tinyxml/tinyxml.h"
#include "Operations.h"
#include "CTool.h"
#include "Profile.h"
#include "CNCPoint.h"
#include "Program.h"
#include "../../src/Geom.h"
#include "DrillingDlg.h"
#include "Tools.h"

#include <sstream>
#include <iomanip>
#include <vector>
#include <algorithm>

extern CHeeksCADInterface* heeksCAD;


void CDrillingParams::set_initial_values( const double depth, const int tool_number )
{
	CNCConfig config;

	config.Read(_T("m_dwell"), &m_dwell, 0);
	config.Read(_T("m_retract_mode"), &m_retract_mode, 0);
	config.Read(_T("m_spindle_mode"), &m_spindle_mode, 0);
	config.Read(_T("m_internal_coolant_on"), &m_internal_coolant_on, false);
	config.Read(_T("m_rapid_to_clearance"), &m_rapid_to_clearance, true);
}

void CDrillingParams::write_values_to_config()
{
	CNCConfig config;

	config.Write(_T("m_dwell"), m_dwell);
	config.Write(_T("m_retract_mode"), m_retract_mode);
	config.Write(_T("m_spindle_mode"), m_spindle_mode);
	config.Write(_T("m_internal_coolant_on"), m_internal_coolant_on);
	config.Write(_T("m_rapid_to_clearance"), m_rapid_to_clearance);
}


static void on_set_spindle_mode(int value, HeeksObj* object, bool from_undo_redo)
{
	((CDrilling*)object)->m_params.m_spindle_mode = value;
	((CDrilling*)object)->m_params.write_values_to_config();
}

static void on_set_retract_mode(int value, HeeksObj* object, bool from_undo_redo)
{
	((CDrilling*)object)->m_params.m_retract_mode = value;
	((CDrilling*)object)->m_params.write_values_to_config();
}

static void on_set_dwell(double value, HeeksObj* object)
{
	((CDrilling*)object)->m_params.m_dwell = value;
	((CDrilling*)object)->m_params.write_values_to_config();
}

static void on_set_internal_coolant(bool value, HeeksObj* object)
{
	((CDrilling*)object)->m_params.m_internal_coolant_on = value;
	((CDrilling*)object)->m_params.write_values_to_config();
}

static void on_set_rapid_to_clearance(bool value, HeeksObj* object)
{
	((CDrilling*)object)->m_params.m_rapid_to_clearance = value;
	((CDrilling*)object)->m_params.write_values_to_config();
}

void CDrillingParams::GetProperties(CDrilling* parent, std::list<Property *> *list)
{
	list->push_back(new PropertyDouble(_("dwell"), m_dwell, parent, on_set_dwell));
	{ // Begin choice scope
		std::list< wxString > choices;

		choices.push_back(_("Rapid retract"));	// Must be 'false' (0)
		choices.push_back(_("Feed retract"));	// Must be 'true' (non-zero)

		int choice = int(m_retract_mode);
		list->push_back(new PropertyChoice(_("retract_mode"), choices, choice, parent, on_set_retract_mode));
	} // End choice scope
	{ // Begin choice scope
		std::list< wxString > choices;

		choices.push_back(_("Keep running"));	// Must be 'false' (0)
		choices.push_back(_("Stop at bottom"));	// Must be 'true' (non-zero)

		int choice = int(m_spindle_mode);
		list->push_back(new PropertyChoice(_("spindle_mode"), choices, choice, parent, on_set_spindle_mode));
	} // End choice scope

	list->push_back(new PropertyCheck(_("internal coolant on"), m_internal_coolant_on, parent, on_set_internal_coolant));
	list->push_back(new PropertyCheck(_("rapid to clearance between positions"), m_rapid_to_clearance, parent, on_set_rapid_to_clearance));
}

void CDrillingParams::WriteXMLAttributes(TiXmlNode *root)
{
	TiXmlElement * element;
	element = heeksCAD->NewXMLElement( "params" );
	heeksCAD->LinkXMLEndChild( root,  element );

	element->SetDoubleAttribute( "dwell", m_dwell);
	element->SetAttribute( "retract_mode", m_retract_mode);
	element->SetAttribute( "spindle_mode", m_spindle_mode);
	element->SetAttribute( "internal_coolant_on", m_internal_coolant_on ? 1:0);
	element->SetAttribute( "rapid_to_clearance", m_rapid_to_clearance ? 1:0);
}

void CDrillingParams::ReadParametersFromXMLElement(TiXmlElement* pElem)
{
	if (pElem->Attribute("dwell")) pElem->Attribute("dwell", &m_dwell);
	if (pElem->Attribute("retract_mode")) pElem->Attribute("retract_mode", &m_retract_mode);
	if (pElem->Attribute("spindle_mode")) pElem->Attribute("spindle_mode", &m_spindle_mode);
	int i = 0;
	if (pElem->Attribute("internal_coolant_on", &i)){m_internal_coolant_on = (i != 0); }
	if (pElem->Attribute("rapid_to_clearance", &i)){m_rapid_to_clearance = (i != 0); }
}

const wxBitmap &CDrilling::GetIcon()
{
	if(!m_active)return GetInactiveIcon();
	static wxBitmap* icon = NULL;
	if(icon == NULL)icon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/drilling.png")));
	return *icon;
}

/**
	This method is called when the CAD operator presses the Python button.  This method generates
	Python source code whose job will be to generate RS-274 GCode.  It's done in two steps so that
	the Python code can be configured to generate GCode suitable for various CNC interpreters.
 */
Python CDrilling::AppendTextToProgram()
{
	Python python;

	python << CDepthOp::AppendTextToProgram();   // Set any private fixtures and change tools (if necessary)

	for (std::list<int>::iterator It = m_points.begin(); It != m_points.end(); It++)
	{
		HeeksObj* object = heeksCAD->GetIDObject(PointType, *It);
		if(object == NULL)continue;
		double p[3];
		if(object->GetEndPoint(p) == false)continue;

		python << _T("drill(")
			<< _T("x=") << p[0]/theApp.m_program->m_units << _T(", ")
			<< _T("y=") << p[1]/theApp.m_program->m_units << _T(", ")
			<< _T("dwell=") << m_params.m_dwell << _T(", ")
			<< _T("depthparams = depthparams, ")
			<< _T("retract_mode=") << m_params.m_retract_mode << _T(", ")
			<< _T("spindle_mode=") << m_params.m_spindle_mode << _T(", ")
			<< _T("internal_coolant_on=") << m_params.m_internal_coolant_on << _T(", ")
			<< _T("rapid_to_clearance=") << m_params.m_rapid_to_clearance
			<< _T(")\n");
        theApp.m_location = make_point(p); // Remember where we are.
	} // End for

	python << _T("end_canned_cycle()\n");

	return(python);
}


/**
	This routine generates a list of coordinates around the circumference of a circle.  It's just used
	to generate data suitable for OpenGL calls to paint a circle.  This graphics is transient but will
	help represent what the GCode will be doing when it's generated.
 */
std::list< CNCPoint > CDrilling::PointsAround(
		const CNCPoint & origin,
		const double radius,
		const unsigned int numPoints ) const
{
	std::list<CNCPoint> results;

	double alpha = 3.1415926 * 2 / numPoints;

	unsigned int i = 0;
	while( i++ < numPoints )
	{
		double theta = alpha * i;
		CNCPoint pointOnCircle( cos( theta ) * radius, sin( theta ) * radius, 0 );
		pointOnCircle += origin;
		results.push_back(pointOnCircle);
	} // End while

	return(results);

} // End PointsAround() routine


/**
	Generate a list of vertices that represent the hole that will be drilled.  Let it be a circle at the top, a
	spiral down the length and a countersunk base.

	This method is only called by the glCommands() method.  This means that the graphics is transient.

	TODO: Handle drilling in any rotational angle. At the moment it only handles drilling 'down' along the 'z' axis
 */

std::list< CNCPoint > CDrilling::DrillBitVertices( const CNCPoint & origin, const double radius, const double length ) const
{
	std::list<CNCPoint> top, spiral, bottom, countersink, result;

	double flutePitch = 5.0;	// 5mm of depth per spiral of the drill bit's flute.
	double countersinkDepth = -1 * radius * tan(31.0); // this is the depth of the countersink cone at the end of the drill bit. (for a typical 118 degree bevel)
	unsigned int numPoints = 20;	// number of points in one circle (360 degrees) i.e. how smooth do we want the graphics
	const double pi = 3.1415926;
	double alpha = 2 * pi / numPoints;

	// Get a circle at the top of the dill bit's path
	top = PointsAround( origin, radius, numPoints );
	top.push_back( *(top.begin()) );	// Close the circle

	double depthPerItteration;
	countersinkDepth = -1 * radius * tan(31.0);	// For a typical (118 degree bevel on the drill bit tip)

	unsigned int l_iNumItterations = numPoints * fabs(length / flutePitch);
	depthPerItteration = (length - countersinkDepth) / l_iNumItterations;

	// Now generate the spirals.

	unsigned int i = 0;
	while( i++ < l_iNumItterations )
	{
		double theta = alpha * i;
		CNCPoint pointOnCircle( cos( theta ) * radius, sin( theta ) * radius, 0 );
		pointOnCircle += origin;

		// And spiral down as we go.
		pointOnCircle.SetZ( pointOnCircle.Z() - (depthPerItteration * i) );

		spiral.push_back(pointOnCircle);
	} // End while

	// And now the countersink at the bottom of the drill bit.
	i = 0;
	while( i++ < numPoints )
	{
		double theta = alpha * i;
		CNCPoint topEdge( cos( theta ) * radius, sin( theta ) * radius, 0 );

		// This is at the top edge of the countersink
		topEdge.SetX( topEdge.X() + origin.X() );
		topEdge.SetY( topEdge.Y() + origin.Y() );
		topEdge.SetZ( origin.Z() - (length - countersinkDepth) );
		spiral.push_back(topEdge);

		// And now at the very point of the countersink
		CNCPoint veryTip( origin );
		veryTip.SetZ( (origin.Z() - length) );

		spiral.push_back(veryTip);
		spiral.push_back(topEdge);
	} // End while

	std::copy( top.begin(), top.end(), std::inserter( result, result.begin() ) );
	std::copy( spiral.begin(), spiral.end(), std::inserter( result, result.end() ) );
	std::copy( countersink.begin(), countersink.end(), std::inserter( result, result.end() ) );

	return(result);

} // End DrillBitVertices() routine


/**
	This is the Graphics Library Commands (from the OpenGL set).  This method calls the OpenGL
	routines to paint the drill action in the graphics window.  The graphics is transient.

	Part of its job is to re-paint the elements that this CDrilling object refers to so that
	we know what CAD objects this CNC operation is referring to.
 */
void CDrilling::glCommands(bool select, bool marked, bool no_color)
{
	CDepthOp::glCommands(select, marked, no_color);

	if (select)
	{
		for (std::list<int>::iterator It = m_points.begin(); It != m_points.end(); It++)
		{
			HeeksObj* point = heeksCAD->GetIDObject(PointType, *It);
			if (point)point->glCommands(select, marked, no_color);;
		}
	}

	else if (heeksCAD->ObjectMarked(this))
	{
		heeksCAD->GetBackgroundColor().best_black_or_white().glColor();

		if (m_tool_number > 0)
		{
			for(HeeksObj* object = theApp.m_program->Tools()->GetFirstChild(); object; object = theApp.m_program->Tools()->GetNextChild())
			{
				if(object->GetType() == ToolType)
				{
					CTool* tool= (CTool*)object;
					if(tool->m_tool_number == m_tool_number)
					{

						for (std::list<int>::iterator It = m_points.begin(); It != m_points.end(); It++)
						{
							HeeksObj* object = heeksCAD->GetIDObject(PointType, *It);
							double p[3];
							if(!object->GetEndPoint(p))continue;
							gp_Pnt point = make_point(p);

							GLdouble start[3], end[3];

							start[0] = point.X();
							start[1] = point.Y();
							start[2] = m_depth_op_params.m_start_depth;

							end[0] = point.X();
							end[1] = point.Y();
							end[2] = m_depth_op_params.m_final_depth;

							glBegin(GL_LINE_STRIP);
							glVertex3dv( start );
							glVertex3dv( end );
							glEnd();

							std::list< CNCPoint > pointsAroundCircle = DrillBitVertices( 	make_point(start), tool->m_params.m_diameter / 2, m_depth_op_params.m_start_depth - m_depth_op_params.m_final_depth);

							glBegin(GL_LINE_STRIP);
							CNCPoint previous = *(pointsAroundCircle.begin());
							for (std::list< CNCPoint >::const_iterator l_itPoint = pointsAroundCircle.begin();
								l_itPoint != pointsAroundCircle.end();
								l_itPoint++)
							{

								glVertex3d( l_itPoint->X(), l_itPoint->Y(), l_itPoint->Z() );
							}
							glEnd();
						}
						break;
					}
				}
			}
		} // End for
	} // End if - then
}


void CDrilling::GetProperties(std::list<Property *> *list)
{
	m_params.GetProperties(this, list);
	CDepthOp::GetProperties(list);
}

HeeksObj *CDrilling::MakeACopy(void)const
{
	return new CDrilling(*this);
}

void CDrilling::CopyFrom(const HeeksObj* object)
{
	if (object->GetType() == GetType())
	{
		operator=(*((CDrilling*)object));
	}
}

CDrilling::CDrilling(	const std::list<int> &points,
        const int tool_number,
        const double depth )
    : CDepthOp(tool_number, DrillingType), m_points(points)
{
    m_params.set_initial_values(depth, tool_number);
}


CDrilling::CDrilling( const CDrilling & rhs ) : CDepthOp( rhs )
{
	m_points = rhs.m_points;
    m_params = rhs.m_params;
}

CDrilling & CDrilling::operator= ( const CDrilling & rhs )
{
	if (this != &rhs)
	{
		CDepthOp::operator=(rhs);
		m_points.clear();
		m_points = rhs.m_points;
		m_params = rhs.m_params;
	}

	return(*this);
}

bool CDrilling::CanAddTo(HeeksObj* owner)
{
    if (owner == NULL) return(false);

	int type = owner->GetType();

	if (type == OperationsType) return(true);

	return(false);
}

bool CDrilling::CanAdd(HeeksObj* object)
{
	return false;
}

void CDrilling::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element = heeksCAD->NewXMLElement( "Drilling" );
	heeksCAD->LinkXMLEndChild( root,  element );
	m_params.WriteXMLAttributes(element);

	for (std::list<int>::iterator It = m_points.begin(); It != m_points.end(); It++)
	{
		TiXmlElement * point = heeksCAD->NewXMLElement( "Point" );
		heeksCAD->LinkXMLEndChild( element, point );
		point->SetAttribute("id", *It );
	} // End for

	WriteBaseXML(element);
}

// static member function
HeeksObj* CDrilling::ReadFromXMLElement(TiXmlElement* element)
{
	CDrilling* new_object = new CDrilling;

	std::list<TiXmlElement *> elements_to_remove;

	// read point ids
	for(TiXmlElement* pElem = heeksCAD->FirstXMLChildElement( element ) ; pElem; pElem = pElem->NextSiblingElement())
	{
		std::string name(pElem->Value());
		if(name == "params"){
			new_object->m_params.ReadParametersFromXMLElement(pElem);
			elements_to_remove.push_back(pElem);
		}
		else if(name == "symbols"){
			for(TiXmlElement* child = heeksCAD->FirstXMLChildElement( pElem ) ; child; child = child->NextSiblingElement())
			{
				int type, id;
				if (child->Attribute("type", &type) && child->Attribute("id", &id))
				{
					if(type == PointType)
					new_object->AddPoint(id);
				}
			} // End for
			elements_to_remove.push_back(pElem);
		} // End if
		else if(name == "Point"){
			int id;
			if(pElem->Attribute("id", &id))
			{
				new_object->AddPoint(id);
			}
		}
	}

	for (std::list<TiXmlElement*>::iterator itElem = elements_to_remove.begin(); itElem != elements_to_remove.end(); itElem++)
	{
		heeksCAD->RemoveXMLChild( element, *itElem);
	}

	new_object->ReadBaseXML(element);

	return new_object;
}

void CDrilling::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
    CDepthOp::GetTools( t_list, p );
}

bool CDrillingParams::operator==( const CDrillingParams & rhs) const
{
	if (m_dwell != rhs.m_dwell) return(false);
	if (m_retract_mode != rhs.m_retract_mode) return(false);
	if (m_spindle_mode != rhs.m_spindle_mode) return(false);
	if (m_internal_coolant_on != rhs.m_internal_coolant_on) return(false);
	if (m_rapid_to_clearance != rhs.m_rapid_to_clearance) return(false);

	return(true);
}


bool CDrilling::operator==( const CDrilling & rhs ) const
{
	if (m_params != rhs.m_params) return(false);

	return(CDepthOp::operator==(rhs));
}

static bool OnEdit(HeeksObj* object)
{
	return DrillingDlg::Do((CDrilling*)object);
}

void CDrilling::GetOnEdit(bool(**callback)(HeeksObj*))
{
	*callback = OnEdit;
}
