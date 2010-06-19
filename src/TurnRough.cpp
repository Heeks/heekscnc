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
#include "interface/PropertyLength.h"
#include "interface/PropertyCheck.h"
#include "interface/PropertyString.h"
#include "tinyxml/tinyxml.h"
#include "interface/Tool.h"
#include "CuttingTool.h"
#include "Reselect.h"
#include "MachineState.h"

#include <sstream>
#include <iomanip>

CTurnRoughParams::CTurnRoughParams()
{
	m_outside = true;
	m_front = true;
	m_facing = false;
	m_clearance = 0.0;
}

void CTurnRoughParams::set_initial_values()
{
	CNCConfig config(ConfigScope());
	config.Read(_T("TurnRoughOutside"), &m_outside, true);
	config.Read(_T("TurnRoughFront"), &m_front, true);
	config.Read(_T("TurnRoughFacing"), &m_facing, false);
	config.Read(_T("TurnRoughClearance"), &m_clearance, 2.0);
}

void CTurnRoughParams::write_values_to_config()
{
	CNCConfig config(ConfigScope());
	config.Write(_T("TurnRoughOutside"), m_outside);
	config.Write(_T("TurnRoughFront"), m_front);
	config.Write(_T("TurnRoughFacing"), m_facing);
	config.Write(_T("TurnRoughClearance"), m_clearance);
}

static void on_set_outside(bool value, HeeksObj* object){((CTurnRough*)object)->m_turn_rough_params.m_outside = value;}
static void on_set_front(bool value, HeeksObj* object){((CTurnRough*)object)->m_turn_rough_params.m_front = value;}
static void on_set_facing(bool value, HeeksObj* object){((CTurnRough*)object)->m_turn_rough_params.m_facing = value;}
static void on_set_clearance(double value, HeeksObj* object){((CTurnRough*)object)->m_turn_rough_params.m_clearance = value;}

void CTurnRoughParams::GetProperties(CTurnRough* parent, std::list<Property *> *list)
{
	list->push_back(new PropertyCheck(_T("outside"), m_outside, parent, on_set_outside));
	list->push_back(new PropertyCheck(_T("front"), m_front, parent, on_set_front));
	list->push_back(new PropertyCheck(_T("facing"), m_facing, parent, on_set_facing));
	list->push_back(new PropertyLength(_T("clearance"), m_clearance, parent, on_set_clearance));
}

void CTurnRoughParams::WriteXMLAttributes(TiXmlNode *root)
{
	TiXmlElement * element;
	element = new TiXmlElement( "params" );
	root->LinkEndChild( element );
	element->SetAttribute("outside", m_outside ? 1:0);
	element->SetAttribute("front", m_front ? 1:0);
	element->SetAttribute("facing", m_facing ? 1:0);
	element->SetDoubleAttribute("clearance", m_clearance);
}

void CTurnRoughParams::ReadFromXMLElement(TiXmlElement* pElem)
{
	int int_for_bool;
	if(pElem->Attribute("outside", &int_for_bool))m_outside = (int_for_bool != 0);
	if(pElem->Attribute("front", &int_for_bool))m_front = (int_for_bool != 0);
	if(pElem->Attribute("facing", &int_for_bool))m_facing = (int_for_bool != 0);
	pElem->Attribute("clearance", &m_clearance);
}

const wxBitmap &CTurnRough::GetIcon()
{
	if(!m_active)return GetInactiveIcon();
	static wxBitmap* icon = NULL;
	if(icon == NULL)icon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/turnrough.png")));
	return *icon;
}

Python CTurnRough::WriteSketchDefn(HeeksObj* sketch, int id_to_use, CMachineState *pMachineState )
{
	Python python;

	if ((sketch->GetShortString() != NULL) && (wxString(sketch->GetShortString()).size() > 0))
	{
		python << wxString::Format(_T("comment(R'%s')\n"), wxString(sketch->GetShortString()).c_str());
	}

	python << wxString::Format(_T("k%d = kurve.new()\n"), id_to_use > 0 ? id_to_use : sketch->m_id);

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
					pMachineState->Fixture().Adjustment(s);
					python << _T("kurve.add_point(k") << sketch_id << _T(", 0, ") << (double) s[0] / theApp.m_program->m_units;
					python << _T(", ") << s[1] / theApp.m_program->m_units << _T(", 0.0, 0.0)\n");
					started = true;
				}
				span_object->GetEndPoint(e);
				pMachineState->Fixture().Adjustment(e);
				if(type == LineType)
				{
					python << _T("kurve.add_point(k") << sketch_id << _T(", 0, ");
					python << e[0] / theApp.m_program->m_units << _T(", ") << e[1] / theApp.m_program->m_units << _T(", 0.0, 0.0)\n");
				}
				else if(type == ArcType)
				{
					span_object->GetCentrePoint(c);
					pMachineState->Fixture().Adjustment(c);
					double pos[3];
					heeksCAD->GetArcAxis(span_object, pos);
					int span_type = (pos[2] >=0) ? 1:-1;
					python << _T("kurve.add_point(k") << sketch_id << _T(", ") << span_type;
					python << _T(", ") << e[0] / theApp.m_program->m_units << _T(", ");
					python << e[1] / theApp.m_program->m_units << _T(", ");
					python << c[0] / theApp.m_program->m_units << _T(", ");
					python << c[1] / theApp.m_program->m_units << _T(")\n");
				}
				else if(type == CircleType)
				{
					span_object->GetCentrePoint(c);

					double centre_minus_radius[3];
					double centre_plus_radius[3];

					double radius = heeksCAD->CircleGetRadius(span_object);

					centre_minus_radius[0] = c[0] - radius;
					centre_minus_radius[1] = c[1];
					centre_minus_radius[2] = c[2];

					centre_plus_radius[0] = c[0] + radius;
					centre_plus_radius[1] = c[1];
					centre_plus_radius[2] = c[2];

					pMachineState->Fixture().Adjustment(c);
					pMachineState->Fixture().Adjustment(centre_plus_radius);
					pMachineState->Fixture().Adjustment(centre_minus_radius);

					python << _T("kurve.add_point(k") << sketch_id << _T(", 0, ");
					python << centre_plus_radius[0] / theApp.m_program->m_units << _T(", ");
					python << centre_plus_radius[1] / theApp.m_program->m_units << _T(", ");
					python << c[0] / theApp.m_program->m_units << _T(", ");
					python << c[1] / theApp.m_program->m_units << _T(")\n");

					python << _T("kurve.add_point(k") << sketch_id << _T(", 1, ");
					python << centre_minus_radius[0] / theApp.m_program->m_units << _T(", ");
					python << centre_minus_radius[1] / theApp.m_program->m_units << _T(", ");
					python << c[0] / theApp.m_program->m_units << _T(", ");
					python << c[1] / theApp.m_program->m_units << _T(")\n");

					python << _T("kurve.add_point(k") << sketch_id << _T(", 1, ");
					python << centre_plus_radius[0] / theApp.m_program->m_units << _T(", ");
					python << centre_plus_radius[1] / theApp.m_program->m_units << _T(", ");
					python << c[0] / theApp.m_program->m_units << _T(", ");
					python << c[1] / theApp.m_program->m_units << _T(")\n");
				}
			}
		}
	}

	python << _T("\n");
	return(python);
}

Python CTurnRough::AppendTextForOneSketch(HeeksObj* object, int sketch, CMachineState *pMachine)
{
	Python python;

	if(object)
	{
		python << WriteSketchDefn(object, sketch, pMachine);

		CCuttingTool *pCuttingTool = CCuttingTool::Find( m_cutting_tool_number );

		// add the machining command
		python << wxString::Format(_T("turning.rough(k%d, "), sketch);
		python << pCuttingTool->m_params.m_corner_radius / theApp.m_program->m_units << _T(", ");
		python << pCuttingTool->m_params.m_tool_angle << _T(", ");
		python << pCuttingTool->m_params.m_front_angle << _T(", ");
		python << pCuttingTool->m_params.m_back_angle << _T(", ");
		python << m_turn_rough_params.m_clearance << _T(")\n");
	}

	return(python);
}

Python CTurnRough::AppendTextToProgram(CMachineState *pMachine)
{
	Python python;

	CCuttingTool *pCuttingTool = CCuttingTool::Find( m_cutting_tool_number );
	if (pCuttingTool == NULL)
	{
		wxMessageBox(_T("Cannot generate GCode for profile without a cutting tool assigned"));
		return(python);
	} // End if - then

	python << CSpeedOp::AppendTextToProgram(pMachine);

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
			heeksCAD->ExtractSeparateSketches(object, new_separate_sketches, false);
			for(std::list<HeeksObj*>::iterator It = new_separate_sketches.begin(); It != new_separate_sketches.end(); It++)
			{
				HeeksObj* one_curve_sketch = *It;
				python << AppendTextForOneSketch(one_curve_sketch, sketch, pMachine);
				delete one_curve_sketch;
			}
		}
		else
		{
			python << AppendTextForOneSketch(object, sketch, pMachine);
		}

		if(re_ordered_sketch)
		{
			delete re_ordered_sketch;
		}
	}

	return(python);
}

void CTurnRough::glCommands(bool select, bool marked, bool no_color)
{
	CSpeedOp::glCommands(select, marked, no_color);
}

void CTurnRough::GetProperties(std::list<Property *> *list)
{
	list->push_back(new PropertyString(_T("TO DO!"), _T("THIS OPERATION DOESN'T WORK YET!"), this, NULL));
	AddSketchesProperties(list, m_sketches);

	m_turn_rough_params.GetProperties(this, list);

	CSpeedOp::GetProperties(list);
}

HeeksObj *CTurnRough::MakeACopy(void)const
{
	return new CTurnRough(*this);
}

void CTurnRough::CopyFrom(const HeeksObj* object)
{
	operator=(*((CTurnRough*)object));
}

CTurnRough::CTurnRough( const CTurnRough & rhs ) : CSpeedOp(rhs)
{
	m_sketches.clear();
    std::copy( rhs.m_sketches.begin(), rhs.m_sketches.end(), std::inserter( m_sketches, m_sketches.begin() ) );

    m_turn_rough_params = rhs.m_turn_rough_params;
}

CTurnRough & CTurnRough::operator= ( const CTurnRough & rhs )
{
	if (this != &rhs)
	{
		CSpeedOp::operator =(rhs);

		m_sketches.clear();
		std::copy( rhs.m_sketches.begin(), rhs.m_sketches.end(), std::inserter( m_sketches, m_sketches.begin() ) );

		m_turn_rough_params = rhs.m_turn_rough_params;
	}

	return(*this);
}


bool CTurnRough::CanAddTo(HeeksObj* owner)
{
	return ((owner != NULL) && (owner->GetType() == OperationsType));
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

static ReselectSketches reselect_sketches;

void CTurnRough::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
	reselect_sketches.m_sketches = &m_sketches;
	reselect_sketches.m_object = this;
	t_list->push_back(&reselect_sketches);

    CSpeedOp::GetTools( t_list, p );
}
