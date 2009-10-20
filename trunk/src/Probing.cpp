// Probing.cpp
/*
 * Copyright (c) 2009, Dan Heeks, Perttu Ahola
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "stdafx.h"
#include "Probing.h"
#include "CNCConfig.h"
#include "ProgramCanvas.h"
#include "interface/HeeksObj.h"
#include "interface/PropertyInt.h"
#include "interface/PropertyDouble.h"
#include "interface/PropertyLength.h"
#include "interface/PropertyChoice.h"
#include "interface/Tool.h"
#include "interface/strconv.h"
#include "tinyxml/tinyxml.h"
#include "Operations.h"
#include "CuttingTool.h"
#include "Profile.h"
#include "Fixture.h"
#include "CNCPoint.h"
#include "PythonStuff.h"

#include <sstream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <memory>

#include <gp_Ax1.hxx>

extern CHeeksCADInterface* heeksCAD;


static void on_set_distance(double value, HeeksObj* object)
{
	((CProbing*)object)->m_distance = value;
}

static void on_set_depth(double value, HeeksObj* object)
{
	((CProbing*)object)->m_depth = value;
}

void CProbe_Centre::AppendTextToProgram( const CFixture *pFixture )
{
	CSpeedOp::AppendTextToProgram( pFixture );

#ifdef UNICODE
	std::wostringstream ss;
#else
	std::ostringstream ss;
#endif
	ss.imbue(std::locale("C"));
	ss<<std::setprecision(10);

	// We're going to be working in relative coordinates based on the assumption
	// that the operator has first jogged the machine to the approximate centre point.

	if (m_direction == eOutside)
	{
		ss << "comment('This program assumes that the machine operator has jogged')\n";
		ss << "comment('the machine to approximatedly the correct location')\n";
		ss << "comment('immediately above the protrusion we are finding the centre of.')\n";
		ss << "comment('This program then jogs out and down in two opposite directions')\n";
		ss << "comment('before probing back towards the centre point looking for the')\n";
		ss << "comment('protrusion.')\n";
		theApp.m_program_canvas->m_textCtrl->AppendText(ss.str().c_str());
		ss.str(_(""));

		AppendTextForSingleProbeOperation( pFixture, gp_Pnt(m_distance,0,0), gp_Pnt(m_distance,0,0), m_depth, gp_Pnt(0,0,0), _("1001"), _("1002") );
		AppendTextForSingleProbeOperation( pFixture, gp_Pnt(-1.0 * m_distance,0,0), gp_Pnt(-1.0 * m_distance,0,0), m_depth, gp_Pnt(0,0,0), _("1003"), _("1004") );
	} // End if - then
	else
	{
		ss << "comment('This program assumes that the machine operator has jogged')\n";
		ss << "comment('the machine to approximatedly the correct location')\n";
		ss << "comment('immediately above the hole we are finding the centre of.')\n";
		ss << "comment('This program then jogs down and probes out in two opposite directions')\n";
		ss << "comment('looking for the workpiece')\n";
		theApp.m_program_canvas->m_textCtrl->AppendText(ss.str().c_str());
		ss.str(_(""));

		AppendTextForSingleProbeOperation( pFixture, gp_Pnt(0,0,0), gp_Pnt(0,0,0), m_depth, gp_Pnt(+1.0 * m_distance,0,0), _("1001"), _("1002") );
		AppendTextForSingleProbeOperation( pFixture, gp_Pnt(0,0,0), gp_Pnt(0,0,0), m_depth, gp_Pnt(-1.0 * m_distance,0,0), _("1003"), _("1004") );
	} // End if - else

	// Now move to the centre of these two intersection points.
	ss << "comment('Move back to the intersection points')\n";
	ss << "comment('NOTE: We set the temporary origin because it was in effect when the values in these variables were established')\n";
	ss << "set_temporary_origin( x=0, y=0, z=0 )\n";
	ss << "rapid_to_midpoint( '" << _("#1001") << "','" << _("#1002") << "', '0', '" << _("#1003") << "','" << _("#1004") << "', '0' )\n";
	ss << "remove_temporary_origin()\n";

	theApp.m_program_canvas->m_textCtrl->AppendText(ss.str().c_str());
	ss.str(_(""));

	if (m_number_of_points == 4)
	{
		if (m_direction == eOutside)
		{
			ss << "comment('This program assumes that the machine operator has jogged')\n";
			ss << "comment('the machine to approximatedly the correct location')\n";
			ss << "comment('immediately above the protrusion we are finding the centre of.')\n";
			ss << "comment('This program then jogs out and down in two opposite directions')\n";
			ss << "comment('before probing back towards the centre point looking for the')\n";
			ss << "comment('protrusion.')\n";
			theApp.m_program_canvas->m_textCtrl->AppendText(ss.str().c_str());
			ss.str(_(""));

			AppendTextForSingleProbeOperation( pFixture, gp_Pnt(0,m_distance,0), gp_Pnt(0,m_distance,0), m_depth, gp_Pnt(0,0,0), _("1001"), _("1002") );
			AppendTextForSingleProbeOperation( pFixture, gp_Pnt(0,-1.0 * m_distance,0), gp_Pnt(0,-1.0 * m_distance,0), m_depth, gp_Pnt(0,0,0), _("1003"), _("1004") );
		} // End if - then
		else
		{
			ss << "comment('This program assumes that the machine operator has jogged')\n";
			ss << "comment('the machine to approximatedly the correct location')\n";
			ss << "comment('immediately above the hole we are finding the centre of.')\n";
			ss << "comment('This program then jogs down and probes out in two opposite directions')\n";
			ss << "comment('looking for the workpiece')\n";
			theApp.m_program_canvas->m_textCtrl->AppendText(ss.str().c_str());
			ss.str(_(""));

			AppendTextForSingleProbeOperation( pFixture, gp_Pnt(0,0,0), gp_Pnt(0,0,0), m_depth, gp_Pnt(0,+1.0 * m_distance,0), _("1001"), _("1002") );
			AppendTextForSingleProbeOperation( pFixture, gp_Pnt(0,0,0), gp_Pnt(0,0,0), m_depth, gp_Pnt(0,-1.0 * m_distance,0), _("1003"), _("1004") );
		} // End if - else

		// Now move to the centre of these two intersection points.
		ss << "comment('Move back to the intersection points')\n";
		ss << "comment('NOTE: We set the temporary origin because it was in effect when the values in these variables were established')\n";
		ss << "set_temporary_origin( x=0, y=0, z=0 )\n";
		ss << "rapid_to_midpoint( '" << _("#1001") << "','" << _("#1002") << "', '0', '" << _("#1003") << "','" << _("#1004") << "', '0' )\n";
		ss << "remove_temporary_origin()\n";

		theApp.m_program_canvas->m_textCtrl->AppendText(ss.str().c_str());
		ss.str(_(""));
	} // End if - then
}


/**
	Generate the Python script that will move in the setup direction, retract out away from the workpiece,
	plunge down and probe back towards the workpiece.  Once found, it should store the results in the
	variables specified.
 */
void CProbing::AppendTextForSingleProbeOperation(
	const CFixture *pFixture,
	const CNCPoint setup_point,
	const CNCPoint retract_point,
	const double depth,
	const CNCPoint probe_point,
	const wxString &intersection_variable_x,
	const wxString &intersection_variable_y ) const
{
#ifdef UNICODE
	std::wostringstream ss;
#else
	std::ostringstream ss;
#endif
	ss.imbue(std::locale("C"));
	ss<<std::setprecision(10);

	// Make sure it's negative as we're stepping down.  There is no option
	// to specify a starting height so this can only be a relative distance.
	double relative_depth = (m_depth > 0)?(-1.0 * m_depth):m_depth;

	ss << "comment('Begin probing operation for a single point.')\n";
	ss << "comment('The results will be stored with respect to the current location of the machine')\n";
	ss << "comment('The machine will be returned to this original position following this single probe operation')\n";
	ss << "probe_single_point("
		<< "point_along_edge_x=" << setup_point.X(true) << ", "
			<< "point_along_edge_y=" << setup_point.Y(true) << ", "
			<< "depth=" << relative_depth / theApp.m_program->m_units << ", "
			<< "retracted_point_x=" << retract_point.X(true) << ", "
			<< "retracted_point_y=" << retract_point.Y(true) << ", "
			<< "destination_point_x=" << probe_point.X(true) << ", "
			<< "destination_point_y=" << probe_point.Y(true) << ", "
			<< "intersection_variable_x='" << intersection_variable_x.c_str() << "', "
			<< "intersection_variable_y='" << intersection_variable_y.c_str() << "' )\n";

	theApp.m_program_canvas->m_textCtrl->AppendText(ss.str().c_str());
}

void CProbe_Edge::AppendTextToProgram( const CFixture *pFixture )
{
	CSpeedOp::AppendTextToProgram( pFixture );

#ifdef UNICODE
	std::wostringstream ss;
#else
	std::ostringstream ss;
#endif
	ss.imbue(std::locale("C"));
	ss<<std::setprecision(10);

	// We're going to be working in relative coordinates based on the assumption
	// that the operator has first jogged the machine to the approximate centre point.

	ss << "comment('This program assumes that the machine operator has jogged')\n";
	ss << "comment('the machine to approximatedly the correct location')\n";
	ss << "comment('immediately above the ";

	if (m_number_of_edges == 1)
	{
		ss << eEdges_t(m_edge) << " edge of the workpiece.')\n";
	}
	else
	{
		ss << eCorners_t(m_corner) << " corner of the workpiece.')\n";
	}

	theApp.m_program_canvas->m_textCtrl->AppendText(ss.str().c_str());
	ss.str(_(""));

	gp_Dir z_direction(0,0,1);

	if (m_number_of_edges == 1)
	{
		switch(m_edge)
		{
		case eBottom:
			AppendTextForSingleProbeOperation( pFixture, 	gp_Pnt(m_distance,                0, 0), 
									gp_Pnt(m_distance, -1.0 * m_retract, 0), m_depth, 
									gp_Pnt(m_distance, +1.0 * m_retract, 0), _("1001"), _("1002") );
			AppendTextForSingleProbeOperation( pFixture, 	gp_Pnt(2.0 * m_distance, 0,0), gp_Pnt(2.0 * m_distance,-1.0 * m_retract,0), m_depth, gp_Pnt(2.0 * m_distance, +1.0 * m_retract, 0), _("1003"), _("1004") );
			break;

		case eTop:
			AppendTextForSingleProbeOperation( pFixture, gp_Pnt(m_distance,0,0), gp_Pnt(m_distance, +1.0 * m_retract,0), m_depth, gp_Pnt(m_distance, -1.0 * m_retract, 0), _("1001"), _("1002") );
			AppendTextForSingleProbeOperation( pFixture, gp_Pnt(2.0 * m_distance,0,0), gp_Pnt(2.0 * m_distance, +1.0 * m_retract,0), m_depth, gp_Pnt(2.0 * m_distance, -1.0 * m_retract, 0), _("1003"), _("1004") );
			break;

		case eLeft:
			AppendTextForSingleProbeOperation( pFixture, gp_Pnt(0,m_distance,0), gp_Pnt(-1.0 * m_retract, m_distance, 0), m_depth, gp_Pnt(+1.0 * m_retract, m_distance,0), _("1001"), _("1002") );
			AppendTextForSingleProbeOperation( pFixture, gp_Pnt(0,2.0 * m_distance,0), gp_Pnt(-1.0 * m_retract, 2.0 * m_distance, 0), m_depth, gp_Pnt(+1.0 * m_retract, 2.0 * m_distance,0), _("1003"), _("1004") );
			break;

		case eRight:
			AppendTextForSingleProbeOperation( pFixture, gp_Pnt(0, m_distance, 0), gp_Pnt(m_retract, m_distance, 0), m_depth, gp_Pnt(-1.0 * m_retract, m_distance, 0), _("1001"), _("1002") );
			AppendTextForSingleProbeOperation( pFixture, gp_Pnt(0, 2.0 * m_distance, 0), gp_Pnt(m_retract, 2.0 * m_distance, 0), m_depth, gp_Pnt(-1.0 * m_retract, 2.0 * m_distance, 0), _("1003"), _("1004") );
			break;
		} // End switch

		// Combine the two probed points into an edge and generate an XML document describing the angle they form.
		ss << "report_probe_results( "
				<< "x1='#1001', "
				<< "y1='#1002', "
				<< "x2='#1003', "
				<< "y2='#1004', "
				<< "xml_file_name='" << this->GetOutputFileName( _(".xml"), true ).c_str() << "')\n";
		theApp.m_program_canvas->m_textCtrl->AppendText(ss.str().c_str());
		ss.str(_(""));
	} // End if - then
	else
	{
		// We're looking for the intersection of two edges.

		switch (m_corner)
		{
		case eBottomLeft:
			// Bottom
			AppendTextForSingleProbeOperation( pFixture, 	gp_Pnt(m_distance,0, 0), gp_Pnt(m_distance, -1.0 * m_retract, 0), m_depth, gp_Pnt(m_distance, +1.0 * m_retract, 0), _("1001"), _("1002") );
			AppendTextForSingleProbeOperation( pFixture, 	gp_Pnt(2.0 * m_distance, 0,0), gp_Pnt(2.0 * m_distance,-1.0 * m_retract,0), m_depth, gp_Pnt(2.0 * m_distance, +1.0 * m_retract, 0), _("1003"), _("1004") );

			// Left
			AppendTextForSingleProbeOperation( pFixture, gp_Pnt(0, m_distance,0), gp_Pnt(-1.0 * m_retract, m_distance, 0), m_depth, gp_Pnt( +1.0 * m_retract, m_distance, 0), _("1005"), _("1006") );
			AppendTextForSingleProbeOperation( pFixture, gp_Pnt(0, 2.0 * m_distance,0), gp_Pnt(-1.0 * m_retract,2.0 * m_distance, 0), m_depth, gp_Pnt( +1.0 * m_retract, 2.0 * m_distance, 0), _("1007"), _("1008") );
			break;

		case eBottomRight:
			// Bottom
			AppendTextForSingleProbeOperation( pFixture, 	gp_Pnt(-1.0 * m_distance, 0, 0), 
									gp_Pnt(-1.0 * m_distance, -1.0 * m_retract, 0), m_depth, 
									gp_Pnt(-1.0 * m_distance, +1.0 * m_retract, 0), _("1001"), _("1002") );
			AppendTextForSingleProbeOperation( pFixture, 	gp_Pnt(-2.0 * m_distance, 0,0), gp_Pnt(-2.0 * m_distance,-1.0 * m_retract,0), m_depth, gp_Pnt(-2.0 * m_distance, +1.0 * m_retract, 0), _("1003"), _("1004") );

			// Right
			AppendTextForSingleProbeOperation( pFixture, gp_Pnt(0, m_distance, 0), gp_Pnt(m_retract, m_distance, 0), m_depth, gp_Pnt(-1.0 * m_retract, m_distance, 0), _("1001"), _("1002") );
			AppendTextForSingleProbeOperation( pFixture, gp_Pnt(0, 2.0 * m_distance, 0), gp_Pnt(m_retract, 2.0 * m_distance, 0), m_depth, gp_Pnt(-1.0 * m_retract, 2.0 * m_distance, 0), _("1003"), _("1004") );

			
			break;

		case eTopLeft:
			// Top
			AppendTextForSingleProbeOperation( pFixture, gp_Pnt(m_distance,0,0), gp_Pnt(m_distance, +1.0 * m_retract,0), m_depth, gp_Pnt(m_distance, -1.0 * m_retract, 0), _("1001"), _("1002") );
			AppendTextForSingleProbeOperation( pFixture, gp_Pnt(2.0 * m_distance,0,0), gp_Pnt(2.0 * m_distance, +1.0 * m_retract,0), m_depth, gp_Pnt(2.0 * m_distance, -1.0 * m_retract, 0), _("1003"), _("1004") );

			// Left
			AppendTextForSingleProbeOperation( pFixture, gp_Pnt(0, m_distance,0), gp_Pnt(1.0 * m_retract, m_distance, 0), m_depth, gp_Pnt(-1.0 * m_retract, m_distance, 0), _("1005"), _("1006") );
			AppendTextForSingleProbeOperation( pFixture, gp_Pnt(0, 2.0 * m_distance,0), gp_Pnt(1.0 * m_retract, 2.0 * m_distance, 0), m_depth, gp_Pnt(-1.0 * m_retract, m_distance, 0), _("1007"), _("1008") );
			break;

		case eTopRight:
			// Top
			AppendTextForSingleProbeOperation( pFixture, gp_Pnt(-1.0 * m_distance,0,0), gp_Pnt(-1.0 * m_distance, 1.0 * m_retract, 0), m_depth, gp_Pnt( -1.0 * m_distance, -1.0 * m_retract, 0), _("1001"), _("1002") );
			AppendTextForSingleProbeOperation( pFixture, gp_Pnt(-2.0 * m_distance,0,0), gp_Pnt(-2.0 * m_distance, 1.0 * m_retract, 0), m_depth, gp_Pnt( -2.0 * m_distance, -1.0 * m_retract, 0), _("1003"), _("1004") );

			// Right
			AppendTextForSingleProbeOperation( pFixture, gp_Pnt(0, -1.0 * m_distance, 0), gp_Pnt(m_retract, -1.0 * m_distance, 0), m_depth, gp_Pnt(-1.0 * m_retract, -1.0 * m_distance, 0), _("1005"), _("1006") );
			AppendTextForSingleProbeOperation( pFixture, gp_Pnt(0, -2.0 * m_distance, 0), gp_Pnt(m_retract, -2.0 * m_distance, 0), m_depth, gp_Pnt(-1.0 * m_retract, -2.0 * m_distance, 0), _("1007"), _("1008") );
			break;
		} // End switch

		// Now report the angle of rotation
		ss << "report_probe_results( "
				<< "x1='#1001', "
				<< "y1='#1002', "
				<< "x2='#1003', "
				<< "y2='#1004', "
				<< "x3='#1005', "
				<< "y3='#1006', "
				<< "x4='#1007', "
				<< "y4='#1008', "
				<< "xml_file_name='" << this->GetOutputFileName( _(".xml"), true ).c_str() << "')\n";

		// And position the cutting tool at the intersection of the two lines.

	} // End if - else

	theApp.m_program_canvas->m_textCtrl->AppendText(ss.str().c_str());
}


void CProbing::GetProperties(std::list<Property *> *list)
{
	list->push_back(new PropertyLength(_("Depth"), m_depth, this, on_set_depth));
	list->push_back(new PropertyLength(_("Distance"), m_distance, this, on_set_distance));
	CSpeedOp::GetProperties(list);
}

static void on_set_number_of_points(int zero_based_offset, HeeksObj* object)
{
	if (zero_based_offset == 0) ((CProbe_Centre *)object)->m_number_of_points = 2;
	if (zero_based_offset == 1) ((CProbe_Centre *)object)->m_number_of_points = 4;
	heeksCAD->Changed();
}

static void on_set_direction(int zero_based_choice, HeeksObj* object)
{
	((CProbe_Centre*)object)->m_direction = CProbing::eProbeDirection_t(zero_based_choice);
}

void CProbe_Centre::GetProperties(std::list<Property *> *list)
{
	{ // Begin choice scope
		int choice = 0;
		std::list< wxString > choices;

		choices.push_back(_("Two"));
		choices.push_back(_("Four"));

		if (m_number_of_points == 2) choice = 0;
		if (m_number_of_points == 4) choice = 1;

		list->push_back(new PropertyChoice(_("Number of points"), choices, choice, this, on_set_number_of_points));
	} // End choice scope

	{ // Begin choice scope
		std::list< wxString > choices;
		int choice = 0;

		// We're probing a single edge.
		for (eProbeDirection_t direction = eInside; direction <= eOutside; direction = eProbeDirection_t(int(direction) + 1))
		{
			if (direction == m_direction) choice = choices.size();

			wxString option;
			option << direction;
			choices.push_back( option );
		} // End for

		list->push_back(new PropertyChoice(_("Direction"), choices, choice, this, on_set_direction));
	} // End choice scope

	CProbing::GetProperties(list);
}

static void on_set_number_of_edges(int zero_based_offset, HeeksObj* object)
{
	((CProbe_Edge*)object)->m_number_of_edges = zero_based_offset + 1;
	heeksCAD->Changed();
}

static void on_set_retract(double value, HeeksObj* object)
{
	((CProbe_Edge*)object)->m_retract = value;
}

static void on_set_edge(int zero_based_choice, HeeksObj* object)
{
	((CProbe_Edge*)object)->m_edge = CProbing::eEdges_t(zero_based_choice);
}


static void on_set_corner(int zero_based_choice, HeeksObj* object)
{
	((CProbe_Edge*)object)->m_corner = CProbing::eCorners_t(zero_based_choice);
}


void CProbe_Edge::GetProperties(std::list<Property *> *list)
{
	list->push_back(new PropertyLength(_("Retract"), m_retract, this, on_set_retract));

	{ // Begin choice scope
		std::list< wxString > choices;

		choices.push_back(_("One"));
		choices.push_back(_("Two"));

		int choice = int(m_number_of_edges - 1);
		list->push_back(new PropertyChoice(_("Number of edges"), choices, choice, this, on_set_number_of_edges));
	} // End choice scope

	if (m_number_of_edges == 1)
	{
		std::list< wxString > choices;
		int choice = 0;

		// We're probing a single edge.
		for (eEdges_t edge = eBottom; edge <= eRight; edge = eEdges_t(int(edge) + 1))
		{
			if (edge == m_edge) choice = choices.size();

			wxString option;
			option << edge;
			choices.push_back( option );
		} // End for

		list->push_back(new PropertyChoice(_("Edge"), choices, choice, this, on_set_edge));
	} // End if - then
	else
	{
		// We're probing two edges.

		std::list< wxString > choices;
		int choice = 0;

		// We're probing a single edge.
		for (eCorners_t corner = eBottomLeft; corner <= eTopRight; corner = eCorners_t(int(corner) + 1))
		{
			if (corner == m_corner) choice = choices.size();

			wxString option;
			option << corner;
			choices.push_back( option );
		} // End for

		list->push_back(new PropertyChoice(_("Edge"), choices, choice, this, on_set_corner));
	} // End if - else

	CProbing::GetProperties(list);
}

HeeksObj *CProbe_Edge::MakeACopy(void)const
{
	return new CProbe_Edge(*this);
}

void CProbe_Edge::CopyFrom(const HeeksObj* object)
{
	operator=(*((CProbe_Edge*)object));
}

HeeksObj *CProbe_Centre::MakeACopy(void)const
{
	return new CProbe_Centre(*this);
}

void CProbe_Centre::CopyFrom(const HeeksObj* object)
{
	operator=(*((CProbe_Centre *)object));
}

bool CProbing::CanAddTo(HeeksObj* owner)
{
	return owner->GetType() == OperationsType;
}


void CProbe_Edge::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element = new TiXmlElement( Ttc(GetTypeString()) );
	root->LinkEndChild( element );  

	element->SetDoubleAttribute("retract", m_retract);
	element->SetAttribute("number_of_edges", m_number_of_edges);

	WriteBaseXML(element);
}

// static member function
HeeksObj* CProbe_Edge::ReadFromXMLElement(TiXmlElement* element)
{
	CProbe_Edge* new_object = new CProbe_Edge();

	if (element->Attribute("retract")) new_object->m_retract = atof(element->Attribute("retract"));
	if (element->Attribute("number_of_edges")) new_object->m_number_of_edges = atoi(element->Attribute("number_of_edges"));

	new_object->ReadBaseXML(element);

	return new_object;
}


void CProbe_Centre::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element = new TiXmlElement( Ttc(GetTypeString()) );
	root->LinkEndChild( element );  

	element->SetAttribute("direction", m_direction);
	element->SetAttribute("number_of_points", m_number_of_points);

	WriteBaseXML(element);
}

// static member function
HeeksObj* CProbe_Centre::ReadFromXMLElement(TiXmlElement* element)
{
	CProbe_Centre* new_object = new CProbe_Centre();

	if (element->Attribute("direction")) new_object->m_direction = atoi(element->Attribute("direction"));
	if (element->Attribute("number_of_points")) new_object->m_number_of_points = atoi(element->Attribute("number_of_points"));
	new_object->ReadBaseXML(element);

	return new_object;
}

void CProbing::WriteBaseXML(TiXmlElement *element)
{
	element->SetDoubleAttribute("depth", m_depth);
	element->SetDoubleAttribute("distance", m_distance);

	CSpeedOp::WriteBaseXML(element);
}

void CProbing::ReadBaseXML(TiXmlElement* element)
{
	if (element->Attribute("depth")) m_depth = atof(element->Attribute("depth"));
	if (element->Attribute("distance")) m_distance = atof(element->Attribute("distance"));

	CSpeedOp::ReadBaseXML(element);
}


void CProbing::glCommands(bool select, bool marked, bool no_color)
{
}


wxString CProbing::GetOutputFileName(const wxString extension, const bool filename_only)
{
	wxString file_name = theApp.m_program->GetOutputFileName();

	// Remove the extension.
	int offset = 0;
	if ((offset = file_name.Find(_("."))) != -1)
	{
		file_name.Remove(offset);
	}

	file_name << _("_");
	file_name << m_title;
	file_name << _("_id_");
	file_name << m_id;
	file_name << extension;

	if (filename_only)
	{
		std::vector<wxString> tokens = Tokens( file_name, wxString(_("/\\")) );
		if (tokens.size() > 0)
		{
			file_name = tokens[tokens.size()-1];
		}
	}

	return(file_name);
}


void CProbing::GeneratePythonPreamble()
{
	// We must setup the theApp.m_program_canvas->m_textCtrl variable before
		// calling the HeeksPyPostProcess() routine.  That's the python script
		// that will be executed.

		theApp.m_program_canvas->m_textCtrl->Clear();

		// add standard stuff at the top
		//hackhack, make it work on unix with FHS
#ifndef WIN32
		theApp.m_program_canvas->AppendText(_T("import sys\n"));
		theApp.m_program_canvas->AppendText(_T("sys.path.insert(0,'/usr/local/lib/heekscnc/')\n"));
#endif

		// machine general stuff
		theApp.m_program_canvas->AppendText(_T("from nc.nc import *\n"));

		// specific machine
		if (theApp.m_program->m_machine.file_name == _T("not found"))
		{
			wxMessageBox(_T("Machine name (defined in Program Properties) not found"));
		} // End if - then
		else
		{
			theApp.m_program_canvas->AppendText(_T("import nc.") + theApp.m_program->m_machine.file_name + _T("\n"));
			theApp.m_program_canvas->AppendText(_T("\n"));
		} // End if - else

		// output file
		theApp.m_program_canvas->AppendText(_T("output('") + this->GetOutputFileName(_(".tap"), false) + _T("')\n"));

		// begin program
		theApp.m_program_canvas->AppendText(_T("program_begin(123, 'Test program')\n"));
		theApp.m_program_canvas->AppendText(_T("absolute()\n"));

		if(theApp.m_program->m_units > 25.0)
		{
			theApp.m_program_canvas->AppendText(_T("imperial()\n"));
		}
		else
		{
			theApp.m_program_canvas->AppendText(_T("metric()\n"));
		}
		theApp.m_program_canvas->AppendText(_T("set_plane(0)\n"));
		theApp.m_program_canvas->AppendText(_T("\n"));
}

class Probe_Centre_GenerateGCode: public Tool 
{

CProbe_Centre *m_pThis;

public:
	Probe_Centre_GenerateGCode() { m_pThis = NULL; }

	// Tool's virtual functions
	const wxChar* GetTitle(){return _("Generate GCode");}

	void Run()
	{
		// We must setup the theApp.m_program_canvas->m_textCtrl variable before
		// calling the HeeksPyPostProcess() routine.  That's the python script
		// that will be executed.

		theApp.m_program_canvas->m_textCtrl->Clear();

		m_pThis->GeneratePythonPreamble();

		CFixture default_fixture(NULL, CFixture::G54 );
		m_pThis->AppendTextToProgram( &default_fixture );

		theApp.m_program_canvas->AppendText(_T("program_end()\n"));

		{
			// clear the output file
			wxFile f(m_pThis->GetOutputFileName(_(".tap"), false).c_str(), wxFile::write);
			if(f.IsOpened())f.Write(_T("\n"));
		}

		HeeksPyPostProcess(theApp.m_program, m_pThis->GetOutputFileName(_(".tap"), false), false );
	}
	wxString BitmapPath(){ return _T("export");}
	wxString previous_path;
	void Set( CProbe_Centre *pThis ) { m_pThis = pThis; }
};

static Probe_Centre_GenerateGCode generate_gcode;

void CProbe_Centre::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{

	generate_gcode.Set( this );

	t_list->push_back( &generate_gcode );
}




class Probe_Edge_GenerateGCode: public Tool 
{

CProbe_Edge *m_pThis;

public:
	Probe_Edge_GenerateGCode() { m_pThis = NULL; }

	// Tool's virtual functions
	const wxChar* GetTitle(){return _("Generate GCode");}

	void Run()
	{
		// We must setup the theApp.m_program_canvas->m_textCtrl variable before
		// calling the HeeksPyPostProcess() routine.  That's the python script
		// that will be executed.

		theApp.m_program_canvas->m_textCtrl->Clear();

		m_pThis->GeneratePythonPreamble();

		CFixture default_fixture(NULL, CFixture::G54 );
		m_pThis->AppendTextToProgram( &default_fixture );

		theApp.m_program_canvas->AppendText(_T("program_end()\n"));

		{
			// clear the output file
			wxFile f(m_pThis->GetOutputFileName(_(".tap"), false).c_str(), wxFile::write);
			if(f.IsOpened())f.Write(_T("\n"));
		}

		HeeksPyPostProcess(theApp.m_program, m_pThis->GetOutputFileName(_(".tap"), false), false );
	}
	wxString BitmapPath(){ return _T("export");}
	wxString previous_path;
	void Set( CProbe_Edge *pThis ) { m_pThis = pThis; }
};

static Probe_Edge_GenerateGCode generate_edge_gcode;

void CProbe_Edge::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{

	generate_edge_gcode.Set( this );

	t_list->push_back( &generate_edge_gcode );
}
