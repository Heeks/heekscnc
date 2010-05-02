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
#include "CuttingTool.h"
#include "geometry.h"
#include "CNCPoint.h"
#include "Reselect.h"

#include <sstream>

// static
double CPocket::max_deviation_for_spline_to_arc = 0.1;

CPocketParams::CPocketParams()
{
	m_step_over = 0.0;
	m_material_allowance = 0.0;
	m_round_corner_factor = 0.0;
	m_starting_place = true;
}

void CPocketParams::set_initial_values(const CCuttingTool::ToolNumber_t cutting_tool_number)
{
    if (cutting_tool_number > 0)
    {
        CCuttingTool *pCuttingTool = CCuttingTool::Find(cutting_tool_number);
        if (pCuttingTool != NULL)
        {
            m_step_over = pCuttingTool->CuttingRadius() * 3.0 / 5.0;
        }
    }
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

static void on_set_round_corner_factor(double value, HeeksObj* object)
{
	((CPocket*)object)->m_pocket_params.m_round_corner_factor = value;
	((CPocket*)object)->WriteDefaultValues();
}

static void on_set_starting_place(int value, HeeksObj* object)
{
	((CPocket*)object)->m_pocket_params.m_starting_place = value;
	((CPocket*)object)->WriteDefaultValues();
}

void CPocketParams::GetProperties(CPocket* parent, std::list<Property *> *list)
{
	list->push_back(new PropertyLength(_("step over"), m_step_over, parent, on_set_step_over));
	list->push_back(new PropertyLength(_("material allowance"), m_material_allowance, parent, on_set_material_allowance));
	list->push_back(new PropertyDouble(_("round corner factor"), m_round_corner_factor, parent, on_set_round_corner_factor));
	list->push_back(new PropertyString(wxString(_T("( ")) + _("for 90 degree corners") + _T(" )"), wxString(_T("( ")) + _("1.5 for square, 1.0 for round")  + _T(" )"), NULL));
	{
		std::list< wxString > choices;
		choices.push_back(_("Boundary"));
		choices.push_back(_("Center"));
		list->push_back(new PropertyChoice(_("starting_place"), choices, m_starting_place, parent, on_set_starting_place));
	}
}

void CPocketParams::WriteXMLAttributes(TiXmlNode *root)
{
	TiXmlElement * element;
	element = new TiXmlElement( "params" );
	root->LinkEndChild( element );
	element->SetDoubleAttribute("step", m_step_over);
	element->SetDoubleAttribute("mat", m_material_allowance);
	element->SetDoubleAttribute("rf", m_round_corner_factor);
	element->SetAttribute("from_center", m_starting_place);
}

void CPocketParams::ReadFromXMLElement(TiXmlElement* pElem)
{
	pElem->Attribute("step", &m_step_over);
	pElem->Attribute("mat", &m_material_allowance);
	pElem->Attribute("rf", &m_round_corner_factor);
	pElem->Attribute("from_center", &m_starting_place);
}

static wxString WriteSketchDefn(HeeksObj* sketch, const CFixture *pFixture, int id_to_use = 0)
{
#ifdef UNICODE
	std::wostringstream gcode;
#else
    std::ostringstream gcode;
#endif
    gcode.imbue(std::locale("C"));
	gcode << std::setprecision(10);

	gcode << wxString::Format(_T("a%d = area.new()\n"), id_to_use > 0 ? id_to_use : sketch->m_id).c_str();

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
				CNCPoint start(pFixture->Adjustment(s));

				if(started && (fabs(s[0] - prev_e[0]) > 0.000000001 || fabs(s[1] - prev_e[1]) > 0.000000001))
				{
					gcode << _T("area.start_new_curve(a");
					gcode << (unsigned int) (id_to_use > 0 ? id_to_use : sketch->m_id);
					gcode << _T(")\n");
					started = false;
				}

				if(!started)
				{
					gcode << _T("area.add_point(a");
					gcode << (unsigned int) (id_to_use > 0 ? id_to_use : sketch->m_id);
					gcode << _T(", 0, ") << start.X(true) << _T(", ") << start.Y(true) << _T(", 0, 0)\n");
					started = true;
				}
				span_object->GetEndPoint(e);
				CNCPoint end(pFixture->Adjustment(e));

				if(type == LineType)
				{
					gcode << _T("area.add_point(a");
					gcode << (unsigned int) (id_to_use > 0 ? id_to_use : sketch->m_id);
					gcode << _T(", 0, ") << end.X(true) << _T(", ") << end.Y(true) << _T(", 0, 0)\n");
				}
				else if(type == ArcType)
				{
					span_object->GetCentrePoint(c);
					CNCPoint centre(pFixture->Adjustment(c));

					double pos[3];
					heeksCAD->GetArcAxis(span_object, pos);
					int span_type = (pos[2] >=0) ? 1:-1;
					gcode << _T("area.add_point(a");
					gcode << (unsigned int) (id_to_use > 0 ? id_to_use : sketch->m_id);
					gcode << _T(", ") << span_type << _T(", ") << end.X(true) << _T(", ") << end.Y(true);
					gcode << _T(", ") << centre.X(true) << _T(", ") << centre.Y(true) << _T(")\n");
				}
				memcpy(prev_e, e, 3*sizeof(double));
			} // End if - then
			else
			{
				if (type == CircleType)
				{
					std::list< std::pair<int, gp_Pnt > > points;
					span_object->GetCentrePoint(c);

					// Setup the four arcs that will make up the circle using UNadjusted
					// coordinates first so that the offsets align with the X and Y axes.
					double small_amount = 0.001;
					double radius = heeksCAD->CircleGetRadius(span_object);

					points.push_back( std::make_pair(LINEAR, gp_Pnt( c[0] - small_amount, c[1] + radius, c[2] )) ); // north (almost)
					points.push_back( std::make_pair(CW, gp_Pnt( c[0], c[1] + radius, c[2] )) ); // north
					points.push_back( std::make_pair(CW, gp_Pnt( c[0] + radius, c[1], c[2] )) ); // east
					points.push_back( std::make_pair(CW, gp_Pnt( c[0], c[1] - radius, c[2] )) ); // south
					points.push_back( std::make_pair(CW, gp_Pnt( c[0] - radius, c[1], c[2] )) ); // west
					points.push_back( std::make_pair(CW, gp_Pnt( c[0] - small_amount, c[1] + radius, c[2] )) ); // north (almost)

					CNCPoint centre(pFixture->Adjustment(c));

					for (std::list< std::pair<int, gp_Pnt > >::iterator l_itPoint = points.begin(); l_itPoint != points.end(); l_itPoint++)
					{
						CNCPoint pnt = pFixture->Adjustment( l_itPoint->second );

						gcode << _T("area.add_point(a");
						gcode << (id_to_use > 0 ? id_to_use : sketch->m_id);
						gcode << _T(", ") << l_itPoint->first << _T(", ");
						gcode << pnt.X(true) << (_T(", ")) << pnt.Y(true);
						gcode << _T(", ") << centre.X(true) << _T(", ") << centre.Y(true) << _T(")\n");
					} // End for
				}
			} // End if - else
		}
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

void CPocket::AppendTextToProgram(const CFixture *pFixture)
{
	wxString gcode = GenerateGCode(pFixture);
	theApp.m_program_canvas->AppendText(gcode.c_str());
}

wxString CPocket::GenerateGCode(const CFixture *pFixture)
{
#ifdef UNICODE
	std::wostringstream gcode;
#else
    std::ostringstream gcode;
#endif
    gcode.imbue(std::locale("C"));
	gcode<<std::setprecision(10);

    ReloadPointers();   // Make sure all the m_sketches values have been converted into children.

	CCuttingTool *pCuttingTool = CCuttingTool::Find( m_cutting_tool_number );
	if (pCuttingTool == NULL)
	{
		wxMessageBox(_T("Cannot generate GCode for pocket without a cutting tool assigned"));
		return(_T(""));
	} // End if - then

	gcode << CDepthOp::GenerateGCode(pFixture).c_str();

    for (HeeksObj *object = GetFirstChild(); object != NULL; object = GetNextChild())
    {
		if(object == NULL || object->GetNumChildren() == 0){
			wxMessageBox(wxString::Format(_("Pocket operation - Sketch doesn't exist")));
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
			gcode << WriteSketchDefn(object, pFixture, object->m_id).c_str();

			// start - assume we are at a suitable clearance height

			// Pocket the area
			gcode << _T("area_funcs.pocket(a") << object->m_id << _T(", tool_diameter/2 + ");
			gcode << m_pocket_params.m_material_allowance / theApp.m_program->m_units;
			gcode << _T(", rapid_down_to_height, start_depth, final_depth, ");
			gcode << m_pocket_params.m_step_over / theApp.m_program->m_units;
			gcode << _T(", step_down, ");
			gcode << m_pocket_params.m_round_corner_factor;
			gcode << _T(", clearance, ");
			gcode << m_pocket_params.m_starting_place << _T(")\n");

			// rapid back up to clearance plane
			gcode << _T("rapid(z = clearance)\n");
		}

		if(re_ordered_sketch)
		{
			delete re_ordered_sketch;
		}
	} // End for

	return(wxString(gcode.str().c_str()));

} // End GenerateGCode() method


void CPocket::WriteDefaultValues()
{
	CDepthOp::WriteDefaultValues();

	CNCConfig config(CPocketParams::ConfigScope());
	config.Write(_T("StepOver"), m_pocket_params.m_step_over);
	config.Write(_T("MaterialAllowance"), m_pocket_params.m_material_allowance);
	config.Write(_T("RoundCornerFactor"), m_pocket_params.m_round_corner_factor);
	config.Write(_T("FromCenter"), m_pocket_params.m_starting_place);
}

void CPocket::ReadDefaultValues()
{
	CDepthOp::ReadDefaultValues();

	CNCConfig config(CPocketParams::ConfigScope());
	config.Read(_T("StepOver"), &m_pocket_params.m_step_over, 1.0);
	config.Read(_T("MaterialAllowance"), &m_pocket_params.m_material_allowance, 0.2);
	config.Read(_T("RoundCornerFactor"), &m_pocket_params.m_round_corner_factor, 1.0);
	config.Read(_T("FromCenter"), &m_pocket_params.m_starting_place, 1);
}

void CPocket::glCommands(bool select, bool marked, bool no_color)
{
	CDepthOp::glCommands( select, marked, no_color );
}

void CPocket::GetProperties(std::list<Property *> *list)
{
	AddSketchesProperties(list, m_sketches);
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
	*this = rhs;	// Call the assignment operator.
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
	return owner->GetType() == OperationsType;
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

CPocket::CPocket(const std::list<int> &sketches, const int cutting_tool_number )
	: CDepthOp(GetTypeString(), &sketches, cutting_tool_number ), m_sketches(sketches)
{
	ReadDefaultValues();
	m_pocket_params.set_initial_values(cutting_tool_number);

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

CPocket::CPocket(const std::list<HeeksObj *> &sketches, const int cutting_tool_number )
	: CDepthOp(GetTypeString(), sketches, cutting_tool_number )
{
	ReadDefaultValues();
	m_pocket_params.set_initial_values(cutting_tool_number);

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

	std::list<int> invalid_sketches;
	for(std::list<int>::iterator l_itSketch = m_sketches.begin(); l_itSketch != m_sketches.end(); l_itSketch++)
	{
		HeeksObj *obj = heeksCAD->GetIDObject( SketchType, *l_itSketch );
		if (obj == NULL)
		{
#ifdef UNICODE
			std::wostringstream l_ossChange;
#else
			std::ostringstream l_ossChange;
#endif

			l_ossChange << _("Invalid reference to sketch") << " id='" << *l_itSketch << "' " << _("in pocket operations") << " id='" << m_id << "'\n";
			changes.push_back(l_ossChange.str().c_str());

			if (apply_changes)
			{
				invalid_sketches.push_back( *l_itSketch );
			} // End if - then
		} // End if - then
	} // End for

	if (apply_changes)
	{
		for(std::list<int>::iterator l_itSketch = invalid_sketches.begin(); l_itSketch != invalid_sketches.end(); l_itSketch++)
		{
			std::list<int>::iterator l_itToRemove = std::find( m_sketches.begin(), m_sketches.end(), *l_itSketch );
			if (l_itToRemove != m_sketches.end())
			{
				m_sketches.erase(l_itToRemove);
			} // End while
		} // End for
	} // End if - then

	if (m_sketches.size() == 0)
	{
#ifdef UNICODE
			std::wostringstream l_ossChange;
#else
			std::ostringstream l_ossChange;
#endif

			l_ossChange << _("No valid sketches upon which to act for pocket operations") << " id='" << m_id << "'\n";
			changes.push_back(l_ossChange.str().c_str());
	} // End if - then


	if (m_cutting_tool_number > 0)
	{
		// Make sure the hole depth isn't greater than the tool's cutting depth.
		CCuttingTool *pCutter = (CCuttingTool *) CCuttingTool::Find( m_cutting_tool_number );

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

		// Also make sure the 'step-over' distance isn't larger than the cutting tool's diameter.
		if ((pCutter != NULL) && ((pCutter->CuttingRadius(false) * 2.0) < m_pocket_params.m_step_over))
		{
			wxString change;
			change << _("The step-over distance for pocket (id=");
			change << m_id;
			change << _(") is larger than the cutting tool's diameter");
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
