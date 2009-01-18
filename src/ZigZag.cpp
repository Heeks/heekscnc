// ZigZag.cpp

#include "stdafx.h"
#include "ZigZag.h"
#include "CNCConfig.h"
#include "ProgramCanvas.h"
#include "../../interface/HeeksObj.h"
#include "../../interface/PropertyDouble.h"
#include "../../interface/PropertyChoice.h"
#include "../../tinyxml/tinyxml.h"

int CZigZag::number_for_stl_file = 1;

void CZigZagParams::set_initial_values(const std::list<int> &solids)
{
	CNCConfig config;
	config.Read(_T("ZigZagToolDiameter"), &m_tool_diameter, 3.0);
	config.Read(_T("ZigZagCornerRadius"), &m_corner_radius, 5.0);

	// these will be replaced by extents of solids
	for(std::list<int>::const_iterator It = solids.begin(); It != solids.end(); It++)
	{
		int solid = *It;
		HeeksObj* object = heeksCAD->GetIDObject(SolidType, solid);
		if(object)object->GetBox(m_box);
	}

	if(!m_box.m_valid)m_box = CBox(-7.0, 7.0, 0.0, -7.0, 7.0, 10.0);

	config.Read(_T("ZigZagDX"), &m_dx, 0.1);
	config.Read(_T("ZigZagDY"), &m_dy, 1.0);
	config.Read(_T("ZigZagHorizFeed"), &m_horizontal_feed_rate, 100.0);
	config.Read(_T("ZigZagVertFeed"), &m_vertical_feed_rate, 100.0);
	config.Read(_T("ZigZagSpindleSpeed"), &m_spindle_speed, 7000);
	config.Read(_T("ZigZagDirection"), &m_direction, 0);
}

void CZigZagParams::write_values_to_config()
{
	CNCConfig config;
	config.Write(_T("ZigZagToolDiameter"), m_tool_diameter);
	config.Write(_T("ZigZagCornerRadius"), m_corner_radius);
	config.Write(_T("ZigZagDX"), m_dx);
	config.Write(_T("ZigZagDY"), m_dy);
	config.Write(_T("ZigZagHorizFeed"), m_horizontal_feed_rate);
	config.Write(_T("ZigZagVertFeed"), m_vertical_feed_rate);
	config.Write(_T("ZigZagSpindleSpeed"), m_spindle_speed);
	config.Write(_T("ZigZagDirection"), m_direction);
}

static void on_set_tool_diameter(double value, HeeksObj* object){((CZigZag*)object)->m_params.m_tool_diameter = value;}
static void on_set_corner_radius(double value, HeeksObj* object){((CZigZag*)object)->m_params.m_corner_radius = value;}
static void on_set_minx(double value, HeeksObj* object){((CZigZag*)object)->m_params.m_box.m_x[0] = value;}
static void on_set_maxx(double value, HeeksObj* object){((CZigZag*)object)->m_params.m_box.m_x[3] = value;}
static void on_set_miny(double value, HeeksObj* object){((CZigZag*)object)->m_params.m_box.m_x[1] = value;}
static void on_set_maxy(double value, HeeksObj* object){((CZigZag*)object)->m_params.m_box.m_x[4] = value;}
static void on_set_z0(double value, HeeksObj* object){((CZigZag*)object)->m_params.m_box.m_x[2] = value;}
static void on_set_z1(double value, HeeksObj* object){((CZigZag*)object)->m_params.m_box.m_x[5] = value;}
static void on_set_dx(double value, HeeksObj* object){((CZigZag*)object)->m_params.m_dx = value;}
static void on_set_dy(double value, HeeksObj* object){((CZigZag*)object)->m_params.m_dy = value;}
static void on_set_horizontal_feed_rate(double value, HeeksObj* object){((CZigZag*)object)->m_params.m_horizontal_feed_rate = value;}
static void on_set_vertical_feed_rate(double value, HeeksObj* object){((CZigZag*)object)->m_params.m_vertical_feed_rate = value;}
static void on_set_spindle_speed(double value, HeeksObj* object){((CZigZag*)object)->m_params.m_spindle_speed = value;}
static void on_set_direction(int value, HeeksObj* object){((CZigZag*)object)->m_params.m_direction = value;}

void CZigZagParams::GetProperties(CZigZag* parent, std::list<Property *> *list)
{
	list->push_back(new PropertyDouble(_("tool diameter"), m_tool_diameter, parent, on_set_tool_diameter));
	list->push_back(new PropertyDouble(_("corner radius"), m_corner_radius, parent, on_set_corner_radius));
	list->push_back(new PropertyDouble(_("minimum x"), m_box.m_x[0], parent, on_set_minx));
	list->push_back(new PropertyDouble(_("maximum x"), m_box.m_x[3], parent, on_set_maxx));
	list->push_back(new PropertyDouble(_("minimum y"), m_box.m_x[1], parent, on_set_miny));
	list->push_back(new PropertyDouble(_("maximum y"), m_box.m_x[4], parent, on_set_maxy));
	list->push_back(new PropertyDouble(_("z0"), m_box.m_x[2], parent, on_set_z0));
	list->push_back(new PropertyDouble(_("z1"), m_box.m_x[5], parent, on_set_z1));
	list->push_back(new PropertyDouble(_("dx"), m_dx, parent, on_set_dx));
	list->push_back(new PropertyDouble(_("dy"), m_dy, parent, on_set_dy));
	list->push_back(new PropertyDouble(_("horizontal feed rate"), m_horizontal_feed_rate, parent, on_set_horizontal_feed_rate));
	list->push_back(new PropertyDouble(_("vertical feed rate"), m_vertical_feed_rate, parent, on_set_vertical_feed_rate));
	list->push_back(new PropertyDouble(_("spindle speed"), m_spindle_speed, parent, on_set_spindle_speed));
	{
		std::list< wxString > choices;
		choices.push_back(_("X"));
		choices.push_back(_("Y"));
		list->push_back(new PropertyChoice(_("direction"), choices, m_direction, parent, on_set_direction));
	}
}

void CZigZagParams::WriteXMLAttributes(TiXmlElement *root)
{
	TiXmlElement * element;
	element = new TiXmlElement( "params" );
	root->LinkEndChild( element );  
	element->SetAttribute("toold", m_tool_diameter);
	element->SetAttribute("cornerrad", m_corner_radius);
	element->SetAttribute("minx", m_box.m_x[0]);
	element->SetAttribute("maxx", m_box.m_x[3]);
	element->SetAttribute("miny", m_box.m_x[1]);
	element->SetAttribute("maxy", m_box.m_x[4]);
	element->SetAttribute("z0", m_box.m_x[2]);
	element->SetAttribute("z1", m_box.m_x[5]);
	element->SetAttribute("dx", m_dx);
	element->SetAttribute("dy", m_dy);
	element->SetAttribute("hfeed", m_horizontal_feed_rate);
	element->SetAttribute("vfeed", m_vertical_feed_rate);
	element->SetAttribute("spin", m_spindle_speed);
	element->SetAttribute("dir", m_direction);
}

void CZigZagParams::ReadFromXMLElement(TiXmlElement* pElem)
{
	// get the attributes
	for(TiXmlAttribute* a = pElem->FirstAttribute(); a; a = a->Next())
	{
		std::string name(a->Name());
		if(name == "toold"){m_tool_diameter = a->DoubleValue();}
		else if(name == "cornerrad"){m_corner_radius = a->DoubleValue();}
		else if(name == "minx"){m_box.m_x[0] = a->DoubleValue();}
		else if(name == "maxx"){m_box.m_x[3] = a->DoubleValue();}
		else if(name == "miny"){m_box.m_x[1] = a->DoubleValue();}
		else if(name == "maxy"){m_box.m_x[4] = a->DoubleValue();}
		else if(name == "z0"){m_box.m_x[2] = a->DoubleValue();}
		else if(name == "z1"){m_box.m_x[5] = a->DoubleValue();}
		else if(name == "dx"){m_dx = a->DoubleValue();}
		else if(name == "dy"){m_dy = a->DoubleValue();}
		else if(name == "hfeed"){m_horizontal_feed_rate = a->DoubleValue();}
		else if(name == "vfeed"){m_vertical_feed_rate = a->DoubleValue();}
		else if(name == "spin"){m_spindle_speed = a->DoubleValue();}
		else if(name == "spin"){m_spindle_speed = a->DoubleValue();}
	}
}

void CZigZag::AppendTextToProgram()
{
	//write stl file
	std::list<HeeksObj*> solids;
	for(std::list<int>::iterator It = m_solids.begin(); It != m_solids.end(); It++)
	{
		HeeksObj* object = heeksCAD->GetIDObject(SolidType, *It);
		if(object)solids.push_back(object);
	}

	wxString filepath = wxString::Format(_T("zigzag%d.stl"), number_for_stl_file);
	number_for_stl_file++;
	heeksCAD->SaveSTLFile(solids, filepath);
	theApp.m_program_canvas->m_textCtrl->AppendText(wxString::Format(_T("c = SphericalCutter(%lf, Point(0,0,7))\n"), m_params.m_tool_diameter/2));
	theApp.m_program_canvas->m_textCtrl->AppendText(wxString::Format(_T("model = ImportModel('%s')\n"), filepath.c_str()));
	theApp.m_program_canvas->m_textCtrl->AppendText(wxString(_T("pg = DropCutter(c, model)\n")));
	theApp.m_program_canvas->m_textCtrl->AppendText(wxString::Format(_T("pathlist = pg.GenerateToolPath(%lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, 0)\n"), m_params.m_box.m_x[0], m_params.m_box.m_x[3], m_params.m_box.m_x[1], m_params.m_box.m_x[4], m_params.m_box.m_x[2], m_params.m_box.m_x[5], m_params.m_dx, m_params.m_dy));
	theApp.m_program_canvas->m_textCtrl->AppendText(wxString(_T("h = HeeksCNCExporter()\n")));
	theApp.m_program_canvas->m_textCtrl->AppendText(wxString(_T("h.AddPathList(pathlist)\n")));
}

void CZigZag::glCommands(bool select, bool marked, bool no_color)
{
	if(0 && marked && !no_color)
	{
		for(std::list<int>::iterator It = m_solids.begin(); It != m_solids.end(); It++)
		{
			int solid = *It;
			HeeksObj* object = heeksCAD->GetIDObject(SolidType, solid);
			if(object)object->glCommands(false, true, false);
		}
	}
}

void CZigZag::GetProperties(std::list<Property *> *list)
{
	m_params.GetProperties(this, list);
	HeeksObj::GetProperties(list);
}

HeeksObj *CZigZag::MakeACopy(void)const
{
	return new CZigZag(*this);
}

void CZigZag::CopyFrom(const HeeksObj* object)
{
	operator=(*((CZigZag*)object));
}

bool CZigZag::CanAddTo(HeeksObj* owner)
{
	return owner->GetType() == OperationsType;
}

void CZigZag::WriteXML(TiXmlElement *root)
{
	TiXmlElement * element = new TiXmlElement( "ZigZag" );
	root->LinkEndChild( element );  
	m_params.WriteXMLAttributes(element);

	// write solid ids
	for(std::list<int>::iterator It = m_solids.begin(); It != m_solids.end(); It++)
	{
		int solid = *It;
		TiXmlElement * solid_element = new TiXmlElement( "solid" );
		element->LinkEndChild( solid_element );  
		solid_element->SetAttribute("id", solid);
	}

	WriteBaseXML(element);
}

// static member function
HeeksObj* CZigZag::ReadFromXMLElement(TiXmlElement* element)
{
	CZigZag* new_object = new CZigZag;

	// read solid ids
	for(TiXmlElement* pElem = TiXmlHandle(element).FirstChildElement().Element(); pElem; pElem = pElem->NextSiblingElement())
	{
		std::string name(pElem->Value());
		if(name == "params"){
			new_object->m_params.ReadFromXMLElement(pElem);
		}
		else if(name == "solid"){
			for(TiXmlAttribute* a = pElem->FirstAttribute(); a; a = a->Next())
			{
				std::string name(a->Name());
				if(name == "id"){
					int id = a->IntValue();
					new_object->m_solids.push_back(id);
				}
			}
		}
	}

	new_object->ReadBaseXML(element);

	return new_object;
}
