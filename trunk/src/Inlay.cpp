// Inlay.cpp
/*
 * Copyright (c) 2009, Dan Heeks, Perttu Ahola
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "stdafx.h"
#include "Inlay.h"
#include "CNCConfig.h"
#include "ProgramCanvas.h"
#include "interface/HeeksObj.h"
#include "interface/ObjList.h"
#include "interface/PropertyInt.h"
#include "interface/PropertyDouble.h"
#include "interface/PropertyLength.h"
#include "interface/PropertyChoice.h"
#include "tinyxml/tinyxml.h"
#include "Operations.h"
#include "CuttingTool.h"
#include "Profile.h"
#include "Fixture.h"
#include "CNCPoint.h"
#include "PythonStuff.h"
#include "Contour.h"
#include "Pocket.h"
#include "MachineState.h"

#include <sstream>
#include <iomanip>
#include <vector>
#include <algorithm>

#include <BRepOffsetAPI_MakeOffset.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Shape.hxx>
#include <BRepTools_WireExplorer.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <gp_Circ.hxx>
#include <ShapeFix_Wire.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRep_Tool.hxx>
#include <BRepTools.hxx>
#include <BRepMesh.hxx>
#include <Poly_Polygon3D.hxx>
#include <gp_Lin.hxx>

extern CHeeksCADInterface* heeksCAD;

/* static */ double CInlay::max_deviation_for_spline_to_arc = 0.1;

void CInlayParams::set_initial_values()
{
	CNCConfig config(ConfigPrefix());
	config.Read(_T("BorderWidth"), (double *) &m_border_width, 0.0);
	config.Read(_T("ClearanceTool"), &m_clearance_tool, 0);
	config.Read(_T("Pass"), (int *) &m_pass, (int) eBoth );
	config.Read(_T("MirrorAxis"), (int *) &m_mirror_axis, (int) eXAxis );
	config.Read(_T("MinCorneringAngle"), (double *) &m_min_cornering_angle, 135.0);

}

void CInlayParams::write_values_to_config()
{
	CNCConfig config(ConfigPrefix());
	config.Write(_T("BorderWidth"), m_border_width);
	config.Write(_T("ClearanceTool"), m_clearance_tool);
	config.Write(_T("Pass"), (int) m_pass );
	config.Write(_T("MirrorAxis"), (int) m_mirror_axis );
	config.Write(_T("MinCorneringAngle"), m_min_cornering_angle);
}

static void on_set_border_width(double value, HeeksObj* object)
{
	((CInlay*)object)->m_params.m_border_width = value;
	((CInlay*)object)->WriteDefaultValues();
}

static void on_set_min_cornering_angle(double value, HeeksObj* object)
{
	((CInlay*)object)->m_params.m_min_cornering_angle = value;
	((CInlay*)object)->WriteDefaultValues();
}

static void on_set_clearance_tool(int value, HeeksObj* object)
{
	((CInlay*)object)->m_params.m_clearance_tool = value;
	((CInlay*)object)->WriteDefaultValues();
}

static void on_set_pass(int value, HeeksObj* object)
{
	((CInlay*)object)->m_params.m_pass = CInlayParams::eInlayPass_t(value);
	((CInlay*)object)->WriteDefaultValues();
}

static void on_set_mirror_axis(int value, HeeksObj* object)
{
	((CInlay*)object)->m_params.m_mirror_axis = CInlayParams::eAxis_t(value);
	((CInlay*)object)->WriteDefaultValues();
}



static void on_set_female_fixture(int value, HeeksObj* object)
{
	if (value == 0)
	{
		((CInlay*)object)->m_params.m_female_before_male_fixtures = true;
	}
	else
	{
		((CInlay*)object)->m_params.m_female_before_male_fixtures = false;
	}

	((CInlay*)object)->WriteDefaultValues();
	heeksCAD->Changed();
}

static void on_set_male_fixture(int value, HeeksObj* object)
{
	if (value == 0)
	{
		((CInlay*)object)->m_params.m_female_before_male_fixtures = false;
	}
	else
	{
		((CInlay*)object)->m_params.m_female_before_male_fixtures = true;
	}

	((CInlay*)object)->WriteDefaultValues();
	heeksCAD->Changed();
}


void CInlayParams::GetProperties(CInlay* parent, std::list<Property *> *list)
{
    list->push_back(new PropertyLength(_("Border Width"), m_border_width, parent, on_set_border_width));

    {
		std::vector< std::pair< int, wxString > > tools = CCuttingTool::FindAllCuttingTools();

		int choice = 0;
        std::list< wxString > choices;
		for (std::vector< std::pair< int, wxString > >::size_type i=0; i<tools.size(); i++)
		{
                	choices.push_back(tools[i].second);

			if (m_clearance_tool == tools[i].first)
			{
                		choice = int(i);
			} // End if - then
		} // End for

		list->push_back(new PropertyChoice(_("Clearance Tool"), choices, choice, parent, on_set_clearance_tool));
	}

	{
	    // Note: these options MUST be in the same order as they are defined in the enum.
		int choice = (int) m_pass;
        std::list< wxString > choices;

		choices.push_back(_("Female half"));
		choices.push_back(_("Male half"));
		choices.push_back(_("Both halves"));

		list->push_back(new PropertyChoice(_("GCode generation"), choices, choice, parent, on_set_pass));
	}

	{
		int choice = (int) m_mirror_axis;
        std::list< wxString > choices;

		choices.push_back(_("X Axis"));
		choices.push_back(_("Y Axis"));

		list->push_back(new PropertyChoice(_("Mirror Axis"), choices, choice, parent, on_set_mirror_axis));
	}

	std::list<CFixture> fixtures = parent->PrivateFixtures();
	if (fixtures.size() == 2)
	{
		// The user has defined two private fixtures.  Add parameters asking which is to be used for
		// the female half and which for the male half.

		{
			int female_choice = 0;
			int male_choice = 1;

			std::list<wxString> choices;
			for (std::list<CFixture>::iterator itFixture = fixtures.begin(); itFixture != fixtures.end(); itFixture++)
			{
				choices.push_back(itFixture->m_title);
			}

			if (m_female_before_male_fixtures)
			{
				female_choice = 0;
				male_choice = 1;
			}
			else
			{
				female_choice = 1;
				male_choice = 0;
			}

			list->push_back(new PropertyChoice(_("Female Op Fixture"), choices, female_choice, parent, on_set_female_fixture));
			list->push_back(new PropertyChoice(_("Male Op Fixture"), choices, male_choice, parent, on_set_male_fixture));
		}
	}

	list->push_back(new PropertyLength(_("Min Cornering Angle (degrees)"), m_min_cornering_angle, parent, on_set_min_cornering_angle));
}

void CInlayParams::WriteXMLAttributes(TiXmlNode *root)
{
	TiXmlElement * element;
	element = new TiXmlElement( "params" );
	root->LinkEndChild( element );

	element->SetAttribute("border", m_border_width);
	element->SetAttribute("clearance_tool", m_clearance_tool);
	element->SetAttribute("pass", (int) m_pass);
	element->SetAttribute("mirror_axis", (int) m_mirror_axis);
	element->SetAttribute("female_before_male_fixtures", (int) (m_female_before_male_fixtures?1:0));
	element->SetAttribute("min_cornering_angle", m_min_cornering_angle);
}

void CInlayParams::ReadParametersFromXMLElement(TiXmlElement* pElem)
{
	pElem->Attribute("border", &m_border_width);
	pElem->Attribute("clearance_tool", &m_clearance_tool);
	pElem->Attribute("pass", (int *) &m_pass);
	pElem->Attribute("mirror_axis", (int *) &m_mirror_axis);

	int temp;
	pElem->Attribute("female_before_male_fixtures", (int *) &temp);
	m_female_before_male_fixtures = (temp != 0);

	if (pElem->Attribute("min_cornering_angle")) pElem->Attribute("min_cornering_angle", &m_min_cornering_angle);
}



const wxBitmap &CInlay::GetIcon()
{
	if(!m_active)return GetInactiveIcon();
	static wxBitmap* icon = NULL;
	if(icon == NULL)icon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/drilling.png")));
	return *icon;
}


/**
    This routine performs two separate functions.  The first is to produce the GCode requied to
    drive the chamfering bit around the material to produce both the male and female halves
    of the inlay operation.  The second function is to generate a set of mirrored sketch
    objects along with their pocketing operations.  These pocket operations are required when
    we generate the male half of the inlay.  We need to machine some material down so that, when
    it is turned upside-down and placed on top of the female piece, the two halves line up
    correctly.  The idea is that these two halves will be glued together and, when the glue
    is dry, the remainder of the male half will be machined off leaving just those sections
    that remain within the female half.

    All this means that the peaks of the male half need to be no higher than the valleys in
    the female half.  This is done on a per-sketch basis because the combination of the
    sketche's geometry and the chamfering tool's geometry means that each sketch may produce
    a pocket whose depth is less than the inlay operation's nominated depth.  In these cases
    we need to create a pocket operation that is bounded by this sketch but will remove most
    of the material directly above the sketch down to the depth that corresponds to the
    depth of the pocket in the female half.

    The other pocket that is produced is a combination of a square border sketch and the
    mirrored versions of all the selected sketches.  This module ensures that the border
    sketch is oriented clockwise and all the internal sketches are oriented counter-clockwise.
    This will allow the pocket to remove all the material between the selected sketches
    down to a height that will mate with the top-most surface of the female half.

    The border is generated based on the bounding box of all the selected sketches as
    well as the border width found in the InlayParams object.

    The two functions of this method are enabled by the 'keep_mirrored_sketches' flag.  When
    this flag is true, the extra mirrored sketches and their corresponding pocket operations
    are generated and added to the data model.  This occurs when the right mouse menu option
    (generate male pocket) is manually chosen.  When the normal GCode generation process
    occurs, the 'keep_mirrored_sketches' flag is false so that only the female, male or
    both halves are generated.
 */

Python CInlay::AppendTextToProgram( CMachineState *pMachineState )
{
    Python python;

	ReloadPointers();

	python << CDepthOp::AppendTextToProgram( pMachineState );

	CCuttingTool *pChamferingBit = (CCuttingTool *) heeksCAD->GetIDObject( CuttingToolType, m_cutting_tool_number );

	if (! pChamferingBit)
	{
	    // No shirt, no shoes, no service.
		return(python);
	}

	Valleys_t valleys = DefineValleys(pMachineState);


	// NOTE: These valleys are NOT aligned with the fixtures yet.  This needs to be done
	// individually for each of the different fixtures later on.

	switch (m_params.m_pass)
	{
	case CInlayParams::eBoth:
		python << FormValleyWalls(valleys, pMachineState).c_str();				// chamfer
		python << FormMountainWalls(valleys, pMachineState).c_str();			// chamfer
		python << FormValleyPockets(valleys, pMachineState).c_str();			// clearance
		python << FormMountainPockets(valleys, pMachineState, true).c_str();	// clearance
		python << FormMountainPockets(valleys, pMachineState, false).c_str();	// clearance

		break;

	case CInlayParams::eFemale:
		python << FormValleyWalls(valleys, pMachineState).c_str();				// chamfer
		python << FormValleyPockets(valleys, pMachineState).c_str();			// clearance
		break;

	case CInlayParams::eMale:
        python << FormMountainWalls(valleys, pMachineState).c_str();			// chamfer
		python << FormMountainPockets(valleys, pMachineState, true).c_str();	// clearance
		python << FormMountainPockets(valleys, pMachineState, false).c_str();	// clearance

		break;
	} // End switch

	return(python);
}


Python CInlay::SelectFixture( CMachineState *pMachineState, const bool female_half )
{
	Python python;

	std::list<CFixture> fixtures = PrivateFixtures();
	switch (fixtures.size())
	{
	case 2:
		if (female_half)
		{
			if (m_params.m_female_before_male_fixtures)
			{
				// Select the first of the two fixtues.
				python << pMachineState->Fixture(*(fixtures.begin()));
			}
			else
			{
				// Select the second of the two fixtures.
				python << pMachineState->Fixture(*(fixtures.rbegin()));
			}
		} // End if - then
		else
		{
			// We're doing the male half.
			if (m_params.m_female_before_male_fixtures)
			{
				// Select the second of the two fixtues.
				python << pMachineState->Fixture(*(fixtures.rbegin()));
			}
			else
			{
				// Select the first of the two fixtures.
				python << pMachineState->Fixture(*(fixtures.begin()));
			}
		} // End if - else
		break;

	case 1:
		python << pMachineState->Fixture(*fixtures.begin());
		break;

	default:
		// No private fixtures have been defined.  Just let the normal fixture
		// selection processing occur.  Nothing to do here.
		break;
	} // End if - then

	return(python);
} // End SelectFixture() method



/**
    This method finds the maximum offset possible for this wire up to the value specified.
 */
double CInlay::FindMaxOffset( const double max_offset_required, TopoDS_Wire wire, const double tolerance ) const
{
    // We will do a 'binary chop' algorithm to minimise the number of offsets we need to
    // calculate.
    double min_offset = 0.0;        // This value will work.
    double max_offset = max_offset_required;     // This value 'may' work.

    try {
        BRepOffsetAPI_MakeOffset offset_wire(wire);
        offset_wire.Perform(max_offset * -1.0);
        if (offset_wire.IsDone())
        {
            TopoDS_Wire extract_the_wire_to_make_sure = TopoDS::Wire(offset_wire.Shape());

            // The maximum offset is possible.
            return(max_offset);
        }
    } // End try
    catch (Standard_Failure & error) {
        (void) error;	// Avoid the compiler warning.
        Handle_Standard_Failure e = Standard_Failure::Caught();
    } // End catch

    // At this point we know that the min_offset will work and the max_offset
    // will not work.  Try half way between and repeat until we're splitting hairs.

    double offset = ((max_offset - min_offset) / 2.0) + min_offset;
    while ((offset > tolerance) && ((max_offset - min_offset) > tolerance))
    {
        try {
            BRepOffsetAPI_MakeOffset offset_wire(TopoDS::Wire(wire));
            offset_wire.Perform(offset * -1.0);
            if (! offset_wire.IsDone())
            {
                // It never seems to get here but it sounded nice anyway.
                // This offset did not work.  Try moving towards min_offset;
                max_offset = offset;
                offset = ((offset - min_offset) / 2.0) + min_offset;
            }
            else
            {
                // If we don't consult the result (by calling the Shape() method), it never
                // tells us if it actually worked or not.
                TopoDS_Wire extract_the_wire_to_make_sure = TopoDS::Wire(offset_wire.Shape());

                // The offset shape was generated fine.
                min_offset = offset;
                offset = ((max_offset - offset) / 2.0) + offset;
            }
        } // End try
        catch (Standard_Failure & error) {
            (void) error;	// Avoid the compiler warning.
            Handle_Standard_Failure e = Standard_Failure::Caught();
            // This offset did not work.  Try moving towards min_offset;
            max_offset = offset;
            offset = ((offset - min_offset) / 2.0) + min_offset;
        } // End catch
    } // End while

    return(offset);
}


/**
	This method finds the angle that the corner-forming vector would form.  It's 180 degrees
	from the mid-angle between the two CNCVectors passed in.
 */
double CInlay::CornerAngle( const std::set<CNCVector> _vectors ) const
{
	if (_vectors.size() == 2)
	{
		gp_Vec reference(0,0,-1);
		std::vector<CNCVector> vectors;
		std::copy( _vectors.begin(), _vectors.end(), std::inserter( vectors, vectors.begin() ) );

		// We should be able to project a vector at half the angle between these two vectors (plus 180 degrees)
		// and up at the angle of the cutting tool.  We should find a known coordinate in this direction (in fact
		// there may be many).  This vector is the toolpath we want to follow to sharpen the corners.

		double angle1 = vectors[0].AngleWithRef( gp_Vec(1,0,0), reference );
		double angle2 = vectors[1].AngleWithRef( gp_Vec(1,0,0), reference );

		while (angle1 < 0) angle1 += (2.0 * PI);
		while (angle2 < 0) angle2 += (2.0 * PI);

		double mid_angle;
		if (angle1 < angle2)
		{
			mid_angle = ((angle2 - angle1) / 2.0) + angle1;
			if ((angle2 - angle1) > PI) mid_angle += PI;
		}
		else
		{
			mid_angle = ((angle1 - angle2) / 2.0) + angle2;
			if ((angle1 - angle2) > PI) mid_angle += PI;
		}

		// At this point mid_angle points back towards the centre of the shape half way
		// between the two edges at this point.  We actually want to look back towards
		// a larger shape so add PI to this mid_angle to point back out away from the
		// middle.

		mid_angle += PI;
		while (mid_angle > (2.0 * PI)) mid_angle -= (2.0 * PI);

		return(mid_angle);
	} // End if - then

	return(0.0);

} // End CornerAngle() method


/**
    Find the two vectors associated with this coordinate.  These represent the two edges joining at
    this coordinate.  If we draw a line at the mid-angle between these two vectors and then we rotate
    that line down at the cutting tool's angle then we will be able to find other corners that are
    made below this one.
 */

CInlay::Corners_t CInlay::FindSimilarCorners( const CNCPoint coordinate, CInlay::Corners_t corners, const CCuttingTool *pChamferingBit ) const
{
	/*
	// Test cases.
	{
		std::set<CNCVector> vs;
		vs.insert( gp_Vec(1,0,0) ); vs.insert( gp_Vec(0,1,0) );
		double angle = CornerAngle( vs ) / (2.0 * PI) * 360.0;
		if (fabs(angle - (45.0 + 180.0)) > tolerance)
		{
			bool badthings = true;
		}
	}

	{
		std::set<CNCVector> vs;
		vs.insert( gp_Vec(-1,0,0) ); vs.insert( gp_Vec(0,1,0) );
		double angle = CornerAngle( vs ) / (2.0 * PI) * 360.0;
		if (fabs(angle - (135.0 + 180.0)) > tolerance)
		{
			bool badthings = true;
		}
	}

	{
		std::set<CNCVector> vs;
		vs.insert( gp_Vec(1,0,0) ); vs.insert( gp_Vec(0,-1,0) );
		double angle = CornerAngle( vs ) / (2.0 * PI) * 360.0;
		if (fabs(angle - 135.0) > tolerance)
		{
			bool badthings = true;
		}
	}

	{
		std::set<CNCVector> vs;
		vs.insert( gp_Vec(-1,0,0) ); vs.insert( gp_Vec(0,-1,0) );
		double angle = CornerAngle( vs ) / (2.0 * PI) * 360.0;
		if (fabs(angle - 45.0) > tolerance)
		{
			bool badthings = true;
		}
	}
	*/

	Corners_t results;
	typedef std::map<CDouble, CNCPoint > ClosestVertices_t;
	ClosestVertices_t closest_vertices;

	if (corners.find(coordinate) == corners.end())
	{
		return(results);	// Empty set.
	}

	// Get a distinct set of depth values from all the valley lines.
	for (Corners_t::iterator itCorner = corners.begin(); itCorner != corners.end(); itCorner++)
	{
		closest_vertices.insert(std::make_pair(itCorner->first.Z(), itCorner->first));
	} // End for

	if (corners[coordinate].size() == 2)
	{
		double reference_angle = CornerAngle(corners[coordinate]);

		// This reference_angle is the angle of the line coming from the corner half way
		// between the two connected edges.  i.e. it bisects the angle formed by the
		// two edges.
		//
		// Form a line that extends along the positive X axis (to start with)
		gp_Pnt endpoint(50,0,0); // Along the X axis.

		// Rotate that line down (around the Y axis) so that it aligns with the cutting edge
		// of the chamfering bit.
		gp_Trsf rotate_to_match_cutting_tool;
		rotate_to_match_cutting_tool.SetRotation( gp_Ax1(gp_Pnt(0,0,0), gp_Dir(0,-1,0)), (-90.0 - pChamferingBit->m_params.m_cutting_edge_angle) / 360.0 * 2.0 * PI );
		endpoint.Transform(rotate_to_match_cutting_tool);

		// Now rotate the line around so that it aligns with the bisecting angle between
		// the two connected edges.
		gp_Trsf rotate;
		rotate.SetRotation( gp_Ax1(gp_Pnt(0,0,0), gp_Dir(0,0,1)), reference_angle );
		endpoint.Transform(rotate);

		// We have been doing these rotations around the origin so far.  Translate this line out
		// to this particular corner point.
		gp_Trsf translate;
		translate.SetTranslation(gp_Pnt(0,0,0), coordinate);
		endpoint.Transform(translate);

		// This should now be close to pointing to where the
		// toolpath wires have their vertices.  It won't be exactly through their vertices
		// due to the fact that the cutting tool won't be able to get right into the top-most
		// corner due to its circular shape.  To allow for this, we will find the closes
		// vertex to this line at each level of depth.  These, together, will form the
		// toolpath required to sharpen the edges.

		// Draw a line in the graphics file for DEBUG purposes ONLY.
		double start[3], end[3];
		CNCPoint s(coordinate); s.ToDoubleArray(start);
		CNCPoint e(endpoint); e.ToDoubleArray(end);
		// heeksCAD->Add(heeksCAD->NewLine(start,end), NULL);

		// The endpoint now represents a vector (from the origin) that points half way between
		// the two edge angles as well as down at the cutting tool's angle.  Find the closest
		// points to this line at each of the valley's depths.  These points will form the
		// toolpath we need.

		gp_Lin cutting_line(s, gp_Dir(gp_Vec(s, e)));

		// Now see how far the other coordinates are away from this line.
		for (Corners_t::iterator itCorner = corners.begin(); itCorner != corners.end(); itCorner++)
		{
			CNCPoint coordinate(itCorner->first);
			double distance = cutting_line.SquareDistance(itCorner->first);

			CNCPoint previous_coordinate(closest_vertices[itCorner->first.Z()]);
			double previous_distance = cutting_line.SquareDistance(previous_coordinate);

			if (distance < previous_distance)
			{
				closest_vertices[CDouble(itCorner->first.Z())] = itCorner->first;
			}
		} // End for

		for (ClosestVertices_t::iterator itClosestVertex = closest_vertices.begin(); itClosestVertex != closest_vertices.end();
				itClosestVertex++)
		{
			results.insert(std::make_pair(itClosestVertex->second, corners[itClosestVertex->second]));
		}
	} // End if - then

	return(results);
}


/**
	We don't want to add the corner-shapenning toolpaths for every intersection of every
	edge.  If the two edges form a sharp corner then we need to do the work but if they
	lay along mostly the same direction then our chamfering bit will be able to run
	between the two edges without further clean-out work required.  This routine uses
	the angles of the two edges to decide whether the cornering work is required.
 */
bool CInlay::CornerNeedsSharpenning(Corners_t::iterator itCorner) const
{
	gp_Vec reference(0,0,-1);
	std::vector<CNCVector> vectors;
	std::copy( itCorner->second.begin(), itCorner->second.end(), std::inserter( vectors, vectors.begin() ) );

	if (vectors.size() != 2)
	{
		// There are not exactly two edges coming into this point.  We can't make
		// a decision based on this geometry.
		return(false);
	}

    double min_cornering_angle_in_radians = (m_params.m_min_cornering_angle / 360.0) * (2.0 * PI);

    double angle1 = vectors[0].AngleWithRef( gp_Vec(1,0,0), reference );
	double angle2 = vectors[1].AngleWithRef( gp_Vec(1,0,0), reference );

	while (angle1 < 0) angle1 += (2.0 * PI);
	while (angle2 < 0) angle2 += (2.0 * PI);

    if (angle1 < angle2)
    {
        double angle = angle2 - angle1;
        if (angle > PI) angle -= PI;
        return(angle < min_cornering_angle_in_radians);
    }
    else
    {
        double angle = angle1 - angle2;
        if (angle > PI) angle -= PI;
        return(angle < min_cornering_angle_in_radians);
    }

} // End of CornerNeedsSharpenning() method


/**
    We need to move from the bottom-most wire to the corresponding corners
    of each wire above it.  This will sharpen the concave corners formed
    between adjacent edges.
 */
Python CInlay::FormCorners( Valley_t & paths, CMachineState *pMachineState ) const
{
	Python python;

	python << pMachineState->CuttingTool(m_cutting_tool_number);	// Select the chamfering bit.

    // Gather a list of all corner coordinates and the angles formed there for each wire.
	Corners_t corners;
    typedef std::set<CNCPoint> Coordinates_t;
    typedef std::vector<CNCPoint> SortedCoordinates_t;
    Coordinates_t coordinates;
    double highest = 0.0;
    double lowest = 0.0;

    for (Valley_t::iterator itPath = paths.begin(); itPath != paths.end(); itPath++)
    {
        for(BRepTools_WireExplorer expEdge(itPath->Wire()); expEdge.More(); expEdge.Next())
        {
            BRepAdaptor_Curve curve(TopoDS_Edge(expEdge.Current()));

            double uStart = curve.FirstParameter();
            double uEnd = curve.LastParameter();
            gp_Pnt PS;
            gp_Vec VS;
            curve.D1(uStart, PS, VS);
            gp_Pnt PE;
            gp_Vec VE;
            curve.D1(uEnd, PE, VE);

			// The vectors indicate the direction from the start of the curve to the end.  We want
			// them to always be with respect to their corresponding point.  To this end, reverse
			// the vector at the end so that it starts at the endpoint and points back towards
			// the curve.  This way, all the points and their vectors are consistent.

			VE *= -1.0;

			if (corners.find(CNCPoint(PS)) == corners.end())
			{
				std::set<CNCVector> start_set; start_set.insert( VS );
				corners.insert( std::make_pair( CNCPoint(PS), start_set ));
			}
			else
			{
				corners[CNCPoint(PS)].insert( VS );
			}

			if (corners.find(CNCPoint(PE)) == corners.end())
			{
				std::set<CNCVector> end_set; end_set.insert( VE );
				corners.insert( std::make_pair( CNCPoint(PE), end_set ));
			}
			else
			{
				corners[CNCPoint(PE)].insert( VE );
			}

            coordinates.insert( CNCPoint(PS) );
            coordinates.insert( CNCPoint(PE) );

            if (PS.Z() > highest) highest = PS.Z();
            if (PS.Z() < lowest) lowest = PS.Z();

            if (PE.Z() > highest) highest = PE.Z();
            if (PE.Z() < lowest) lowest = PE.Z();
        } // End for
    } // End for

    // Sort the coordinates of all the edges so that they're arranged geographically from the
	// current machine location.  We want to minimize rapid movements between these corner-forming
	// toolpaths.
    SortedCoordinates_t sorted_coordinates;
    std::copy( coordinates.begin(), coordinates.end(), std::inserter( sorted_coordinates, sorted_coordinates.begin() ));
    for (SortedCoordinates_t::iterator l_itPoint = sorted_coordinates.begin(); l_itPoint != sorted_coordinates.end(); l_itPoint++)
    {
        if (l_itPoint == sorted_coordinates.begin())
        {
            sort_points_by_distance compare( pMachineState->Location() );
            std::sort( sorted_coordinates.begin(), sorted_coordinates.end(), compare );
        } // End if - then
        else
        {
            // We've already begun.  Just sort based on the previous point's location.
            SortedCoordinates_t::iterator l_itNextPoint = l_itPoint;
            l_itNextPoint++;

            if (l_itNextPoint != sorted_coordinates.end())
            {
                sort_points_by_distance compare( *l_itPoint );
                std::sort( l_itNextPoint, sorted_coordinates.end(), compare );
            } // End if - then
        } // End if - else
    } // End for

	// We now have all the coordinates and vectors of all the edges in the wire.  Look at
    // each coordinate, discard duplicate vectors and find the angle between the two
    // vectors formed at each coordinate.

    gp_Vec reference( 0, 0, -1 );    // Looking from the top down.
    for (SortedCoordinates_t::iterator itCoordinate = sorted_coordinates.begin(); itCoordinate != sorted_coordinates.end(); itCoordinate++)
    {
		if (CDouble(highest) > CDouble(itCoordinate->Z()))
		{
			// This is a corner on one of the lower level wires.  Don't bother forming corners
			// here as this point will have been picked up when the top-level wire was
			// processed.
			continue;
		}

		if (corners[*itCoordinate].size() == 2)
		{
			Corners_t similar = FindSimilarCorners(*itCoordinate, corners, CCuttingTool::Find(pMachineState->CuttingTool()));

			// We don't want to form corners on two intersecting edges if the angle of intersection
            // is too shallow.  i.e. if there are two lines that are mostly pointing in the same
            // direction, we don't want to waste time forming the corners at their intersection.
			// Discard corners that do not require special attention.
			for (Corners_t::iterator itSimilar = similar.begin(); itSimilar != similar.end(); /* increment within loop */ )
			{
				if (CornerNeedsSharpenning(itSimilar) == false)
				{
				    Corners_t::iterator itNext = itSimilar;
				    itNext++;
					similar.erase(itSimilar);
					itSimilar = itNext;
				}
				else
				{
					itSimilar++;
				}
			} // End for

			if (similar.size() < 2)
			{
				// We require at least one top and one bottom point to make a toolpath.
				continue;
			}

			// The path between these corners represents the toolpath required to sharpen the corner.  Move
			// from the bottom to the top.  The CDouble class is nothing more than a C++ class to handle
			// the comparison of double values while taking into account the geometry tolerance configured.
			std::list<CDouble> depths;

			for (Corners_t::iterator itCorner = similar.begin(); itCorner != similar.end(); itCorner++)
			{
				depths.push_back(itCorner->first.Z());
			} // End for

			depths.sort();

			CNCPoint top_corner;
			CNCPoint bottom_corner;

			// Obtain the top-most and bottom-most corner coordinates from these 'similar' corners.
			for (Corners_t::iterator itCorner = similar.begin(); itCorner != similar.end(); itCorner++)
			{
				if (itCorner == similar.begin())
				{
					top_corner = itCorner->first;
					bottom_corner = itCorner->first;
				}
				else
				{
					if (top_corner.Z() < itCorner->first.Z()) top_corner = itCorner->first;
					if (bottom_corner.Z() > itCorner->first.Z()) bottom_corner = itCorner->first;
				}
			}

			// We have done all this processing on un-adjusted coordinates so far.  Adjust the bottom and top
			// coordinates to align with the fixture.
			bottom_corner = pMachineState->Fixture().Adjustment(bottom_corner);
			top_corner = pMachineState->Fixture().Adjustment(top_corner);

			// Rapid into place first.
			python << _T("comment(") << PythonString(_("sharpen corner")) << _T(")\n");
			python << _T("rapid(z=") << this->m_depth_op_params.m_clearance_height / theApp.m_program->m_units << _T(")\n");
			python << _T("rapid(x=") << bottom_corner.X(true) << _T(", y=") << bottom_corner.Y(true) << _T(")\n");
			python << _T("rapid(x=") << bottom_corner.X(true) << _T(", y=") << bottom_corner.Y(true) << _T(", z=") << this->m_depth_op_params.m_rapid_down_to_height / theApp.m_program->m_units << _T(")\n");
			python << _T("feed(x=") << bottom_corner.X(true) << _T(", y=") << bottom_corner.Y(true) << _T(", z=") << bottom_corner.Z(true) << _T(")\n");
			pMachineState->Location(bottom_corner);

			// Top corner
			python << _T("feed(x=") << top_corner.X(true) << _T(", y=") << top_corner.Y(true) << _T(", z=") << top_corner.Z(true) << _T(")\n");
			pMachineState->Location(top_corner);

			// Now get back up to clearance height.
			CNCPoint temp(pMachineState->Location());
			temp.SetZ(this->m_depth_op_params.m_clearance_height);
			pMachineState->Location(temp);

			python << _T("rapid(z=") << this->m_depth_op_params.m_clearance_height / theApp.m_program->m_units << _T(")\n");
		} // End if - then
    }

	return(python);
} // End FormCorners() method

/**
	This method returns the toolpath wires for all valleys of all sketches.  Each valley
	includes a set of closed wires that surround the valley at a particular depth (height)
	setting.
 */
CInlay::Valleys_t CInlay::DefineValleys(CMachineState *pMachineState)
{
	Valleys_t valleys;

	ReloadPointers();

	double tolerance = heeksCAD->GetTolerance();

	typedef double Depth_t;

	CCuttingTool *pChamferingBit = (CCuttingTool *) heeksCAD->GetIDObject( CuttingToolType, m_cutting_tool_number );

    // For all selected sketches.
	for (HeeksObj *object = GetFirstChild(); object != NULL; object = GetNextChild())
	{
	    // Convert them to a list of wire objects.
		std::list<TopoDS_Shape> wires;
		if (heeksCAD->ConvertSketchToFaceOrWire( object, wires, false))
		{
			// The wire(s) represent the sketch objects for a tool path.
			try {
			    // For all wires in this sketch...
				for(std::list<TopoDS_Shape>::iterator It2 = wires.begin(); It2 != wires.end(); It2++)
				{
					TopoDS_Shape& wire_to_fix = *It2;
					ShapeFix_Wire fix;
					fix.Load( TopoDS::Wire(wire_to_fix) );
					fix.FixReorder();

					TopoDS_Shape wire = fix.Wire();

					// DO NOT align wires with the fixture YET.  When we form
					// the corners, we will assume the wires are all in the XY plane.
					// We will rotate the wire later in the process.
					// We also need to align the male and female halves differently due
					// to the possible use of two fixtures.

                    // We want to figure out what the maximum offset is at the maximum depth atainable
                    // by the chamfering bit.  Within this smallest of wires, we need to pocket with
                    // the clearance tool.  From this point outwards (or inwards for male operations)
                    // we need to move sideways by half the chamfering bit's diameter until we hit the
                    // outer edge.

                    double angle = pChamferingBit->m_params.m_cutting_edge_angle / 360.0 * 2.0 * PI;
                    double max_offset = (m_depth_op_params.m_start_depth - m_depth_op_params.m_final_depth) * tan(angle);

                    // If this is too far for this sketch's geometry, figure out what the maximum offset is.
                    max_offset = FindMaxOffset( max_offset, TopoDS::Wire(wire), m_depth_op_params.m_step_down * tan(angle) / 10.0 );

                    double max_plunge_possible = max_offset * tan(angle);

                    // We know how far down the bit could be plunged based on the bit's geometry as well
                    // as the sketech's geometry.  See if this is too deep for our use.
                    double max_plunge_for_chamfering_bit = pChamferingBit->m_params.m_cutting_edge_height * cos(angle);
                    double plunge = max_plunge_possible;
                    if (plunge > max_plunge_for_chamfering_bit)
                    {
                        plunge = max_plunge_for_chamfering_bit;
                    }

                    // We need to keep a record of the wires we've machined so that we can figure out what
                    // shapenning moves (clearing out the corners) we need to make at the end.  These will
                    // be between concave corners of adjacent edges and will move up to the corresponding
                    // corners of the edge shape immediately above.

                    Valley_t valley;

                    // Even though we didn't machine this top wire, we need to use it to generate the
                    // corner movements.
                    Path path;
                    path.Depth( m_depth_op_params.m_start_depth );
                    path.Offset(0.0);
                    path.Wire(TopoDS::Wire(wire));
                    valley.push_back(path);

					typedef double Depth_t;
					typedef std::list< Depth_t > Depths_t;
					Depths_t depths;

                    // machine at this depth around the wire in ever increasing loops until we hit the outside wire (offset = 0)
                    for (double offset = max_offset; offset >= tolerance; /* decrement within loop */ )
                    {
                        double max_depth = offset / tan(angle) * -1.0;
                        double step_down = m_depth_op_params.m_step_down;
                        if (m_depth_op_params.m_step_down > (-1.0 * max_depth))
                        {
                            step_down = max_depth * -1.0;
                        }
                        else
                        {
                            step_down = m_depth_op_params.m_step_down;
                        }

                        for (double depth = m_depth_op_params.m_start_depth - step_down; ((step_down > tolerance) && (depth >= max_depth)); /* increment within loop */ )
                        {


							if (std::find(depths.begin(), depths.end(), depth) == depths.end()) depths.push_back( depth );

							// Machine here with the chamfering bit.
							try {
								BRepOffsetAPI_MakeOffset offset_wire(TopoDS::Wire(wire));
								offset_wire.Perform(offset * -1.0);

								if (offset_wire.IsDone())
								{
									TopoDS_Wire toolpath = TopoDS::Wire(offset_wire.Shape());
									gp_Trsf translation;
									translation.SetTranslation( gp_Vec( gp_Pnt(0,0,0), gp_Pnt( 0,0,depth)));
									BRepBuilderAPI_Transform translate(translation);
									translate.Perform(toolpath, false);
									toolpath = TopoDS::Wire(translate.Shape());

                                    Path path;
                                    path.Depth(depth);
                                    path.Offset(offset);
									path.Wire(toolpath);

									valley.push_back( path );
								} // End if - then
							} // End try
							catch (Standard_Failure & error) {
								(void) error;	// Avoid the compiler warning.
								Handle_Standard_Failure e = Standard_Failure::Caught();
							} // End catch

                            if ((depth - step_down) > max_depth)
                            {
                                depth -= step_down;
                            }
                            else
                            {
                                if (depth > max_depth)
                                {
                                    depth = max_depth;
                                }
                                else
                                {
                                    depth = max_depth - 1.0;  // Force exit from loop
                                }
                            }
                        } // End for

                        if ((offset - (pChamferingBit->m_params.m_diameter / 1.0)) > tolerance)
                        {
                            offset -= (pChamferingBit->m_params.m_diameter / 1.0);
                        }
                        else
                        {
                            if (offset > tolerance)
                            {
                                offset = tolerance;
                            }
                            else
                            {
                                offset = tolerance / 2.0;
                                break;
                            }
                        }
                    } // End for

					valleys.push_back(valley);
				} // End for
			} // End try
			catch (Standard_Failure & error) {
					(void) error;	// Avoid the compiler warning.
					Handle_Standard_Failure e = Standard_Failure::Caught();
			} // End catch
		} // End if - then
	} // End for

	return(valleys);

} // End DefineValleys() method




Python CInlay::FormValleyWalls( CInlay::Valleys_t valleys, CMachineState *pMachineState  )
{
	Python python;

	python << _T("comment(") << PythonString(_("Form valley walls")) << _T(")\n");
	python << pMachineState->CuttingTool(m_cutting_tool_number);  // select the chamfering bit.
	python << SelectFixture(pMachineState, true);	// Select female fixture (if appropriate)

    double tolerance = heeksCAD->GetTolerance();

	CNCPoint last_position(0,0,0);
	for (Valleys_t::iterator itValley = valleys.begin(); itValley != valleys.end(); itValley++)
	{
	    // Get a list of depths and offsets for this valley.
	    std::list<double> depths;
	    std::list<double> offsets;
		for (Valley_t::iterator itPath = itValley->begin(); itPath != itValley->end(); itPath++)
		{
			if (std::find(depths.begin(), depths.end(), itPath->Depth()) == depths.end()) depths.push_back(itPath->Depth());
            if (std::find(offsets.begin(), offsets.end(), itPath->Offset()) == offsets.end()) offsets.push_back(itPath->Offset());
		} // End for

		depths.sort();
		depths.reverse();

		offsets.sort();
		offsets.reverse();

        for (std::list<double>::iterator itOffset = offsets.begin(); itOffset != offsets.end(); itOffset++)
        {
            for (std::list<double>::iterator itDepth = depths.begin(); itDepth != depths.end(); itDepth++)
            {
                // We don't want a toolpath at the top surface.
                if (fabs(*itDepth - m_depth_op_params.m_start_depth) > tolerance)
                {
                    Path path;
                    path.Offset(*itOffset);
                    path.Depth(*itDepth);

                    Valley_t::iterator itPath = std::find(itValley->begin(),  itValley->end(), path);
                    // for (Valley_t::iterator itPath = itValley->begin(); itPath != itValley->end(); itPath++)
                    if (itPath != itValley->end())
                    {

                        // Rotate this wire to align with the fixture.
                        BRepBuilderAPI_Transform transform(pMachineState->Fixture().GetMatrix());
                        TopoDS_Wire wire(itPath->Wire());
                        transform.Perform(wire, false);
                        wire = TopoDS::Wire(transform.Shape());

                        python << CContour::GeneratePathFromWire(wire,
                                                                pMachineState,
                                                                m_depth_op_params.m_clearance_height,
                                                                m_depth_op_params.m_rapid_down_to_height );
                    } // End for
                } // End if - then
            } // End for
        } // End for

        // Now run through the wires map and generate the toolpaths that will sharpen
        // the concave corners formed between adjacent edges.
        python << FormCorners( *itValley, pMachineState );
	} // End for

	return(python);

} // End FormValleyWalls() method




Python CInlay::FormValleyPockets( CInlay::Valleys_t valleys, CMachineState *pMachineState  )
{
	Python python;

	python << _T("comment(") << PythonString(_("Form valley pockets")) << _T(")\n");

	python << SelectFixture(pMachineState, true);	// Select female fixture (if appropriate)
	python << pMachineState->CuttingTool(m_params.m_clearance_tool);	// Select the clearance tool.

	CNCPoint last_position(0,0,0);
	for (Valleys_t::iterator itValley = valleys.begin(); itValley != valleys.end(); itValley++)
	{
		// Find the largest offset and the largest depth values.
	    double min_depth = 0.0;
	    double max_offset = 0.0;
		for (Valley_t::iterator itPath = itValley->begin(); itPath != itValley->end(); itPath++)
		{
			if (itPath->Offset() > max_offset) max_offset = itPath->Offset();
			if (itPath->Depth() < min_depth) min_depth = itPath->Depth();
		} // End for

        Path path;
        path.Offset(max_offset);
        path.Depth(min_depth);

        Valley_t::iterator itPath = std::find(itValley->begin(),  itValley->end(), path);
        if (itPath != itValley->end())
        {
            // We need to generate a pocket operation based on this tool_path_wire
            // and using the Clearance Tool.  Without this, the chamfering bit would need
            // to machine out the centre of the valley as well as the walls.

            // Rotate this wire to align with the fixture.
            BRepBuilderAPI_Transform transform(pMachineState->Fixture().GetMatrix());
            TopoDS_Wire wire(itPath->Wire());
            transform.Perform(wire, false);
            wire = TopoDS::Wire(transform.Shape());

            HeeksObj *pBoundary = heeksCAD->NewSketch();
            if (heeksCAD->ConvertWireToSketch(wire, pBoundary, heeksCAD->GetTolerance()))
            {
                std::list<HeeksObj *> objects;
                objects.push_back(pBoundary);

                // Save the fixture and pass in one that has no rotation.  The sketch has already
                // been rotated.  We don't want to apply the rotations twice.
                CFixture save_fixture(pMachineState->Fixture());

                CFixture straight(pMachineState->Fixture());
                straight.m_params.m_yz_plane = 0.0;
                straight.m_params.m_xz_plane = 0.0;
                straight.m_params.m_xy_plane = 0.0;
                straight.m_params.m_pivot_point = gp_Pnt(0.0, 0.0, 0.0);
                pMachineState->Fixture(straight);    // Replace with a straight fixture.

                TopoDS_Wire pocket_area;
                // DeterminePocketArea(pBoundary, pMachineState, &pocket_area);

                CPocket *pPocket = new CPocket( objects, m_params.m_clearance_tool );
                pPocket->m_depth_op_params = m_depth_op_params;
                pPocket->m_depth_op_params.m_final_depth = itPath->Depth();
                pPocket->m_speed_op_params = m_speed_op_params;
                python << pPocket->AppendTextToProgram(pMachineState);
                delete pPocket;		// We don't need it any more.

                // Reinstate the original fixture.
                pMachineState->Fixture(save_fixture);
            } // End if - then
        } // End if - then
	} // End for

	return(python);

} // End FormValleyPockets() method


Python CInlay::FormMountainPockets( CInlay::Valleys_t valleys, CMachineState *pMachineState, const bool only_above_mountains  )
{
	Python python;

	if (only_above_mountains)
	{
		python << _T("comment(") << PythonString(_("Form mountain pockets above short mountains")) << _T(")\n");
	}
	else
	{
		python << _T("comment(") << PythonString(_("Form mountains")) << _T(")\n");
	}

    double tolerance = heeksCAD->GetTolerance();

	python << SelectFixture(pMachineState, false);	// Select male fixture (if appropriate)
	python << pMachineState->CuttingTool(m_params.m_clearance_tool);	// Select the clearance tool.

	// Use the parameters to determine if we're going to mirror the selected
    // sketches around the X or Y axis.
	gp_Ax1 mirror_axis;
	if (m_params.m_mirror_axis == CInlayParams::eXAxis)
	{
        mirror_axis = gp_Ax1(gp_Pnt(0,0,0), gp_Dir(1,0,0));
	}
	else
	{
	    mirror_axis = gp_Ax1(gp_Pnt(0,0,0), gp_Dir(0,1,0));
	}

	// Find the maximum valley depth for all valleys.  Once the valleys are rotated (upside-down), this
	// will become the maximum mountain height.  This, in turn, will be used as the top-most surface
	// of the male half.  i.e. the '0' height when machining the male half.
	double max_valley_depth = 0.0;
	for (Valleys_t::iterator itValley = valleys.begin(); itValley != valleys.end(); itValley++)
	{
		for (Valley_t::iterator itPath = itValley->begin(); itPath != itValley->end(); itPath++)
		{
			if (itPath->Depth() < max_valley_depth)
			{
				max_valley_depth = itPath->Depth();
			} // End if - then
		} // End for
	} // End for

	double max_mountain_height = max_valley_depth * -1.0;

	typedef std::list<HeeksObj *> MirroredSketches_t;
	MirroredSketches_t mirrored_sketches;

	std::list<HeeksObj *> pockets;

	CBox bounding_box;

	// For all valleys.
	CNCPoint last_position(0,0,0);
	for (Valleys_t::iterator itValley = valleys.begin(); itValley != valleys.end(); itValley++)
	{
		// For this valley, see if it's shorter than the tallest valley.  If it is, we need to pocket
		// out the material directly above the valley (down to this valley's peak).  This clears the way
		// to start forming this valley's walls.

		std::list<double> depths;
		for (Valley_t::iterator itPath = itValley->begin(); itPath != itValley->end(); itPath++)
		{
			depths.push_back(itPath->Depth());
		} // End for

		depths.sort();

		double mountain_height = *(depths.begin()) * -1.0;

		depths.reverse();
		double base_height = *(depths.begin()) * -1.0;

		// We need to generate a pocket operation based on this tool_path_wire
		// and using the Clearance Tool.  Without this, the chamfering bit would need
		// to machine out the centre of the valley as well as the walls.

		Path path;
		path.Depth(base_height * -1.0);
		path.Offset(0.0);

		Valley_t::iterator itPath = std::find(itValley->begin(), itValley->end(), path);
        if (itPath != itValley->end())
        {
            // It's the male half we're generating.  Rotate the wire around one
            // of the two axes so that we end up machining the reverse of the
            // female half.
            gp_Trsf rotation;
            TopoDS_Wire tool_path_wire(itPath->Wire());

            rotation.SetRotation( mirror_axis, PI );
            BRepBuilderAPI_Transform rotate(rotation);
            rotate.Perform(tool_path_wire, false);
            tool_path_wire = TopoDS::Wire(rotate.Shape());

            // Rotate this wire to align with the fixture.
            BRepBuilderAPI_Transform transform(pMachineState->Fixture().GetMatrix());
            transform.Perform(tool_path_wire, false);
            tool_path_wire = TopoDS::Wire(transform.Shape());

            HeeksObj *pBoundary = heeksCAD->NewSketch();
            if (heeksCAD->ConvertWireToSketch(tool_path_wire, pBoundary, heeksCAD->GetTolerance()))
            {
                // Make sure this sketch is oriented counter-clockwise.  We will ensure the
                // bounding sketch is oriented clockwise so that the pocket operation
                // removes the intervening material.
                for (int i=0; (heeksCAD->GetSketchOrder(pBoundary) != SketchOrderTypeCloseCCW) && (i<4); i++)
                {
                    // At least try to make them all consistently oriented.
                    heeksCAD->ReOrderSketch( pBoundary, SketchOrderTypeCloseCCW );
                } // End for

                std::list<HeeksObj *> objects;
                objects.push_back(pBoundary);

                if ((max_mountain_height - mountain_height) > tolerance)
                {
                    CPocket *pPocket = new CPocket( objects, m_params.m_clearance_tool );
                    pPocket->m_depth_op_params = m_depth_op_params;
                    pPocket->m_speed_op_params = m_speed_op_params;
                    pPocket->m_depth_op_params.m_start_depth = 0.0;
                    pPocket->m_depth_op_params.m_final_depth = (-1.0 * (max_mountain_height - mountain_height));
                    pPocket->m_speed_op_params = m_speed_op_params;
                    pockets.push_back(pPocket);

                    if (only_above_mountains)
                    {
                        // Save the fixture and pass in one that has no rotation.  The sketch has already
                        // been rotated.  We don't want to apply the rotations twice.
                        CFixture save_fixture(pMachineState->Fixture());

                        CFixture straight(pMachineState->Fixture());
                        straight.m_params.m_yz_plane = 0.0;
                        straight.m_params.m_xz_plane = 0.0;
                        straight.m_params.m_xy_plane = 0.0;
                        straight.m_params.m_pivot_point = gp_Pnt(0.0, 0.0, 0.0);
                        pMachineState->Fixture(straight);    // Replace with a straight fixture.

                        python << pPocket->AppendTextToProgram(pMachineState);

                        pMachineState->Fixture(save_fixture);
                    }
                } // End if - then

                CBox box;
                pBoundary->GetBox(box);
                bounding_box.Insert(box);

                mirrored_sketches.push_back(pBoundary);
            }
        } // End if - then
	} // End for


	if (! only_above_mountains)
	{
		// Make sure the bounding box is one tool radius larger than all the sketches that
		// have been mirrored.  We need to create one large sketch with the bounding box
		// 'order' in one direction and all the mirrored sketch's orders in the other
		// direction.  We can then create a pocket operation to remove the material between
		// the mirrored sketches down to the inverted 'top surface' depth.

		double border_width = m_params.m_border_width;
		if ((CCuttingTool::Find(m_params.m_clearance_tool) != NULL) &&
			(border_width <= (2.0 * CCuttingTool::Find( m_params.m_clearance_tool)->CuttingRadius())))
		{
			border_width = (2.0 * CCuttingTool::Find( m_params.m_clearance_tool)->CuttingRadius());
			border_width += 1; // Make sure there really is room.  Add 1mm to be sure.
		}
		HeeksObj* bounding_sketch = heeksCAD->NewSketch();

		double start[3];
		double end[3];

		// left edge
		start[0] = bounding_box.MinX() - border_width;
		start[1] = bounding_box.MinY() - border_width;
		start[2] = bounding_box.MinZ();

		end[0] = bounding_box.MinX() - border_width;
		end[1] = bounding_box.MaxY() + border_width;
		end[2] = bounding_box.MinZ();

		bounding_sketch->Add( heeksCAD->NewLine( start, end ), NULL );

		// top edge
		start[0] = bounding_box.MinX() - border_width;
		start[1] = bounding_box.MaxY() + border_width;
		start[2] = bounding_box.MinZ();

		end[0] = bounding_box.MaxX() + border_width;
		end[1] = bounding_box.MaxY() + border_width;
		end[2] = bounding_box.MinZ();

		bounding_sketch->Add( heeksCAD->NewLine( start, end ), NULL );

		// right edge
		start[0] = bounding_box.MaxX() + border_width;
		start[1] = bounding_box.MaxY() + border_width;
		start[2] = bounding_box.MinZ();

		end[0] = bounding_box.MaxX() + border_width;
		end[1] = bounding_box.MinY() - border_width;
		end[2] = bounding_box.MinZ();

		bounding_sketch->Add( heeksCAD->NewLine( start, end ), NULL );

		// bottom edge
		start[0] = bounding_box.MaxX() + border_width;
		start[1] = bounding_box.MinY() - border_width;
		start[2] = bounding_box.MinZ();

		end[0] = bounding_box.MinX() - border_width;
		end[1] = bounding_box.MinY() - border_width;
		end[2] = bounding_box.MinZ();

		bounding_sketch->Add( heeksCAD->NewLine( start, end ), NULL );

		// Make sure this border sketch is oriented clockwise.  We will ensure the
		// enclosed sketches are oriented counter-clockwise so that the pocket operation
		// removes the intervening material.
		for (int i=0; (heeksCAD->GetSketchOrder(bounding_sketch) != SketchOrderTypeCloseCW) && (i<4); i++)
		{
			// At least try to make them all consistently oriented.
			heeksCAD->ReOrderSketch( bounding_sketch, SketchOrderTypeCloseCW );
		} // End for

		for (MirroredSketches_t::iterator itObject = mirrored_sketches.begin(); itObject != mirrored_sketches.end(); itObject++)
		{
			std::list<HeeksObj*> new_lines_and_arcs;
			for (HeeksObj *child = (*itObject)->GetFirstChild(); child != NULL; child = (*itObject)->GetNextChild())
			{
				new_lines_and_arcs.push_back(child);
			}

			((ObjList *)bounding_sketch)->Add( new_lines_and_arcs );
		} // End for

		// This bounding_sketch is now in a form that is suitable for machining with a pocket operation.  We need
		// to reduce the areas between the mirrored sketches down to a level that will eventually mate with the
		// female section's top surface.

		std::list<HeeksObj *> bounding_sketch_list;
		bounding_sketch_list.push_back( bounding_sketch );

		CPocket *bounding_sketch_pocket = new CPocket(bounding_sketch_list, m_params.m_clearance_tool);
		bounding_sketch_pocket->m_pocket_params.m_material_allowance = 0.0;
		bounding_sketch_pocket->m_depth_op_params = m_depth_op_params;
		bounding_sketch_pocket->m_speed_op_params = m_speed_op_params;
		bounding_sketch_pocket->m_depth_op_params.m_start_depth = 0.0;
		bounding_sketch_pocket->m_depth_op_params.m_final_depth = -1.0 * max_mountain_height;

        // Save the fixture and pass in one that has no rotation.  The sketch has already
        // been rotated.  We don't want to apply the rotations twice.
        CFixture save_fixture(pMachineState->Fixture());

        CFixture straight(pMachineState->Fixture());
        straight.m_params.m_yz_plane = 0.0;
        straight.m_params.m_xz_plane = 0.0;
        straight.m_params.m_xy_plane = 0.0;
        straight.m_params.m_pivot_point = gp_Pnt(0.0, 0.0, 0.0);
        pMachineState->Fixture(straight);    // Replace with a straight fixture.

		python << bounding_sketch_pocket->AppendTextToProgram(pMachineState);

		// Reinstate the original fixture.
		pMachineState->Fixture(save_fixture);

		pockets.push_back(bounding_sketch_pocket);
	} // End if - then

	// We don't need these objects any more.  Deleting the pocket will also delete the sketch (its child).
	for (std::list<HeeksObj *>::iterator itPocket = pockets.begin(); itPocket != pockets.end(); itPocket++)
	{
		delete *itPocket;		// We don't need it any more.
	} // End for

	return(python);

} // End FormMountainPockets() method



Python CInlay::FormMountainWalls( CInlay::Valleys_t valleys, CMachineState *pMachineState  )
{
	Python python;

	python << _T("comment(") << PythonString(_("Form mountain walls")) << _T(")\n");

	python << SelectFixture(pMachineState, false);	// Select male fixture (if appropriate)
	python << pMachineState->CuttingTool(m_cutting_tool_number);	// Select the chamfering bit.

    double tolerance = heeksCAD->GetTolerance();

    // Use the parameters to determine if we're going to mirror the selected
    // sketches around the X or Y axis.
	gp_Ax1 mirror_axis;
	if (m_params.m_mirror_axis == CInlayParams::eXAxis)
	{
        mirror_axis = gp_Ax1(gp_Pnt(0,0,0), gp_Dir(1,0,0));
	}
	else
	{
	    mirror_axis = gp_Ax1(gp_Pnt(0,0,0), gp_Dir(0,1,0));
	}

	// Find the maximum valley depth for all valleys.  Once the valleys are rotated (upside-down), this
	// will become the maximum mountain height.  This, in turn, will be used as the top-most surface
	// of the male half.  i.e. the '0' height when machining the male half.
	double max_valley_depth = 0.0;
	for (Valleys_t::iterator itValley = valleys.begin(); itValley != valleys.end(); itValley++)
	{
		for (Valley_t::iterator itPath = itValley->begin(); itPath != itValley->end(); itPath++)
		{
			if (itPath->Depth() < max_valley_depth)
			{
				max_valley_depth = itPath->Depth();
			} // End if - then
		} // End for
	} // End for

	CNCPoint last_position(0,0,0);
	for (Valleys_t::iterator itValley = valleys.begin(); itValley != valleys.end(); itValley++)
	{
		std::list<double> depths;
		std::list<double> offsets;
		for (Valley_t::iterator itPath = itValley->begin(); itPath != itValley->end(); itPath++)
		{
			depths.push_back(itPath->Depth());
			offsets.push_back(itPath->Offset());
		} // End for

		depths.sort();
		depths.unique();
		depths.reverse();

		offsets.sort();
		offsets.unique();
		offsets.reverse();  // Go from the largest offset (inside) to the smallest offset (outside)

		for (std::list<double>::iterator itOffset = offsets.begin(); itOffset != offsets.end(); itOffset++)
		{
            // Find the highest wire for this offset in the valley.  When it's mirrored, this will become
            // the lowest wire for the mountain.  We will then step down to it from the top level.
            Valley_t::iterator itPath = itValley->end();
            for (std::list<double>::iterator itDepth = depths.begin(); itDepth != depths.end(); itDepth++)
            {
                Path path;
                path.Offset(*itOffset);
                path.Depth(*itDepth);

                Valley_t::iterator itThisPath = std::find(itValley->begin(), itValley->end(), path);
                if (itThisPath != itValley->end())
                {
                    if (itPath == itValley->end())
                    {
                        itPath = itThisPath;
                    }
                    else
                    {
                        if (itThisPath->Depth() < itPath->Depth())
                        {
                            itPath = itThisPath;
                        }
                    }
                }
            } // End for

            // It's the male half we're generating.  Rotate the wire around one
            // of the two axes so that we end up machining the reverse of the
            // female half.
            gp_Trsf rotation;
            TopoDS_Wire top_level_wire(itPath->Wire());

            rotation.SetRotation( mirror_axis, PI );
            BRepBuilderAPI_Transform rotate(rotation);
            rotate.Perform(top_level_wire, false);
            top_level_wire = TopoDS::Wire(rotate.Shape());

            if (fabs(max_valley_depth) > tolerance)
            {
                // And offset the wire 'down' so that the maximum depth reached during the
                // female half's processing ends up being at the 'top-most' surface of the
                // male half we're producing.
                gp_Trsf translation;
                translation.SetTranslation( gp_Vec( gp_Pnt(0,0,0), gp_Pnt( 0,0, itPath->Depth())));
                BRepBuilderAPI_Transform translate(translation);
                translate.Perform(top_level_wire, false);
                top_level_wire = TopoDS::Wire(translate.Shape());
            }

            double final_depth = max_valley_depth - itPath->Depth();
            for (double depth = m_depth_op_params.m_start_depth - m_depth_op_params.m_step_down;
            // for (double depth = m_depth_op_params.m_start_depth;
                    depth >= final_depth; /* decrement within loop */ )
            {
                TopoDS_Wire tool_path_wire(top_level_wire);

                if (fabs(depth) > tolerance)
                {
                    // And offset the wire 'down' so that the maximum depth reached during the
                    // female half's processing ends up being at the 'top-most' surface of the
                    // male half we're producing.
                    gp_Trsf translation;
                    translation.SetTranslation( gp_Vec( gp_Pnt(0,0,0), gp_Pnt( 0,0, depth)));
                    BRepBuilderAPI_Transform translate(translation);
                    translate.Perform(tool_path_wire, false);
                    tool_path_wire = TopoDS::Wire(translate.Shape());
                }

                // Rotate this wire to align with the fixture.
                BRepBuilderAPI_Transform transform(pMachineState->Fixture().GetMatrix());
                transform.Perform(tool_path_wire, false);
                tool_path_wire = TopoDS::Wire(transform.Shape());

                python << pMachineState->CuttingTool(m_cutting_tool_number);  // Select the chamfering bit.

                python << CContour::GeneratePathFromWire(tool_path_wire,
                                                        pMachineState,
                                                        m_depth_op_params.m_clearance_height,
                                                        m_depth_op_params.m_rapid_down_to_height );

                if (depth == final_depth)
                {
                    depth -= m_depth_op_params.m_step_down; // break out
                    break;
                }
                else
                {
                    if ((depth - final_depth) > m_depth_op_params.m_step_down)
                    {
                        depth -= m_depth_op_params.m_step_down;
                    }
                    else
                    {
                        depth = final_depth;
                    }
                } // End if - else
            } // End for
		} // End for
	} // End for

	return(python);

} // End FormMountainWalls() method













/**
	This is the Graphics Library Commands (from the OpenGL set).  This method calls the OpenGL
	routines to paint the drill action in the graphics window.  The graphics is transient.

	Part of its job is to re-paint the elements that this CInlay object refers to so that
	we know what CAD objects this CNC operation is referring to.
 */
void CInlay::glCommands(bool select, bool marked, bool no_color)
{
	CDepthOp::glCommands( select, marked, no_color );
}




void CInlay::GetProperties(std::list<Property *> *list)
{
	m_params.GetProperties(this, list);
	CDepthOp::GetProperties(list);
}

HeeksObj *CInlay::MakeACopy(void)const
{
	return new CInlay(*this);
}

void CInlay::CopyFrom(const HeeksObj* object)
{
	operator=(*((CInlay*)object));
}

bool CInlay::CanAddTo(HeeksObj* owner)
{
	return ((owner != NULL) && (owner->GetType() == OperationsType));
}

void CInlay::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element = new TiXmlElement( "Inlay" );
	root->LinkEndChild( element );
	m_params.WriteXMLAttributes(element);

    if (m_symbols.size() > 0)
    {
        TiXmlElement * symbols;
        symbols = new TiXmlElement( "symbols" );
        element->LinkEndChild( symbols );

        for (Symbols_t::const_iterator l_itSymbol = m_symbols.begin(); l_itSymbol != m_symbols.end(); l_itSymbol++)
        {
            TiXmlElement * symbol = new TiXmlElement( "symbol" );
            symbols->LinkEndChild( symbol );
            symbol->SetAttribute("type", l_itSymbol->first );
            symbol->SetAttribute("id", l_itSymbol->second );
        } // End for
    } // End if - then

	WriteBaseXML(element);
}

// static member function
HeeksObj* CInlay::ReadFromXMLElement(TiXmlElement* element)
{
	CInlay* new_object = new CInlay;
	std::list<TiXmlElement *> elements_to_remove;

	// read point and circle ids
	for(TiXmlElement* pElem = TiXmlHandle(element).FirstChildElement().Element(); pElem; pElem = pElem->NextSiblingElement())
	{
		std::string name(pElem->Value());
		if(name == "params"){
			new_object->m_params.ReadParametersFromXMLElement(pElem);
			elements_to_remove.push_back(pElem);
		}
		else if(name == "symbols"){
			for(TiXmlElement* child = TiXmlHandle(pElem).FirstChildElement().Element(); child; child = child->NextSiblingElement())
			{
				if (child->Attribute("type") && child->Attribute("id"))
				{
					new_object->AddSymbol( atoi(child->Attribute("type")), atoi(child->Attribute("id")) );
				}
			} // End for
			elements_to_remove.push_back(pElem);
		} // End if
	}

	for (std::list<TiXmlElement*>::iterator itElem = elements_to_remove.begin(); itElem != elements_to_remove.end(); itElem++)
	{
		element->RemoveChild(*itElem);
	}

	new_object->ReadBaseXML(element);

	return new_object;
}


/**
	This method adjusts any parameters that don't make sense.  It should report a list
	of changes in the list of strings.
 */
std::list<wxString> CInlay::DesignRulesAdjustment(const bool apply_changes)
{
	std::list<wxString> changes;

	return(changes);

} // End DesignRulesAdjustment() method


/**
    This method returns TRUE if the type of symbol is suitable for reference as a source of location
 */
bool CInlay::CanAdd( HeeksObj *object )
{
    if (object == NULL) return(false);

    switch (object->GetType())
    {
        case SketchType:
		case FixtureType:
            return(true);

        default:
            return(false);
    }
}


void CInlay::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
    CDepthOp::GetTools( t_list, p );
}





static void on_set_spline_deviation(double value, HeeksObj* object){
	CInlay::max_deviation_for_spline_to_arc = value;
	CInlay::WriteToConfig();
}

// static
void CInlay::GetOptions(std::list<Property *> *list)
{
	list->push_back ( new PropertyDouble ( _("Inlay spline deviation"), max_deviation_for_spline_to_arc, NULL, on_set_spline_deviation ) );
}


void CInlay::ReloadPointers()
{
	for (Symbols_t::iterator l_itSymbol = m_symbols.begin(); l_itSymbol != m_symbols.end(); l_itSymbol++)
	{
		HeeksObj *object = heeksCAD->GetIDObject( l_itSymbol->first, l_itSymbol->second );
		if (object != NULL)
		{
			Add( object, NULL );
		}
	}

	m_symbols.clear();
}


CInlay::CInlay( const CInlay & rhs ) : CDepthOp( rhs )
{
    m_params.set_initial_values();
	*this = rhs;	// Call the assignment operator.
}

CInlay & CInlay::operator= ( const CInlay & rhs )
{
	if (this != &rhs)
	{
		m_params = rhs.m_params;
		m_symbols.clear();
		std::copy( rhs.m_symbols.begin(), rhs.m_symbols.end(), std::inserter( m_symbols, m_symbols.begin() ) );

		CDepthOp::operator=( rhs );
	}

	return(*this);
}



