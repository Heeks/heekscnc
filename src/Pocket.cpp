// Pocket.cpp

#include "stdafx.h"
#include "Pocket.h"
#include "CNCConfig.h"
#include "ProgramCanvas.h"
#include "../../interface/HeeksObj.h"
#include "../../interface/PropertyDouble.h"
#include "../../interface/PropertyString.h"
#include "../../interface/PropertyChoice.h"
#include "../../interface/PropertyVertex.h"
#include "../../interface/PropertyCheck.h"
#include "../../tinyxml/tinyxml.h"

CPocketParams::CPocketParams()
{
	m_tool_diameter = 0.0;
	m_step_over = 0.0;
	m_step_down = 0.0;
	m_material_allowance = 0.0;
	m_round_corner_factor = 0.0;
	m_clearance_height = 0.0;
	m_final_depth = 0.0;
	m_rapid_down_to_height = 0.0;
	m_horizontal_feed_rate = 0.0;
	m_vertical_feed_rate = 0.0;
	m_spindle_speed = 0.0;
}

void CPocketParams::set_initial_values()
{
	CNCConfig config;
	config.Read(_T("PocketToolDiameter"), &m_tool_diameter, 3.0);
	config.Read(_T("PocketStepOver"), &m_step_over, 1.0);
	config.Read(_T("PocketStepDown"), &m_step_down, 1.0);
	config.Read(_T("PocketMaterialAllowance"), &m_material_allowance, 0.2);
	config.Read(_T("PocketRoundCornerFactor"), &m_round_corner_factor, 1.5);
	config.Read(_T("PocketClearanceHeight"), &m_clearance_height, 5.0);
	config.Read(_T("PocketFinalDepth"), &m_final_depth, -0.1);
	config.Read(_T("PocketRapidDown"), &m_rapid_down_to_height, 2.0);
	config.Read(_T("PocketHorizFeed"), &m_horizontal_feed_rate, 100.0);
	config.Read(_T("PocketVertFeed"), &m_vertical_feed_rate, 100.0);
	config.Read(_T("PocketSpindleSpeed"), &m_spindle_speed, 7000);
}

void CPocketParams::write_values_to_config()
{
	CNCConfig config;
	config.Write(_T("PocketToolDiameter"), m_tool_diameter);
	config.Write(_T("PocketStepOver"), m_step_over);
	config.Write(_T("PocketStepDown"), m_step_down);
	config.Write(_T("PocketMaterialAllowance"), m_material_allowance);
	config.Write(_T("PocketRoundCornerFactor"), m_round_corner_factor);
	config.Write(_T("PocketClearanceHeight"), m_clearance_height);
	config.Write(_T("PocketFinalDepth"), m_final_depth);
	config.Write(_T("PocketRapidDown"), m_rapid_down_to_height);
	config.Write(_T("PocketHorizFeed"), m_horizontal_feed_rate);
	config.Write(_T("PocketVertFeed"), m_vertical_feed_rate);
	config.Write(_T("PocketSpindleSpeed"), m_spindle_speed);
}

static void on_set_tool_diameter(double value, HeeksObj* object){((CPocket*)object)->m_params.m_tool_diameter = value;}
static void on_set_step_over(double value, HeeksObj* object){((CPocket*)object)->m_params.m_step_over = value;}
static void on_set_step_down(double value, HeeksObj* object){((CPocket*)object)->m_params.m_step_down = value;}
static void on_set_material_allowance(double value, HeeksObj* object){((CPocket*)object)->m_params.m_material_allowance = value;}
static void on_set_round_corner_factor(double value, HeeksObj* object){((CPocket*)object)->m_params.m_round_corner_factor = value;}
static void on_set_clearance_height(double value, HeeksObj* object){((CPocket*)object)->m_params.m_clearance_height = value;}
static void on_set_final_depth(double value, HeeksObj* object){((CPocket*)object)->m_params.m_final_depth = value;}
static void on_set_rapid_down_to_height(double value, HeeksObj* object){((CPocket*)object)->m_params.m_rapid_down_to_height = value;}
static void on_set_horizontal_feed_rate(double value, HeeksObj* object){((CPocket*)object)->m_params.m_horizontal_feed_rate = value;}
static void on_set_vertical_feed_rate(double value, HeeksObj* object){((CPocket*)object)->m_params.m_vertical_feed_rate = value;}
static void on_set_spindle_speed(double value, HeeksObj* object){((CPocket*)object)->m_params.m_spindle_speed = value;}

void CPocketParams::GetProperties(CPocket* parent, std::list<Property *> *list)
{
	list->push_back(new PropertyDouble(_("tool diameter"), m_tool_diameter, parent, on_set_tool_diameter));
	list->push_back(new PropertyDouble(_("step over"), m_step_over, parent, on_set_step_over));
	list->push_back(new PropertyDouble(_("step down"), m_step_down, parent, on_set_step_down));
	list->push_back(new PropertyDouble(_("material allowance"), m_material_allowance, parent, on_set_material_allowance));
	list->push_back(new PropertyDouble(_("round corner factor"), m_round_corner_factor, parent, on_set_round_corner_factor));
	list->push_back(new PropertyString(wxString(_T("( ")) + _("for 90 degree corners") + _T(" )"), wxString(_T("( ")) + _("1.5 for square, 1.0 for round")  + _T(" )"), NULL));
	list->push_back(new PropertyDouble(_("clearance height"), m_clearance_height, parent, on_set_clearance_height));
	list->push_back(new PropertyDouble(_("final depth"), m_final_depth, parent, on_set_final_depth));
	list->push_back(new PropertyDouble(_("rapid down to height"), m_rapid_down_to_height, parent, on_set_rapid_down_to_height));
	list->push_back(new PropertyDouble(_("horizontal feed rate"), m_horizontal_feed_rate, parent, on_set_horizontal_feed_rate));
	list->push_back(new PropertyDouble(_("vertical feed rate"), m_vertical_feed_rate, parent, on_set_vertical_feed_rate));
	list->push_back(new PropertyDouble(_("spindle speed"), m_spindle_speed, parent, on_set_spindle_speed));
}

void CPocketParams::WriteXMLAttributes(TiXmlNode *root)
{
	TiXmlElement * element;
	element = new TiXmlElement( "params" );
	root->LinkEndChild( element );  
	element->SetDoubleAttribute("toold", m_tool_diameter);
	element->SetDoubleAttribute("step", m_step_over);
	element->SetDoubleAttribute("mat", m_material_allowance);
	element->SetDoubleAttribute("down", m_step_down);
	element->SetDoubleAttribute("rf", m_round_corner_factor);
	element->SetDoubleAttribute("clear", m_clearance_height);
	element->SetDoubleAttribute("depth", m_final_depth);
	element->SetDoubleAttribute("r", m_rapid_down_to_height);
	element->SetDoubleAttribute("hfeed", m_horizontal_feed_rate);
	element->SetDoubleAttribute("vfeed", m_vertical_feed_rate);
	element->SetDoubleAttribute("spin", m_spindle_speed);
}

void CPocketParams::ReadFromXMLElement(TiXmlElement* pElem)
{
	// get the attributes
	for(TiXmlAttribute* a = pElem->FirstAttribute(); a; a = a->Next())
	{
		std::string name(a->Name());
		if(name == "toold"){m_tool_diameter = a->DoubleValue();}
		else if(name == "step"){m_step_over = a->DoubleValue();}
		else if(name == "down"){m_step_down = a->DoubleValue();}
		else if(name == "mat"){m_material_allowance = a->DoubleValue();}
		else if(name == "rf"){m_round_corner_factor = a->DoubleValue();}
		else if(name == "clear"){m_clearance_height = a->DoubleValue();}
		else if(name == "depth"){m_final_depth = a->DoubleValue();}
		else if(name == "r"){m_rapid_down_to_height = a->DoubleValue();}
		else if(name == "hfeed"){m_horizontal_feed_rate = a->DoubleValue();}
		else if(name == "vfeed"){m_vertical_feed_rate = a->DoubleValue();}
		else if(name == "spin"){m_spindle_speed = a->DoubleValue();}
	}
}

static void WriteSketchDefn(HeeksObj* sketch)
{
	theApp.m_program_canvas->m_textCtrl->AppendText(wxString::Format(_T("a%d = area.new()\n"), sketch->m_id));

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
					theApp.m_program_canvas->m_textCtrl->AppendText(wxString::Format(_T("area.start_new_curve(a%d)\n"), sketch->m_id));
					started = false;
				}

				if(!started)
				{
					theApp.m_program_canvas->m_textCtrl->AppendText(wxString::Format(_T("area.add_point(a%d, %d, %g, %g, %g, %g)\n"), sketch->m_id, 0, s[0], s[1], 0.0, 0.0));
					started = true;
				}
				span_object->GetEndPoint(e);
				if(type == LineType)
				{
					theApp.m_program_canvas->m_textCtrl->AppendText(wxString::Format(_T("area.add_point(a%d, %d, %g, %g, %g, %g)\n"), sketch->m_id, 0, e[0], e[1], 0.0, 0.0));
				}
				else if(type == ArcType)
				{
					span_object->GetCentrePoint(c);
					double pos[3];
					heeksCAD->GetArcAxis(span_object, pos);
					int span_type = (pos[2] >=0) ? 1:-1;
					theApp.m_program_canvas->m_textCtrl->AppendText(wxString::Format(_T("area.add_point(a%d, %d, %g, %g, %g, %g)\n"), sketch->m_id, span_type, e[0], e[1], c[0], c[1]));
				}
				memcpy(prev_e, e, 3*sizeof(double));
			}
		}
	}

	theApp.m_program_canvas->m_textCtrl->AppendText(_T("\n"));
}

void CPocket::AppendTextToProgram()
{
	theApp.m_program_canvas->m_textCtrl->AppendText(wxString::Format(_T("clearance = float(%g)\n"), m_params.m_clearance_height));
	theApp.m_program_canvas->m_textCtrl->AppendText(wxString::Format(_T("rapid_down_to_height = float(%g)\n"), m_params.m_rapid_down_to_height));
	theApp.m_program_canvas->m_textCtrl->AppendText(wxString::Format(_T("final_depth = float(%g)\n"), m_params.m_final_depth));
	theApp.m_program_canvas->m_textCtrl->AppendText(wxString::Format(_T("tool_diameter = float(%g)\n"), m_params.m_tool_diameter));
	theApp.m_program_canvas->m_textCtrl->AppendText(wxString::Format(_T("spindle(%g)\n"), m_params.m_spindle_speed));
	theApp.m_program_canvas->m_textCtrl->AppendText(wxString::Format(_T("feedrate(%g)\n"), m_params.m_horizontal_feed_rate));
	theApp.m_program_canvas->m_textCtrl->AppendText(wxString::Format(_T("tool_change(1)\n")));
	for(std::list<int>::iterator It = m_sketches.begin(); It != m_sketches.end(); It++)
	{
		int sketch = *It;

		// write an area definition
		HeeksObj* object = heeksCAD->GetIDObject(SketchType, sketch);
		if(object == NULL || object->GetNumChildren() == 0)continue;

		if(object)
		{
			WriteSketchDefn(object);

			// start - assume we are at a suitable clearance height

			// Pocket the area
			theApp.m_program_canvas->m_textCtrl->AppendText(wxString::Format(_T("stdops.pocket(a%d, tool_diameter/2 + %g, rapid_down_to_height, final_depth, %g, %g, %g)\n"), sketch, m_params.m_material_allowance, m_params.m_step_over, m_params.m_step_down, m_params.m_round_corner_factor));

			// rapid back up to clearance plane
			theApp.m_program_canvas->m_textCtrl->AppendText(wxString(_T("rapid(z = clearance)\n")));			
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
	m_params.GetProperties(this, list);
	HeeksObj::GetProperties(list);
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
	m_params.WriteXMLAttributes(element);

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

	// read sketch ids
	for(TiXmlElement* pElem = TiXmlHandle(element).FirstChildElement().Element(); pElem; pElem = pElem->NextSiblingElement())
	{
		std::string name(pElem->Value());
		if(name == "params"){
			new_object->m_params.ReadFromXMLElement(pElem);
		}
		else if(name == "sketch"){
			for(TiXmlAttribute* a = pElem->FirstAttribute(); a; a = a->Next())
			{
				std::string name(a->Name());
				if(name == "id"){
					int id = a->IntValue();
					new_object->m_sketches.push_back(id);
				}
			}
		}
	}

	new_object->ReadBaseXML(element);

	return new_object;
}
