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
#include "interface/HeeksObj.h"
#include "interface/PropertyDouble.h"
#include "interface/PropertyString.h"
#include "interface/PropertyChoice.h"
#include "interface/PropertyVertex.h"
#include "interface/PropertyCheck.h"
#include "tinyxml/tinyxml.h"

#include <sstream>

CPocketParams::CPocketParams()
{
	m_step_over = 0.0;
	m_material_allowance = 0.0;
	m_round_corner_factor = 0.0;
	m_starting_place = true;
}

void CPocketParams::set_initial_values()
{
	CNCConfig config;
	config.Read(_T("PocketStepOver"), &m_step_over, 1.0);
	config.Read(_T("PocketMaterialAllowance"), &m_material_allowance, 0.2);
	config.Read(_T("PocketRoundCornerFactor"), &m_round_corner_factor, 1.5);
	config.Read(_T("FromCenter"), &m_starting_place, 1);
}

void CPocketParams::write_values_to_config()
{
	CNCConfig config;
	config.Write(_T("PocketStepOver"), m_step_over);
	config.Write(_T("PocketMaterialAllowance"), m_material_allowance);
	config.Write(_T("PocketRoundCornerFactor"), m_round_corner_factor);
	config.Write(_T("FromCenter"), m_starting_place);
}

static void on_set_step_over(double value, HeeksObj* object){((CPocket*)object)->m_pocket_params.m_step_over = value;}
static void on_set_material_allowance(double value, HeeksObj* object){((CPocket*)object)->m_pocket_params.m_material_allowance = value;}
static void on_set_round_corner_factor(double value, HeeksObj* object){((CPocket*)object)->m_pocket_params.m_round_corner_factor = value;}
static void on_set_starting_place(int value, HeeksObj* object){((CPocket*)object)->m_pocket_params.m_starting_place = value;}

void CPocketParams::GetProperties(CPocket* parent, std::list<Property *> *list)
{
	list->push_back(new PropertyDouble(_("step over"), m_step_over, parent, on_set_step_over));
	list->push_back(new PropertyDouble(_("material allowance"), m_material_allowance, parent, on_set_material_allowance));
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

static void WriteSketchDefn(HeeksObj* sketch, int id_to_use = 0)
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
				if(started && (fabs(s[0] - prev_e[0]) > 0.000000001 || fabs(s[1] - prev_e[1]) > 0.000000001))
				{
					theApp.m_program_canvas->m_textCtrl->AppendText(wxString::Format(_T("area.start_new_curve(a%d)\n"), id_to_use > 0 ? id_to_use : sketch->m_id));
					started = false;
				}

				if(!started)
				{
#ifdef UNICODE
					std::wostringstream ss;
#else
					std::ostringstream ss;
#endif
					ss.imbue(std::locale("C"));

					ss << "area.add_point(a" << (id_to_use > 0 ? id_to_use : sketch->m_id) << ", 0, " << s[0] << ", " << s[1] << ", 0, 0)\n";
					theApp.m_program_canvas->m_textCtrl->AppendText(ss.str().c_str());
					started = true;
				}
				span_object->GetEndPoint(e);
				if(type == LineType)
				{
#ifdef UNICODE
					std::wostringstream ss;
#else
					std::ostringstream ss;
#endif
					ss.imbue(std::locale("C"));

					ss << "area.add_point(a" << (id_to_use > 0 ? id_to_use : sketch->m_id) << ", 0, " << e[0] << ", " << e[1] << ", 0, 0)\n";
					theApp.m_program_canvas->m_textCtrl->AppendText(ss.str().c_str());
				}
				else if(type == ArcType)
				{
					span_object->GetCentrePoint(c);
					double pos[3];
					heeksCAD->GetArcAxis(span_object, pos);
					int span_type = (pos[2] >=0) ? 1:-1;
#ifdef UNICODE
					std::wostringstream ss;
#else
					std::ostringstream ss;
#endif
					ss.imbue(std::locale("C"));

					ss << "area.add_point(a" << (id_to_use > 0 ? id_to_use : sketch->m_id) << ", " << span_type << ", " << e[0] << ", " << e[1] << ", " << c[0] << ", " << c[1] << ")\n";
					theApp.m_program_canvas->m_textCtrl->AppendText(ss.str().c_str());
				}
				memcpy(prev_e, e, 3*sizeof(double));
			}
		}
	}

	theApp.m_program_canvas->m_textCtrl->AppendText(_T("\n"));
}

void CPocket::AppendTextToProgram()
{
	CDepthOp::AppendTextToProgram();

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
		if(order != SketchOrderTypeCloseCW && order != SketchOrderTypeCloseCCW && order != SketchOrderTypeMultipleCurves)
		{
			re_ordered_sketch = object->MakeACopy();
			heeksCAD->ReOrderSketch(re_ordered_sketch, SketchOrderTypeReOrder);
			object = re_ordered_sketch;
			order = heeksCAD->GetSketchOrder(object);
			if(order != SketchOrderTypeCloseCW && order != SketchOrderTypeCloseCCW && order != SketchOrderTypeMultipleCurves)
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
			WriteSketchDefn(object, sketch);

			// start - assume we are at a suitable clearance height

			// Pocket the area
 #ifdef UNICODE
			std::wostringstream ss;
#else
			std::ostringstream ss;
#endif
			ss.imbue(std::locale("C"));
			ss << "area_funcs.pocket(a" << sketch <<", tool_diameter/2 + " << m_pocket_params.m_material_allowance << ", rapid_down_to_height, start_depth, final_depth, " << m_pocket_params.m_step_over << ", step_down, " << m_pocket_params.m_round_corner_factor << ", clearance, " << m_pocket_params.m_starting_place << ")\n";
			theApp.m_program_canvas->m_textCtrl->AppendText(ss.str().c_str());

			//theApp.m_program_canvas->m_textCtrl->AppendText(wxString::Format(_T("area_funcs.pocket(a%d, tool_diameter/2 + %g, rapid_down_to_height, final_depth, %g, %g, %g)\n"), sketch, m_pocket_params.m_material_allowance, m_pocket_params.m_step_over, m_pocket_params.m_step_down, m_pocket_params.m_round_corner_factor));

			// rapid back up to clearance plane
			theApp.m_program_canvas->m_textCtrl->AppendText(wxString(_T("rapid(z = clearance)\n")));			
		}

		if(re_ordered_sketch)
		{
			delete re_ordered_sketch;
		}
	}
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
