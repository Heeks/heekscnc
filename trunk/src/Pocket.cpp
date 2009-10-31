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

#include <sstream>

CPocketParams::CPocketParams()
{
	m_step_over = 0.0;
	m_material_allowance = 0.0;
	m_round_corner_factor = 0.0;
	m_starting_place = true;
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

static void WriteSketchDefn(HeeksObj* sketch, const CFixture *pFixture, int id_to_use = 0)
{
	theApp.m_program_canvas->m_textCtrl->AppendText(wxString::Format(_T("a%d = area.new()\n"), id_to_use > 0 ? id_to_use : sketch->m_id));

	bool started = false;

	double prev_e[3];

	for(HeeksObj* span_object = sketch->GetFirstChild(); span_object; span_object = sketch->GetNextChild())
	{
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
					theApp.m_program_canvas->AppendText(_T("area.start_new_curve(a"));
					theApp.m_program_canvas->AppendText(id_to_use > 0 ? id_to_use : sketch->m_id);
					theApp.m_program_canvas->AppendText(_T(")\n"));
					started = false;
				}

				if(!started)
				{
					theApp.m_program_canvas->AppendText(_T("area.add_point(a"));
					theApp.m_program_canvas->AppendText(id_to_use > 0 ? id_to_use : sketch->m_id);
					theApp.m_program_canvas->AppendText(_T(", 0, "));
					theApp.m_program_canvas->AppendText(start.X(true));
					theApp.m_program_canvas->AppendText(_T(", "));
					theApp.m_program_canvas->AppendText(start.Y(true));
					theApp.m_program_canvas->AppendText(_T(", 0, 0)\n"));
					started = true;
				}
				span_object->GetEndPoint(e);
				CNCPoint end(pFixture->Adjustment(e));

				if(type == LineType)
				{
					theApp.m_program_canvas->AppendText(_T("area.add_point(a"));
					theApp.m_program_canvas->AppendText(id_to_use > 0 ? id_to_use : sketch->m_id);
					theApp.m_program_canvas->AppendText(_T(", 0, "));
					theApp.m_program_canvas->AppendText(end.X(true));
					theApp.m_program_canvas->AppendText(_T(", "));
					theApp.m_program_canvas->AppendText(end.Y(true));
					theApp.m_program_canvas->AppendText(_T(", 0, 0)\n"));
				}
				else if(type == ArcType)
				{
					span_object->GetCentrePoint(c);
					CNCPoint centre(pFixture->Adjustment(c));

					double pos[3];
					heeksCAD->GetArcAxis(span_object, pos);
					int span_type = (pos[2] >=0) ? 1:-1;
					theApp.m_program_canvas->AppendText(_T("area.add_point(a"));
					theApp.m_program_canvas->AppendText(id_to_use > 0 ? id_to_use : sketch->m_id);
					theApp.m_program_canvas->AppendText(_T(", "));
					theApp.m_program_canvas->AppendText(span_type);
					theApp.m_program_canvas->AppendText(_T(", "));
					theApp.m_program_canvas->AppendText(end.X(true));
					theApp.m_program_canvas->AppendText(_T(", "));
					theApp.m_program_canvas->AppendText(end.Y(true));
					theApp.m_program_canvas->AppendText(_T(", "));
					theApp.m_program_canvas->AppendText(centre.X(true));
					theApp.m_program_canvas->AppendText(_T(", "));
					theApp.m_program_canvas->AppendText(centre.Y(true));
					theApp.m_program_canvas->AppendText(_T(")\n"));
				}
				memcpy(prev_e, e, 3*sizeof(double));
			} // End if - then
			else
			{
				if (type == CircleType)
				{
#ifdef UNICODE
					std::wostringstream l_ossPythonCode;
#else
					std::ostringstream l_ossPythonCode;
#endif

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

						l_ossPythonCode << (_T("area.add_point(a"));
						l_ossPythonCode << (id_to_use > 0 ? id_to_use : sketch->m_id);
						l_ossPythonCode << _T(", ") << l_itPoint->first << _T(", ");
						l_ossPythonCode << pnt.X(true);
						l_ossPythonCode << (_T(", "));
						l_ossPythonCode << pnt.Y(true);
						l_ossPythonCode << (_T(", "));
						l_ossPythonCode << centre.X(true);
						l_ossPythonCode << (_T(", "));
						l_ossPythonCode << centre.Y(true);
						l_ossPythonCode << (_T(")\n"));
					} // End for

					theApp.m_program_canvas->AppendText(l_ossPythonCode.str().c_str());
				}
			} // End if - else
		}
	}

	theApp.m_program_canvas->AppendText(_T("\n"));
}

void CPocket::AppendTextToProgram(const CFixture *pFixture)
{
	CCuttingTool *pCuttingTool = CCuttingTool::Find( m_cutting_tool_number );
	if (pCuttingTool == NULL)
	{
		wxMessageBox(_T("Cannot generate GCode for pocket without a cutting tool assigned"));
		return;
	} // End if - then

	CDepthOp::AppendTextToProgram(pFixture);

	for(std::list<int>::iterator It = m_sketches.begin(); It != m_sketches.end(); It++)
	{
		int sketch = *It;

		// write an area definition
		HeeksObj* object = heeksCAD->GetIDObject(SketchType, sketch);
		if(object == NULL || object->GetNumChildren() == 0){
			wxMessageBox(wxString::Format(_("Pocket operation - Sketch doesn't exist - %d"), sketch));
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
						wxMessageBox(wxString::Format(_("Pocket operation - Sketch must be a closed shape - sketch %d"), sketch));
						delete re_ordered_sketch;
						continue;
					}
					break;

				default:
					{
						wxMessageBox(wxString::Format(_("Pocket operation - Badly ordered sketch - sketch %d"), sketch));
						delete re_ordered_sketch;
						continue;
					}
					break;
				}
			}
		}

		if(object)
		{
			WriteSketchDefn(object, pFixture, sketch);

			// start - assume we are at a suitable clearance height

			// Pocket the area
			theApp.m_program_canvas->AppendText(_T("area_funcs.pocket(a"));
			theApp.m_program_canvas->AppendText(sketch);
			theApp.m_program_canvas->AppendText(_T(", tool_diameter/2 + "));
			theApp.m_program_canvas->AppendText(m_pocket_params.m_material_allowance / theApp.m_program->m_units);
			theApp.m_program_canvas->AppendText(_T(", rapid_down_to_height, start_depth, final_depth, "));
			theApp.m_program_canvas->AppendText(m_pocket_params.m_step_over / theApp.m_program->m_units);
			theApp.m_program_canvas->AppendText(_T(", step_down, "));
			theApp.m_program_canvas->AppendText(m_pocket_params.m_round_corner_factor);
			theApp.m_program_canvas->AppendText(_T(", clearance, "));
			theApp.m_program_canvas->AppendText(m_pocket_params.m_starting_place);
			theApp.m_program_canvas->AppendText(_T(")\n"));

			// rapid back up to clearance plane
			theApp.m_program_canvas->AppendText(_T("rapid(z = clearance)\n"));			
		}

		if(re_ordered_sketch)
		{
			delete re_ordered_sketch;
		}
	}
}
void CPocket::WriteDefaultValues()
{
	CDepthOp::WriteDefaultValues();

	CNCConfig config;
	config.Write(wxString(GetTypeString()) + _T("StepOver"), m_pocket_params.m_step_over);
	config.Write(wxString(GetTypeString()) + _T("MaterialAllowance"), m_pocket_params.m_material_allowance);
	config.Write(wxString(GetTypeString()) + _T("RoundCornerFactor"), m_pocket_params.m_round_corner_factor);
	config.Write(wxString(GetTypeString()) + _T("FromCenter"), m_pocket_params.m_starting_place);
}

void CPocket::ReadDefaultValues()
{
	CDepthOp::ReadDefaultValues();

	CNCConfig config;
	config.Read(wxString(GetTypeString()) + _T("StepOver"), &m_pocket_params.m_step_over, 1.0);
	config.Read(wxString(GetTypeString()) + _T("MaterialAllowance"), &m_pocket_params.m_material_allowance, 0.2);
	config.Read(wxString(GetTypeString()) + _T("RoundCornerFactor"), &m_pocket_params.m_round_corner_factor, 1.5);
	config.Read(wxString(GetTypeString()) + _T("FromCenter"), &m_pocket_params.m_starting_place, 1);
}

void CPocket::glCommands(bool select, bool marked, bool no_color)
{
	if(marked && !no_color)
	{
		for(std::list<int>::iterator It = m_sketches.begin(); It != m_sketches.end(); It++)
		{
			int sketch = *It;
			HeeksObj* object = heeksCAD->GetIDObject(SketchType, sketch);
			if(object)object->glCommands(false, true, false);
		}
	}
}

void CPocket::GetProperties(std::list<Property *> *list)
{
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

	// read profile parameters
	TiXmlElement* params = TiXmlHandle(element).FirstChildElement("params").Element();
	if(params)new_object->m_pocket_params.ReadFromXMLElement(params);

	// read sketch ids
	for(TiXmlElement* sketch = TiXmlHandle(element).FirstChildElement("sketch").Element(); sketch; sketch = sketch->NextSiblingElement())
	{
		int id = 0;
		sketch->Attribute("id", &id);
		if(id)new_object->m_sketches.push_back(id);
	}

	// read common parameters
	new_object->ReadBaseXML(element);

	return new_object;
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
	} // End if - then

	std::list<wxString> depth_op_changes = CDepthOp::DesignRulesAdjustment( apply_changes );
	std::copy( depth_op_changes.begin(), depth_op_changes.end(), std::inserter( changes, changes.end() ) );

	return(changes);

} // End DesignRulesAdjustment() method
