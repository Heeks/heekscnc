// CuttingTool.cpp
/*
 * Copyright (c) 2009, Dan Heeks, Perttu Ahola
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "stdafx.h"
#include "CuttingTool.h"
#include "CNCConfig.h"
#include "ProgramCanvas.h"
#include "interface/HeeksObj.h"
#include "interface/PropertyInt.h"
#include "interface/PropertyDouble.h"
#include "interface/PropertyChoice.h"
#include "tinyxml/tinyxml.h"

#include <sstream>

extern CHeeksCADInterface* heeksCAD;


void CCuttingToolParams::set_initial_values()
{
	CNCConfig config;

	config.Read(_T("m_pocket_number"), &m_pocket_number, 1);
	config.Read(_T("m_tool_number"), &m_tool_number, m_pocket_number);
	config.Read(_T("m_diameter"), &m_diameter, 12.7);
	config.Read(_T("m_x_offset"), &m_x_offset, 0);
	config.Read(_T("m_tool_length_offset"), &m_tool_length_offset, 0);
	config.Read(_T("m_orientation"), &m_orientation, 9);
}

void CCuttingToolParams::write_values_to_config()
{
	CNCConfig config;
	config.Write(_T("m_pocket_number"), m_pocket_number);
	config.Write(_T("m_tool_number"), m_tool_number);
	config.Write(_T("m_diameter"), m_diameter);
	config.Write(_T("m_x_offset"), m_x_offset);
	config.Write(_T("m_tool_length_offset"), m_tool_length_offset);
	config.Write(_T("m_orientation"), m_orientation);
}

static void on_set_pocket_number(int value, HeeksObj* object){((CCuttingTool*)object)->m_params.m_pocket_number = value;}
static void on_set_tool_number(int value, HeeksObj* object){((CCuttingTool*)object)->m_params.m_tool_number = value;}
static void on_set_diameter(double value, HeeksObj* object){((CCuttingTool*)object)->m_params.m_diameter = value;}
static void on_set_x_offset(double value, HeeksObj* object){((CCuttingTool*)object)->m_params.m_x_offset = value;}
static void on_set_tool_length_offset(double value, HeeksObj* object){((CCuttingTool*)object)->m_params.m_tool_length_offset = value;}
static void on_set_orientation(int value, HeeksObj* object){((CCuttingTool*)object)->m_params.m_orientation = value;}

void CCuttingToolParams::GetProperties(CCuttingTool* parent, std::list<Property *> *list)
{
	list->push_back(new PropertyInt(_("pocket_number"), m_pocket_number, parent, on_set_pocket_number));
	list->push_back(new PropertyInt(_("tool_number"), m_tool_number, parent, on_set_tool_number));
	list->push_back(new PropertyDouble(_("diameter"), m_diameter, parent, on_set_diameter));
	list->push_back(new PropertyDouble(_("x_offset"), m_x_offset, parent, on_set_x_offset));
	list->push_back(new PropertyDouble(_("tool_length_offset"), m_x_offset, parent, on_set_tool_length_offset));
	list->push_back(new PropertyInt(_("orientation"), m_orientation, parent, on_set_orientation));
}

void CCuttingToolParams::WriteXMLAttributes(TiXmlNode *root)
{
	TiXmlElement * element;
	element = new TiXmlElement( "params" );
	root->LinkEndChild( element );  

	std::ostringstream l_ossValue;

	l_ossValue.str(""); l_ossValue << m_pocket_number;
	element->SetAttribute("pocket_number", l_ossValue.str().c_str());

	l_ossValue.str(""); l_ossValue << m_tool_number;
	element->SetAttribute("tool_number", l_ossValue.str().c_str() );

	element->SetDoubleAttribute("diameter", m_diameter);
	element->SetDoubleAttribute("x_offset", m_x_offset);
	element->SetDoubleAttribute("tool_length_offset", m_tool_length_offset);

	l_ossValue.str(""); l_ossValue << m_orientation;
	element->SetAttribute("orientation", l_ossValue.str().c_str() );
}

void CCuttingToolParams::ReadParametersFromXMLElement(TiXmlElement* pElem)
{
	if (pElem->Attribute("pocket_number")) m_pocket_number = atoi(pElem->Attribute("pocket_number"));
	if (pElem->Attribute("tool_number")) m_tool_number = atoi(pElem->Attribute("tool_number"));
	if (pElem->Attribute("diameter")) m_diameter = atof(pElem->Attribute("diameter"));
	if (pElem->Attribute("x_offset")) m_x_offset = atof(pElem->Attribute("x_offset"));
	if (pElem->Attribute("tool_length_offset")) m_tool_length_offset = atof(pElem->Attribute("tool_length_offset"));
	if (pElem->Attribute("orientation")) m_orientation = atoi(pElem->Attribute("orientation"));
}

/**
	This method is called when the CAD operator presses the Python button.  This method generates
	Python source code whose job will be to generate RS-274 GCode.  It's done in two steps so that
	the Python code can be configured to generate GCode suitable for various CNC interpreters.
 */
void CCuttingTool::AppendTextToProgram()
{
	COp::AppendTextToProgram();

#ifdef UNICODE
	std::wostringstream ss;
#else
    std::ostringstream ss;
#endif
    ss.imbue(std::locale("C"));

	// The G10 command can be used (within EMC2) to add a tool to the tool
        // table from within a program.
        // G10 L1 P[tool number] R[radius] X[offset] Z[offset] Q[orientation]

	ss << "tool_defn( id=" << m_params.m_tool_number << ", "
		<< "name=None, ";
	if (m_params.m_diameter > 0)
	{
		ss << "radius=" << m_params.m_diameter / 2 << ", ";
	} // End if - then
	else
	{
		ss << "radius=None, ";
	} // End if - else

	if (m_params.m_tool_length_offset > 0)
	{
		ss << "length=" << m_params.m_tool_length_offset;
	} // End if - then
	else
	{
		ss << "length=None";
	} // End if - else
	
	ss << ")\n";

	theApp.m_program_canvas->m_textCtrl->AppendText(ss.str().c_str());
}



/**
	This is the Graphics Library Commands (from the OpenGL set).  This method calls the OpenGL
	routines to paint the drill action in the graphics window.  The graphics is transient.

	Part of its job is to re-paint the elements that this CCuttingTool object refers to so that
	we know what CAD objects this CNC operation is referring to.
 */
void CCuttingTool::glCommands(bool select, bool marked, bool no_color)
{
}


void CCuttingTool::GetProperties(std::list<Property *> *list)
{
	m_params.GetProperties(this, list);
	HeeksObj::GetProperties(list);
}

HeeksObj *CCuttingTool::MakeACopy(void)const
{
	return new CCuttingTool(*this);
}

void CCuttingTool::CopyFrom(const HeeksObj* object)
{
	operator=(*((CCuttingTool*)object));
}

bool CCuttingTool::CanAddTo(HeeksObj* owner)
{
	return owner->GetType() == OperationsType;
}

void CCuttingTool::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element = new TiXmlElement( "CuttingTool" );
	root->LinkEndChild( element );  
	m_params.WriteXMLAttributes(element);
	WriteBaseXML(element);
}

// static member function
HeeksObj* CCuttingTool::ReadFromXMLElement(TiXmlElement* element)
{
	CCuttingTool* new_object = new CCuttingTool;

	// read point and circle ids
	for(TiXmlElement* pElem = TiXmlHandle(element).FirstChildElement().Element(); pElem; pElem = pElem->NextSiblingElement())
	{
		std::string name(pElem->Value());
		if(name == "params"){
			new_object->m_params.ReadParametersFromXMLElement(pElem);
		}
	}

	new_object->ReadBaseXML(element);

	return new_object;
}


