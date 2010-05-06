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
#include "interface/PropertyVertex.h"
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
	((CProbing*)object)->GenerateMeaningfullName();
	heeksCAD->Changed();	// Force a re-draw from glCommands()
}

static void on_set_depth(double value, HeeksObj* object)
{
	((CProbing*)object)->m_depth = value;
	((CProbing*)object)->GenerateMeaningfullName();
	heeksCAD->Changed();	// Force a re-draw from glCommands()
}

Python CProbe_Centre::AppendTextToProgram( const CFixture *pFixture )
{
	Python python;

	python << CSpeedOp::AppendTextToProgram( pFixture );

	double probe_offset_x = 0.0;
	double probe_offset_y = 0.0;

	double probe_radius = 0.0;

	if (m_cutting_tool_number > 0)
	{
		CCuttingTool *pCuttingTool = CCuttingTool::Find( m_cutting_tool_number);
		if (pCuttingTool != NULL)
		{
			probe_radius = pCuttingTool->CuttingRadius(true);
			probe_offset_x = 0.0 - pCuttingTool->m_params.m_probe_offset_x;
			probe_offset_y = 0.0 - pCuttingTool->m_params.m_probe_offset_y;
		} // End if - then
	} // End if - then

	// We're going to be working in relative coordinates based on the assumption
	// that the operator has first jogged the machine to the approximate centre point.

	CProbing::PointsList_t points = GetPoints();

	if (m_direction == eOutside)
	{
		python << _T("comment(") << PythonString(_("This program assumes that the machine operator has jogged")) << _T(")\n");
		python << _T("comment(") << PythonString(_("the machine to approximatedly the correct location")) << _T(")\n");
		python << _T("comment(") << PythonString(_("immediately above the protrusion we are finding the centre of.")) << _T(")\n");
		python << _T("comment(") << PythonString(_("This program then jogs out and down in two opposite directions")) << _T(")\n");
		python << _T("comment(") << PythonString(_("before probing back towards the centre point looking for the")) << _T(")\n");
		python << _T("comment(") << PythonString(_("protrusion.")) << _T(")\n");

		if ((m_alignment == eXAxis) || (m_number_of_points == 4))
		{
			python << AppendTextForSingleProbeOperation( points[0].second, points[1].second, points[2].second.Z(false), points[3].second,
								       	_T("1001"), _T("1002"), -1.0 * probe_radius, 0 );
			python << AppendTextForSingleProbeOperation( points[5].second, points[6].second, points[7].second.Z(false), points[8].second,
										_T("1003"), _T("1004"), +1.0 * probe_radius, 0 );

            // Now move to the centre of these two intersection points.
            python << _T("comment(") << PythonString(_("Move back to the intersection points")) << _T(")\n");
            python << _T("comment(") << PythonString(_("NOTE: We set the temporary origin because it was in effect when the values in these variables were established")) << _T(")\n");
            python << _T("set_temporary_origin( x=0, y=0, z=0 )\n");
            python << _T("rapid_to_midpoint(") << _T("x1='[#1001 + ") << probe_offset_x << _T("]',") << _T("x2='[#1003 + ") << probe_offset_x << _T("]')\n");
            python << _T("remove_temporary_origin()\n");
		} // End if - then
		else
		{
			python << AppendTextForSingleProbeOperation( points[0].second, points[1].second, points[2].second.Z(false), points[3].second,
									_T("1001"), _T("1002"), 0, -1.0 * probe_radius );
			python << AppendTextForSingleProbeOperation( points[5].second, points[6].second, points[7].second.Z(false), points[8].second,
									_T("1003"), _T("1004"), 0, +1.0 * probe_radius );

            // Now move to the centre of these two intersection points.
            python << _T("comment(") << PythonString(_("Move back to the intersection points")) << _T(")\n");
            python << _T("comment(") << PythonString(_("NOTE: We set the temporary origin because it was in effect when the values in these variables were established")) << _T(")\n");
            python << _T("set_temporary_origin( x=0, y=0, z=0 )\n");
            python << _T("rapid_to_midpoint(") << _T("y1='[#1002 + ") << probe_offset_y << _T("]',") << _T("y2='[#1004 + ") << probe_offset_y << _T("]')\n");
            python << _T("remove_temporary_origin()\n");
		} // End if - else
	} // End if - then
	else
	{
		python << _T("comment(") << PythonString(_("This program assumes that the machine operator has jogged")) << _T(")\n");
		python << _T("comment(") << PythonString(_("the machine to approximatedly the correct location")) << _T(")\n");
		python << _T("comment(") << PythonString(_("immediately above the hole we are finding the centre of.")) << _T(")\n");
		python << _T("comment(") << PythonString(_("This program then jogs down and probes out in two opposite directions")) << _T(")\n");
		python << _T("comment(") << PythonString(_("looking for the workpiece")) << _T(")\n");

		if ((m_alignment == eXAxis) || (m_number_of_points == 4))
		{
			python << AppendTextForSingleProbeOperation( points[0].second, points[1].second, points[2].second.Z(false), points[3].second,
									_T("1001"), _T("1002"), +1.0 * probe_radius, 0 );
			python << AppendTextForSingleProbeOperation( points[5].second, points[6].second, points[7].second.Z(false), points[8].second,
									_T("1003"), _T("1004"), -1.0 * probe_radius, 0 );

            // Now move to the centre of these two intersection points.
            python << _T("comment(") << PythonString(_("Move back to the intersection points")) << _T(")\n");
            python << _T("comment(") << PythonString(_("NOTE: We set the temporary origin because it was in effect when the values in these variables were established")) << _T(")\n");
            python << _T("set_temporary_origin( x=0, y=0, z=0 )\n");
            python << _T("rapid_to_midpoint(") << _T("x1='[#1001 + ") << probe_offset_x << _T("]',") << _T("x2='[#1003 + ") << probe_offset_x << _T("]')\n");
            python << _T("remove_temporary_origin()\n");
		} // End if - then
		else
		{
			// eYAxis:
			python << AppendTextForSingleProbeOperation( points[0].second, points[1].second, points[2].second.Z(false), points[3].second,
									_T("1001"), _T("1002"), 0, +1.0 * probe_radius );
			python << AppendTextForSingleProbeOperation( points[5].second, points[6].second, points[7].second.Z(false), points[8].second,
									_T("1003"), _T("1004"), 0, -1.0 * probe_radius  );

            // Now move to the centre of these two intersection points.
            python << _T("comment(") << PythonString(_("Move back to the intersection points")) << _T(")\n");
            python << _T("comment(") << PythonString(_("NOTE: We set the temporary origin because it was in effect when the values in these variables were established")) << _T(")\n");
            python << _T("set_temporary_origin( x=0, y=0, z=0 )\n");
            python << _T("rapid_to_midpoint(") << _T("y1='[#1002 + ") << probe_offset_y << _T("]',") << _T("y2='[#1004 + ") << probe_offset_y << _T("]')\n");
            python << _T("remove_temporary_origin()\n");
		} // End if - else
	} // End if - else



	if (m_number_of_points == 4)
	{
		if (m_direction == eOutside)
		{
			python << _T("comment(") << PythonString(_("This program assumes that the machine operator has jogged")) << _T(")\n");
			python << _T("comment(") << PythonString(_("the machine to approximatedly the correct location")) << _T(")\n");
			python << _T("comment(") << PythonString(_("immediately above the protrusion we are finding the centre of.")) << _T(")\n");
			python << _T("comment(") << PythonString(_("This program then jogs out and down in two opposite directions")).c_str() << _T(")\n");
			python << _T("comment(") << PythonString(_("before probing back towards the centre point looking for the")) << _T(")\n");
			python << _T("comment(") << PythonString(_("protrusion.")) << _T(")\n");

			python << AppendTextForSingleProbeOperation( points[10].second, points[11].second, points[12].second.Z(false), points[13].second,
									_T("1005"), _T("1006"), 0, -1.0 * probe_radius );
			python << AppendTextForSingleProbeOperation( points[15].second, points[16].second, points[17].second.Z(false), points[18].second,
									_T("1007"), _T("1008"), 0, +1.0 * probe_radius );
		} // End if - then
		else
		{
			python << _T("comment(") << PythonString(_("This program assumes that the machine operator has jogged")) << _T(")\n");
			python << _T("comment(") << PythonString(_("the machine to approximatedly the correct location")) << _T(")\n");
			python << _T("comment(") << PythonString(_("immediately above the hole we are finding the centre of.")) << _T(")\n");
			python << _T("comment(") << PythonString(_("This program then jogs down and probes out in two opposite directions")) << _T(")\n");
			python << _T("comment(") << PythonString(_("looking for the workpiece")) << _T(")\n");

			python << AppendTextForSingleProbeOperation( points[10].second, points[11].second, points[12].second.Z(false), points[13].second,
									_T("1005"), _T("1006"), 0, +1.0 * probe_radius );
			python << AppendTextForSingleProbeOperation( points[15].second, points[16].second, points[17].second.Z(false), points[18].second,
									_T("1007"), _T("1008"), 0, -1.0 * probe_radius );
		} // End if - else

		// Now move to the centre of these two intersection points.
		python << _T("comment(") << PythonString(_("Move back to the intersection points")) << _T(")\n");
		python << _T("comment(") << PythonString(_("NOTE: We set the temporary origin because it was in effect when the values in these variables were established")) << _T(")\n");
		python << _T("set_temporary_origin( x=0, y=0, z=0 )\n");
		python << _T("rapid_to_midpoint(") << _T("y1='[#1006 + ") << probe_offset_y << _T("]',") << _T("y2='[#1008 + ") << probe_offset_y << _T("]')\n");
		python << _T("remove_temporary_origin()\n");

		python << _T("report_probe_results( ")
				<< _T("x1='[[[[#1001-#1003]/2.0]+#1003] + ") << probe_offset_x << _T("]', ")
				<< _T("y1='[[[[#1006-#1008]/2.0]+#1008] + ") << probe_offset_y << _T("]', ");
		python << _T("xml_file_name=") << PythonString(this->GetOutputFileName( _T(".xml"), true )) << _T(")\n");
	} // End if - then

	return(python);
}


Python CProbe_Grid::AppendTextToProgram( const CFixture *pFixture )
{
	Python python;

	python << CSpeedOp::AppendTextToProgram( pFixture );

	// We're going to be working in relative coordinates based on the assumption
	// that the operator has first jogged the machine to the approximate starting point.
	int variable = 1001;
	bool first_probed_point = true;
	std::list<gp_Pnt> probed_points;

	python << _T("open_log_file(xml_file_name=") << PythonString(this->GetOutputFileName( _T(".xml"), true ).c_str()).c_str() << _T(")\n");
	python << _T("log_message('<POINTS>')\n");

	CProbing::PointsList_t points = GetPoints();
	for (CProbing::PointsList_t::const_iterator l_itPoint = points.begin(); l_itPoint != points.end(); l_itPoint++)
	{
		switch (l_itPoint->first)
		{
		case eRapid:
			break;

		case eProbe:
			{
				// We've already moved to this point ourselves so tell the AppendTextForDownwardProbingOperation()
				// method that the X,Y coordinate is 0,0.  We're only interested in the Z point anyway and this
				// will avoid extra rapid movements.
				wxString var;
				var << variable;

				python << AppendTextForDownwardProbingOperation( PythonString(l_itPoint->second.X(true)).c_str(), PythonString(l_itPoint->second.Y(true)).c_str(), m_depth, var.c_str() );
                probed_points.push_back( gp_Pnt( l_itPoint->second.X(true), l_itPoint->second.Y(true), variable ) );

                if (! m_for_fixture_measurement)
                {
                    python << _T("log_coordinate(x='[") << l_itPoint->second.X(true) << _T("]', ")
                                        _T("y='[") << l_itPoint->second.Y(true) << _T("]', ")
                                        _T("z='[#") << var.c_str() << _T("]')\n");
                }

                first_probed_point = false;
                variable++;
			}
			break;

		case eEndOfData:
			break;
		} // End switch


	} // End for

    if (m_for_fixture_measurement)
    {
        // Now report on the sets of probed points along each axis.
        gp_Pnt top_left(*probed_points.begin());
        gp_Pnt top_right(*probed_points.begin());
        gp_Pnt bottom_left(*probed_points.begin());
        gp_Pnt bottom_right(*probed_points.begin());

        for (std::list<gp_Pnt>::iterator itPoint = probed_points.begin(); itPoint != probed_points.end(); itPoint++)
        {
           if ((itPoint->X() <= top_left.X()) && (itPoint->Y() >= top_left.Y())) top_left = *itPoint;
           if ((itPoint->X() >= top_right.X()) && (itPoint->Y() >= top_right.Y())) top_right = *itPoint;
           if ((itPoint->X() <= bottom_left.X()) && (itPoint->Y() <= bottom_left.Y())) bottom_left = *itPoint;
           if ((itPoint->X() >= bottom_right.X()) && (itPoint->Y() <= bottom_right.Y())) bottom_right = *itPoint;
        }

        // Log the longest two edges along the X and Y axes in a form that will be suitable
        // for reading back into the Fixture object.
        python << _T("log_coordinate( ")
            << _T("x='[") << bottom_left.X() << _T("]', ")
            << _T("y='[") << bottom_left.Y() << _T("]', ")
            << _T("z='[#") << bottom_left.Z() << _T("]')\n");

        python << _T("log_coordinate( ")
            << _T("x='[") << top_left.X() << _T("]', ")
            << _T("y='[") << top_left.Y() << _T("]', ")
            << _T("z='[#") << top_left.Z() << _T("]')\n");

        python << _T("log_coordinate( ")
            << _T("x='[") << top_left.X() - bottom_left.X() << _T("]', ")
            << _T("y='[") << top_left.Y() - bottom_left.Y() << _T("]', ")
            << _T("z='0.0' )\n");

        python << _T("log_coordinate( ")
            << _T("x='[") << bottom_left.X() << _T("]', ")
            << _T("y='[") << bottom_left.Y() << _T("]', ")
            << _T("z='[#") << bottom_left.Z() << _T("]')\n");

        python << _T("log_coordinate( ")
            << _T("x='[") << bottom_right.X() << _T("]', ")
            << _T("y='[") << bottom_right.Y() << _T("]', ")
            << _T("z='[#") << bottom_right.Z() << _T("]')\n");

        python << _T("log_coordinate( ")
            << _T("x='[") << bottom_right.X() - bottom_left.X() << _T("]', ")
            << _T("y='[") << bottom_right.Y() - bottom_left.Y() << _T("]', ")
            << _T("z='0.0' )\n");
    }

    python << _T("log_message('</POINTS>')\n");
    python << _T("close_log_file()\n");

	return(python);
}



const wxBitmap &CProbing::GetIcon()
{
	if(!m_active)return GetInactiveIcon();
	static wxBitmap* icon = NULL;
	if(icon == NULL)icon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/probe.png")));
	return *icon;
}

/**
	Generate the Python script that will move in the setup direction, retract out away from the workpiece,
	plunge down and probe back towards the workpiece.  Once found, it should store the results in the
	variables specified.
 */
Python CProbing::AppendTextForSingleProbeOperation(
	const CNCPoint setup_point,
	const CNCPoint retract_point,
	const double depth,
	const CNCPoint probe_point,
	const wxString &intersection_variable_x,
	const wxString &intersection_variable_y,
        const double probe_offset_x_component,
	const double probe_offset_y_component	) const
{
	Python python;

	// Make sure it's negative as we're stepping down.  There is no option
	// to specify a starting height so this can only be a relative distance.
	double relative_depth = (depth > 0)?(-1.0 * depth):depth;

	python << _T("comment('Begin probing operation for a single point.')\n");
	python << _T("comment('The results will be stored with respect to the current location of the machine')\n");
	python << _T("comment('The machine will be returned to this original position following this single probe operation')\n");
	python << _T("probe_single_point(")
		<< _T("point_along_edge_x=") << setup_point.X(true) << _T(", ")
			<< _T("point_along_edge_y=") << setup_point.Y(true) << _T(", ")
			<< _T("depth=") << relative_depth / theApp.m_program->m_units << _T(", ")
			<< _T("retracted_point_x=") << retract_point.X(true) << _T(", ")
			<< _T("retracted_point_y=") << retract_point.Y(true) << _T(", ")
			<< _T("destination_point_x=") << probe_point.X(true) << _T(", ")
			<< _T("destination_point_y=") << probe_point.Y(true) << _T(", ")
			<< _T("intersection_variable_x='") << intersection_variable_x.c_str() << _T("', ")
			<< _T("intersection_variable_y='") << intersection_variable_y.c_str() << _T("', ")
            << _T("probe_offset_x_component='") << probe_offset_x_component << _T("', ")
			<< _T("probe_offset_y_component='") << probe_offset_y_component << _T("' )\n");

	return(python);
}

/**
	Generate the Python script that will move to the setup point and then probe directly down to
	find the Z coordinate at that point.  Once found, it should store the results in the
	variable specified.
 */
Python CProbing::AppendTextForDownwardProbingOperation(
	const wxString setup_point_x,
	const wxString setup_point_y,
	const double depth,
	const wxString &intersection_variable_z ) const
{
	Python python;

	// Make sure it's negative as we're stepping down.  There is no option
	// to specify a starting height so this can only be a relative distance.
	double relative_depth = (depth > 0)?(-1.0 * depth):depth;

	python << _T("comment('Begin probing operation for a single point.')\n");
	python << _T("comment('The results will be stored with respect to the current location of the machine')\n");
	python << _T("comment('The machine will be returned to this original position following this single probe operation')\n");

	python << _T("probe_downward_point(")
		<< _T("x='") << setup_point_x.c_str() << _T("', ")
			<< _T("y='") << setup_point_y.c_str() << _T("', ")
			<< _T("depth=") << relative_depth / theApp.m_program->m_units << _T(", ")
			<< _T("intersection_variable_z='") << intersection_variable_z.c_str() << _T("' )\n");

	return(python);
}


Python CProbe_Edge::AppendTextToProgram( const CFixture *pFixture )
{
	Python python;

	python << CSpeedOp::AppendTextToProgram( pFixture );

	// We're going to be working in relative coordinates based on the assumption
	// that the operator has first jogged the machine to the approximate centre point.

	python << _T("comment(") << PythonString(_("This program assumes that the machine operator has jogged")) << _T(")\n");
	python << _T("comment(") << PythonString(_("the machine to approximatedly the correct location")) << _T(")\n");
	python << _T("comment('immediately above the ");

	if (m_number_of_edges == 1)
	{
		python << eEdges_t(m_edge) << _T(" edge of the workpiece.')\n");
	}
	else
	{
		python << eCorners_t(m_corner) << _T(" corner of the workpiece.')\n");
	}

	double probe_offset_x = 0.0;
	double probe_offset_y = 0.0;
	double probe_radius = 0.0;

	if (m_cutting_tool_number > 0)
	{
		CCuttingTool *pCuttingTool = CCuttingTool::Find( m_cutting_tool_number);
		if (pCuttingTool != NULL)
		{
			probe_radius = pCuttingTool->CuttingRadius(true);
			probe_offset_x = 0.0 - pCuttingTool->m_params.m_probe_offset_x;
			probe_offset_y = 0.0 - pCuttingTool->m_params.m_probe_offset_y;
		} // End if - then
	} // End if - then

	gp_Dir z_direction(0,0,1);

	CProbing::PointsList_t points = GetPoints();

	if (m_number_of_edges == 1)
	{
		gp_Vec reference_vector;
		switch(m_edge)
		{
		case eBottom:
			reference_vector = gp_Vec( 1, 0, 0 );
			python << AppendTextForSingleProbeOperation( points[0].second, points[1].second, points[2].second.Z(false), points[3].second,
								       	_T("1001"), _T("1002"), 0, +1.0 * probe_radius );
			if (m_check_levels)
			{
				wxString setup_point_x;
				wxString setup_point_y;
				setup_point_x << _T("[ #1001 ]");
				setup_point_y << _T("[ #1002 + ") << probe_radius << _T(" ]");

				python << AppendTextForDownwardProbingOperation( setup_point_x, setup_point_y, m_depth, _T("1003") );
			} // End if - then
			python << AppendTextForSingleProbeOperation( points[5].second, points[6].second, points[7].second.Z(false), points[8].second,
										_T("1004"), _T("1005"), 0, +1.0 * probe_radius );
			if (m_check_levels)
			{
				wxString setup_point_x;
				wxString setup_point_y;
				setup_point_x << _T("[ #1004 ]");
				setup_point_y << _T("[ #1005 + ") << probe_radius << _T(" ]");

				python << AppendTextForDownwardProbingOperation( setup_point_x, setup_point_y, m_depth, _T("1006") );
			} // End if - then
			break;

		case eTop:
			reference_vector = gp_Vec( 1, 0, 0 );
			python << AppendTextForSingleProbeOperation( points[0].second, points[1].second, points[2].second.Z(false), points[3].second,
										_T("1001"), _T("1002"), 0, -1.0 * probe_radius );
			if (m_check_levels)
			{
				wxString setup_point_x;
				wxString setup_point_y;
				setup_point_x << _T("[ #1001 ]");
				setup_point_y << _T("[ #1002 + ") << -1.0 * probe_radius << _T(" ]");

				python << AppendTextForDownwardProbingOperation( setup_point_x, setup_point_y, m_depth, _T("1003") );
			} // End if - then

			python << AppendTextForSingleProbeOperation( points[5].second, points[6].second, points[7].second.Z(false), points[8].second,
										_T("1004"), _T("1005"), 0, -1.0 * probe_radius );
			if (m_check_levels)
			{
				wxString setup_point_x;
				wxString setup_point_y;
				setup_point_x << _T("[ #1004 ]");
				setup_point_y << _T("[ #1005 + ") << -1.0 * probe_radius << _T(" ]");

				python << AppendTextForDownwardProbingOperation( setup_point_x, setup_point_y, m_depth, _T("1006") );
			} // End if - then
			break;

		case eLeft:
			reference_vector = gp_Vec( 0, 1, 0 );
			python << AppendTextForSingleProbeOperation( points[0].second, points[1].second, points[2].second.Z(false), points[3].second,
									_T("1001"), _T("1002"), +1.0 * probe_radius, 0 );
			if (m_check_levels)
			{
				wxString setup_point_x;
				wxString setup_point_y;
				setup_point_x << _T("[ #1001 + ") << +1.0 * probe_radius << _T(" ]");
				setup_point_y << _T("[ #1002 ]");

				python << AppendTextForDownwardProbingOperation( setup_point_x, setup_point_y, m_depth, _T("1003") );
			} // End if - then

			python << AppendTextForSingleProbeOperation( points[5].second, points[6].second, points[7].second.Z(false), points[8].second,
									_T("1004"), _T("1005"), +1.0 * probe_radius,0 );
			if (m_check_levels)
			{
				wxString setup_point_x;
				wxString setup_point_y;
				setup_point_x << _T("[ #1004 + ") << +1.0 * probe_radius << _T(" ]");
				setup_point_y << _T("[ #1005 ]");

				python << AppendTextForDownwardProbingOperation( setup_point_x, setup_point_y, m_depth, _T("1006") );
			} // End if - then
			break;

		case eRight:
			reference_vector = gp_Vec( 0, 1, 0 );
			python << AppendTextForSingleProbeOperation( points[0].second, points[1].second, points[2].second.Z(false), points[3].second,
									_T("1001"), _T("1002"), -1.0 * probe_radius, 0 );
			if (m_check_levels)
			{
				wxString setup_point_x;
				wxString setup_point_y;
				setup_point_x << _T("[ #1001 + ") << -1.0 * probe_radius << _T(" ]");
				setup_point_y << _T("[ #1002 ]");

				python << AppendTextForDownwardProbingOperation( setup_point_x, setup_point_y, m_depth, _T("1003") );
			} // End if - then
			python << AppendTextForSingleProbeOperation( points[5].second, points[6].second, points[7].second.Z(false), points[8].second,
									_T("1004"), _T("1005"), -1.0 * probe_radius, 0 );
			if (m_check_levels)
			{
				wxString setup_point_x;
				wxString setup_point_y;
				setup_point_x << _T("[ #1004 + ") << -1.0 * probe_radius << _T(" ]");
				setup_point_y << _T("[ #1005 ]");

				python << AppendTextForDownwardProbingOperation( setup_point_x, setup_point_y, m_depth, _T("1006") );
			} // End if - then
			break;
		} // End switch

		// Combine the two probed points into an edge and generate an XML document describing the angle they form.
		python << _T("report_probe_results( ")
				<< _T("x1='[#1001 + ") << probe_offset_x << _T("]', ")
				<< _T("y1='[#1002 + ") << probe_offset_y << _T("]', ")
				<< _T("x2='[#1004 + ") << probe_offset_x << _T("]', ")
				<< _T("y2='[#1005 + ") << probe_offset_y << _T("]', ")
				<< _T("x3='") << reference_vector.X() << _T("', ")
				<< _T("y3='") << reference_vector.Y() << _T("', ");

		if (m_check_levels)
		{
			python << _T("z1='#1003', ");
			python << _T("z2='#1006', ");
			python << _T("z3='") << reference_vector.Z() << _T("', ");
		} // End if - then

		python << _T("xml_file_name=") << PythonString(this->GetOutputFileName( _T(".xml"), true )) << _T(")\n");
	} // End if - then
	else
	{
		// We're looking for the intersection of two edges.

		gp_Vec ref1, ref2;

		switch (m_corner)
		{
		case eBottomLeft:
			// Bottom
			ref1 = gp_Vec(1,0,0);
			ref2 = gp_Vec(0,1,0);
			python << AppendTextForSingleProbeOperation( points[0].second, points[1].second, points[2].second.Z(false), points[3].second,
									_T("1001"), _T("1002"), 0, +1.0 * probe_radius );
			if (m_check_levels)
			{
				wxString setup_point_x;
				wxString setup_point_y;
				setup_point_x << _T("[ #1001 ]");
				setup_point_y << _T("[ #1002 + ") << +1.0 * probe_radius << _T(" ]");

				python << AppendTextForDownwardProbingOperation( setup_point_x, setup_point_y, m_depth, _T("1003") );
			} // End if - then

			python << AppendTextForSingleProbeOperation( points[5].second, points[6].second, points[7].second.Z(false), points[8].second,
									_T("1004"), _T("1005"), 0, +1.0 * probe_radius );
			if (m_check_levels)
			{
				wxString setup_point_x;
				wxString setup_point_y;
				setup_point_x << _T("[ #1004 ]");
				setup_point_y << _T("[ #1005 + ") << +1.0 * probe_radius << _T(" ]");

				python << AppendTextForDownwardProbingOperation( setup_point_x, setup_point_y, m_depth, _T("1006") );
			} // End if - then

			// Left
			python << AppendTextForSingleProbeOperation( points[10].second, points[11].second, points[12].second.Z(false), points[13].second,
									_T("1007"), _T("1008"), +1.0 * probe_radius, 0 );
			if (m_check_levels)
			{
				wxString setup_point_x;
				wxString setup_point_y;
				setup_point_x << _T("[ #1007 + ") << +1.0 * probe_radius << _T(" ]");
				setup_point_y << _T("[ #1008 ]");

				python << AppendTextForDownwardProbingOperation( setup_point_x, setup_point_y, m_depth, _T("1009") );
			} // End if - then
			python << AppendTextForSingleProbeOperation( points[15].second, points[16].second, points[17].second.Z(false), points[18].second,
									_T("1010"), _T("1011"), +1.0 * probe_radius, 0 );
			if (m_check_levels)
			{
				wxString setup_point_x;
				wxString setup_point_y;
				setup_point_x << _T("[ #1010 + ") << +1.0 * probe_radius << _T(" ]");
				setup_point_y << _T("[ #1011 ]");

				python << AppendTextForDownwardProbingOperation( setup_point_x, setup_point_y, m_depth, _T("1012") );
			} // End if - then
			break;

		case eBottomRight:
			ref1 = gp_Vec(-1,0,0);
			ref2 = gp_Vec(0,1,0);
			// Bottom
			python << AppendTextForSingleProbeOperation( points[0].second, points[1].second, points[2].second.Z(false), points[3].second,
									_T("1001"), _T("1002"), 0, +1.0 * probe_radius );
			if (m_check_levels)
			{
				wxString setup_point_x;
				wxString setup_point_y;
				setup_point_x << _T("[ #1001 ]");
				setup_point_y << _T("[ #1002 + ") << +1.0 * probe_radius << _T(" ]");

				python << AppendTextForDownwardProbingOperation( setup_point_x, setup_point_y, m_depth, _T("1003") );
			} // End if - then

			python << AppendTextForSingleProbeOperation( points[5].second, points[6].second, points[7].second.Z(false), points[8].second,
									_T("1004"), _T("1005"), 0, +1.0 * probe_radius );
			if (m_check_levels)
			{
				wxString setup_point_x;
				wxString setup_point_y;
				setup_point_x << _T("[ #1004 ]");
				setup_point_y << _T("[ #1005 + ") << +1.0 * probe_radius << _T(" ]");

				python << AppendTextForDownwardProbingOperation( setup_point_x, setup_point_y, m_depth, _T("1006") );
			} // End if - then

			// Right
			python << AppendTextForSingleProbeOperation( points[10].second, points[11].second, points[12].second.Z(false), points[13].second,
									_T("1007"), _T("1008"), -1.0 * probe_radius, 0 );
			if (m_check_levels)
			{
				wxString setup_point_x;
				wxString setup_point_y;
				setup_point_x << _T("[ #1007 + ") << -1.0 * probe_radius << _T(" ]");
				setup_point_y << _T("[ #1008 ]");

				python << AppendTextForDownwardProbingOperation( setup_point_x, setup_point_y, m_depth, _T("1009") );
			} // End if - then

			python << AppendTextForSingleProbeOperation( points[15].second, points[16].second, points[17].second.Z(false), points[18].second,
									_T("1010"), _T("1011"), -1.0 * probe_radius, 0 );
			if (m_check_levels)
			{
				wxString setup_point_x;
				wxString setup_point_y;
				setup_point_x << _T("[ #1010 + ") << -1.0 * probe_radius << _T(" ]");
				setup_point_y << _T("[ #1011 ]");

				python << AppendTextForDownwardProbingOperation( setup_point_x, setup_point_y, m_depth, _T("1012") );
			} // End if - then


			break;

		case eTopLeft:
			ref1 = gp_Vec(1,0,0);
			ref2 = gp_Vec(0,-1,0);
			// Top
			python << AppendTextForSingleProbeOperation( points[0].second, points[1].second, points[2].second.Z(false), points[3].second,
									_T("1001"), _T("1002"), 0, -1.0 * probe_radius );
			if (m_check_levels)
			{
				wxString setup_point_x;
				wxString setup_point_y;
				setup_point_x << _T("[ #1001 ]");
				setup_point_y << _T("[ #1002 + ") << -1.0 * probe_radius << _T(" ]");

				python << AppendTextForDownwardProbingOperation( setup_point_x, setup_point_y, m_depth, _T("1003") );
			} // End if - then

			python << AppendTextForSingleProbeOperation( points[5].second, points[6].second, points[7].second.Z(false), points[8].second,
									_T("1004"), _T("1005"), 0, -1.0 * probe_radius );
			if (m_check_levels)
			{
				wxString setup_point_x;
				wxString setup_point_y;
				setup_point_x << _T("[ #1004 ]");
				setup_point_y << _T("[ #1005 + ") << -1.0 * probe_radius << _T(" ]");

				python << AppendTextForDownwardProbingOperation( setup_point_x, setup_point_y, m_depth, _T("1006") );
			} // End if - then

			// Left
			python << AppendTextForSingleProbeOperation( points[10].second, points[11].second, points[12].second.Z(false), points[13].second,
									_T("1007"), _T("1008"), +1.0 * probe_radius, 0 );
			if (m_check_levels)
			{
				wxString setup_point_x;
				wxString setup_point_y;
				setup_point_x << _T("[ #1007 + ") << +1.0 * probe_radius << _T(" ]");
				setup_point_y << _T("[ #1008 ]");

				python << AppendTextForDownwardProbingOperation( setup_point_x, setup_point_y, m_depth, _T("1009") );
			} // End if - then

			python << AppendTextForSingleProbeOperation( points[15].second, points[16].second, points[17].second.Z(false), points[18].second,
									_T("1010"), _T("1011"), +1.0 * probe_radius, 0 );
			if (m_check_levels)
			{
				wxString setup_point_x;
				wxString setup_point_y;
				setup_point_x << _T("[ #1010 + ") << +1.0 * probe_radius << _T(" ]");
				setup_point_y << _T("[ #1011 ]");

				python << AppendTextForDownwardProbingOperation( setup_point_x, setup_point_y, m_depth, _T("1012") );
			} // End if - then
			break;

		case eTopRight:
			ref1 = gp_Vec(-1,0,0);
			ref2 = gp_Vec(0,-1,0);
			// Top
			python << AppendTextForSingleProbeOperation( points[0].second, points[1].second, points[2].second.Z(false), points[3].second,
									_T("1001"), _T("1002"), 0, -1.0 * probe_radius );
			if (m_check_levels)
			{
				wxString setup_point_x;
				wxString setup_point_y;
				setup_point_x << _T("[ #1001 ]");
				setup_point_y << _T("[ #1002 + ") << -1.0 * probe_radius << _T(" ]");

				python << AppendTextForDownwardProbingOperation( setup_point_x, setup_point_y, m_depth, _T("1003") );
			} // End if - then

			python << AppendTextForSingleProbeOperation( points[5].second, points[6].second, points[7].second.Z(false), points[8].second,
									_T("1004"), _T("1005"), 0, -1.0 * probe_radius );
			if (m_check_levels)
			{
				wxString setup_point_x;
				wxString setup_point_y;
				setup_point_x << _T("[ #1004 ]");
				setup_point_y << _T("[ #1005 + ") << -1.0 * probe_radius << _T(" ]");

				python << AppendTextForDownwardProbingOperation( setup_point_x, setup_point_y, m_depth, _T("1006") );
			} // End if - then

			// Right
			python << AppendTextForSingleProbeOperation( points[10].second, points[11].second, points[12].second.Z(false), points[13].second,
									_T("1007"), _T("1008"), -1.0 * probe_radius, 0 );
			if (m_check_levels)
			{
				wxString setup_point_x;
				wxString setup_point_y;
				setup_point_x << _T("[ #1007 + ") << -1.0 * probe_radius << _T(" ]");
				setup_point_y << _T("[ #1008 ]");

				python << AppendTextForDownwardProbingOperation( setup_point_x, setup_point_y, m_depth, _T("1009") );
			} // End if - then

			python << AppendTextForSingleProbeOperation( points[15].second, points[16].second, points[17].second.Z(false), points[18].second,
									_T("1010"), _T("1011"), -1.0 * probe_radius, 0 );
			if (m_check_levels)
			{
				wxString setup_point_x;
				wxString setup_point_y;
				setup_point_x << _T("[ #1010 + ") << -1.0 * probe_radius << _T(" ]");
				setup_point_y << _T("[ #1011 ]");

				python << AppendTextForDownwardProbingOperation( setup_point_x, setup_point_y, m_depth, _T("1012") );
			} // End if - then
			break;
		} // End switch

		// Now report the angle of rotation
		python << _T("report_probe_results( ")
				<< _T("x1='[#1001 + ") << probe_offset_x << _T("]', ")
				<< _T("y1='[#1002 + ") << probe_offset_y << _T("]', ")
				<< _T("x2='[#1004 + ") << probe_offset_x << _T("]', ")
				<< _T("y2='[#1005 + ") << probe_offset_y << _T("]', ")
				<< _T("x3='") << ref1.X() << _T("', ")
				<< _T("y3='") << ref1.Y() << _T("', ")
				<< _T("x4='[#1007 + ") << probe_offset_x << _T("]', ")
				<< _T("y4='[#1008 + ") << probe_offset_y << _T("]', ")
				<< _T("x5='[#1010 + ") << probe_offset_x << _T("]', ")
				<< _T("y5='[#1011 + ") << probe_offset_y << _T("]', ")
				<< _T("x6='") << ref2.X() << _T("', ")
				<< _T("y6='") << ref2.Y() << _T("', ");
		if (m_check_levels)
		{
			python << _T("z1='#1003', ");
			python << _T("z2='#1006', ");
			python << _T("z3='") << ref1.Z() << _T("', ");
			python << _T("z4='#1009', ");
			python << _T("z5='#1012', ");
			python << _T("z6='") << ref2.Z() << _T("', ");
		}

		python << _T("xml_file_name=") << PythonString(this->GetOutputFileName( _T(".xml"), true )) << _T("')\n");

		// And position the cutting tool at the intersection of the two lines.
		// This should be safe as the 'probe_single_point() call made in the AppendTextForSingleOperation() routine returns
		// the machine's position to the originally jogged position.  This is expected to be above the workpiece
		// at a same movement height.
		python << _T("set_temporary_origin( x=0, y=0, z=0 )\n");
		python << _T("rapid_to_intersection( ")
				<< _T("x1='[#1001 + ") << probe_offset_x << _T("]', ")
				<< _T("y1='[#1002 + ") << probe_offset_y << _T("]', ")
				<< _T("x2='[#1004 + ") << probe_offset_x << _T("]', ")
				<< _T("y2='[#1005 + ") << probe_offset_y << _T("]', ")
				<< _T("x3='[#1007 + ") << probe_offset_x << _T("]', ")
				<< _T("y3='[#1008 + ") << probe_offset_y << _T("]', ")
				<< _T("x4='[#1010 + ") << probe_offset_x << _T("]', ")
				<< _T("y4='[#1011 + ") << probe_offset_y << _T("]', ")
				<< _T("intersection_x='#1013', ")
				<< _T("intersection_y='#1014', ")
				<< _T("ua_numerator='#1015', ")
				<< _T("ua_denominator='#1016', ")
				<< _T("ub_numerator='#1017', ")
				<< _T("ua='#1018', ")
				<< _T("ub='#1019' )\n");
		python << _T("remove_temporary_origin()\n");

		// We're now sitting at the corner.  If this position represents the m_corner_coordinate
		// then we need to calculate where the m_final_coordinate lays (along these rotated axes)
		// and move there.

		python << _T("set_temporary_origin( x=0, y=0, z=0 )\n");
		python << _T("rapid_to_rotated_coordinate( ")
				<< _T("x1='[#1001 + ") << probe_offset_x << _T("]', ")
				<< _T("y1='[#1002 + ") << probe_offset_y << _T("]', ")
				<< _T("x2='[#1004 + ") << probe_offset_x << _T("]', ")
				<< _T("y2='[#1005 + ") << probe_offset_y << _T("]', ")
				<< _T("ref_x='") << ref1.X() << _T("', ")
				<< _T("ref_y='") << ref1.Y() << _T("', ")
				<< _T("x_current=") << m_corner_coordinate.X(true) << _T(", ")
				<< _T("y_current=") << m_corner_coordinate.Y(true) << _T(", ")
				<< _T("x_final=") << m_final_coordinate.X(true) << _T(", ")
				<< _T("y_final=") << m_final_coordinate.Y(true) << _T(")\n");
		python << _T("remove_temporary_origin()\n");

	} // End if - else

	return(python);
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

static void on_set_num_x_points(int value, HeeksObj* object)
{
	((CProbe_Grid*)object)->m_num_x_points = value;
	((CProbe_Grid*)object)->GenerateMeaningfullName();
	heeksCAD->Changed();
}

static void on_set_num_y_points(int value, HeeksObj* object)
{
	((CProbe_Grid*)object)->m_num_y_points = value;
	((CProbe_Grid*)object)->GenerateMeaningfullName();
	heeksCAD->Changed();
}

static void on_set_reason(int value, HeeksObj* object)
{
	((CProbe_Grid*)object)->m_for_fixture_measurement = (value != 0);
	((CProbe_Grid*)object)->GenerateMeaningfullName();
	heeksCAD->Changed();
}

void CProbe_Grid::GetProperties(std::list<Property *> *list)
{
    if (! m_for_fixture_measurement)
    {
        list->push_back(new PropertyInt(_("Num points along X"), m_num_x_points, this, on_set_num_x_points));
        list->push_back(new PropertyInt(_("Num points along Y"), m_num_y_points, this, on_set_num_y_points));
    }

    std::list< wxString > choices;
	choices.push_back ( wxString ( _("Point Cloud") ) );
	choices.push_back ( wxString ( _("Fixture Measurement") ) );
	list->push_back ( new PropertyChoice ( _("Point Data Format"),  choices, (m_for_fixture_measurement?1:0), this, on_set_reason ) );

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

static void on_set_check_levels(int zero_based_choice, HeeksObj* object)
{
	((CProbe_Edge*)object)->m_check_levels = zero_based_choice;
	((CProbe_Edge*)object)->GenerateMeaningfullName();
	heeksCAD->Changed();	// Force a re-draw from glCommands()
}

static void on_set_corner_coordinate(const double *vt, HeeksObj* object)
{
	((CProbe_Edge *)object)->m_corner_coordinate = CNCPoint(vt);
}

static void on_set_final_coordinate(const double *vt, HeeksObj* object)
{
	((CProbe_Edge *)object)->m_final_coordinate = CNCPoint(vt);
}

void CProbe_Edge::GetProperties(std::list<Property *> *list)
{
	list->push_back(new PropertyLength(_("Retract"), m_retract, this, on_set_retract));

	{ // Begin choice scope
		std::list< wxString > choices;

		choices.push_back(_("False"));
		choices.push_back(_("True"));

		int choice = m_check_levels;
		list->push_back(new PropertyChoice(_("Check Levels"), choices, choice, this, on_set_check_levels));
	} // End choice scope

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

		double corner_coordinate[3], final_coordinate[3];
		m_corner_coordinate.ToDoubleArray(corner_coordinate);
		m_final_coordinate.ToDoubleArray(final_coordinate);

		list->push_back(new PropertyVertex(_("Corner Coordinate"), corner_coordinate, this, on_set_corner_coordinate));
		list->push_back(new PropertyVertex(_("Final Coordinate"), final_coordinate, this, on_set_final_coordinate));
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

HeeksObj *CProbe_Grid::MakeACopy(void)const
{
	return new CProbe_Grid(*this);
}

void CProbe_Grid::CopyFrom(const HeeksObj* object)
{
	operator=(*((CProbe_Grid *)object));
}


bool CProbing::CanAddTo(HeeksObj* owner)
{
	return owner->GetType() == OperationsType;
}


CProbing::CProbing( const CProbing & rhs) : CSpeedOp(rhs)
{
	*this = rhs;	// Call the assignment operator.
}

CProbing & CProbing::operator= ( const CProbing & rhs )
{
	if (this != &rhs)
	{
		CSpeedOp::operator=( rhs );

		m_depth = rhs.m_depth;
		m_distance = rhs.m_distance;
	}

	return(*this);
}


CProbe_Centre::CProbe_Centre( const CProbe_Centre & rhs ) : CProbing(rhs)
{
	*this = rhs;	// Call the assignment operator.
}

CProbe_Centre & CProbe_Centre::operator= ( const CProbe_Centre & rhs )
{
	if (this != &rhs)
	{
		CProbing::operator =(rhs);

		m_direction = rhs.m_direction;
		m_number_of_points = rhs.m_number_of_points;
		m_alignment = rhs.m_alignment;
	}

	return(*this);
}

CProbe_Grid::CProbe_Grid( const CProbe_Grid & rhs ) : CProbing(rhs)
{
	*this = rhs;	// Call the assignment operator.
}

CProbe_Grid & CProbe_Grid::operator= ( const CProbe_Grid & rhs )
{
	if (this != &rhs)
	{
		CProbing::operator =(rhs);

		m_num_x_points = rhs.m_num_x_points;
		m_num_y_points = rhs.m_num_y_points;
		m_for_fixture_measurement = rhs.m_for_fixture_measurement;
	}

	return(*this);
}


CProbe_Edge::CProbe_Edge( const CProbe_Edge & rhs ) : CProbing(rhs)
{
	*this = rhs;	// Call the assignment operator.
}

CProbe_Edge & CProbe_Edge::operator= ( const CProbe_Edge & rhs )
{
	if (this != &rhs)
	{
		CProbing::operator =(rhs);

		m_retract = rhs.m_retract;
		m_number_of_edges = rhs.m_number_of_edges;
		m_edge = rhs.m_edge;
		m_corner = rhs.m_corner;
		m_check_levels = rhs.m_check_levels;

		m_corner_coordinate = rhs.m_corner_coordinate;
		m_final_coordinate = rhs.m_final_coordinate;
	}

	return(*this);
}



void CProbe_Edge::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element = new TiXmlElement( Ttc(GetTypeString()) );
	root->LinkEndChild( element );

	element->SetDoubleAttribute("retract", m_retract);
	element->SetAttribute("number_of_edges", m_number_of_edges);
	element->SetAttribute("edge", m_edge);
	element->SetAttribute("corner", m_corner);
	element->SetAttribute("check_levels", m_check_levels);
	element->SetDoubleAttribute("corner_coordinate_x", m_corner_coordinate.X());
	element->SetDoubleAttribute("corner_coordinate_y", m_corner_coordinate.Y());
	element->SetDoubleAttribute("corner_coordinate_z", m_corner_coordinate.Z());
	element->SetDoubleAttribute("final_coordinate_x", m_final_coordinate.X());
	element->SetDoubleAttribute("final_coordinate_y", m_final_coordinate.Y());
	element->SetDoubleAttribute("final_coordinate_z", m_final_coordinate.Z());

	WriteBaseXML(element);
}

// static member function
HeeksObj* CProbe_Edge::ReadFromXMLElement(TiXmlElement* element)
{
	CProbe_Edge* new_object = new CProbe_Edge();

	if (element->Attribute("retract")) new_object->m_retract = atof(element->Attribute("retract"));
	if (element->Attribute("number_of_edges")) new_object->m_number_of_edges = atoi(element->Attribute("number_of_edges"));
	if (element->Attribute("edge")) new_object->m_edge = CProbing::eEdges_t(atoi(element->Attribute("edge")));
	if (element->Attribute("corner")) new_object->m_corner = CProbing::eCorners_t(atoi(element->Attribute("corner")));
	if (element->Attribute("check_levels")) new_object->m_check_levels = atoi(element->Attribute("check_levels"));

	if (element->Attribute("corner_coordinate_x"))
		new_object->m_corner_coordinate.SetX(atof(element->Attribute("corner_coordinate_x")));
	if (element->Attribute("corner_coordinate_y"))
		new_object->m_corner_coordinate.SetY(atof(element->Attribute("corner_coordinate_y")));
	if (element->Attribute("corner_coordinate_z"))
		new_object->m_corner_coordinate.SetZ(atof(element->Attribute("corner_coordinate_z")));

	if (element->Attribute("final_coordinate_x"))
		new_object->m_final_coordinate.SetX(atof(element->Attribute("final_coordinate_x")));
	if (element->Attribute("final_coordinate_y"))
		new_object->m_final_coordinate.SetY(atof(element->Attribute("final_coordinate_y")));
	if (element->Attribute("final_coordinate_z"))
		new_object->m_final_coordinate.SetZ(atof(element->Attribute("final_coordinate_z")));

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

void CProbe_Grid::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element = new TiXmlElement( Ttc(GetTypeString()) );
	root->LinkEndChild( element );

	element->SetAttribute("num_x_points", m_num_x_points);
	element->SetAttribute("num_y_points", m_num_y_points);
	element->SetAttribute("for_fixture_measurement", (m_for_fixture_measurement?1:0));

	WriteBaseXML(element);
}

// static member function
HeeksObj* CProbe_Grid::ReadFromXMLElement(TiXmlElement* element)
{
	CProbe_Grid* new_object = new CProbe_Grid();

	if (element->Attribute("num_x_points")) new_object->m_num_x_points = atoi(element->Attribute("num_x_points"));
	if (element->Attribute("num_y_points")) new_object->m_num_y_points = atoi(element->Attribute("num_y_points"));
	if (element->Attribute("for_fixture_measurement")) new_object->m_for_fixture_measurement = (atoi(element->Attribute("for_fixture_measurement")) != 0);

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


void CProbe_Grid::glCommands(bool select, bool marked, bool no_color)
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

	while (file_name.Find(_T(" ")) != -1) file_name.Replace(_T(" "), _T("_"));

	return(file_name);
}


Python CProbing::GeneratePythonPreamble()
{
	Python python;

	// We must setup the theApp.m_program_canvas->m_textCtrl variable before
		// calling the HeeksPyPostProcess() routine.  That's the python script
		// that will be executed.

		theApp.m_program_canvas->m_textCtrl->Clear();

		// add standard stuff at the top
		//hackhack, make it work on unix with FHS
        python << _T("import sys\n");

    #ifndef WIN32
        python << _T("sys.path.insert(0,'/usr/local/lib/heekscnc/')\n");
    #endif

    python << _T("sys.path.insert(0,") << PythonString(theApp.GetDllFolder()) << _T(")\n");
    python << _T("import math\n");


	// machine general stuff
	python << _T("from nc.nc import *\n");

	// specific machine
	if (theApp.m_program->m_machine.file_name == _T("not found"))
	{
		wxMessageBox(_T("Machine name (defined in Program Properties) not found"));
	} // End if - then
	else
	{
		python << _T("import nc.") << theApp.m_program->m_machine.file_name << _T("\n");
		python << _T("\n");
	} // End if - else

    {
        // output file
        python << _T("output(") << PythonString( this->GetOutputFileName(_T(".tap"), false)).c_str() << _T(")\n");
    }

	// begin program
	python << _T("program_begin(123, 'Test program')\n");
	python << _T("absolute()\n");

	if(theApp.m_program->m_units > 25.0)
	{
		python << _T("imperial()\n");
	}
	else
	{
		python << _T("metric()\n");
	}
	python << _T("set_plane(0)\n");
	python << _T("\n");

	return(python);
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
		Python python;

		theApp.m_program_canvas->m_textCtrl->Clear();

		python << m_pThis->GeneratePythonPreamble();

		CFixture default_fixture(NULL, CFixture::G54 );
		python << m_pThis->AppendTextToProgram( &default_fixture );

		python << _T("program_end()\n");
		theApp.m_program->m_python_program = python;

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

	CProbing::GetTools( t_list, p );
}

class Probe_Grid_GenerateGCode: public Tool
{

CProbe_Grid *m_pThis;

public:
	Probe_Grid_GenerateGCode() { m_pThis = NULL; }

	// Tool's virtual functions
	const wxChar* GetTitle(){return _("Generate GCode");}

	void Run()
	{
		// We must setup the theApp.m_program_canvas->m_textCtrl variable before
		// calling the HeeksPyPostProcess() routine.  That's the python script
		// that will be executed.

		Python python;

		theApp.m_program_canvas->m_textCtrl->Clear();

		python << m_pThis->GeneratePythonPreamble();

		CFixture default_fixture(NULL, CFixture::G54 );
		python << m_pThis->AppendTextToProgram( &default_fixture );

		python << _T("program_end()\n");
		theApp.m_program->m_python_program = python;

		{
			// clear the output file
			wxFile f(m_pThis->GetOutputFileName(_T(".tap"), false).c_str(), wxFile::write);
			if(f.IsOpened())f.Write(_T("\n"));
		}

		HeeksPyPostProcess(theApp.m_program, m_pThis->GetOutputFileName(_T(".tap"), false), false );
	}
	wxString BitmapPath(){ return _T("export");}
	wxString previous_path;
	void Set( CProbe_Grid *pThis ) { m_pThis = pThis; }
};

static Probe_Grid_GenerateGCode generate_grid_gcode;

void CProbe_Grid::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{

	generate_grid_gcode.Set( this );

	t_list->push_back( &generate_grid_gcode );

	CProbing::GetTools( t_list, p );
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
		Python python;

		theApp.m_program_canvas->m_textCtrl->Clear();

		python << m_pThis->GeneratePythonPreamble();

		CFixture default_fixture(NULL, CFixture::G54 );
		python << m_pThis->AppendTextToProgram( &default_fixture );

		python << _T("program_end()\n");
		theApp.m_program->m_python_program = python;

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

	CProbing::GetTools( t_list, p );
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


CProbing::PointsList_t CProbe_Grid::GetPoints() const
{
	CProbing::PointsList_t points;

    if (m_for_fixture_measurement)
    {
        points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(0, 0, 0) ) );
        points.push_back( std::make_pair( CProbing::eProbe, CNCPoint(0, 0, -1.0 * m_depth) ) );
        points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(0, 0, 0) ) );

        points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(m_distance, 0, 0) ) );
        points.push_back( std::make_pair( CProbing::eProbe, CNCPoint(m_distance, 0, -1.0 * m_depth) ) );
        points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(m_distance, 0, 0) ) );

        points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(0, m_distance, 0) ) );
        points.push_back( std::make_pair( CProbing::eProbe, CNCPoint(0, m_distance, -1.0 * m_depth) ) );
        points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(0, m_distance, 0) ) );
    }
    else
    {
        for (int x_hole = 0; x_hole < m_num_x_points; x_hole++)
        {
            for (int y_hole = 0; y_hole < m_num_y_points; y_hole++)
            {
                points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(x_hole * m_distance, y_hole * m_distance, 0) ) );
                points.push_back( std::make_pair( CProbing::eProbe, CNCPoint(x_hole * m_distance, y_hole * m_distance, -1.0 * m_depth) ) );
                points.push_back( std::make_pair( CProbing::eRapid, CNCPoint(x_hole * m_distance, y_hole * m_distance, 0) ) );
            }
        }
    }

	points.push_back( std::make_pair( CProbing::eEndOfData, CNCPoint(0,0,0) ) );

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

	m_title << _T(" at ") << CCuttingTool::FractionalRepresentation( m_distance / theApp.m_program->m_units, 64 );
	if (theApp.m_program->m_units > 1) m_title << _T(" inch intervals");
	else m_title << _T(" mm intervals");

}

void CProbe_Centre::GenerateMeaningfullName()
{
	if (m_direction == eOutside)
	{
		m_title = _("Probe protrusion along ");
		m_title << eAlignment_t(m_alignment);
		m_title << _T(" min ") << CCuttingTool::FractionalRepresentation( m_distance / theApp.m_program->m_units, 64 );
		if (theApp.m_program->m_units > 1) m_title << _T(" inches");
		else m_title << _T(" mm");
	} // End if - then
	else
	{
		m_title = _("Probe hole along ");
		m_title << eAlignment_t(m_alignment);
		m_title << _T(" max ") << CCuttingTool::FractionalRepresentation( m_distance / theApp.m_program->m_units, 64 );
		if (theApp.m_program->m_units > 1) m_title << _T(" inches");
		else m_title << _T(" mm");
	} // End if - else

	if (m_number_of_points == 4)
	{
		if (m_direction == eOutside)
		{
			m_title = _("Probe protrusion");
			m_title << _T(" min ") << CCuttingTool::FractionalRepresentation( m_distance / theApp.m_program->m_units, 64 );
			if (theApp.m_program->m_units > 1) m_title << _T(" inches");
			else m_title << _T(" mm");
		} // End if - then
		else
		{
			m_title = _("Probe hole");
			m_title << _T(" max ") << CCuttingTool::FractionalRepresentation( m_distance / theApp.m_program->m_units, 64 );
			if (theApp.m_program->m_units > 1) m_title << _T(" inches");
			else m_title << _T(" mm");
		} // End if - else
	} // End if - then
}

void CProbe_Grid::GenerateMeaningfullName()
{
    if (m_for_fixture_measurement)
    {
        m_title = _("Probe Fixture Tilt");
    }
    else
    {
        m_title = _("Probe ");
        m_title << this->m_num_x_points << _T(" x ") << m_num_y_points << _T(" ") << _("grid");
    }
}

void CProbing::GenerateMeaningfullName()
{
    switch(m_operation_type)
    {
        case ProbeGridType:
            ((CProbe_Grid*)this)->GenerateMeaningfullName();
            break;

        case ProbeEdgeType:
            ((CProbe_Edge*)this)->GenerateMeaningfullName();
            break;

        case ProbeCentreType:
            ((CProbe_Centre*)this)->GenerateMeaningfullName();
            break;
    }
}



void CProbing::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
    CSpeedOp::GetTools( t_list, p );
}

void CProbe_Edge::OnChangeUnits(const double units)
{
    GenerateMeaningfullName();
}

void CProbe_Centre::OnChangeUnits(const double units)
{
    GenerateMeaningfullName();
}

void CProbe_Grid::OnChangeUnits(const double units)
{
    GenerateMeaningfullName();
}
