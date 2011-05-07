// Contour.cpp
/*
 * Copyright (c) 2009, Dan Heeks, Perttu Ahola
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "stdafx.h"
#include "Contour.h"
#include "CNCConfig.h"
#include "ProgramCanvas.h"
#include "interface/HeeksObj.h"
#include "interface/PropertyInt.h"
#include "interface/PropertyDouble.h"
#include "interface/PropertyLength.h"
#include "interface/PropertyChoice.h"
#include "tinyxml/tinyxml.h"
#include "Operations.h"
#include "CTool.h"
#include "Profile.h"
#include "Fixture.h"
#include "Fixtures.h"
#include "CNCPoint.h"
#include "PythonStuff.h"
#include "MachineState.h"
#include "Program.h"

#include <sstream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <functional>

#include <BRepOffsetAPI_MakeOffset.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Shape.hxx>
#include <BRepTools_WireExplorer.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <gp_Circ.hxx>
#include <ShapeFix_Wire.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRep_Tool.hxx>
#include <BRepTools.hxx>
#include <BRepMesh.hxx>
#include <Poly_Polygon3D.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <Handle_BRepAdaptor_HCurve.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <Adaptor3d_HCurve.hxx>
#include <Adaptor3d_Curve.hxx>

extern CHeeksCADInterface* heeksCAD;

using namespace std;

/* static */ double CContour::max_deviation_for_spline_to_arc = 0.1;


static void on_set_tool_on_side(int value, HeeksObj* object){
	switch(value)
	{
	case 0:
		((CContour*)object)->m_params.m_tool_on_side = CContourParams::eLeftOrOutside;
		break;
	case 1:
		((CContour*)object)->m_params.m_tool_on_side = CContourParams::eRightOrInside;
		break;
	default:
		((CContour*)object)->m_params.m_tool_on_side = CContourParams::eOn;
		break;
	}
	((CContour*)object)->WriteDefaultValues();
}

static void on_set_entry_move_type(int value, HeeksObj *object)
{
    ((CContour*)object)->m_params.m_entry_move_type = CContourParams::EntryMove_t(value);
    ((CContour*)object)->WriteDefaultValues();
}

void CContourParams::GetProperties(CContour* parent, std::list<Property *> *list)
{
    {
        std::list< wxString > choices;

        SketchOrderType order = SketchOrderTypeUnknown;

        if(parent->GetNumChildren() > 0)
        {
            HeeksObj* sketch = NULL;

            for (HeeksObj *object = parent->GetFirstChild(); ((sketch == NULL) && (object != NULL)); object = parent->GetNextChild())
            {
                if (object->GetType() == SketchType)
                {
                    sketch = object;
                }
            }

            if(sketch != NULL)
            {
                order = heeksCAD->GetSketchOrder(sketch);
                switch(order)
                {
                case SketchOrderTypeOpen:
                    choices.push_back(_("Left"));
                    choices.push_back(_("Right"));
                    break;

                case SketchOrderTypeCloseCW:
                case SketchOrderTypeCloseCCW:
                    choices.push_back(_("Outside"));
                    choices.push_back(_("Inside"));
                    break;

                default:
                    choices.push_back(_("Outside or Left"));
                    choices.push_back(_("Inside or Right"));
                    break;
                }
            }

            choices.push_back(_("On"));

            int choice = int(CContourParams::eOn);
            switch (parent->m_params.m_tool_on_side)
            {
                case CContourParams::eRightOrInside:	choice = 1;
                        break;

                case CContourParams::eOn:	choice = 2;
                        break;

                case CContourParams::eLeftOrOutside:	choice = 0;
                        break;
            } // End switch

            list->push_back(new PropertyChoice(_("tool on side"), choices, choice, parent, on_set_tool_on_side));
        }
    }

    {
        int choice = int(parent->m_params.m_entry_move_type);
        std::list<wxString> choices;
        for (CContourParams::EntryMove_t move = CContourParams::ePlunge; move <= CContourParams::eRamp; move = CContourParams::EntryMove_t(int(move) + 1))
        {
            wxString description;
            description << move;
            choices.push_back( description );
        }

        list->push_back(new PropertyChoice(_("entry move type"), choices, choice, parent, on_set_entry_move_type));
    }

}

void CContourParams::WriteDefaultValues()
{
	CNCConfig config(ConfigPrefix());
	config.Write(_T("ToolOnSide"), (int) m_tool_on_side);
	config.Write(_T("EntryMoveType"), (int) m_entry_move_type);
}

void CContourParams::ReadDefaultValues()
{
	CNCConfig config(ConfigPrefix());
	config.Read(_T("ToolOnSide"), (int *) &m_tool_on_side, (int) eOn);
	config.Read(_T("EntryMoveType"), (int *) &m_entry_move_type, (int) ePlunge);
}

void CContour::WriteDefaultValues()
{
    m_params.WriteDefaultValues();
	CDepthOp::WriteDefaultValues();
}

void CContour::ReadDefaultValues()
{
    m_params.ReadDefaultValues();
	CDepthOp::ReadDefaultValues();
}

void CContourParams::WriteXMLAttributes(TiXmlNode *root)
{
	TiXmlElement * element;
	element = new TiXmlElement( "params" );
	root->LinkEndChild( element );

	element->SetAttribute("side", m_tool_on_side);
	element->SetAttribute("entry_move_type", int(m_entry_move_type));
}

void CContourParams::ReadParametersFromXMLElement(TiXmlElement* pElem)
{
	if(pElem->Attribute("side")) pElem->Attribute("side", (int *) &m_tool_on_side);
	if (pElem->Attribute("entry_move_type")) pElem->Attribute("entry_move_type", (int *) &m_entry_move_type);
}


const wxBitmap &CContour::GetIcon()
{
	if(!m_active)return GetInactiveIcon();
	static wxBitmap* icon = NULL;
	if(icon == NULL)icon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/drilling.png")));
	return *icon;
}

/* static */ bool CContour::Clockwise( const gp_Circ & circle )
{
	return(circle.Axis().Direction().Z() <= 0);
}


/* static */ CNCPoint CContour::GetStart(const TopoDS_Edge &edge)
{
    BRepAdaptor_Curve curve(edge);
    double uStart = curve.FirstParameter();
    gp_Pnt PS;
    gp_Vec VS;
    curve.D1(uStart, PS, VS);

    return(PS);
}

/* static */ CNCPoint CContour::GetEnd(const TopoDS_Edge &edge)
{
    BRepAdaptor_Curve curve(edge);
    double uEnd = curve.LastParameter();
    gp_Pnt PE;
    gp_Vec VE;
    curve.D1(uEnd, PE, VE);

    return(PE);
}

/* static */ double CContour::GetLength(const TopoDS_Edge &edge)
{
    BRepAdaptor_Curve curve(edge);
    return(GCPnts_AbscissaPoint::Length(curve));
}


struct EdgeComparison : public binary_function<const TopoDS_Edge &, const TopoDS_Edge &, bool >
{
    EdgeComparison( const TopoDS_Edge & edge )
    {
        m_reference_edge = edge;
    }

    bool operator()( const TopoDS_Edge & lhs, const TopoDS_Edge & rhs ) const
    {

        std::vector<double> lhs_distances;
        lhs_distances.push_back( CContour::GetStart(m_reference_edge).Distance( CContour::GetStart(lhs) ) );
        lhs_distances.push_back( CContour::GetStart(m_reference_edge).Distance( CContour::GetEnd(lhs) ) );
        lhs_distances.push_back( CContour::GetEnd(m_reference_edge).Distance( CContour::GetStart(lhs) ) );
        lhs_distances.push_back( CContour::GetEnd(m_reference_edge).Distance( CContour::GetEnd(lhs) ) );
        std::sort(lhs_distances.begin(), lhs_distances.end());

        std::vector<double> rhs_distances;
        rhs_distances.push_back( CContour::GetStart(m_reference_edge).Distance( CContour::GetStart(rhs) ) );
        rhs_distances.push_back( CContour::GetStart(m_reference_edge).Distance( CContour::GetEnd(rhs) ) );
        rhs_distances.push_back( CContour::GetEnd(m_reference_edge).Distance( CContour::GetStart(rhs) ) );
        rhs_distances.push_back( CContour::GetEnd(m_reference_edge).Distance( CContour::GetEnd(rhs) ) );
        std::sort(rhs_distances.begin(), rhs_distances.end());

        return(*(lhs_distances.begin()) < *(rhs_distances.begin()));
    }

    TopoDS_Edge m_reference_edge;
};

/* static */ std::vector<TopoDS_Edge> CContour::SortEdges( const TopoDS_Wire & wire )
{
    std::vector<TopoDS_Edge> edges;

	for(BRepTools_WireExplorer expEdge(TopoDS::Wire(wire)); expEdge.More(); expEdge.Next())
	{
	    edges.push_back( TopoDS_Edge(expEdge.Current()) );
	} // End for

	for (std::vector<TopoDS_Edge>::iterator l_itEdge = edges.begin(); l_itEdge != edges.end(); l_itEdge++)
    {
        if (l_itEdge == edges.begin())
        {
            // It's the first edge.  Find the edge whose endpoint is closest to gp_Pnt(0,0,0) so that
            // the resutls of this sorting are consistent.  When we just use the first edge in the
            // wire, we end up with different results every time.  We want consistency so that, if we
            // use this Contour operation as a location for drilling a relief hole (one day), we want
            // to be sure the machining will begin from a consistently known location.

            std::vector<TopoDS_Edge>::iterator l_itStartingEdge = edges.begin();
            gp_Pnt closest_point = GetStart(*l_itStartingEdge);
            if (GetEnd(*l_itStartingEdge).Distance(gp_Pnt(0,0,0)) < closest_point.Distance(gp_Pnt(0,0,0)))
            {
                closest_point = GetEnd(*l_itStartingEdge);
            }
            for (std::vector<TopoDS_Edge>::iterator l_itCheck = edges.begin(); l_itCheck != edges.end(); l_itCheck++)
            {
                if (GetStart(*l_itCheck).Distance(gp_Pnt(0,0,0)) < closest_point.Distance(gp_Pnt(0,0,0)))
                {
                    closest_point = GetStart(*l_itCheck);
                    l_itStartingEdge = l_itCheck;
                }

                if (GetEnd(*l_itCheck).Distance(gp_Pnt(0,0,0)) < closest_point.Distance(gp_Pnt(0,0,0)))
                {
                    closest_point = GetEnd(*l_itCheck);
                    l_itStartingEdge = l_itCheck;
                }
            }

            EdgeComparison compare( *l_itStartingEdge );
            std::sort( edges.begin(), edges.end(), compare );
        } // End if - then
        else
        {
            // We've already begun.  Just sort based on the previous point's location.
            std::vector<TopoDS_Edge>::iterator l_itNextEdge = l_itEdge;
            l_itNextEdge++;

            if (l_itNextEdge != edges.end())
            {
                EdgeComparison compare( *l_itEdge );
                std::sort( l_itNextEdge, edges.end(), compare );
            } // End if - then
        } // End if - else
    } // End for

    return(edges);

} // End SortEdges() method


/**
    When we're starting a new sequence of edges, we want to run along the first edge
    so that we end up nearby to the next edge in the sorted sequence.  If we go in the
    wrong direction then we're just going to have to rapid up to clearance height and
    move to the beginning of the next edge anyway.  This routine returns 'true' if
    the next edge is closer to the 'end' of this edge and 'false' if it's closer to
    the 'beginning' of this edge.  This tell us whether we want to run forwards
    or backwards along this edge so that we're setup ready to machine the next edge.
 */
/* static */ bool CContour::DirectionTowarardsNextEdge( const TopoDS_Edge &from, const TopoDS_Edge &to )
{
    const bool forwards = true;
    const bool backwards = false;

    bool direction = forwards;

    double min_distance = 9999999;  // Some big number.
    if (GetStart(from).XYDistance( GetEnd( to )) < min_distance)
    {
        min_distance = GetStart( from ).XYDistance( GetEnd( to ));
        direction = backwards;
    }

    if (GetEnd(from).XYDistance( GetEnd( to )) < min_distance)
    {
        min_distance = GetEnd(from).XYDistance( GetEnd( to ));
        direction = forwards;
    }

    if (GetStart(from).XYDistance( GetStart( to )) < min_distance)
    {
        min_distance = GetStart(from).XYDistance( GetStart( to ));
        direction = backwards;
    }

    if (GetEnd(from).XYDistance( GetStart( to )) < min_distance)
    {
        min_distance = GetEnd(from).XYDistance( GetStart( to ));
        direction = forwards;
    }

    return(direction);
}


/**
	We're itterating through the vector of TopoDS_Edge objects forwards
	and backwards until we're either deep enough for our purposes or we
	come to a discontinuity between the two edges.  This routine simply
	looks for the ends (as both beginning and end) of the vector and
	allows the offset to loop around the vector if necessary.
 */
/* static */ std::vector<TopoDS_Edge>::size_type CContour::NextOffset(
	const std::vector<TopoDS_Edge> &edges,
	const std::vector<TopoDS_Edge>::size_type edges_offset,
	const int direction )
{
	if ((direction > 0) && (edges_offset == edges.size()-1))
	{
		return(0);  // Loop around.
	}

	if ((direction < 0) && (edges_offset == 0))
	{
		return(edges.size()-1); // Loop around.
	}

	return(edges_offset + direction);
}


/* static */ bool CContour::EdgesJoin( const TopoDS_Edge &a, const TopoDS_Edge &b )
{
	double tolerance = heeksCAD->GetTolerance();

	if (GetStart(a).XYDistance(GetStart(b)) < tolerance) return(true);
	if (GetStart(a).XYDistance(GetEnd(b)) < tolerance) return(true);
	if (GetEnd(a).XYDistance(GetStart(b)) < tolerance) return(true);
	if (GetEnd(a).XYDistance(GetEnd(b)) < tolerance) return(true);

	return(false);
}


/**
    For these edges, plan a path to get the  tool from the current
    depth down to the first edge's depth at no more than the gradient
    specified for the  tool.  This might mean running right around
    all edges spiralling down until the required depth or it might mean
    running along part of the wire backwards and forwards until the depth
    is achieved.

    This routine should ramp the tool down along the edges until the tool's
    depth is at the starting edge's depth.  At the end of this routine, the
    tool's location should be at the starting point of the itStartingEdge
    so that it can progress, at this depth, right around the whole wire.
 */
/* static */ Python CContour::GenerateRampedEntry(
    ::size_t starting_edge_offset,
    std::vector<TopoDS_Edge> & edges,
	CMachineState *pMachineState,
	const double end_z )
{
    Python python;

    if (edges.size() == 0) return(python);  // Empty.
    if (starting_edge_offset == edges.size()-1) starting_edge_offset = 0;  // loop around if necessary.

    double tolerance = heeksCAD->GetTolerance();

    // Get the gradient from the  tool's definition.
    CTool *pTool = CTool::Find( pMachineState->Tool() );
    if (pTool == NULL)
    {
        return(python); // empty.
    }

    double gradient = pTool->Gradient();

    const TopoDS_Shape &first_edge = edges[starting_edge_offset];
    BRepAdaptor_Curve top_curve(TopoDS::Edge(first_edge));
    gp_Pnt point;
    top_curve.D0(top_curve.FirstParameter(),point);
    double initial_tool_depth = pMachineState->Location().Z();

    CNCPoint end_point(pMachineState->Location());
    end_point.SetZ(end_z); // This is where we want to be when we've finished.

    python << _T("comment('Ramp down to ") << end_z << _T(" at a gradient of ") << gradient << _T("')\n");

    int direction = +1;	// Move forwards through the list of edges (for now)

    if (pMachineState->Location().XYDistance(GetStart(edges[starting_edge_offset])) < pMachineState->Location().XYDistance(GetEnd(edges[starting_edge_offset])))
    {
        // The machine is closer to the starting point of this edge.  Find out whether the end point is closer
        // to the next edge or the previous edge.  Set the direction of edge iteration accordingly.
        int next_edge_offset = NextOffset(edges,starting_edge_offset,+1);
        int previous_edge_offset = NextOffset(edges,starting_edge_offset,-1);

        // We're going to end up at this edge's end point.
        CNCPoint reference_point(GetEnd(edges[starting_edge_offset]));
        double next_distance = reference_point.XYDistance(GetStart(edges[next_edge_offset]));
        if (reference_point.XYDistance(GetEnd(edges[next_edge_offset])) < next_distance)
        {
            next_distance = reference_point.XYDistance(GetEnd(edges[next_edge_offset]));
        }

        double previous_distance = reference_point.XYDistance(GetStart(edges[previous_edge_offset]));
        if (reference_point.XYDistance(GetEnd(edges[previous_edge_offset])) < previous_distance)
        {
            previous_distance = reference_point.XYDistance(GetEnd(edges[previous_edge_offset]));
        }

        if (next_distance < previous_edge_offset)
        {
            direction = +1;
        }
        else
        {
            direction = -1;
        }
    }
    else
    {
        // We're going to end up at this edge's start point.  See which direction the next nearest edge is from here.
        int next_edge_offset = NextOffset(edges,starting_edge_offset,+1);
        int previous_edge_offset = NextOffset(edges,starting_edge_offset,-1);

        // We're going to end up at this edge's end point.
        CNCPoint reference_point(GetStart(edges[starting_edge_offset]));
        double next_distance = reference_point.XYDistance(GetStart(edges[next_edge_offset]));
        if (reference_point.XYDistance(GetEnd(edges[next_edge_offset])) < next_distance)
        {
            next_distance = reference_point.Distance(GetEnd(edges[next_edge_offset]));
        }

        double previous_distance = reference_point.Distance(GetStart(edges[previous_edge_offset]));
        if (reference_point.XYDistance(GetEnd(edges[previous_edge_offset])) < previous_distance)
        {
            previous_distance = reference_point.XYDistance(GetEnd(edges[previous_edge_offset]));
        }

        if (next_distance < previous_edge_offset)
        {
            direction = +1;
        }
        else
        {
            direction = -1;
        }
    }

    // Ideally we would ramp down to half the step-down height in one direction and then come
    // back the other direction.  That way we would be back at the starting point by the
    // time we're at the step-down height.
    double half_way_down = ((initial_tool_depth - end_z) / 2.0) + end_z;

    double goal_z = end_z;

    // If the tool's diameter is too large for this gradient and step-down then don't go
    // half way and then back again.  Instead, go all the way out and come back at this
    // base height.
    if ((initial_tool_depth - (gradient * pTool->CuttingRadius() * 2.0)) > half_way_down)
    {
        goal_z = half_way_down;    // We're going to be moving more than one  tool radius so aim
                                    // for half way down and then double back to find final depth.
    }

    ::size_t edge_offset = starting_edge_offset;
    while (fabs(pMachineState->Location().Z() - end_z) > tolerance)
    {
        bool direction_has_changed = false;     // Avoid the double direction change.
        const TopoDS_Shape &E = edges[edge_offset];
        BRepAdaptor_Curve curve(TopoDS::Edge(E));
        double edge_length = GCPnts_AbscissaPoint::Length (curve);

        double distance_remaining = pMachineState->Location().Z() - goal_z;

        // We need to go distance_remaining over the edge_length at no more
        // than the gradient.  See if we can do that within this edge's length.
        // If so, figure out what point marks the necessary depth.
        double depth_possible_with_this_edge = gradient * edge_length * -1.0;   // positive number representing a depth (distance)
        if (distance_remaining <= depth_possible_with_this_edge)
        {
            // We don't need to traverse this whole edge before we're at depth.  Find
            // the point along this edge at which we will be at depth.
            // The FirstParameter() and the LastParameter() are not always lengths but they
            // do form a numeric representation of a distance along the element.  For lines
            // they are lengths and for arcs they are radians.  In any case, we can
            // use the proportion of the length to come up with a 'U' parameter for
            // this edge that indicates the point along the edge.

            if (pMachineState->Location().XYDistance(GetStart(edges[edge_offset])) < pMachineState->Location().XYDistance(GetEnd(edges[edge_offset])))
            {
                // We're going from FirstParameter() towards LastParameter()
                double proportion = distance_remaining / depth_possible_with_this_edge;
                double U = ((curve.LastParameter() - curve.FirstParameter()) * proportion) + curve.FirstParameter();

                // The point_at_full_depth indicates where we will be when we are at depth.  Run
                // along the edge down to this point
                python << GeneratePathForEdge( edges[edge_offset], curve.FirstParameter(), U, true, pMachineState, goal_z );

                if (DirectionTowarardsNextEdge( edges[edge_offset], edges[NextOffset(edges,edge_offset,(int) ((fabs(goal_z - end_z) > tolerance)?direction * -1.0:direction))] ))
                {
                    // We're heading towards the end of this edge.
                    python << GeneratePathForEdge( edges[edge_offset], U, curve.LastParameter(), true, pMachineState, goal_z );
                }
                else
                {
                    // We're heading towards the beginning of this edge.
                    python << GeneratePathForEdge( edges[edge_offset], U, curve.FirstParameter(), false, pMachineState, goal_z );
                }
            }
            else
            {
                // We're going from LastParameter() towards FirstParameter()
                double proportion = 1.0 - (distance_remaining / depth_possible_with_this_edge);
                double U = ((curve.LastParameter() - curve.FirstParameter()) * proportion) + curve.FirstParameter();

                // The point_at_full_depth indicates where we will be when we are at depth.  Run
                // along the edge down to this point
                python << GeneratePathForEdge( edges[edge_offset], curve.LastParameter(), U, false, pMachineState, goal_z );

                if (DirectionTowarardsNextEdge( edges[edge_offset], edges[NextOffset(edges,edge_offset,(int) ((fabs(goal_z - end_z) > tolerance)?direction * -1.0:direction))] ))
                {
                    // We're heading towards the end of this edge.
                    python << GeneratePathForEdge( edges[edge_offset], U, curve.LastParameter(), true, pMachineState, goal_z );
                }
                else
                {
                    // We're heading towards the beginning of this edge.
                    python << GeneratePathForEdge( edges[edge_offset], U, curve.FirstParameter(), false, pMachineState, goal_z );
                }
            }

            if (fabs(goal_z - end_z) > tolerance)
            {
                direction_has_changed = true;
                direction *= -1.0;  // Reverse direction
                goal_z = end_z;   // Changed goal.
            }
        }
        else
        {
            // This edge is not long enough for us to reach our goal depth.  Run all the way along this edge.
            double z_for_this_run = pMachineState->Location().Z() - depth_possible_with_this_edge;

            if (pMachineState->Location().XYDistance(GetStart(edges[edge_offset])) < pMachineState->Location().XYDistance(GetEnd(edges[edge_offset])))
            {
                python << GeneratePathForEdge( edges[edge_offset], curve.FirstParameter(), curve.LastParameter(), true, pMachineState, z_for_this_run );
            }
            else
            {
                python << GeneratePathForEdge( edges[edge_offset], curve.LastParameter(), curve.FirstParameter(), false, pMachineState, z_for_this_run );
            }
        }

        if (EdgesJoin( edges[edge_offset], edges[NextOffset(edges,edge_offset,direction)] ) == false)
        {
            if (! direction_has_changed)
            {
                // Reverse direction
                direction *= -1.0;
                direction_has_changed = true;
            }
        }
        else
        {
            // Step through the array of edges in the current direction (looping from end to start (or vice versa) if necessary)
            edge_offset = NextOffset(edges,edge_offset,direction);
        }
    } // while

    while (pMachineState->Location() != end_point)
    {
        const TopoDS_Shape &E = edges[edge_offset];
        BRepAdaptor_Curve curve(TopoDS::Edge(E));

        if (pMachineState->Location().XYDistance(GetStart(edges[edge_offset])) < pMachineState->Location().XYDistance(GetEnd(edges[edge_offset])))
        {
            python << GeneratePathForEdge( edges[edge_offset], curve.FirstParameter(), curve.LastParameter(), true, pMachineState, pMachineState->Location().Z() );
        }
        else
        {
            python << GeneratePathForEdge( edges[edge_offset], curve.LastParameter(), curve.FirstParameter(), false, pMachineState, pMachineState->Location().Z() );
        }

        if (EdgesJoin( edges[edge_offset], edges[NextOffset(edges,edge_offset,direction)] ) == false)
        {
            // Reverse direction
            direction *= -1.0;
        }
        else
        {
            edge_offset = NextOffset(edges,edge_offset,direction);
        }
    } // End while

    python << _T("comment('end ramp')\n");
	return(python);
}


/**
    The min_gradient is the minimum gradient the toolpath may follow before reaching the required depth.
    The gradient being a measurement of rise over run.  Since we're moving downwards, the 'rise' value
    will always be negative; producing a negative gradient.  A min_gradient of STRAIGHT_PLUNGE (special
    double value) indicates that ramping is not required.
 */

/* static */ Python CContour::GeneratePathFromWire(
	const TopoDS_Wire & wire,
	CMachineState *pMachineState,       // for both fixture and last_position.
	const double clearance_height,
	const double rapid_down_to_height,
	const double start_depth,
	const CContourParams::EntryMove_t entry_move_type )
{
	Python python;

    std::vector<TopoDS_Edge> edges = SortEdges(wire);

    for (std::vector<TopoDS_Edge>::size_type i=0; i<edges.size(); i++)
	{
		const TopoDS_Shape &E = edges[i];

		BRepAdaptor_Curve curve(TopoDS::Edge(E));

		double uStart = curve.FirstParameter();
		double uEnd = curve.LastParameter();
		gp_Pnt PS;
		gp_Vec VS;
		curve.D1(uStart, PS, VS);
		gp_Pnt PE;
		gp_Vec VE;
		curve.D1(uEnd, PE, VE);

		if (pMachineState->Location() == CNCPoint(PS))
		{
			// We're heading towards the PE point.
			python << GeneratePathForEdge(edges[i], uStart, uEnd, true, pMachineState, PE.Z());
		} // End if - then
		else if (pMachineState->Location() == CNCPoint(PE))
		{
			// We're goint towards PS
			python << GeneratePathForEdge(edges[i], uEnd, uStart, false, pMachineState, PS.Z());
		}
		else
		{
			// We need to move to the start BEFORE machining this line.
			CNCPoint start(PS);
			CNCPoint end(PE);
			bool isForwards = true;

			if (i < (edges.size()-1))
			{
                if (! DirectionTowarardsNextEdge( edges[i], edges[i+1] ))
                {
                    // The next edge is closer to this edge's start point.  reverse direction
                    // so that the next movement is better.

                    double temp = uStart;
					uStart = uEnd;
					uEnd = temp;

					CNCPoint temppnt(start);
					start = end;
					end = temppnt;

					isForwards = false;
                }
			}

            // If the tool is directly above the beginning of the next edge, we don't want
            // to move to clearance height and rapid across to it.
            CNCPoint s(start); s.SetZ(0.0); // Flatten start
            CNCPoint l(pMachineState->Location()); l.SetZ(0.0);
            if (s != l)
            {
                // Move up above workpiece to relocate to the start of the next edge.
                python << _T("rapid(z=") << clearance_height / theApp.m_program->m_units << _T(")\n");
                python << _T("rapid(x=") << start.X(true) << _T(", y=") << start.Y(true) << _T(")\n");
                python << _T("rapid(z=") << rapid_down_to_height / theApp.m_program->m_units << _T(")\n");
                python << _T("feed(z=") << start_depth / theApp.m_program->m_units << _T(")\n");
                CNCPoint where(start);
                where.SetZ(start_depth / theApp.m_program->m_units);
                pMachineState->Location(where);
            }

            switch (entry_move_type)
            {
                case CContourParams::ePlunge:
                    python << _T("feed(z=") << start.Z(true) << _T(")\n");
                    pMachineState->Location().SetZ( start.Z() );;
                    break;

                case CContourParams::eRamp:
                {
                    python << GenerateRampedEntry( i, edges, pMachineState, start.Z() );
                }
                    break;
            } // End switch

            python << GeneratePathForEdge(edges[i], uStart, uEnd, isForwards, pMachineState, PE.Z());
		}
	}

	return(python);
}



/**
    We want to move the  tool along this edge.  If the tool's current
    position aligns with either the first_parameter's location or the last_parameter's
    location then run to the opposite end.  If the tool's location does not align
    with either of these locations then go from first to last.
    NOTE: Ignore the Z value when looking for these alignments.
 */
/* static */ Python CContour::GeneratePathForEdge(
	const TopoDS_Edge &edge,
	const double first_parameter,
	const double last_parameter,
	const bool direction,
	CMachineState *pMachineState,
	const double end_z )
{
	Python python;

	double tolerance = heeksCAD->GetTolerance();

	BRepAdaptor_Curve curve(edge);
	GeomAbs_CurveType curve_type = curve.GetType();

	double uStart = first_parameter;
    double uEnd = last_parameter;
    gp_Pnt PS;
    gp_Vec VS;
    curve.D1(uStart, PS, VS);
    PS.SetZ(pMachineState->Location().Z());
    gp_Pnt PE;
    gp_Vec VE;
    curve.D1(uEnd, PE, VE);
    PE.SetZ(end_z);

    bool forwards = direction;

    if (pMachineState->Location() == CNCPoint(PE))
    {
        // We are already at the end point.  Just return.
        return python;
    }

  	switch(curve_type)
	{
		case GeomAbs_Line:
			// make a line
		{
			gp_Pnt PS;
			gp_Vec VS;
			curve.D1(uStart, PS, VS);
			PS.SetZ(pMachineState->Location().Z());
			gp_Pnt PE;
			gp_Vec VE;
			curve.D1(uEnd, PE, VE);
			PE.SetZ(end_z);

			std::list<CNCPoint> points;
			points.push_back(PS);
			points.push_back(PE);

			for (std::list<CNCPoint>::iterator itPoint = points.begin(); itPoint != points.end(); itPoint++)
            {
                if (itPoint->Distance(pMachineState->Location()) > tolerance)
                {
                    python << _T("feed(x=") << itPoint->X(true) << _T(", y=") << itPoint->Y(true) << _T(", z=") << itPoint->Z(true) << _T(")\n");
                    pMachineState->Location(*itPoint);
                }
            } // End for
		}
		break;

		case GeomAbs_Circle:
		if ((pMachineState->Fixture().m_params.m_xz_plane == 0.0) && (pMachineState->Fixture().m_params.m_yz_plane == 0.0))
		{
			gp_Pnt PS;
			gp_Vec VS;
			curve.D1(uStart, PS, VS);
			gp_Pnt PE;
			gp_Vec VE;
			curve.D1(uEnd, PE, VE);
			gp_Circ circle = curve.Circle();

            // Arc towards PE
            CNCPoint point(PE);
            CNCPoint centre( circle.Location() );
            bool l_bClockwise = Clockwise(circle);
            if (! forwards) l_bClockwise = ! l_bClockwise;

            std::list<CNCPoint> points;
            double period = curve.Period();
            double u = uStart;
			if (uStart < uEnd)
			{
				double step_down = (pMachineState->Location().Z() - end_z) / ((uEnd - uStart) / (period/4.0));
				double z = pMachineState->Location().Z();

				for (u = uStart; u <= uEnd; u += (period/4.0))
				{
					gp_Pnt p;
					gp_Vec v;
					curve.D1(u, p, v);
					p.SetZ(z);
					z -= step_down;
					points.push_back( p );
				}
			}
			else
			{
				double step_down = (pMachineState->Location().Z() - end_z) / ((uStart - uEnd) / (period/4.0));
				double z = pMachineState->Location().Z();

				for (u = uStart; u >= uEnd; u -= (period/4.0))
				{
					gp_Pnt p;
					gp_Vec v;
					curve.D1(u, p, v);
					p.SetZ(z);
					z -= step_down;
					points.push_back( p );
				}
				// l_bClockwise = ! l_bClockwise;
			}

            if ((points.size() > 0) && (*points.rbegin() != CNCPoint(PE)))
            {
                CNCPoint point(PE);
                point.SetZ(end_z);
                points.push_back( point );
            }

            for (std::list<CNCPoint>::iterator itPoint = points.begin(); itPoint != points.end(); itPoint++)
            {
                if (itPoint->Distance(pMachineState->Location()) > tolerance)
                {
                    CNCPoint offset = centre - pMachineState->Location();
                    python << (l_bClockwise?_T("arc_cw("):_T("arc_ccw(")) << _T("x=") << itPoint->X(true) << _T(", y=") << itPoint->Y(true) << _T(", z=") << itPoint->Z(true) << _T(", ")
                        << _T("i=") << offset.X(true) << _T(", j=") << offset.Y(true);
                    // if (offset.Z(true) > tolerance) python << _T(", k=") << offset.Z(true);
                    python << _T(")\n");
                    pMachineState->Location(*itPoint);
                }
            } // End for

            break; // Allow the circle option to fall through to the 'default' option if
                    // there is any rotation in the YZ and/or XZ planes.
        }

		default:
		{
			// make lots of small lines
			BRepBuilderAPI_MakeEdge edge_builder(curve.Curve().Curve(), uStart, uEnd);
			TopoDS_Edge edge(edge_builder.Edge());

			BRepTools::Clean(edge);
			BRepMesh::Mesh(edge, max_deviation_for_spline_to_arc);

			TopLoc_Location L;
			Handle(Poly_Polygon3D) Polyg = BRep_Tool::Polygon3D(edge, L);
			if (!Polyg.IsNull()) {
				const TColgp_Array1OfPnt& Points = Polyg->Nodes();
				Standard_Integer po;
				int i = 0;
				std::list<CNCPoint> interpolated_points;
				for (po = Points.Lower(); po <= Points.Upper(); po++, i++) {
					CNCPoint p = (Points.Value(po)).Transformed(L);
					interpolated_points.push_back(p);
				} // End for

				// See if we should go from the start to the end or the end to the start.
				if (first_parameter > last_parameter)
				{
					// We need to go from the end to the start.  Reverse the point locations to
					// make this easier.

					interpolated_points.reverse();
				} // End if - then


				double step_down = (pMachineState->Location().Z() - end_z) / interpolated_points.size();
				double z = pMachineState->Location().Z();
				for (std::list<CNCPoint>::iterator itPoint = interpolated_points.begin(); itPoint != interpolated_points.end(); itPoint++)
				{
					itPoint->SetZ( z );
					z -= step_down;

					if (*itPoint != pMachineState->Location())
					{
						python << _T("feed(x=") << itPoint->X(true) << _T(", y=") << itPoint->Y(true) << _T(", z=") << itPoint->Z(true) << _T(")\n");
						pMachineState->Location(*itPoint);
					} // End if - then
				} // End for
			} // End if - then
		}
		break;
	} // End switch

	return(python);
}




/**
	This method is called when the CAD operator presses the Python button.  This method generates
	Python source code whose job will be to generate RS-274 GCode.  It's done in two steps so that
	the Python code can be configured to generate GCode suitable for various CNC interpreters.
 */
Python CContour::AppendTextToProgram( CMachineState *pMachineState )
{
	Python python;

	ReloadPointers();

	python << CDepthOp::AppendTextToProgram( pMachineState );

	unsigned int number_of_bad_sketches = 0;
	double tolerance = heeksCAD->GetTolerance();

	CTool *pTool = CTool::Find( m_tool_number );
	if (! pTool)
	{
		return(python);
	}

    for (HeeksObj *object = GetFirstChild(); object != NULL; object = GetNextChild())
    {
        std::list<TopoDS_Shape> wires;
        if (! heeksCAD->ConvertSketchToFaceOrWire( object, wires, false))
        {
            number_of_bad_sketches++;
        } // End if - then
        else
        {
            // The wire(s) represent the sketch objects for a tool path.
            if (object->GetShortString() != NULL)
            {
                python << _T("comment(") << PythonString(object->GetShortString()).c_str() << _T(")\n");
            }

            try {
                for(std::list<TopoDS_Shape>::iterator It2 = wires.begin(); It2 != wires.end(); It2++)
                {
                    TopoDS_Shape& wire_to_fix = *It2;
                    ShapeFix_Wire fix;
                    fix.Load( TopoDS::Wire(wire_to_fix) );
                    fix.FixReorder();

                    TopoDS_Shape wire = fix.Wire();

                    BRepBuilderAPI_Transform transform1(pMachineState->Fixture().GetMatrix(CFixture::YZ));
                    transform1.Perform(wire, false);
                    wire = transform1.Shape();

                    BRepBuilderAPI_Transform transform2(pMachineState->Fixture().GetMatrix(CFixture::XZ));
                    transform2.Perform(wire, false);
                    wire = transform2.Shape();

                    BRepBuilderAPI_Transform transform3(pMachineState->Fixture().GetMatrix(CFixture::XY));
                    transform3.Perform(wire, false);
                    wire = transform3.Shape();

                    BRepOffsetAPI_MakeOffset offset_wire(TopoDS::Wire(wire));

                    // Now generate a toolpath along this wire.
                    std::list<double> depths = GetDepths();

                    for (std::list<double>::iterator itDepth = depths.begin(); itDepth != depths.end(); itDepth++)
                    {
                        double radius = pTool->CuttingRadius(false,m_depth_op_params.m_start_depth - *itDepth);

                        if (m_params.m_tool_on_side == CContourParams::eLeftOrOutside) radius *= +1.0;
                        if (m_params.m_tool_on_side == CContourParams::eRightOrInside) radius *= -1.0;
                        if (m_params.m_tool_on_side == CContourParams::eOn) radius = 0.0;

                        TopoDS_Wire tool_path_wire(TopoDS::Wire(wire));

                        double offset = radius;
                        if (offset < 0) offset *= -1.0;

                        if (offset > tolerance)
                        {
                            offset_wire.Perform(radius);
                            if (! offset_wire.IsDone())
                            {
                                break;
                            }
                            tool_path_wire = TopoDS::Wire(offset_wire.Shape());
                        }

                        if ((m_params.m_tool_on_side == CContourParams::eOn) || (offset > tolerance))
                        {
                            gp_Trsf matrix;

                            matrix.SetTranslation( gp_Vec( gp_Pnt(0,0,0), gp_Pnt( 0,0,*itDepth)));
                            BRepBuilderAPI_Transform transform(matrix);
                            transform.Perform(tool_path_wire, false); // notice false as second parameter
                            tool_path_wire = TopoDS::Wire(transform.Shape());

                            python << GeneratePathFromWire(	tool_path_wire,
                                                            pMachineState,
                                                            m_depth_op_params.m_clearance_height,
                                                            m_depth_op_params.m_rapid_safety_space,
                                                            m_depth_op_params.m_start_depth,
                                                            m_params.m_entry_move_type );
                        } // End if - then
                    } // End for
                } // End for
            } // End try
            catch (Standard_Failure & error) {
                (void) error;	// Avoid the compiler warning.
                Handle_Standard_Failure e = Standard_Failure::Caught();
                number_of_bad_sketches++;
            } // End catch
        } // End if - else
    } // End for

    if (pMachineState->Location().Z() < (m_depth_op_params.m_clearance_height / theApp.m_program->m_units))
    {
        // Move up above workpiece to relocate to the start of the next operation.
        python << _T("rapid(z=") << m_depth_op_params.m_clearance_height / theApp.m_program->m_units << _T(")\n");

        CNCPoint where(pMachineState->Location());
        where.SetZ(m_depth_op_params.m_clearance_height / theApp.m_program->m_units);
        pMachineState->Location(where);
    }

    if (number_of_bad_sketches > 0)
    {
        wxString message;
        message << _("Failed to create contours around ") << number_of_bad_sketches << _(" sketches");
        wxMessageBox(message);
    }

	return(python);
}




/**
	This is the Graphics Library Commands (from the OpenGL set).  This method calls the OpenGL
	routines to paint the drill action in the graphics window.  The graphics is transient.

	Part of its job is to re-paint the elements that this CContour object refers to so that
	we know what CAD objects this CNC operation is referring to.
 */
void CContour::glCommands(bool select, bool marked, bool no_color)
{
	CDepthOp::glCommands( select, marked, no_color );
}




void CContour::GetProperties(std::list<Property *> *list)
{
	m_params.GetProperties(this, list);
	CDepthOp::GetProperties(list);
}

HeeksObj *CContour::MakeACopy(void)const
{
	return new CContour(*this);
}

void CContour::CopyFrom(const HeeksObj* object)
{
	operator=(*((CContour*)object));
}

bool CContour::CanAddTo(HeeksObj* owner)
{
	return ((owner != NULL) && (owner->GetType() == OperationsType));
}

void CContour::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element = new TiXmlElement( "Contour" );
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
HeeksObj* CContour::ReadFromXMLElement(TiXmlElement* element)
{
	CContour* new_object = new CContour;
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
std::list<wxString> CContour::DesignRulesAdjustment(const bool apply_changes)
{
	std::list<wxString> changes;

	return(changes);

} // End DesignRulesAdjustment() method


/**
    This method returns TRUE if the type of symbol is suitable for reference as a source of location
 */
bool CContour::CanAdd( HeeksObj *object )
{
    if (object == NULL) return(false);

    switch (object->GetType())
    {
        case CircleType:
        case SketchType:
		case LineType:
		case FixtureType:
            return(true);

        default:
            return(false);
    }
}


void CContour::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
    CDepthOp::GetTools( t_list, p );
}

static void on_set_spline_deviation(double value, HeeksObj* object){
	CContour::max_deviation_for_spline_to_arc = value;
	CContour::WriteToConfig();
}

// static
void CContour::GetOptions(std::list<Property *> *list)
{
	list->push_back ( new PropertyDouble ( _("Contour spline deviation"), max_deviation_for_spline_to_arc, NULL, on_set_spline_deviation ) );
}


void CContour::ReloadPointers()
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


CContour::CContour( const CContour & rhs ) : CDepthOp( rhs )
{
    m_params = rhs.m_params;
    m_symbols.clear();
	std::copy( rhs.m_symbols.begin(), rhs.m_symbols.end(), std::inserter( m_symbols, m_symbols.begin() ) );
}

CContour & CContour::operator= ( const CContour & rhs )
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

bool CContour::operator==( const CContour & rhs ) const
{
	if (m_params != rhs.m_params) return(false);

	return(CDepthOp::operator==(rhs));
}

bool CContour::IsDifferent(HeeksObj* other)
{
	return(! (*this == (*((CContour *) other))));
}

