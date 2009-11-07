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
#include "CuttingTool.h"
#include "interface/HeeksColor.h"
#include "NCCode.h"

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
	heeksCAD->Changed();	// Force a re-draw from glCommands()
}

static void on_set_depth(double value, HeeksObj* object)
{
	((CProbing*)object)->m_depth = value;
	heeksCAD->Changed();	// Force a re-draw from glCommands()
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

	double probe_radius = 0.0;
	if (m_cutting_tool_number > 0)
	{
		CCuttingTool *pCuttingTool = CCuttingTool::Find( m_cutting_tool_number);
		if (pCuttingTool != NULL)
		{
			probe_radius = pCuttingTool->CuttingRadius(true);
		} // End if - then
	} // End if - then

	// We're going to be working in relative coordinates based on the assumption
	// that the operator has first jogged the machine to the approximate centre point.

	CProbing::PointsList_t points = GetPoints();

	if (m_direction == eOutside)
	{
		ss << "comment('This program assumes that the machine operator has jogged')\n";
		ss << "comment('the machine to approximatedly the correct location')\n";
		ss << "comment('immediately above the protrusion we are finding the centre of.')\n";
		ss << "comment('This program then jogs out and down in two opposite directions')\n";
		ss << "comment('before probing back towards the centre point looking for the')\n";
		ss << "comment('protrusion.')\n";
		theApp.m_program_canvas->m_textCtrl->AppendText(ss.str().c_str());
		ss.str(_T(""));

		if ((m_alignment == eXAxis) || (m_number_of_points == 4))
		{
			AppendTextForSingleProbeOperation( pFixture, points[0].second, points[1].second, points[2].second.Z(false), points[3].second,
								       	_T("1001"), _T("1002"), -1.0 * probe_radius, 0 );
			AppendTextForSingleProbeOperation( pFixture, points[5].second, points[6].second, points[7].second.Z(false), points[8].second, 
										_T("1003"), _T("1004"), +1.0 * probe_radius, 0 );
		} // End if - then
		else
		{
			AppendTextForSingleProbeOperation( pFixture, points[0].second, points[1].second, points[2].second.Z(false), points[3].second,
									_T("1001"), _T("1002"), 0, -1.0 * probe_radius );
			AppendTextForSingleProbeOperation( pFixture, points[5].second, points[6].second, points[7].second.Z(false), points[8].second, 
									_T("1003"), _T("1004"), 0, +1.0 * probe_radius );
		} // End if - else
	} // End if - then
	else
	{
		ss << "comment('This program assumes that the machine operator has jogged')\n";
		ss << "comment('the machine to approximatedly the correct location')\n";
		ss << "comment('immediately above the hole we are finding the centre of.')\n";
		ss << "comment('This program then jogs down and probes out in two opposite directions')\n";
		ss << "comment('looking for the workpiece')\n";
		theApp.m_program_canvas->m_textCtrl->AppendText(ss.str().c_str());
		ss.str(_T(""));

		if ((m_alignment == eXAxis) || (m_number_of_points == 4))
		{
			AppendTextForSingleProbeOperation( pFixture, points[0].second, points[1].second, points[2].second.Z(false), points[3].second,
									_T("1001"), _T("1002"), +1.0 * probe_radius, 0 );
			AppendTextForSingleProbeOperation( pFixture, points[5].second, points[6].second, points[7].second.Z(false), points[8].second, 
									_T("1003"), _T("1004"), -1.0 * probe_radius, 0 );
		} // End if - then
		else
		{
			// eYAxis:
			AppendTextForSingleProbeOperation( pFixture, points[0].second, points[1].second, points[2].second.Z(false), points[3].second,
									_T("1001"), _T("1002"), 0, +1.0 * probe_radius );
			AppendTextForSingleProbeOperation( pFixture, points[5].second, points[6].second, points[7].second.Z(false), points[8].second, 
									_T("1003"), _T("1004"), 0, -1.0 * probe_radius  );
		} // End if - else
	} // End if - else

	// Now move to the centre of these two intersection points.
	ss << "comment('Move back to the intersection points')\n";
	ss << "comment('NOTE: We set the temporary origin because it was in effect when the values in these variables were established')\n";
	ss << "set_temporary_origin( x=0, y=0, z=0 )\n";
	ss << "rapid_to_midpoint( '" << _T("#1001") << "','" << _T("#1002") << "', '0', '" << _T("#1003") << "','" << _T("#1004") << "', '0' )\n";
	ss << "remove_temporary_origin()\n";

	theApp.m_program_canvas->m_textCtrl->AppendText(ss.str().c_str());
	ss.str(_T(""));

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
			ss.str(_T(""));

			AppendTextForSingleProbeOperation( pFixture, points[10].second, points[11].second, points[12].second.Z(false), points[13].second,
									_T("1001"), _T("1002"), 0, -1.0 * probe_radius );
			AppendTextForSingleProbeOperation( pFixture, points[15].second, points[16].second, points[17].second.Z(false), points[18].second, 
									_T("1003"), _T("1004"), 0, +1.0 * probe_radius );
		} // End if - then
		else
		{
			ss << "comment('This program assumes that the machine operator has jogged')\n";
			ss << "comment('the machine to approximatedly the correct location')\n";
			ss << "comment('immediately above the hole we are finding the centre of.')\n";
			ss << "comment('This program then jogs down and probes out in two opposite directions')\n";
			ss << "comment('looking for the workpiece')\n";
			theApp.m_program_canvas->m_textCtrl->AppendText(ss.str().c_str());
			ss.str(_T(""));

			AppendTextForSingleProbeOperation( pFixture, points[10].second, points[11].second, points[12].second.Z(false), points[13].second,
									_T("1001"), _T("1002"), 0, +1.0 * probe_radius );
			AppendTextForSingleProbeOperation( pFixture, points[15].second, points[16].second, points[17].second.Z(false), points[18].second, 
									_T("1003"), _T("1004"), 0, -1.0 * probe_radius );
		} // End if - else

		// Now move to the centre of these two intersection points.
		ss << "comment('Move back to the intersection points')\n";
		ss << "comment('NOTE: We set the temporary origin because it was in effect when the values in these variables were established')\n";
		ss << "set_temporary_origin( x=0, y=0, z=0 )\n";
		ss << "rapid_to_midpoint( '" << _T("#1001") << "','" << _T("#1002") << "', '0', '" << _T("#1003") << "','" << _T("#1004") << "', '0' )\n";
		ss << "remove_temporary_origin()\n";

		theApp.m_program_canvas->m_textCtrl->AppendText(ss.str().c_str());
		ss.str(_T(""));
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
	const wxString &intersection_variable_y,
        const double probe_radius_x_component,
	const double probe_radius_y_component	) const
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
	double relative_depth = (depth > 0)?(-1.0 * depth):depth;

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
			<< "intersection_variable_y='" << intersection_variable_y.c_str() << "', "
		        << "probe_radius_x_component='" << probe_radius_x_component << "', "
			<< "probe_radius_y_component='" << probe_radius_y_component << "' )\n";

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

	double probe_radius = 0.0;
	if (m_cutting_tool_number > 0)
	{
		CCuttingTool *pCuttingTool = CCuttingTool::Find( m_cutting_tool_number);
		if (pCuttingTool != NULL)
		{
			probe_radius = pCuttingTool->CuttingRadius(true);
		} // End if - then
	} // End if - then

	theApp.m_program_canvas->m_textCtrl->AppendText(ss.str().c_str());
	ss.str(_T(""));

	gp_Dir z_direction(0,0,1);

	CProbing::PointsList_t points = GetPoints();

	if (m_number_of_edges == 1)
	{
		switch(m_edge)
		{
		case eBottom:
			AppendTextForSingleProbeOperation( pFixture, points[0].second, points[1].second, points[2].second.Z(false), points[3].second,
								       	_T("1001"), _T("1002"), 0, +1.0 * probe_radius );
			AppendTextForSingleProbeOperation( pFixture, points[5].second, points[6].second, points[7].second.Z(false), points[8].second, 
										_T("1003"), _T("1004"), 0, +1.0 * probe_radius );
			break;

		case eTop:
			AppendTextForSingleProbeOperation( pFixture, points[0].second, points[1].second, points[2].second.Z(false), points[3].second,
										_T("1001"), _T("1002"), 0, -1.0 * probe_radius );
			AppendTextForSingleProbeOperation( pFixture, points[5].second, points[6].second, points[7].second.Z(false), points[8].second, 
										_T("1003"), _T("1004"), 0, -1.0 * probe_radius );
			break;

		case eLeft:
			AppendTextForSingleProbeOperation( pFixture, points[0].second, points[1].second, points[2].second.Z(false), points[3].second,
									_T("1001"), _T("1002"), +1.0 * probe_radius, 0 );
			AppendTextForSingleProbeOperation( pFixture, points[5].second, points[6].second, points[7].second.Z(false), points[8].second, 
									_T("1003"), _T("1004"), +1.0 * probe_radius,0 );
			break;

		case eRight:
			AppendTextForSingleProbeOperation( pFixture, points[0].second, points[1].second, points[2].second.Z(false), points[3].second,
									_T("1001"), _T("1002"), -1.0 * probe_radius, 0 );
			AppendTextForSingleProbeOperation( pFixture, points[5].second, points[6].second, points[7].second.Z(false), points[8].second, 
									_T("1003"), _T("1004"), -1.0 * probe_radius, 0 );
			break;
		} // End switch

		// Combine the two probed points into an edge and generate an XML document describing the angle they form.
		ss << "report_probe_results( "
				<< "x1='#1001', "
				<< "y1='#1002', "
				<< "x2='#1003', "
				<< "y2='#1004', "
				<< "xml_file_name='" << this->GetOutputFileName( _T(".xml"), true ).c_str() << "')\n";
		theApp.m_program_canvas->m_textCtrl->AppendText(ss.str().c_str());
		ss.str(_T(""));
	} // End if - then
	else
	{
		// We're looking for the intersection of two edges.

		switch (m_corner)
		{
		case eBottomLeft:
			// Bottom
			AppendTextForSingleProbeOperation( pFixture, points[0].second, points[1].second, points[2].second.Z(false), points[3].second,
									_T("1001"), _T("1002"), 0, +1.0 * probe_radius );
			AppendTextForSingleProbeOperation( pFixture, points[5].second, points[6].second, points[7].second.Z(false), points[8].second, 
									_T("1003"), _T("1004"), 0, +1.0 * probe_radius );

			// Left
			AppendTextForSingleProbeOperation( pFixture, points[10].second, points[11].second, points[12].second.Z(false), points[13].second,
									_T("1005"), _T("1006"), +1.0 * probe_radius, 0 );
			AppendTextForSingleProbeOperation( pFixture, points[15].second, points[16].second, points[17].second.Z(false), points[18].second, 
									_T("1007"), _T("1008"), +1.0 * probe_radius, 0 );
			break;

		case eBottomRight:
			// Bottom
			AppendTextForSingleProbeOperation( pFixture, points[0].second, points[1].second, points[2].second.Z(false), points[3].second,
									_T("1001"), _T("1002"), 0, +1.0 * probe_radius );
			AppendTextForSingleProbeOperation( pFixture, points[5].second, points[6].second, points[7].second.Z(false), points[8].second, 
									_T("1003"), _T("1004"), 0, +1.0 * probe_radius );

			// Right
			AppendTextForSingleProbeOperation( pFixture, points[10].second, points[11].second, points[12].second.Z(false), points[13].second,
									_T("1005"), _T("1006"), -1.0 * probe_radius, 0 );
			AppendTextForSingleProbeOperation( pFixture, points[15].second, points[16].second, points[17].second.Z(false), points[18].second, 
									_T("1007"), _T("1008"), -1.0 * probe_radius, 0 );

			
			break;

		case eTopLeft:
			// Top
			AppendTextForSingleProbeOperation( pFixture, points[0].second, points[1].second, points[2].second.Z(false), points[3].second,
									_T("1001"), _T("1002"), 0, -1.0 * probe_radius );
			AppendTextForSingleProbeOperation( pFixture, points[5].second, points[6].second, points[7].second.Z(false), points[8].second, 
									_T("1003"), _T("1004"), 0, -1.0 * probe_radius );

			// Left
			AppendTextForSingleProbeOperation( pFixture, points[10].second, points[11].second, points[12].second.Z(false), points[13].second,
									_T("1005"), _T("1006"), -1.0 * probe_radius, 0 );
			AppendTextForSingleProbeOperation( pFixture, points[15].second, points[16].second, points[17].second.Z(false), points[18].second, 
									_T("1007"), _T("1008"), -1.0 * probe_radius, 0 );
			break;

		case eTopRight:
			// Top
			AppendTextForSingleProbeOperation( pFixture, points[0].second, points[1].second, points[2].second.Z(false), points[3].second,
									_T("1001"), _T("1002"), 0, -1.0 * probe_radius );
			AppendTextForSingleProbeOperation( pFixture, points[5].second, points[6].second, points[7].second.Z(false), points[8].second, 
									_T("1003"), _T("1004"), 0, -1.0 * probe_radius );

			// Right
			AppendTextForSingleProbeOperation( pFixture, points[10].second, points[11].second, points[12].second.Z(false), points[13].second,
									_T("1005"), _T("1006"), -1.0 * probe_radius, 0 );
			AppendTextForSingleProbeOperation( pFixture, points[15].second, points[16].second, points[17].second.Z(false), points[18].second, 
									_T("1007"), _T("1008"), -1.0 * probe_radius, 0 );
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
				<< "xml_file_name='" << this->GetOutputFileName( _T(".xml"), true ).c_str() << "')\n";

		// And position the cutting tool at the intersection of the two lines.
		// This should be safe as the 'probe_single_point() call made in the AppendTextForSingleOperation() routine returns
		// the machine's position to the originally jogged position.  This is expected to be above the workpiece
		// at a same movement height.
		ss << "set_temporary_origin( x=0, y=0, z=0 )\n";
		ss << "rapid_to_intersection( "
				<< "x1='#1001', "
				<< "y1='#1002', "
				<< "x2='#1003', "
				<< "y2='#1004', "
				<< "x3='#1005', "
				<< "y3='#1006', "
				<< "x4='#1007', "
				<< "y4='#1008', "
				<< "intersection_x='#1009', "
				<< "intersection_y='#1010', "
				<< "ua_numerator='#1011', "
				<< "ua_denominator='#1012', "
				<< "ub_numerator='#1013', "
				<< "ua='#1014', "
				<< "ub='#1015' )\n";
		ss << "remove_temporary_origin()\n";
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
	((CProbe_Centre*)object)->GenerateMeaningfullName();
	heeksCAD->Changed();	// Force a re-draw from glCommands()
}

static void on_set_direction(int zero_based_choice, HeeksObj* object)
{
	((CProbe_Centre*)object)->m_direction = CProbing::eProbeDirection_t(zero_based_choice);
	((CProbe_Centre*)object)->GenerateMeaningfullName();
	heeksCAD->Changed();	// Force a re-draw from glCommands()
}

static void on_set_alignment(int zero_based_choice, HeeksObj* object)
{
	((CProbe_Centre*)object)->m_alignment = CProbing::eAlignment_t(zero_based_choice);
	((CProbe_Centre*)object)->GenerateMeaningfullName();
	heeksCAD->Changed();	// Force a re-draw from glCommands()
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

	if (m_number_of_points == 2)
	{ // Begin choice scope
		std::list< wxString > choices;
		int choice = 0;

		// We're probing a single edge.
		for (eAlignment_t alignment = eXAxis; alignment <= eYAxis; alignment = eAlignment_t(int(alignment) + 1))
		{
			if (alignment == m_alignment) choice = choices.size();

			wxString option;
			option << alignment;
			choices.push_back( option );
		} // End for

		list->push_back(new PropertyChoice(_("Alignment"), choices, choice, this, on_set_alignment));
	} // End choice scope

	CProbing::GetProperties(list);
}

static void on_set_number_of_edges(int zero_based_offset, HeeksObj* object)
{
	((CProbe_Edge*)object)->m_number_of_edges = zero_based_offset + 1;
	((CProbe_Edge*)object)->GenerateMeaningfullName();
	heeksCAD->Changed();
}

static void on_set_retract(double value, HeeksObj* object)
{
	((CProbe_Edge*)object)->m_retract = value;
	((CProbe_Edge*)object)->GenerateMeaningfullName();
	heeksCAD->Changed();	// Force a re-draw from glCommands()
}

static void on_set_edge(int zero_based_choice, HeeksObj* object)
{
	((CProbe_Edge*)object)->m_edge = CProbing::eEdges_t(zero_based_choice);
	((CProbe_Edge*)object)->GenerateMeaningfullName();
	heeksCAD->Changed();	// Force a re-draw from glCommands()
}


static void on_set_corner(int zero_based_choice, HeeksObj* object)
{
	((CProbe_Edge*)object)->m_corner = CProbing::eCorners_t(zero_based_choice);
	((CProbe_Edge*)object)->GenerateMeaningfullName();
	heeksCAD->Changed();	// Force a re-draw from glCommands()
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
	element->SetAttribute("alignment", m_alignment);

	WriteBaseXML(element);
}

// static member function
HeeksObj* CProbe_Centre::ReadFromXMLElement(TiXmlElement* element)
{
	CProbe_Centre* new_object = new CProbe_Centre();

	if (element->Attribute("direction")) new_object->m_direction = atoi(element->Attribute("direction"));
	if (element->Attribute("number_of_points")) new_object->m_number_of_points = atoi(element->Attribute("number_of_points"));
	if (element->Attribute("alignment")) new_object->m_alignment = atoi(element->Attribute("alignment"));

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

void CProbe_Edge::glCommands(bool select, bool marked, bool no_color)
{
	CNCPoint last_point(0,0,0);

	CProbing::PointsList_t points = GetPoints();
	for (CProbing::PointsList_t::const_iterator l_itPoint = points.begin(); l_itPoint != points.end(); l_itPoint++)
	{
		switch (l_itPoint->first)
		{
		case eRapid:
			CNCCode::Color(ColorRapidType).glColor();
			glBegin(GL_LINE_STRIP);
			glVertex3d( last_point.X(), last_point.Y(), last_point.Z() );
			glVertex3d( l_itPoint->second.X(), l_itPoint->second.Y(), l_itPoint->second.Z() );
			glEnd();
			break;

		case eProbe:
			CNCCode::Color(ColorFeedType).glColor();
			glBegin(GL_LINE_STRIP);
			glVertex3d( last_point.X(), last_point.Y(), last_point.Z() );
			glVertex3d( l_itPoint->second.X(), l_itPoint->second.Y(), l_itPoint->second.Z() );
			glEnd();
			break;

		case eEndOfData:
			break;
		} // End switch

		last_point = l_itPoint->second;
	} // End for
	
	CProbing::glCommands(select,marked,no_color);
}

void CProbe_Centre::glCommands(bool select, bool marked, bool no_color)
{
	CNCPoint last_point(0,0,0);

	CProbing::PointsList_t points = GetPoints();
	for (CProbing::PointsList_t::const_iterator l_itPoint = points.begin(); l_itPoint != points.end(); l_itPoint++)
	{
		switch (l_itPoint->first)
		{
		case eRapid:
			CNCCode::Color(ColorRapidType).glColor();
			glBegin(GL_LINE_STRIP);
			glVertex3d( last_point.X(), last_point.Y(), last_point.Z() );
			glVertex3d( l_itPoint->second.X(), l_itPoint->second.Y(), l_itPoint->second.Z() );
			glEnd();
			break;

		case eProbe:
			CNCCode::Color(ColorFeedType).glColor();
			glBegin(GL_LINE_STRIP);
			glVertex3d( last_point.X(), last_point.Y(), last_point.Z() );
			glVertex3d( l_itPoint->second.X(), l_itPoint->second.Y(), l_itPoint->second.Z() );
			glEnd();
			break;

		case eEndOfData:
			break;
		} // End switch

		last_point = l_itPoint->second;
	} // End for
	
	CProbing::glCommands(select,marked,no_color);
}


wxString CProbing::GetOutputFileName(const wxString extension, const bool filename_only)
{
	wxString file_name = theApp.m_program->GetOutputFileName();

	// Remove the extension.
	int offset = 0;
	if ((offset = file_name.Find(_T("."))) != -1)
	{
		file_name.Remove(offset);
	}

	file_name << _T("_");
	file_name << m_title;
	file_name << _T("_id_");
	file_name << m_id;
	file_name << extension;

	if (filename_only)
	{
		std::vector<wxString> tokens = Tokens( file_name, wxString(_T("/\\")) );
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
		theApp.m_program_canvas->AppendText(_T("output('") + this->GetOutputFileName(_T(".tap"), false) + _T("')\n"));

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
			wxFile f(m_pThis->GetOutputFileName(_T(".tap"), false).c_str(), wxFile::write);
			if(f.IsOpened())f.Write(_T("\n"));
		}

		HeeksPyPostProcess(theApp.m_program, m_pThis->GetOutputFileName(_T(".tap"), false), false );
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
			wxFile f(m_pThis->GetOutputFileName(_T(".tap"), false).c_str(), wxFile::write);
			if(f.IsOpened())f.Write(_T("\n"));
		}

		HeeksPyPostProcess(theApp.m_program, m_pThis->GetOutputFileName(_T(".tap"), false), false );
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

/**
	Based on this object's parameters, determine a set of coordinates (relative to (0,0,0)) that
	the machine will move through when probing.
 */
CProbing::PointsList_t CProbe_Edge::GetPoints() const
{
	CProbing::PointsList_t points;

	if (m_number_of_edges == 1)
	{
		switch (m_edge)
		{
		case eBottom:
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(m_distance, 0, 0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(m_distance, -1.0 * m_retract, 0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(m_distance, -1.0 * m_retract, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eProbe, CNCPoint(m_distance, +1.0 * m_retract, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eEndOfData, CNCPoint(0,0,0) ) );

			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(2.0 * m_distance, 0,0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(2.0 * m_distance,-1.0 * m_retract,0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(2.0 * m_distance,-1.0 * m_retract, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eProbe, CNCPoint(2.0 * m_distance, +1.0 * m_retract, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eEndOfData, CNCPoint(0,0,0) ) );
			break;

		case eTop:
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(m_distance, 0, 0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(m_distance, +1.0 * m_retract,0 ) ));
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(m_distance, +1.0 * m_retract, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eProbe, CNCPoint(m_distance, -1.0 * m_retract, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eEndOfData, CNCPoint(0,0,0) ) );

			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(2.0 * m_distance, 0, 0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(2.0 * m_distance, +1.0 * m_retract,0 )) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(2.0 * m_distance, +1.0 * m_retract, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eProbe, CNCPoint(2.0 * m_distance, -1.0 * m_retract, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eEndOfData, CNCPoint(0,0,0) ) );
			break;

		case eLeft:
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(0,m_distance,0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(-1.0 * m_retract, m_distance, 0 )) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(-1.0 * m_retract, m_distance, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eProbe, CNCPoint(+1.0 * m_retract, m_distance, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eEndOfData, CNCPoint(0,0,0) ) );

			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(0,2.0 * m_distance,0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(-1.0 * m_retract, 2.0 * m_distance, 0 )) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(-1.0 * m_retract, 2.0 * m_distance, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eProbe, CNCPoint(+1.0 * m_retract, 2.0 * m_distance, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eEndOfData, CNCPoint(0,0,0) ) );
			break;

		case eRight:
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(0,m_distance,0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(+1.0 * m_retract, m_distance, 0 )) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(+1.0 * m_retract, m_distance, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eProbe, CNCPoint(-1.0 * m_retract, m_distance, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eEndOfData, CNCPoint(0,0,0) ) );

			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(0,2.0 * m_distance,0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(+1.0 * m_retract, 2.0 * m_distance, 0 )) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(+1.0 * m_retract, 2.0 * m_distance, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eProbe, CNCPoint(-1.0 * m_retract, 2.0 * m_distance, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eEndOfData, CNCPoint(0,0,0) ) );
			break;
		} // End switch
	} // End if - then
	else
	{
		// We're using corners (number_of_edges = 2)
		switch (m_corner)
		{
		case eBottomLeft:
			// Bottom
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(m_distance, 0, 0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(m_distance, -1.0 * m_retract, 0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(m_distance, -1.0 * m_retract, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eProbe, CNCPoint(m_distance, +1.0 * m_retract, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eEndOfData, CNCPoint(0,0,0) ) );

			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(2.0 * m_distance, 0, 0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(2.0 * m_distance, -1.0 * m_retract, 0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(2.0 * m_distance, -1.0 * m_retract, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eProbe, CNCPoint(2.0 * m_distance, +1.0 * m_retract, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eEndOfData, CNCPoint(0,0,0) ) );

			// Left
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(0,m_distance,0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(-1.0 * m_retract, m_distance, 0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(-1.0 * m_retract, m_distance, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eProbe, CNCPoint(+1.0 * m_retract, m_distance, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eEndOfData, CNCPoint(0,0,0) ) );

			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(0,2.0 * m_distance,0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(-1.0 * m_retract, 2.0 * m_distance, 0 )) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(-1.0 * m_retract, 2.0 * m_distance, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eProbe, CNCPoint(+1.0 * m_retract, 2.0 * m_distance, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eEndOfData, CNCPoint(0,0,0) ) );
			break;

		case eBottomRight:
			// Bottom
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(-1.0 * m_distance, 0, 0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(-1.0 * m_distance, -1.0 * m_retract, 0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(-1.0 * m_distance, -1.0 * m_retract, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eProbe, CNCPoint(-1.0 * m_distance, +1.0 * m_retract, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eEndOfData, CNCPoint(0,0,0) ) );

			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(-2.0 * m_distance, 0, 0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(-2.0 * m_distance, -1.0 * m_retract, 0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(-2.0 * m_distance, -1.0 * m_retract, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eProbe, CNCPoint(-2.0 * m_distance, +1.0 * m_retract, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eEndOfData, CNCPoint(0,0,0) ) );

			// Right
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(0,m_distance,0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(+1.0 * m_retract, m_distance, 0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(+1.0 * m_retract, m_distance, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eProbe, CNCPoint(-1.0 * m_retract, m_distance, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eEndOfData, CNCPoint(0,0,0) ) );

			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(0,2.0 * m_distance,0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(+1.0 * m_retract, 2.0 * m_distance, 0 )) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(+1.0 * m_retract, 2.0 * m_distance, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eProbe, CNCPoint(-1.0 * m_retract, 2.0 * m_distance, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eEndOfData, CNCPoint(0,0,0) ) );			
			break;

		case eTopLeft:
			// Top
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(m_distance, 0, 0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(m_distance, +1.0 * m_retract,0 )) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(m_distance, +1.0 * m_retract, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eProbe, CNCPoint(m_distance, -1.0 * m_retract, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eEndOfData, CNCPoint(0,0,0) ) );

			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(2.0 * m_distance, 0, 0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(2.0 * m_distance, +1.0 * m_retract,0 )) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(2.0 * m_distance, +1.0 * m_retract, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eProbe, CNCPoint(2.0 * m_distance, -1.0 * m_retract, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eEndOfData, CNCPoint(0,0,0) ) );

			// Left
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(0,-1.0 * m_distance,0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(-1.0 * m_retract, -1.0 * m_distance, 0 )) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(-1.0 * m_retract, -1.0 * m_distance, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eProbe, CNCPoint(+1.0 * m_retract, -1.0 * m_distance, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eEndOfData, CNCPoint(0,0,0) ) );

			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(0,-2.0 * m_distance,0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(-1.0 * m_retract, -2.0 * m_distance, 0 )) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(-1.0 * m_retract, -2.0 * m_distance, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eProbe, CNCPoint(+1.0 * m_retract, -2.0 * m_distance, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eEndOfData, CNCPoint(0,0,0) ) );
			break;

		case eTopRight:
			// Top
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(-1.0 * m_distance, 0, 0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(-1.0 * m_distance, +1.0 * m_retract,0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(-1.0 * m_distance, +1.0 * m_retract, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eProbe, CNCPoint(-1.0 * m_distance, -1.0 * m_retract, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eEndOfData, CNCPoint(0,0,0) ) );

			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(-2.0 * m_distance, 0, 0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(-2.0 * m_distance, +1.0 * m_retract,0 )) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(-2.0 * m_distance, +1.0 * m_retract, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eProbe, CNCPoint(-2.0 * m_distance, -1.0 * m_retract, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eEndOfData, CNCPoint(0,0,0) ) );

			// Right
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(0,-1.0 * m_distance,0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(+1.0 * m_retract, -1.0 * m_distance, 0 )) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(+1.0 * m_retract, -1.0 * m_distance, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eProbe, CNCPoint(-1.0 * m_retract, -1.0 * m_distance, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eEndOfData, CNCPoint(0,0,0) ) );

			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(0,-2.0 * m_distance,0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(+1.0 * m_retract, -2.0 * m_distance, 0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(+1.0 * m_retract, -2.0 * m_distance, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eProbe, CNCPoint(-1.0 * m_retract, -2.0 * m_distance, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eEndOfData, CNCPoint(0,0,0) ) );
			break;
		} // End switch
	} // End if - else

	return(points);

} // End SetPoints() method



/**
	Based on this object's parameters, determine a set of coordinates (relative to (0,0,0)) that
	the machine will move through when probing.
 */
CProbing::PointsList_t CProbe_Centre::GetPoints() const
{
	CProbing::PointsList_t points;

	if (m_direction == eOutside)
	{
		if ((m_alignment == eXAxis) || (m_number_of_points == 4))
		{
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(m_distance, 0, 0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(m_distance, 0, 0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(m_distance, 0, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eProbe, CNCPoint(0,0 , -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eEndOfData, CNCPoint(0,0,0) ) );

			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(-1.0 * m_distance, 0, 0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(-1.0 * m_distance, 0, 0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(-1.0 * m_distance, 0, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eProbe, CNCPoint(0, 0 , -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eEndOfData, CNCPoint(0,0,0) ) );
		} // End if - then
		else
		{
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(0, m_distance, 0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(0, m_distance, 0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(0, m_distance, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eProbe, CNCPoint(0, 0 , -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eEndOfData, CNCPoint(0,0,0) ) );

			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(0, -1.0 * m_distance, 0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(0, -1.0 * m_distance, 0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(0, -1.0 * m_distance, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eProbe, CNCPoint(0, 0 , -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eEndOfData, CNCPoint(0,0,0) ) );
		} // End if - else
	} // End if - then
	else
	{
		// From inside - outwards

		if ((m_alignment == eXAxis) || (m_number_of_points == 4))
		{
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(0, 0, 0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(0, 0, 0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(0, 0, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eProbe, CNCPoint(+1.0 * m_distance ,0 , -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eEndOfData, CNCPoint(0,0,0) ) );

			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(0, 0, 0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(0, 0, 0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(0, 0, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eProbe, CNCPoint(-1.0 * m_distance ,0 , -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eEndOfData, CNCPoint(0,0,0) ) );
		} // End if - then
		else
		{
			// eYAxis
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(0, 0, 0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(0, 0, 0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(0, 0, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eProbe, CNCPoint(0, +1.0 * m_distance , -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eEndOfData, CNCPoint(0,0,0) ) );

			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(0, 0, 0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(0, 0, 0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(0, 0, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eProbe, CNCPoint(0, -1.0 * m_distance , -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eEndOfData, CNCPoint(0,0,0) ) );
		} // End if - else
	} // End if - else

	if (m_number_of_points == 4)
	{
		if (m_direction == eOutside)
		{
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(0, m_distance, 0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(0, m_distance, 0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(0, m_distance, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eProbe, CNCPoint(0, 0 , -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eEndOfData, CNCPoint(0,0,0) ) );

			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(0, -1.0 * m_distance, 0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(0, -1.0 * m_distance, 0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(0, -1.0 * m_distance, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eProbe, CNCPoint(0, 0 , -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eEndOfData, CNCPoint(0,0,0) ) );
		}
		else
		{
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(0, 0, 0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(0, 0, 0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(0, 0, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eProbe, CNCPoint(0, +1.0 * m_distance , -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eEndOfData, CNCPoint(0,0,0) ) );

			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(0, 0, 0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(0, 0, 0) ) );
			points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(0, 0, -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eProbe, CNCPoint(0, -1.0 * m_distance , -1.0 * m_depth) ) );
			points.push_back( std::make_pair( CProbing::eEndOfData, CNCPoint(0,0,0) ) );
		} // End if - else
	} // End if - then

	return(points);

} // End SetPoints() method


void CProbe_Edge::GenerateMeaningfullName()
{
	if (m_number_of_edges == 1)
	{
		m_title = _("Probe ");
		m_title << eEdges_t(m_edge) << _(" edge");
	} // End if - then
	else
	{
		// We're looking for the intersection of two edges.

		m_title = _("Probe ");
		m_title << eCorners_t(m_corner) << _(" corner");
	} // End if - else
}

void CProbe_Centre::GenerateMeaningfullName()
{
	if (m_direction == eOutside)
	{
		m_title = _("Probe protrusion along ");
		m_title << eAlignment_t(m_alignment);
	} // End if - then
	else
	{
		m_title = _("Probe hole along ");
		m_title << eAlignment_t(m_alignment);
	} // End if - else

	if (m_number_of_points == 4)
	{
		if (m_direction == eOutside)
		{
			m_title = _("Probe protrusion");
		} // End if - then
		else
		{
			m_title = _("Probe hole");
		} // End if - else
	} // End if - then
}
