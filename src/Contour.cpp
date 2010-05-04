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
#include "CuttingTool.h"
#include "Profile.h"
#include "Fixture.h"
#include "CNCPoint.h"
#include "PythonStuff.h"

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

extern CHeeksCADInterface* heeksCAD;

/* static */ double CContour::max_deviation_for_spline_to_arc = 0.1;

void CContourParams::set_initial_values()
{
	CNCConfig config(ConfigPrefix());
	config.Read(_T("ToolOnSide"), (int *) &m_tool_on_side, (int) eOn);
}

void CContourParams::write_values_to_config()
{
	CNCConfig config(ConfigPrefix());
	config.Write(_T("ToolOnSide"), (int) m_tool_on_side);
}

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

void CContourParams::GetProperties(CContour* parent, std::list<Property *> *list)
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

void CContourParams::WriteXMLAttributes(TiXmlNode *root)
{
	TiXmlElement * element;
	element = new TiXmlElement( "params" );
	root->LinkEndChild( element );

	element->SetAttribute("side", m_tool_on_side);
}

void CContourParams::ReadParametersFromXMLElement(TiXmlElement* pElem)
{
	int int_for_enum;
	if(pElem->Attribute("side", &int_for_enum))m_tool_on_side = (eSide)int_for_enum;
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


/* static */ gp_Pnt CContour::GetStart(const TopoDS_Edge &edge)
{
    BRepAdaptor_Curve curve(edge);
    double uStart = curve.FirstParameter();
    gp_Pnt PS;
    gp_Vec VS;
    curve.D1(uStart, PS, VS);

    return(PS);
}

/* static */ gp_Pnt CContour::GetEnd(const TopoDS_Edge &edge)
{
    BRepAdaptor_Curve curve(edge);
    double uEnd = curve.LastParameter();
    gp_Pnt PE;
    gp_Vec VE;
    curve.D1(uEnd, PE, VE);

    return(PE);
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
    if (GetStart(from).Distance( GetEnd( to )) < min_distance)
    {
        min_distance = GetStart( from ).Distance( GetEnd( to ));
        direction = backwards;
    }

    if (GetEnd(from).Distance( GetEnd( to )) < min_distance)
    {
        min_distance = GetEnd(from).Distance( GetEnd( to ));
        direction = forwards;
    }

    if (GetStart(from).Distance( GetStart( to )) < min_distance)
    {
        min_distance = GetStart(from).Distance( GetStart( to ));
        direction = backwards;
    }

    if (GetEnd(from).Distance( GetStart( to )) < min_distance)
    {
        min_distance = GetEnd(from).Distance( GetStart( to ));
        direction = forwards;
    }

    return(direction);
}






/* static */ wxString CContour::GeneratePathFromWire(
	const TopoDS_Wire & wire,
	CNCPoint & last_position,
	const CFixture *pFixture,
	const double clearance_height,
	const double rapid_down_to_height )
{
#ifdef UNICODE
	std::wostringstream gcode;
#else
    std::ostringstream gcode;
#endif
    gcode.imbue(std::locale("C"));
	gcode<<std::setprecision(10);

	double tolerance = heeksCAD->GetTolerance();

    std::vector<TopoDS_Edge> edges = SortEdges(wire);

    for (std::vector<TopoDS_Edge>::size_type i=0; i<edges.size(); i++)
	{
		const TopoDS_Shape &E = edges[i];

		// enum GeomAbs_CurveType
		// 0 - GeomAbs_Line
		// 1 - GeomAbs_Circle
		// 2 - GeomAbs_Ellipse
		// 3 - GeomAbs_Hyperbola
		// 4 - GeomAbs_Parabola
		// 5 - GeomAbs_BezierCurve
		// 6 - GeomAbs_BSplineCurve
		// 7 - GeomAbs_OtherCurve

		BRepAdaptor_Curve curve(TopoDS::Edge(E));
		GeomAbs_CurveType curve_type = curve.GetType();

		switch(curve_type)
		{
			case GeomAbs_Line:
				// make a line
			{
				double uStart = curve.FirstParameter();
				double uEnd = curve.LastParameter();
				gp_Pnt PS;
				gp_Vec VS;
				curve.D1(uStart, PS, VS);
				gp_Pnt PE;
				gp_Vec VE;
				curve.D1(uEnd, PE, VE);

				if (last_position == CNCPoint(PS))
				{
					// We're heading towards the PE point.
					CNCPoint point(PE);
					gcode << _T("feed(x=") << point.X(true) << _T(", y=") << point.Y(true) << _T(", z=") << point.Z(true) << _T(")\n");
					last_position = point;
				} // End if - then
				else if (last_position == CNCPoint(PE))
				{
					CNCPoint point(PS);
					gcode << _T("feed(x=") << point.X(true) << _T(", y=") << point.Y(true) << _T(", z=") << point.Z(true) << _T(")\n");
					last_position = point;
				}
				else
				{
					// We need to move to the start BEFORE machining this line.
					CNCPoint start(PS);
					CNCPoint end(PE);

					if (i < (edges.size()-1))
					{
                        if (! DirectionTowarardsNextEdge( edges[i], edges[i+1] ))
                        {
                            // The next edge is closer to this edge's start point.  reverse direction
                            // so that the next movement is better.

                            CNCPoint temp = start;
                            start = end;
                            end = temp;
                        }
					}

					gcode << _T("rapid(z=") << clearance_height / theApp.m_program->m_units << _T(")\n");
					gcode << _T("rapid(x=") << start.X(true) << _T(", y=") << start.Y(true) << _T(")\n");
					gcode << _T("rapid(z=") << rapid_down_to_height / theApp.m_program->m_units << _T(")\n");
					gcode << _T("feed(z=") << start.Z(true) << _T(")\n");

					gcode << _T("feed(x=") << end.X(true) << _T(", y=") << end.Y(true) << _T(", z=") << end.Z(true) << _T(")\n");
					last_position = end;
				}
			}
			break;

            /*
            // It would be great if we could do the BiArc movements along the spline that Dan put in for
            // Profile operations.  I will get there eventually.
            case GeomAbs_BSplineCurve:
            */


			case GeomAbs_Circle:
			if ((pFixture->m_params.m_xz_plane == 0.0) && (pFixture->m_params.m_yz_plane == 0.0))
			{
				double uStart = curve.FirstParameter();
				double uEnd = curve.LastParameter();
				gp_Pnt PS;
				gp_Vec VS;
				curve.D1(uStart, PS, VS);
				gp_Pnt PE;
				gp_Vec VE;
				curve.D1(uEnd, PE, VE);
				gp_Circ circle = curve.Circle();


                // It's an Arc.

                if (last_position == CNCPoint(PS))
                {
                    // Arc towards PE
                    CNCPoint point(PE);
                    CNCPoint centre( circle.Location() );
                    bool l_bClockwise = Clockwise(circle);

                     std::list<CNCPoint> points;
                    double period = curve.Period();
                    double u = uStart;
                    for (u = uStart; u <= uEnd; u += (period/4.0))
                    {
                        gp_Pnt p;
                        gp_Vec v;
                        curve.D1(u, p, v);
                        points.push_back( p );
                    }
                    if (*points.rbegin() != CNCPoint(PE))
                    {
                        points.push_back( CNCPoint(PE) );
                    }

                    for (std::list<CNCPoint>::iterator itPoint = points.begin(); itPoint != points.end(); itPoint++)
                    {
                        if (itPoint->Distance(last_position) > tolerance)
                        {
                            CNCPoint offset = centre - last_position;
                            gcode << (l_bClockwise?_T("arc_cw("):_T("arc_ccw(")) << _T("x=") << itPoint->X(true) << _T(", y=") << itPoint->Y(true) << _T(", z=") << itPoint->Z(true) << _T(", ")
                                << _T("i=") << offset.X(true) << _T(", j=") << offset.Y(true);
                            if (offset.Z(true) > tolerance) gcode << _T(", k=") << offset.Z(true);
                            gcode << _T(")\n");
                            last_position = *itPoint;
                        }
                    } // End for
                }
                else if (last_position == CNCPoint(PE))
                {
                    // Arc towards PS
                    CNCPoint point(PS);
                    CNCPoint centre( circle.Location() );
                    bool l_bClockwise = ! Clockwise(circle);

                    std::list<CNCPoint> points;
                    double period = curve.Period();
                    for (double u = uEnd; u >= uStart; u -= (period/4.0))
                    {
                        gp_Pnt p;
                        gp_Vec v;
                        curve.D1(u, p, v);
                        points.push_back( p );
                    }
                    if (*points.rbegin() != CNCPoint(PS))
                    {
                        points.push_back( CNCPoint(PS) );
                    }

                    for (std::list<CNCPoint>::iterator itPoint = points.begin(); itPoint != points.end(); itPoint++)
                    {
                        if (itPoint->Distance(last_position) > tolerance)
                        {
                            CNCPoint offset = centre - last_position;

                            gcode << (l_bClockwise?_T("arc_cw("):_T("arc_ccw(")) << _T("x=") << itPoint->X(true) << _T(", y=") << itPoint->Y(true) << _T(", z=") << itPoint->Z(true) << _T(", ")
                                << _T("i=") << offset.X(true) << _T(", j=") << offset.Y(true);
                            if (offset.Z(true) > tolerance) gcode << _T(", k=") << offset.Z(true);
                            gcode << _T(")\n");
                            last_position = *itPoint;
                        }
                    } // End for
                }
                else
                {
                    // Move to PS first.
                    std::list<CNCPoint> points;
                    double period = curve.Period();

                    CNCPoint start(PS);
                    CNCPoint end(PE);
                    CNCPoint centre( circle.Location() );
                    bool l_bClockwise = Clockwise(circle);

                    for (double u = uStart; u <= uEnd; u += (period/4.0))
                    {
                        gp_Pnt p;
                        gp_Vec v;
                        curve.D1(u, p, v);
                        points.push_back( p );
                    }
                    if (*points.rbegin() != CNCPoint(PE))
                    {
                        points.push_back( CNCPoint(PE) );
                    }

                    if (i < (edges.size()-1))
                    {
                        if (! DirectionTowarardsNextEdge( edges[i], edges[i+1] ))
                        {
                            // The next edge is closer to this edge's start point.  reverse direction
                            // so that the next movement is better.

                            CNCPoint temp = start;
                            start = end;
                            end = temp;
                            l_bClockwise = ! l_bClockwise;

                            points.clear();
                            for (double u = uEnd; u >= uStart; u -= (period/4.0))
                            {
                                gp_Pnt p;
                                gp_Vec v;
                                curve.D1(u, p, v);
                                points.push_back( p );
                            }
                            if (*points.rbegin() != CNCPoint(PS))
                            {
                                points.push_back( CNCPoint(PS) );
                            }
                        }
                    }

					gcode << _T("rapid(z=") << clearance_height / theApp.m_program->m_units << _T(")\n");
                    gcode << _T("rapid(x=") << points.begin()->X(true) << _T(", y=") << points.begin()->Y(true) << _T(")\n");
                    gcode << _T("rapid(z=") << rapid_down_to_height / theApp.m_program->m_units << _T(")\n");
                    gcode << _T("feed(z=") << points.begin()->Z(true) << _T(")\n");

                    last_position = *(points.begin());

                    for (std::list<CNCPoint>::iterator itPoint = points.begin(); itPoint != points.end(); itPoint++)
                    {
                        if (itPoint->Distance(last_position) > tolerance)
                        {
                            CNCPoint offset = centre - last_position;

                            gcode << (l_bClockwise?_T("arc_cw("):_T("arc_ccw(")) << _T("x=") << itPoint->X(true) << _T(", y=") << itPoint->Y(true) << _T(", z=") << itPoint->Z(true) << _T(", ")
                                << _T("i=") << offset.X(true) << _T(", j=") << offset.Y(true);
                            if (offset.Z(true) > tolerance) gcode << _T(", k=") << offset.Z(true);
                            gcode << _T(")\n");
                            last_position = *itPoint;
                        }
                    } // End for
                }
				break;
			}
			else
			{
			    // We've rotated the arcs in either the xz or yz planes.  The GCode will produce a helical arc in these
			    // situations while we need a rotated arc.  They're not quite the same.  To that end, we will produce
			    // a more accurate representation by following the arcs with small lines.  Fall through to the 'default'
			    // option which will stroke the arcs into small lines.
			}

			default:
			{
				// make lots of small lines
				double uStart = curve.FirstParameter();
				double uEnd = curve.LastParameter();
				gp_Pnt PS;
				gp_Vec VS;
				curve.D1(uStart, PS, VS);
				gp_Pnt PE;
				gp_Vec VE;
				curve.D1(uEnd, PE, VE);

				TopoDS_Edge edge(TopoDS::Edge(E));
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
					if (*interpolated_points.rbegin() == last_position)
					{
						// We need to go from the end to the start.  Reverse the point locations to
						// make this easier.

						interpolated_points.reverse();
					} // End if - then

					if (*interpolated_points.begin() != last_position)
					{
						// This curve is not nearby to the last_position.  Rapid to the start
						// point to start this off.

						// We need to move to the start BEFORE machining this line.
						CNCPoint start(last_position);
						CNCPoint end(*interpolated_points.begin());

						gcode << _T("rapid(z=") << clearance_height / theApp.m_program->m_units << _T(")\n");
						gcode << _T("rapid(x=") << end.X(true) << _T(", y=") << end.Y(true) << _T(")\n");
						gcode << _T("rapid(z=") << rapid_down_to_height / theApp.m_program->m_units << _T(")\n");
						gcode << _T("feed(z=") << end.Z(true) << _T(")\n");

						last_position = end;
					}

					for (std::list<CNCPoint>::iterator itPoint = interpolated_points.begin(); itPoint != interpolated_points.end(); itPoint++)
					{
						if (*itPoint != last_position)
						{
							gcode << _T("feed(x=") << itPoint->X(true) << _T(", y=") << itPoint->Y(true) << _T(", z=") << itPoint->Z(true) << _T(")\n");
							last_position = *itPoint;							
						} // End if - then
					} // End for
				} // End if - then
			}
			break;
		} // End switch
	}

	gcode << _T("rapid(z=") << clearance_height / theApp.m_program->m_units << _T(")\n");
	last_position.SetZ( clearance_height );

	return(wxString(gcode.str().c_str()));
}



/**
	This method is called when the CAD operator presses the Python button.  This method generates
	Python source code whose job will be to generate RS-274 GCode.  It's done in two steps so that
	the Python code can be configured to generate GCode suitable for various CNC interpreters.
 */
void CContour::AppendTextToProgram( const CFixture *pFixture )
{
	ReloadPointers();

#ifdef UNICODE
	std::wostringstream gcode;
#else
    std::ostringstream gcode;
#endif
    gcode.imbue(std::locale("C"));
	gcode<<std::setprecision(10);

	CDepthOp::AppendTextToProgram( pFixture );

	unsigned int number_of_bad_sketches = 0;
	double tolerance = heeksCAD->GetTolerance();

	CCuttingTool *pCuttingTool = CCuttingTool::Find( m_cutting_tool_number );
	if (! pCuttingTool)
	{
		return;
	}

    CNCPoint last_position(0.0, 0.0, 0.0);

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
                gcode << _T("comment(") << PythonString(object->GetShortString()).c_str() << _T(")\n");
            }

            try {
                for(std::list<TopoDS_Shape>::iterator It2 = wires.begin(); It2 != wires.end(); It2++)
                {
                    TopoDS_Shape& wire_to_fix = *It2;
                    ShapeFix_Wire fix;
                    fix.Load( TopoDS::Wire(wire_to_fix) );
                    fix.FixReorder();

                    TopoDS_Shape wire = fix.Wire();

                    BRepBuilderAPI_Transform transform(pFixture->GetMatrix());
                    transform.Perform(wire, false);
                    wire = transform.Shape();

                    BRepOffsetAPI_MakeOffset offset_wire(TopoDS::Wire(wire));

                    // Now generate a toolpath along this wire.
                    std::list<double> depths = GetDepths();

                    for (std::list<double>::iterator itDepth = depths.begin(); itDepth != depths.end(); itDepth++)
                    {
                        double radius = pCuttingTool->CuttingRadius(false,m_depth_op_params.m_start_depth - *itDepth);

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

                            gcode << GeneratePathFromWire(	tool_path_wire,
                                                            last_position,
                                                            pFixture,
                                                            m_depth_op_params.m_clearance_height,
                                                            m_depth_op_params.m_rapid_down_to_height ).c_str();
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

	theApp.m_program_canvas->m_textCtrl->AppendText(gcode.str().c_str());
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
	return owner->GetType() == OperationsType;
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
    switch (object->GetType())
    {
        case CircleType:
        case SketchType:
		case LineType:
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
		if (CanAdd(object))
		{
			Add( object, NULL );
		}
	}

	m_symbols.clear();
}


CContour::CContour( const CContour & rhs ) : CDepthOp( rhs )
{
    m_params.set_initial_values();
	*this = rhs;	// Call the assignment operator.
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


