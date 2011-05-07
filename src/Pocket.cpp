// Pocket.cpp
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "stdafx.h"
#include "Pocket.h"
#include "CNCConfig.h"
#include "ProgramCanvas.h"
#include "Program.h"
#include "interface/HeeksObj.h"
#include "interface/PropertyDouble.h"
#include "interface/PropertyLength.h"
#include "interface/PropertyString.h"
#include "interface/PropertyChoice.h"
#include "interface/PropertyVertex.h"
#include "interface/PropertyCheck.h"
#include "tinyxml/tinyxml.h"
#include "CTool.h"
#include "CNCPoint.h"
#include "Reselect.h"
#include "MachineState.h"
#include "PocketDlg.h"

#include <sstream>

// static
double CPocket::max_deviation_for_spline_to_arc = 0.1;

CPocketParams::CPocketParams()
{
	m_step_over = 0.0;
	m_material_allowance = 0.0;
	m_starting_place = true;
	m_keep_tool_down_if_poss = true;
	m_use_zig_zag = true;
	m_zig_angle = 0.0;
	m_zig_unidirectional = false;
	m_entry_move = ePlunge;
}

void CPocketParams::set_initial_values(const CTool::ToolNumber_t tool_number)
{
    if (tool_number > 0)
    {
        CTool *pTool = CTool::Find(tool_number);
        if (pTool != NULL)
        {
            m_step_over = pTool->CuttingRadius() * 3.0 / 5.0;
        }
    }
}

static void on_set_entry_move(int value, HeeksObj* object)
{
	((CPocket*)object)->m_pocket_params.m_entry_move = (CPocketParams::eEntryStyle) value;
	((CPocket*)object)->WriteDefaultValues();
}

static void on_set_step_over(double value, HeeksObj* object)
{
	((CPocket*)object)->m_pocket_params.m_step_over = value;
	((CPocket*)object)->WriteDefaultValues();
}

static void on_set_material_allowance(double value, HeeksObj* object)
{
	((CPocket*)object)->m_pocket_params.m_material_allowance = value;
	((CPocket*)object)->WriteDefaultValues();
}

static void on_set_starting_place(int value, HeeksObj* object)
{
	((CPocket*)object)->m_pocket_params.m_starting_place = value;
	((CPocket*)object)->WriteDefaultValues();
}

static void on_set_keep_tool_down(bool value, HeeksObj* object)
{
	((CPocket*)object)->m_pocket_params.m_keep_tool_down_if_poss = value;
	((CPocket*)object)->WriteDefaultValues();
}

static void on_set_use_zig_zag(bool value, HeeksObj* object)
{
	((CPocket*)object)->m_pocket_params.m_use_zig_zag = value;
	((CPocket*)object)->WriteDefaultValues();
}

static void on_set_zig_angle(double value, HeeksObj* object)
{
	((CPocket*)object)->m_pocket_params.m_zig_angle = value;
	((CPocket*)object)->WriteDefaultValues();
}

static void on_set_zig_uni(bool value, HeeksObj* object)
{
	((CPocket*)object)->m_pocket_params.m_zig_unidirectional = value;
	((CPocket*)object)->WriteDefaultValues();
}

void CPocketParams::GetProperties(CPocket* parent, std::list<Property *> *list)
{
	list->push_back(new PropertyLength(_("step over"), m_step_over, parent, on_set_step_over));
	list->push_back(new PropertyLength(_("material allowance"), m_material_allowance, parent, on_set_material_allowance));
	{
		std::list< wxString > choices;
		choices.push_back(_("Boundary"));
		choices.push_back(_("Center"));
		list->push_back(new PropertyChoice(_("starting_place"), choices, m_starting_place, parent, on_set_starting_place));
	}
	{
		std::list< wxString > choices;
		choices.push_back(_("Plunge"));
		choices.push_back(_("Ramp"));
		choices.push_back(_("Helical"));
		list->push_back(new PropertyChoice(_("entry_move"), choices, m_entry_move, parent, on_set_entry_move));
	}
	list->push_back(new PropertyCheck(_("keep tool down"), m_keep_tool_down_if_poss, parent, on_set_keep_tool_down));
	list->push_back(new PropertyCheck(_("use zig zag"), m_use_zig_zag, parent, on_set_use_zig_zag));
	if(m_use_zig_zag)
	{
		list->push_back(new PropertyDouble(_("zig angle"), m_zig_angle, parent, on_set_zig_angle));
		list->push_back(new PropertyCheck(_("unidirectional"), m_zig_unidirectional, parent, on_set_zig_uni));
	}
}

void CPocketParams::WriteXMLAttributes(TiXmlNode *root)
{
	TiXmlElement * element;
	element = new TiXmlElement( "params" );
	root->LinkEndChild( element );
	element->SetDoubleAttribute("step", m_step_over);
	element->SetDoubleAttribute("mat", m_material_allowance);
	element->SetAttribute("from_center", m_starting_place);
	element->SetAttribute("keep_tool_down", m_keep_tool_down_if_poss ? 1:0);
	element->SetAttribute("use_zig_zag", m_use_zig_zag ? 1:0);
	element->SetDoubleAttribute("zig_angle", m_zig_angle);
	element->SetAttribute("zig_unidirectional", m_zig_unidirectional ? 1:0);
	element->SetAttribute("entry_move", (int) m_entry_move);
}

void CPocketParams::ReadFromXMLElement(TiXmlElement* pElem)
{
	pElem->Attribute("step", &m_step_over);
	pElem->Attribute("mat", &m_material_allowance);
	pElem->Attribute("from_center", &m_starting_place);
	int int_for_bool = false;
	pElem->Attribute("keep_tool_down", &int_for_bool);
	m_keep_tool_down_if_poss = (int_for_bool != 0);
	pElem->Attribute("use_zig_zag", &int_for_bool);
	m_use_zig_zag = (int_for_bool != 0);
	pElem->Attribute("zig_angle", &m_zig_angle);
	pElem->Attribute("zig_unidirectional", &int_for_bool);
	m_zig_unidirectional = (int_for_bool != 0);
	int int_for_entry_move = (int) ePlunge;
	pElem->Attribute("entry_move", &int_for_entry_move);
	m_entry_move = (eEntryStyle) int_for_entry_move;
}

//static wxString WriteCircleDefn(HeeksObj* sketch, CMachineState *pMachineState) {
//#ifdef UNICODE
//	std::wostringstream gcode;
//#else
//	std::ostringstream gcode;
//#endif
//	gcode.imbue(std::locale("C"));
//	gcode << std::setprecision(10);
//	std::list<std::pair<int, gp_Pnt> > points;
//	span_object->GetCentrePoint(c);
//
//	// Setup the four arcs that will make up the circle using UNadjusted
//	// coordinates first so that the offsets align with the X and Y axes.
//	double small_amount = 0.001;
//	double radius = heeksCAD->CircleGetRadius(span_object);
//
//	points.push_back(std::make_pair(LINEAR, gp_Pnt(c[0], c[1] + radius, c[2]))); // north
//	points.push_back(std::make_pair(CW, gp_Pnt(c[0] + radius, c[1], c[2]))); // east
//	points.push_back(std::make_pair(CW, gp_Pnt(c[0], c[1] - radius, c[2]))); // south
//	points.push_back(std::make_pair(CW, gp_Pnt(c[0] - radius, c[1], c[2]))); // west
//	points.push_back(std::make_pair(CW, gp_Pnt(c[0], c[1] + radius, c[2]))); // north
//
//	CNCPoint centre(pMachineState->Fixture().Adjustment(c));
//
//	gcode << _T("c = area.Curve()\n");
//	for (std::list<std::pair<int, gp_Pnt> >::iterator l_itPoint =
//			points.begin(); l_itPoint != points.end(); l_itPoint++) {
//		CNCPoint pnt = pMachineState->Fixture().Adjustment(l_itPoint->second);
//
//		gcode << _T("c.append(area.Vertex(") << l_itPoint->first
//				<< _T(", area.Point(");
//		gcode << pnt.X(true) << (_T(", ")) << pnt.Y(true);
//		gcode << _T("), area.Point(") << centre.X(true) << _T(", ")
//				<< centre.Y(true) << _T(")))\n");
//	} // End for
//	gcode << _T("a.append(c)\n");
//}
static wxString WriteSketchDefn(HeeksObj* sketch, CMachineState *pMachineState)
{
#ifdef UNICODE
	std::wostringstream gcode;
#else
    std::ostringstream gcode;
#endif
    gcode.imbue(std::locale("C"));
	gcode << std::setprecision(10);

	bool started = false;

	double prev_e[3];

	std::list<HeeksObj*> new_spans;
	for(HeeksObj* span = sketch->GetFirstChild(); span; span = sketch->GetNextChild())
	{
		if(span->GetType() == SplineType)
		{
			heeksCAD->SplineToBiarcs(span, new_spans, CPocket::max_deviation_for_spline_to_arc);
		}
		else
		{
			new_spans.push_back(span->MakeACopy());
		}
	}

	for(std::list<HeeksObj*>::iterator It = new_spans.begin(); It != new_spans.end(); It++)
	{
		HeeksObj* span_object = *It;

		double s[3] = {0, 0, 0};
		double e[3] = {0, 0, 0};
		double c[3] = {0, 0, 0};

		if(span_object){
			int type = span_object->GetType();

			if(type == LineType || type == ArcType)
			{
				span_object->GetStartPoint(s);
				CNCPoint start(pMachineState->Fixture().Adjustment(s));

				if(started && (fabs(s[0] - prev_e[0]) > 0.0001 || fabs(s[1] - prev_e[1]) > 0.0001))
				{
					gcode << _T("a.append(c)\n");
					started = false;
				}

				if(!started)
				{
					gcode << _T("c = area.Curve()\n");
					gcode << _T("c.append(area.Vertex(0, area.Point(") << start.X(true) << _T(", ") << start.Y(true) << _T("), area.Point(0, 0)))\n");
					started = true;
				}
				span_object->GetEndPoint(e);
				CNCPoint end(pMachineState->Fixture().Adjustment(e));

				if(type == LineType)
				{
					gcode << _T("c.append(area.Vertex(0, area.Point(") << end.X(true) << _T(", ") << end.Y(true) << _T("), area.Point(0, 0)))\n");
				}
				else if(type == ArcType)
				{
					span_object->GetCentrePoint(c);
					CNCPoint centre(pMachineState->Fixture().Adjustment(c));

					double pos[3];
					heeksCAD->GetArcAxis(span_object, pos);
					int span_type = (pos[2] >=0) ? 1:-1;
					gcode << _T("c.append(area.Vertex(") << span_type << _T(", area.Point(") << end.X(true) << _T(", ") << end.Y(true);
					gcode << _T("), area.Point(") << centre.X(true) << _T(", ") << centre.Y(true) << _T(")))\n");
				}
				memcpy(prev_e, e, 3*sizeof(double));
			} // End if - then
			else
			{
				if (type == CircleType)
				{
					if(started)
					{
						gcode << _T("a.append(c)\n");
						started = false;
					}

					std::list< std::pair<int, gp_Pnt > > points;
					span_object->GetCentrePoint(c);

					// Setup the four arcs that will make up the circle using UNadjusted
					// coordinates first so that the offsets align with the X and Y axes.
					double radius = heeksCAD->CircleGetRadius(span_object);

					points.push_back( std::make_pair(0, gp_Pnt( c[0], c[1] + radius, c[2] )) ); // north
					points.push_back( std::make_pair(-1, gp_Pnt( c[0] + radius, c[1], c[2] )) ); // east
					points.push_back( std::make_pair(-1, gp_Pnt( c[0], c[1] - radius, c[2] )) ); // south
					points.push_back( std::make_pair(-1, gp_Pnt( c[0] - radius, c[1], c[2] )) ); // west
					points.push_back( std::make_pair(-1, gp_Pnt( c[0], c[1] + radius, c[2] )) ); // north

					CNCPoint centre(pMachineState->Fixture().Adjustment(c));

					gcode << _T("c = area.Curve()\n");
					for (std::list< std::pair<int, gp_Pnt > >::iterator l_itPoint = points.begin(); l_itPoint != points.end(); l_itPoint++)
					{
						CNCPoint pnt = pMachineState->Fixture().Adjustment( l_itPoint->second );

						gcode << _T("c.append(area.Vertex(") << l_itPoint->first << _T(", area.Point(");
						gcode << pnt.X(true) << (_T(", ")) << pnt.Y(true);
						gcode << _T("), area.Point(") << centre.X(true) << _T(", ") << centre.Y(true) << _T(")))\n");
					} // End for
					gcode << _T("a.append(c)\n");
				}
			} // End if - else
		}
	}

	if(started)
	{
		gcode << _T("a.append(c)\n");
		started = false;
	}

	// delete the spans made
	for(std::list<HeeksObj*>::iterator It = new_spans.begin(); It != new_spans.end(); It++)
	{
		HeeksObj* span = *It;
		delete span;
	}

	gcode << _T("\n");
	return(wxString(gcode.str().c_str()));
}

const wxBitmap &CPocket::GetIcon()
{
	if(!m_active)return GetInactiveIcon();
	static wxBitmap* icon = NULL;
	if(icon == NULL)icon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/pocket.png")));
	return *icon;
}

Python CPocket::AppendTextToProgram(CMachineState *pMachineState)
{
	Python python;

	ReloadPointers();   // Make sure all the m_sketches values have been converted into children.

	CTool *pTool = CTool::Find( m_tool_number );
	if (pTool == NULL)
	{
		wxMessageBox(_T("Cannot generate GCode for pocket without a tool assigned"));
		return(python);
	} // End if - then


	python << CDepthOp::AppendTextToProgram(pMachineState);

	python << _T("a = area.Area()\n");
	python << _T("entry_moves = []\n");

    for (HeeksObj *object = GetFirstChild(); object != NULL; object = GetNextChild())
    {
		if(object == NULL) {
			wxMessageBox(wxString::Format(_("Pocket operation - Sketch doesn't exist")));
			continue;
		}
		int type = object->GetType();
		double c[3] = {0, 0, 0};
		double radius;

		switch (type) {

		case CircleType:
			if (m_pocket_params.m_entry_move == CPocketParams::eHelical) {
				GetCentrePoint(c);
				radius = heeksCAD->CircleGetRadius(object);
				python << _T("# entry_moves.append(circle(") << c[0]/theApp.m_program->m_units << _T(", ") << c[1]/theApp.m_program->m_units << _T(", ")<< c[2]/theApp.m_program->m_units << _T(", ") << radius/theApp.m_program->m_units ;
				python << _T("))\n") ;
			} else {
				wxLogMessage(_T("circle found in pocket operation (id=%d) but entry move is not helical, id=%d"), GetID(),object->GetID());
			}
			continue;

		case PointType:
			if (m_pocket_params.m_entry_move == CPocketParams::eHelical) {
				memset( c, 0, sizeof(c) );
				heeksCAD->VertexGetPoint( object, c);
				python << _T("# entry_moves.append(point(") << c[0]/theApp.m_program->m_units << _T(", ") << c[1]/theApp.m_program->m_units << _T("))\n");

			} else {
				wxLogMessage(_T("point found in pocket operation (id=%d) but entry move is not helical, id=%d"), GetID(), object->GetID());
			}
			continue;

		default:
			break;
		}
		if (object->GetNumChildren() == 0){
			wxMessageBox(wxString::Format(_("Pocket operation - Sketch %d has no children"), object->GetID()));
			continue;
		}

		HeeksObj* re_ordered_sketch = NULL;
		SketchOrderType order = heeksCAD->GetSketchOrder(object);
		if( 	(order != SketchOrderTypeCloseCW) &&
			(order != SketchOrderTypeCloseCCW) &&
			(order != SketchOrderTypeMultipleCurves) &&
			(order != SketchOrderHasCircles))
		{
			re_ordered_sketch = object->MakeACopy();
			heeksCAD->ReOrderSketch(re_ordered_sketch, SketchOrderTypeReOrder);
			object = re_ordered_sketch;
			order = heeksCAD->GetSketchOrder(object);
			if(	(order != SketchOrderTypeCloseCW) &&
				(order != SketchOrderTypeCloseCCW) &&
				(order != SketchOrderTypeMultipleCurves) &&
				(order != SketchOrderHasCircles))
			{
				switch(heeksCAD->GetSketchOrder(object))
				{
				case SketchOrderTypeOpen:
					{
						wxMessageBox(wxString::Format(_("Pocket operation - Sketch must be a closed shape - sketch %d"), object->m_id));
						delete re_ordered_sketch;
						continue;
					}
					break;

				default:
					{
						wxMessageBox(wxString::Format(_("Pocket operation - Badly ordered sketch - sketch %d"), object->m_id));
						delete re_ordered_sketch;
						continue;
					}
					break;
				}
			}
		}

		if(object)
		{
			python << WriteSketchDefn(object, pMachineState);
		}

		if(re_ordered_sketch)
		{
			delete re_ordered_sketch;
		}
	} // End for

	// reorder the area, the outside curves must be made anti-clockwise and the insides clockwise
	python << _T("a.Reorder()\n");

	// start - assume we are at a suitable clearance height

	// make a parameter of area_funcs.pocket() eventually
	// 0..plunge, 1..ramp, 2..helical
	python << _T("entry_style = ") <<  m_pocket_params.m_entry_move << _T("\n");

	// Pocket the area
	python << _T("area_funcs.pocket(a, tool_diameter/2, ");
	python << m_pocket_params.m_material_allowance / theApp.m_program->m_units;
	python << _T(", rapid_safety_space, start_depth, final_depth, ");
	python << m_pocket_params.m_step_over / theApp.m_program->m_units;
	python << _T(", step_down, clearance, ");
	python << m_pocket_params.m_starting_place;
	python << (m_pocket_params.m_keep_tool_down_if_poss ? _T(", True") : _T(", False"));
	python << (m_pocket_params.m_use_zig_zag ? _T(", True") : _T(", False"));
	python << _T(", ") << m_pocket_params.m_zig_angle;
	python << _T(", zig_unidirectional = ") << (m_pocket_params.m_zig_unidirectional ? _T("True") : _T("False"));
	python << _T(")\n");

	// rapid back up to clearance plane
	python << _T("rapid(z = clearance)\n");

	return(python);

} // End AppendTextToProgram() method


void CPocket::WriteDefaultValues()
{
	CDepthOp::WriteDefaultValues();

	CNCConfig config(CPocketParams::ConfigScope());
	config.Write(_T("StepOver"), m_pocket_params.m_step_over);
	config.Write(_T("MaterialAllowance"), m_pocket_params.m_material_allowance);
	config.Write(_T("FromCenter"), m_pocket_params.m_starting_place);
	config.Write(_T("KeepToolDown"), m_pocket_params.m_keep_tool_down_if_poss);
	config.Write(_T("UseZigZag"), m_pocket_params.m_use_zig_zag);
	config.Write(_T("ZigAngle"), m_pocket_params.m_zig_angle);
	config.Write(_T("ZigUnidirectional"), m_pocket_params.m_zig_unidirectional);
	config.Write(_T("DecentStrategy"), m_pocket_params.m_entry_move);
}

void CPocket::ReadDefaultValues()
{
	CDepthOp::ReadDefaultValues();

	CNCConfig config(CPocketParams::ConfigScope());
	config.Read(_T("StepOver"), &m_pocket_params.m_step_over, 1.0);
	config.Read(_T("MaterialAllowance"), &m_pocket_params.m_material_allowance, 0.2);
	config.Read(_T("FromCenter"), &m_pocket_params.m_starting_place, 1);
	config.Read(_T("KeepToolDown"), &m_pocket_params.m_keep_tool_down_if_poss, true);
	config.Read(_T("UseZigZag"), &m_pocket_params.m_use_zig_zag, false);
	config.Read(_T("ZigAngle"), &m_pocket_params.m_zig_angle);
	config.Read(_T("ZigUnidirectional"), &m_pocket_params.m_zig_unidirectional, false);
	int int_for_entry_move = CPocketParams::ePlunge;
	config.Read(_T("DecentStrategy"), &int_for_entry_move);
	m_pocket_params.m_entry_move = (CPocketParams::eEntryStyle) int_for_entry_move;
}

void CPocket::glCommands(bool select, bool marked, bool no_color)
{
	CDepthOp::glCommands( select, marked, no_color );
}

void CPocket::GetProperties(std::list<Property *> *list)
{
	AddSketchesProperties(list, this);
	m_pocket_params.GetProperties(this, list);
	CDepthOp::GetProperties(list);
}

HeeksObj *CPocket::MakeACopy(void)const
{
	return new CPocket(*this);
}

void CPocket::CopyFrom(const HeeksObj* object)
{
	operator=(*((CPocket*)object));
}

CPocket::CPocket( const CPocket & rhs ) : CDepthOp(rhs)
{
	m_sketches.clear();
	std::copy( rhs.m_sketches.begin(), rhs.m_sketches.end(), std::inserter( m_sketches, m_sketches.begin() ) );
	m_pocket_params = rhs.m_pocket_params;
}

CPocket & CPocket::operator= ( const CPocket & rhs )
{
	if (this != &rhs)
	{
		CDepthOp::operator=(rhs);
		m_sketches.clear();
		std::copy( rhs.m_sketches.begin(), rhs.m_sketches.end(), std::inserter( m_sketches, m_sketches.begin() ) );

		m_pocket_params = rhs.m_pocket_params;
		// static double max_deviation_for_spline_to_arc;
	}

	return(*this);
}

bool CPocket::CanAddTo(HeeksObj* owner)
{
	return ((owner != NULL) && (owner->GetType() == OperationsType));
}

void CPocket::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element = new TiXmlElement( "Pocket" );
	root->LinkEndChild( element );
	m_pocket_params.WriteXMLAttributes(element);

	// write sketch ids
	for(std::list<int>::iterator It = m_sketches.begin(); It != m_sketches.end(); It++)
	{
		int sketch = *It;
		TiXmlElement * sketch_element = new TiXmlElement( "sketch" );
		element->LinkEndChild( sketch_element );
		sketch_element->SetAttribute("id", sketch);
	}

	WriteBaseXML(element);
}

// static member function
HeeksObj* CPocket::ReadFromXMLElement(TiXmlElement* element)
{
	CPocket* new_object = new CPocket;

	std::list<TiXmlElement *> elements_to_remove;

	// read profile parameters
	TiXmlElement* params = TiXmlHandle(element).FirstChildElement("params").Element();
	if(params)
	{
		new_object->m_pocket_params.ReadFromXMLElement(params);
		elements_to_remove.push_back(params);
	}

	// read sketch ids
	for(TiXmlElement* sketch = TiXmlHandle(element).FirstChildElement("sketch").Element(); sketch; sketch = sketch->NextSiblingElement())
	{
		if ((wxString(Ctt(sketch->Value())) == wxString(_T("sketch"))) &&
			(sketch->Attribute("id") != NULL) &&
			(sketch->Attribute("title") == NULL))
		{
			int id = 0;
			sketch->Attribute("id", &id);
			if(id)
			{
				new_object->m_sketches.push_back(id);
			}

			elements_to_remove.push_back(sketch);
		} // End if - then
	}

	for (std::list<TiXmlElement*>::iterator itElem = elements_to_remove.begin(); itElem != elements_to_remove.end(); itElem++)
	{
		element->RemoveChild(*itElem);
	}

	// read common parameters
	new_object->ReadBaseXML(element);

	return new_object;
}

CPocket::CPocket(const std::list<int> &sketches, const int tool_number )
	: CDepthOp(GetTypeString(), &sketches, tool_number ), m_sketches(sketches)
{
	ReadDefaultValues();
	m_pocket_params.set_initial_values(tool_number);

	for (Sketches_t::iterator sketch = m_sketches.begin(); sketch != m_sketches.end(); sketch++)
	{
		HeeksObj *object = heeksCAD->GetIDObject( SketchType, *sketch );
		if (object != NULL)
		{
			Add( object, NULL );
		}
	}

	m_sketches.clear();
}

CPocket::CPocket(const std::list<HeeksObj *> &sketches, const int tool_number )
	: CDepthOp(GetTypeString(), sketches, tool_number )
{
	ReadDefaultValues();
	m_pocket_params.set_initial_values(tool_number);

	for (std::list<HeeksObj *>::const_iterator sketch = sketches.begin(); sketch != sketches.end(); sketch++)
	{
		Add( *sketch, NULL );
	}
}



/**
	The old version of the CDrilling object stored references to graphics as type/id pairs
	that get read into the m_symbols list.  The new version stores these graphics references
	as child elements (based on ObjList).  If we read in an old-format file then the m_symbols
	list will have data in it for which we don't have children.  This routine converts
	these type/id pairs into the HeeksObj pointers as children.
 */
void CPocket::ReloadPointers()
{
	for (Sketches_t::iterator symbol = m_sketches.begin(); symbol != m_sketches.end(); symbol++)
	{
		HeeksObj *object = heeksCAD->GetIDObject( SketchType, *symbol );
		if (object != NULL)
		{
			Add( object, NULL );
		}
	}

	m_sketches.clear();	// We don't want to convert them twice.

	CDepthOp::ReloadPointers();
}



/**
	This method adjusts any parameters that don't make sense.  It should report a list
	of changes in the list of strings.
 */
std::list<wxString> CPocket::DesignRulesAdjustment(const bool apply_changes)
{
	std::list<wxString> changes;

	int num_sketches = 0;
	for(HeeksObj *obj = GetFirstChild(); obj != NULL; obj = GetNextChild())
	{
		if (obj->GetType() == SketchType)
		{
		    num_sketches++;
		}
	} // End if - then

	if (num_sketches == 0)
	{
#ifdef UNICODE
			std::wostringstream l_ossChange;
#else
			std::ostringstream l_ossChange;
#endif

			l_ossChange << _("No valid sketches upon which to act for pocket operations") << " id='" << m_id << "'\n";
			changes.push_back(l_ossChange.str().c_str());
	} // End if - then


	if (m_tool_number > 0)
	{
		// Make sure the hole depth isn't greater than the tool's depth.
		CTool *pCutter = (CTool *) CTool::Find( m_tool_number );

		if ((pCutter != NULL) && (pCutter->m_params.m_cutting_edge_height < m_depth_op_params.m_final_depth))
		{
			// The tool we've chosen can't cut as deep as we've setup to go.

#ifdef UNICODE
			std::wostringstream l_ossChange;
#else
			std::ostringstream l_ossChange;
#endif

			l_ossChange << _("Adjusting depth of pocket") << " id='" << m_id << "' " << _("from") << " '"
				<< m_depth_op_params.m_final_depth << "' " << _("to") << " "
				<< pCutter->m_params.m_cutting_edge_height << " " << _("due to cutting edge length of selected tool") << "\n";
			changes.push_back(l_ossChange.str().c_str());

			if (apply_changes)
			{
				m_depth_op_params.m_final_depth = pCutter->m_params.m_cutting_edge_height;
			} // End if - then
		} // End if - then

		// Also make sure the 'step-over' distance isn't larger than the tool's diameter.
		if ((pCutter != NULL) && ((pCutter->CuttingRadius(false) * 2.0) < m_pocket_params.m_step_over))
		{
			wxString change;
			change << _("The step-over distance for pocket (id=");
			change << m_id;
			change << _(") is larger than the tool's diameter");
			changes.push_back(change);

			if (apply_changes)
			{
				m_pocket_params.m_step_over = (pCutter->CuttingRadius(false) * 2.0);
			} // End if - then
		} // End if - then
	} // End if - then

	std::list<wxString> depth_op_changes = CDepthOp::DesignRulesAdjustment( apply_changes );
	std::copy( depth_op_changes.begin(), depth_op_changes.end(), std::inserter( changes, changes.end() ) );

	return(changes);

} // End DesignRulesAdjustment() method

static void on_set_spline_deviation(double value, HeeksObj* object){
	CPocket::max_deviation_for_spline_to_arc = value;
	CPocket::WriteToConfig();
}

// static
void CPocket::GetOptions(std::list<Property *> *list)
{
	list->push_back ( new PropertyDouble ( _("Pocket spline deviation"), max_deviation_for_spline_to_arc, NULL, on_set_spline_deviation ) );
}

// static
void CPocket::ReadFromConfig()
{
	CNCConfig config(CPocketParams::ConfigScope());
	config.Read(_T("PocketSplineDeviation"), &max_deviation_for_spline_to_arc, 0.1);
}

// static
void CPocket::WriteToConfig()
{
	CNCConfig config(CPocketParams::ConfigScope());
	config.Write(_T("PocketSplineDeviation"), max_deviation_for_spline_to_arc);
}

static ReselectSketches reselect_sketches;

void CPocket::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
	reselect_sketches.m_sketches = &m_sketches;
	reselect_sketches.m_object = this;
	t_list->push_back(&reselect_sketches);

    CDepthOp::GetTools( t_list, p );
}

bool CPocketParams::operator==(const CPocketParams & rhs) const
{
	if (m_starting_place != rhs.m_starting_place) return(false);
	if (m_material_allowance != rhs.m_material_allowance) return(false);
	if (m_step_over != rhs.m_step_over) return(false);
	if (m_keep_tool_down_if_poss != rhs.m_keep_tool_down_if_poss) return(false);
	if (m_use_zig_zag != rhs.m_use_zig_zag) return(false);
	if (m_zig_angle != rhs.m_zig_angle) return(false);
	if (m_zig_unidirectional != rhs.m_zig_unidirectional) return(false);
	if (m_entry_move != rhs.m_entry_move) return(false);

	return(true);
}

bool CPocket::operator==(const CPocket & rhs) const
{
	if (m_pocket_params != rhs.m_pocket_params) return(false);

	return(CDepthOp::operator==(rhs));
}

static bool OnEdit(HeeksObj* object)
{
	PocketDlg dlg(heeksCAD->GetMainFrame(), (CPocket*)object);
	if(dlg.ShowModal() == wxID_OK)
	{
		dlg.GetData((CPocket*)object);
		((CPocket*)object)->WriteDefaultValues();
		return true;
	}
	return false;
}

void CPocket::GetOnEdit(bool(**callback)(HeeksObj*))
{
	*callback = OnEdit;
}
