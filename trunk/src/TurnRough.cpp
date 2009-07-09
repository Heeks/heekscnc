// TurnRough.cpp
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "stdafx.h"
#include "TurnRough.h"
#include "CNCConfig.h"
#include "ProgramCanvas.h"
#include "Program.h"
#include "interface/HeeksObj.h"
#include "interface/PropertyDouble.h"
#include "interface/PropertyChoice.h"
#include "interface/PropertyVertex.h"
#include "interface/PropertyCheck.h"
#include "interface/PropertyString.h"
#include "tinyxml/tinyxml.h"
#include "interface/Tool.h"
#include "CuttingTool.h"

#include <sstream>
#include <iomanip>

CTurnRoughParams::CTurnRoughParams()
{
	m_tool_number = 0;
	m_cutter_radius = 1.0;
	m_driven_point = 0;
	m_front_angle = 95;
	m_tool_angle = 60;
	m_back_angle = 25;
	m_outside = true;
	m_front = true;
	m_face = false;
}

void CTurnRoughParams::set_initial_values()
{
	CNCConfig config;
	config.Read(_T("TurnRoughCutterRadius"), &m_cutter_radius, 1);
	// to do, the other parameters
}

void CTurnRoughParams::write_values_to_config()
{
	CNCConfig config;
	config.Write(_T("TurnRoughCutterRadius"), m_cutter_radius);
	// to do, the other parameters
}

void CTurnRoughParams::GetProperties(CTurnRough* parent, std::list<Property *> *list)
{

}

void CTurnRoughParams::WriteXMLAttributes(TiXmlNode *root)
{
	TiXmlElement * element;
	element = new TiXmlElement( "params" );
	root->LinkEndChild( element );  
	//element->SetDoubleAttribute("side", m_tool_on_side);
}

void CTurnRoughParams::ReadFromXMLElement(TiXmlElement* pElem)
{
	//pElem->Attribute("side", &m_tool_on_side);
}

void CTurnRough::WriteSketchDefn(HeeksObj* sketch, int id_to_use)
{
	if ((sketch->GetShortString() != NULL) && (wxString(sketch->GetShortString()).size() > 0))
	{
		theApp.m_program_canvas->m_textCtrl->AppendText(wxString::Format(_T("comment('%s')\n"), wxString(sketch->GetShortString()).c_str()));
	}

	theApp.m_program_canvas->m_textCtrl->AppendText(wxString::Format(_T("k%d = kurve.new()\n"), id_to_use > 0 ? id_to_use : sketch->m_id));

	bool started = false;
	int sketch_id = (id_to_use > 0 ? id_to_use : sketch->m_id);

	for(HeeksObj* span_object = sketch->GetFirstChild(); span_object; span_object = sketch->GetNextChild())
	{
		double s[3] = {0, 0, 0};
		double e[3] = {0, 0, 0};
		double c[3] = {0, 0, 0};

		if(span_object){
			int type = span_object->GetType();
			if(type == LineType || type == ArcType || type == CircleType)
			{
				if(!started && type != CircleType)
				{
					span_object->GetStartPoint(s);
					theApp.m_program_canvas->AppendText(_T("kurve.add_point(k"));
					theApp.m_program_canvas->AppendText(sketch_id);
					theApp.m_program_canvas->AppendText(_T(", 0, "));
					theApp.m_program_canvas->AppendText(s[0] / theApp.m_program->m_units);
					theApp.m_program_canvas->AppendText(_T(", "));
					theApp.m_program_canvas->AppendText(s[1] / theApp.m_program->m_units);
					theApp.m_program_canvas->AppendText(_T(", 0.0, 0.0)\n"));
					started = true;
				}
				span_object->GetEndPoint(e);
				if(type == LineType)
				{
					theApp.m_program_canvas->AppendText(_T("kurve.add_point(k"));
					theApp.m_program_canvas->AppendText(sketch_id);
					theApp.m_program_canvas->AppendText(_T(", 0, "));
					theApp.m_program_canvas->AppendText(e[0] / theApp.m_program->m_units);
					theApp.m_program_canvas->AppendText(_T(", "));
					theApp.m_program_canvas->AppendText(e[1] / theApp.m_program->m_units);
					theApp.m_program_canvas->AppendText(_T(", 0.0, 0.0)\n"));
				}
				else if(type == ArcType)
				{
					span_object->GetCentrePoint(c);
					double pos[3];
					heeksCAD->GetArcAxis(span_object, pos);
					int span_type = (pos[2] >=0) ? 1:-1;
					theApp.m_program_canvas->AppendText(_T("kurve.add_point(k"));
					theApp.m_program_canvas->AppendText(sketch_id);
					theApp.m_program_canvas->AppendText(_T(", "));
					theApp.m_program_canvas->AppendText(span_type);
					theApp.m_program_canvas->AppendText(_T(", "));
					theApp.m_program_canvas->AppendText(e[0] / theApp.m_program->m_units);
					theApp.m_program_canvas->AppendText(_T(", "));
					theApp.m_program_canvas->AppendText(e[1] / theApp.m_program->m_units);
					theApp.m_program_canvas->AppendText(_T(", "));
					theApp.m_program_canvas->AppendText(c[0] / theApp.m_program->m_units);
					theApp.m_program_canvas->AppendText(_T(", "));
					theApp.m_program_canvas->AppendText(c[1] / theApp.m_program->m_units);
					theApp.m_program_canvas->AppendText(_T(")\n"));
				}
				else if(type == CircleType)
				{
					span_object->GetCentrePoint(c);

					double radius = heeksCAD->CircleGetRadius(span_object);
					theApp.m_program_canvas->AppendText(_T("kurve.add_point(k"));
					theApp.m_program_canvas->AppendText(sketch_id);
					theApp.m_program_canvas->AppendText(_T(", 0, "));
					theApp.m_program_canvas->AppendText((c[0] + radius) / theApp.m_program->m_units);
					theApp.m_program_canvas->AppendText(_T(", "));
					theApp.m_program_canvas->AppendText(c[1] / theApp.m_program->m_units);
					theApp.m_program_canvas->AppendText(_T(", "));
					theApp.m_program_canvas->AppendText(c[0] / theApp.m_program->m_units);
					theApp.m_program_canvas->AppendText(_T(", "));
					theApp.m_program_canvas->AppendText(c[1] / theApp.m_program->m_units);
					theApp.m_program_canvas->AppendText(_T(")\n"));
					theApp.m_program_canvas->AppendText(_T("kurve.add_point(k"));
					theApp.m_program_canvas->AppendText(sketch_id);
					theApp.m_program_canvas->AppendText(_T(", 1, "));
					theApp.m_program_canvas->AppendText((c[0] - radius) / theApp.m_program->m_units);
					theApp.m_program_canvas->AppendText(_T(", "));
					theApp.m_program_canvas->AppendText(c[1] / theApp.m_program->m_units);
					theApp.m_program_canvas->AppendText(_T(", "));
					theApp.m_program_canvas->AppendText(c[0] / theApp.m_program->m_units);
					theApp.m_program_canvas->AppendText(_T(", "));
					theApp.m_program_canvas->AppendText(c[1] / theApp.m_program->m_units);
					theApp.m_program_canvas->AppendText(_T(")\n"));
					theApp.m_program_canvas->AppendText(_T("kurve.add_point(k"));
					theApp.m_program_canvas->AppendText(sketch_id);
					theApp.m_program_canvas->AppendText(_T(", 1, "));
					theApp.m_program_canvas->AppendText((c[0] + radius) / theApp.m_program->m_units);
					theApp.m_program_canvas->AppendText(_T(", "));
					theApp.m_program_canvas->AppendText(c[1] / theApp.m_program->m_units);
					theApp.m_program_canvas->AppendText(_T(", "));
					theApp.m_program_canvas->AppendText(c[0] / theApp.m_program->m_units);
					theApp.m_program_canvas->AppendText(_T(", "));
					theApp.m_program_canvas->AppendText(c[1] / theApp.m_program->m_units);
					theApp.m_program_canvas->AppendText(_T(")\n"));
				}
			}
		}
	}

	theApp.m_program_canvas->AppendText(_T("\n"));
}

void CTurnRough::AppendTextForOneSketch(HeeksObj* object, int sketch)
{
	if(object)
	{
		WriteSketchDefn(object, sketch);

		// add the machining command
		theApp.m_program_canvas->AppendText(wxString::Format(_T("turning.rough(k%d, "), sketch));
		theApp.m_program_canvas->AppendText(m_turn_rough_params.m_cutter_radius / theApp.m_program->m_units);
		theApp.m_program_canvas->AppendText(_T(", "));
		theApp.m_program_canvas->AppendText(m_turn_rough_params.m_front_angle);
		theApp.m_program_canvas->AppendText(_T(")\n"));
		// to do, the other parameters
	}
}

void CTurnRough::AppendTextToProgram()
{
	COp::AppendTextToProgram();

	for(std::list<int>::iterator It = m_sketches.begin(); It != m_sketches.end(); It++)
	{
		int sketch = *It;

		// write a kurve definition
		HeeksObj* object = heeksCAD->GetIDObject(SketchType, sketch);
		if(object == NULL || object->GetNumChildren() == 0)continue;

		HeeksObj* re_ordered_sketch = NULL;
		SketchOrderType sketch_order = heeksCAD->GetSketchOrder(object);
		if(sketch_order == SketchOrderTypeBad)
		{
			re_ordered_sketch = object->MakeACopy();
			heeksCAD->ReOrderSketch(re_ordered_sketch, SketchOrderTypeReOrder);
			object = re_ordered_sketch;
		}

		if(sketch_order == SketchOrderTypeMultipleCurves || sketch_order == SketchOrderHasCircles)
		{
			std::list<HeeksObj*> new_separate_sketches;
			heeksCAD->ExtractSeparateSketches(object, new_separate_sketches);
			for(std::list<HeeksObj*>::iterator It = new_separate_sketches.begin(); It != new_separate_sketches.end(); It++)
			{
				HeeksObj* one_curve_sketch = *It;
				AppendTextForOneSketch(one_curve_sketch, sketch);
				delete one_curve_sketch;
			}
		}
		else
		{
			AppendTextForOneSketch(object, sketch);
		}

		if(re_ordered_sketch)
		{
			delete re_ordered_sketch;
		}
	}
}

void CTurnRough::glCommands(bool select, bool marked, bool no_color)
{
	if(marked && !no_color)
	{
		// show the sketches as highlighted
		for(std::list<int>::iterator It = m_sketches.begin(); It != m_sketches.end(); It++)
		{
			int sketch = *It;
			HeeksObj* object = heeksCAD->GetIDObject(SketchType, sketch);
			if(object)object->glCommands(false, true, false);
		}
	}
}

void CTurnRough::GetProperties(std::list<Property *> *list)
{
	list->push_back(new PropertyString(_T("TO DO!"), _T("THIS OPERATION DOESN'T WORK YET!"), this, NULL));

	m_turn_rough_params.GetProperties(this, list);

	COp::GetProperties(list);
}

HeeksObj *CTurnRough::MakeACopy(void)const
{
	return new CTurnRough(*this);
}

void CTurnRough::CopyFrom(const HeeksObj* object)
{
	operator=(*((CTurnRough*)object));
}

bool CTurnRough::CanAddTo(HeeksObj* owner)
{
	return owner->GetType() == OperationsType;
}

void CTurnRough::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element = new TiXmlElement( "TurnRough" );
	root->LinkEndChild( element );  
	m_turn_rough_params.WriteXMLAttributes(element);

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
HeeksObj* CTurnRough::ReadFromXMLElement(TiXmlElement* element)
{
	CTurnRough* new_object = new CTurnRough;

	// read parameters
	TiXmlElement* params = TiXmlHandle(element).FirstChildElement("params").Element();
	if(params)new_object->m_turn_rough_params.ReadFromXMLElement(params);

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
