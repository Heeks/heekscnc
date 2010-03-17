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

extern CHeeksCADInterface* heeksCAD;


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



void CContourParams::GetProperties(CContour* parent, std::list<Property *> *list)
{
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
    int int_for_enum = m_tool_on_side;
	pElem->Attribute("side", &int_for_enum);
	m_tool_on_side = (eSide)int_for_enum;
}


/* static */ bool CContour::Clockwise( const gp_Circ & circle )
{
	return(circle.Axis().Direction().Z() < 0);
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


typedef struct EdgeComparison : public binary_function<const TopoDS_Edge &, const TopoDS_Edge &, bool >
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

std::vector<TopoDS_Edge> CContour::SortEdges( const TopoDS_Wire & wire ) const
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
            // It's the first point.  Reference this to zero so that the order makes some sense.  It would
            // be nice, eventually, to have this first reference point be the last point produced by the
            // previous NC operation.  i.e. where the last operation left off, we should start drilling close
            // by.

            EdgeComparison compare( *(edges.begin()) );
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




wxString CContour::GeneratePathFromWire( const TopoDS_Wire & wire, CNCPoint & last_position ) const
{
	wxString gcode;
	double tolerance = heeksCAD->GetTolerance();

    /*
    ShapeFix_Wire fixWire;
    fixWire.Load(wire);
    fixWire.FixReorder();
    // fixWire.FixConnected(); //to ensure vertices are shared btw edges
    // fixWire.FixClosed();
    bool performed = fixWire.Perform();

    // TopoDS_Wire profileWire = fixWire.WireAPIMake();
    TopoDS_Wire profileWire = fixWire.Wire();
*/

/*
    Handle(ShapeFix_Wire) spSFW = new ShapeFix_Wire;
spSFW->SetPrecision(Precision::Confusion());
spSFW->Load (wire);

spSFW->FixSelfIntersectionMode () = 1;
spSFW->FixNonAdjacentIntersectingEdgesMode () = 1;
spSFW->ModifyTopologyMode () = true;
spSFW->ModifyRemoveLoopMode () = true;
spSFW->ClosedWireMode () = true;

spSFW->FixReorder();
spSFW->FixConnected();

bool performed = spSFW->Perform();
bool selfIntFixed = spSFW->FixSelfIntersection();
Handle(ShapeExtend_WireData) spWD = spSFW->WireData();
TopoDS_Wire profileWire = spSFW->Wire();
*/

    std::vector<TopoDS_Edge> edges = SortEdges(wire);
    for (std::vector<TopoDS_Edge>::iterator l_itEdge = edges.begin(); l_itEdge != edges.end(); l_itEdge++)
    // for(BRepTools_WireExplorer expEdge(TopoDS::Wire(wire)); expEdge.More(); expEdge.Next())
	{
		// const TopoDS_Shape &E = expEdge.Current();
		const TopoDS_Edge &E = *l_itEdge;

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

				if (last_position.Distance(PS) < tolerance)
				{
					// We're heading towards the PE point.
					CNCPoint point(PE);
					gcode << _T("feed(x=") << point.X(true) << _T(", y=") << point.Y(true) << _T(", z=") << point.Z(true) << _T(")\n");
					last_position = point;
				} // End if - then
				else if (last_position.Distance(PE) < tolerance)
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

					gcode << _T("rapid(z=") << m_depth_op_params.m_clearance_height / theApp.m_program->m_units << _T(")\n");
					gcode << _T("rapid(x=") << start.X(true) << _T(", y=") << start.Y(true) << _T(")\n");
					gcode << _T("feed(z=") << start.Z(true) << _T(")\n");

					gcode << _T("feed(x=") << end.X(true) << _T(", y=") << end.Y(true) << _T(", z=") << end.Z(true) << _T(")\n");
					last_position = end;
				}
			}
			break;

			case GeomAbs_Circle:
				// make an arc
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
				if(curve.IsPeriodic())
				{
					double period = curve.Period();
					double uHalf = uStart + period/2;
					gp_Pnt PH;
					gp_Vec VH;
					curve.D1(uHalf, PH, VH);
					if (last_position.Distance(PS) < tolerance)
					{
						// Arc towards PH
						CNCPoint point(PH);
						CNCPoint centre( circle.Location() );
						bool l_bClockwise = Clockwise(circle);

						CNCPoint offset = centre - CNCPoint(PS);

						gcode << (l_bClockwise?_T("arc_cw("):_T("arc_ccw(")) << _T("x=") << point.X(true) << _T(", y=") << point.Y(true) << _T(", z=") << point.Z(true) << _T(", ")
							<< _T("i=") << offset.X(true) << _T(", j=") << offset.Y(true) << _T(", k=") << offset.Z(true) << _T(")\n");
						last_position = point;
					}
					else if (last_position.Distance(PH) < tolerance)
					{
						// Arc towards PS
						CNCPoint point(PS);
						CNCPoint centre( circle.Location() );
						bool l_bClockwise = Clockwise(circle);

						CNCPoint offset = centre - CNCPoint(PH);

						gcode << (l_bClockwise?_T("arc_cw("):_T("arc_ccw(")) << _T("x=") << point.X(true) << _T(", y=") << point.Y(true) << _T(", z=") << point.Z(true) << _T(", ")
							<< _T("i=") << offset.X(true) << _T(", j=") << offset.Y(true) << _T(", k=") << offset.Z(true) << _T(")\n");
						last_position = point;
					}
					else
					{
						// Move to PS first.
						CNCPoint start(PS);

						gcode << _T("rapid(z=") << m_depth_op_params.m_clearance_height / theApp.m_program->m_units << _T(")\n");
						gcode << _T("rapid(x=") << start.X(true) << _T(", y=") << start.Y(true) << _T(")\n");
						gcode << _T("feed(z=") << start.Z(true) << _T(")\n");

						CNCPoint point(PE);
						CNCPoint centre( circle.Location() );
						bool l_bClockwise = Clockwise(circle);

						CNCPoint offset = centre - CNCPoint(PS);

						gcode << (l_bClockwise?_T("arc_cw("):_T("arc_ccw(")) << _T("x=") << point.X(true) << _T(", y=") << point.Y(true) << _T(", z=") << point.Z(true) << _T(", ")
							<< _T("i=") << offset.X(true) << _T(", j=") << offset.Y(true) << _T(", k=") << offset.Z(true) << _T(")\n");
						last_position = point;
					}
				}
				else
				{
					if (last_position.Distance(PS) < tolerance)
					{
						// Arc towards PE

						CNCPoint point(PE);
						CNCPoint centre( circle.Location() );
						bool l_bClockwise = Clockwise(circle);

						CNCPoint offset = centre - CNCPoint(PS);

						gcode << (l_bClockwise?_T("arc_cw("):_T("arc_ccw(")) << _T("x=") << point.X(true) << _T(", y=") << point.Y(true) << _T(", z=") << point.Z(true) << _T(", ")
							<< _T("i=") << offset.X(true) << _T(", j=") << offset.Y(true) << _T(", k=") << offset.Z(true) << _T(")\n");
						last_position = point;
					}
					else if (last_position.Distance(PE) < tolerance)
					{
						// Arc towards PS
						CNCPoint point(PS);
						CNCPoint centre( circle.Location() );
						bool l_bClockwise = Clockwise(circle);

						CNCPoint offset = centre - CNCPoint(PE);

						gcode << (l_bClockwise?_T("arc_cw("):_T("arc_ccw(")) << _T("x=") << point.X(true) << _T(", y=") << point.Y(true) << _T(", z=") << point.Z(true) << _T(", ")
							<< _T("i=") << offset.X(true) << _T(", j=") << offset.Y(true) << _T(", k=") << offset.Z(true) << _T(")\n");
						last_position = point;
					}
					else
					{
						// Move to PS first.
						CNCPoint start(PS);

						gcode << _T("rapid(z=") << m_depth_op_params.m_clearance_height / theApp.m_program->m_units << _T(")\n");
						gcode <<_T( "rapid(x=") << start.X(true) << _T(", y=") << start.Y(true) << _T(")\n");
						gcode << _T("feed(z=") << start.Z(true) << _T(")\n");

						CNCPoint point(PE);
						CNCPoint centre( circle.Location() );
						bool l_bClockwise = Clockwise(circle);

						CNCPoint offset = centre - CNCPoint(PS);

						gcode << (l_bClockwise?_T("arc_cw"):_T("arc_ccw")) << _T("(x=") << point.X(true) << _T(", y=") << point.Y(true) << _T(", z=") << point.Z(true) << _T(", ")
							<< _T("i=") << offset.X(true) << _T(", j=") << offset.Y(true) << _T(", k=") << offset.Z(true) << _T(")\n");
						last_position = point;
					}
				}
			}
			break;

            /*
            // It would be great if we could do the BiArc movements along the spline that Dan put in for
            // Profile operations.  I will get there eventually.
            case GeomAbs_BSplineCurve:
            */

			/*
			default:
			{
				// make lots of small lines
				BRepTools::Clean(TopoDS::Edge(E));
				BRepMesh::Mesh(TopoDS::Edge(E), deviation);

				TopLoc_Location L;
				Handle(Poly_Polygon3D) Polyg = BRep_Tool::Polygon3D(edge, L);
				if (!Polyg.IsNull()) {
					const TColgp_Array1OfPnt& Points = Polyg->Nodes();
					Standard_Integer po;
					int i = 0;
					for (po = Points.Lower(); po <= Points.Upper(); po++, i++) {
						CNCPoint p = (Points.Value(po)).Transformed(L);

						HLine* new_object = new HLine(prev_p, p, &wxGetApp().current_color);
						sketch->Add(new_object, NULL);

						if (last_point.Distance(p) < tolerance)
						{
							// No change in position.
						} // End if - then
						else if (last_point.Distance(PE) < tolerance)
						{
							CNCPoint point(PS);
							gcode << "feed(x=" << point.X(true) << ", y=" << point.Y(true) << ", z=" << point.Z(true) << ")\n";
							last_position = point;
						}
						else
						{
							// We need to move to the start BEFORE machining this line.
							CNCPoint start(PS);
							CNCPoint end(PE);

							gcode << "rapid(z=" << m_depth_op_params.m_clearance_height / theApp.m_program->m_units << ")\n";
							gcode << "rapid(x=" << start.X(true) << ", y=" << start.Y(true) << ")\n";
							gcode << "feed(z=" << start.Z(true) << ")\n";

							gcode << "feed(x=" << end.X(true) << ", y=" << end.Y(true) << ", z=" << end.Z(true) << ")\n";
							last_position = end;
						}

					} // End for
				} // End if - then
			}
			break;
			*/
		} // End switch
	}

	return(gcode);
}



/**
	This method is called when the CAD operator presses the Python button.  This method generates
	Python source code whose job will be to generate RS-274 GCode.  It's done in two steps so that
	the Python code can be configured to generate GCode suitable for various CNC interpreters.
 */
void CContour::AppendTextToProgram( const CFixture *pFixture )
{
	wxString gcode;
	CDepthOp::AppendTextToProgram( pFixture );

	double deviation = heeksCAD->GetTolerance();
	unsigned int number_of_bad_sketches = 0;

	CCuttingTool *pCuttingTool = CCuttingTool::Find( m_cutting_tool_number );
	if (! pCuttingTool)
	{
		return;
	}

	CNCPoint last_position(0.0, 0.0, 0.0);

	for (Symbols_t::const_iterator l_itSymbol = m_symbols.begin(); l_itSymbol != m_symbols.end(); l_itSymbol++)
	{
		HeeksObj* object = heeksCAD->GetIDObject(l_itSymbol->first, l_itSymbol->second);
		if (object != NULL)
		{
			std::list<TopoDS_Shape> wires;
			if (! heeksCAD->ConvertSketchToFaceOrWire( object, wires, false))
			{
				number_of_bad_sketches++;
			} // End if - then
			else
			{
				// The wire(s) represent the sketch objects for a tool path.  We need to apply
				// either the 'inside', 'on' or 'outside' attributes by translating the wires
				// by the cutting tool's radius first.

				try {
					for(std::list<TopoDS_Shape>::iterator It2 = wires.begin(); It2 != wires.end(); It2++)
					{
					    TopoDS_Shape& wire_to_fix = *It2;
					    ShapeFix_Wire fix;
					    fix.Load( TopoDS::Wire(wire_to_fix) );
					    fix.FixReorder();


						TopoDS_Shape wire = fix.Wire();
						BRepOffsetAPI_MakeOffset offset_wire(TopoDS::Wire(wire));

						double radius = pCuttingTool->CuttingRadius();
						if (m_params.m_tool_on_side == CContourParams::eRightOrInside) radius *= -1.0;
						if (m_params.m_tool_on_side == CContourParams::eOn) radius = 0.0;

						offset_wire.Perform(radius);
						TopoDS_Wire tool_path_wire(TopoDS::Wire(offset_wire.Shape()));

						// Now generate a toolpath along this wire.
						gcode << GeneratePathFromWire(tool_path_wire, last_position );
					}
				} // End try
				catch (Standard_Failure) {
					Handle_Standard_Failure e = Standard_Failure::Caught();
					number_of_bad_sketches++;
				} // End catch
			} // End if - else
		} // End if - then
	} // End for

	theApp.m_program_canvas->m_textCtrl->AppendText(gcode.c_str());
}




/**
	This is the Graphics Library Commands (from the OpenGL set).  This method calls the OpenGL
	routines to paint the drill action in the graphics window.  The graphics is transient.

	Part of its job is to re-paint the elements that this CContour object refers to so that
	we know what CAD objects this CNC operation is referring to.
 */
void CContour::glCommands(bool select, bool marked, bool no_color)
{
	if(marked && !no_color)
	{
		for (Symbols_t::const_iterator l_itSymbol = m_symbols.begin(); l_itSymbol != m_symbols.end(); l_itSymbol++)
		{
			HeeksObj* object = heeksCAD->GetIDObject(l_itSymbol->first, l_itSymbol->second);

			// If we found something, ask its CAD code to draw itself highlighted.
			if(object)object->glCommands(false, true, false);
		} // End for
	} // End if - then
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


void CContour::GetProperties(std::list<Property *> *list)
{
    {
		std::list< wxString > choices;

		SketchOrderType order = SketchOrderTypeUnknown;

		if(m_symbols.size() == 1)
		{
			HeeksObj* sketch = heeksCAD->GetIDObject(m_symbols.front().first, m_symbols.front().second);
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
		}

		choices.push_back(_("On"));

		int choice = int(CContourParams::eOn);
		switch (m_params.m_tool_on_side)
		{
			case CContourParams::eRightOrInside:	choice = 1;
					break;

			case CContourParams::eOn:	choice = 2;
					break;

			case CContourParams::eLeftOrOutside:	choice = 0;
					break;
		} // End switch

		list->push_back(new PropertyChoice(_("tool on side"), choices, choice, this, on_set_tool_on_side));
	}

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

	WriteBaseXML(element);
}

// static member function
HeeksObj* CContour::ReadFromXMLElement(TiXmlElement* element)
{
	CContour* new_object = new CContour;

	// read point and circle ids
	for(TiXmlElement* pElem = TiXmlHandle(element).FirstChildElement().Element(); pElem; pElem = pElem->NextSiblingElement())
	{
		std::string name(pElem->Value());
		if(name == "params"){
			new_object->m_params.ReadParametersFromXMLElement(pElem);
		}
		else if(name == "symbols"){
			for(TiXmlElement* child = TiXmlHandle(pElem).FirstChildElement().Element(); child; child = child->NextSiblingElement())
			{
				if (child->Attribute("type") && child->Attribute("id"))
				{
					new_object->AddSymbol( atoi(child->Attribute("type")), atoi(child->Attribute("id")) );
				}
			} // End for
		} // End if
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
/* static */ bool CContour::ValidType( const int object_type )
{
    switch (object_type)
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


