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
#include "interface/PropertyLength.h"
#include "interface/PropertyChoice.h"
#include "interface/PropertyString.h"
#include "tinyxml/tinyxml.h"

#include <sstream>

extern CHeeksCADInterface* heeksCAD;



void CCuttingToolParams::set_initial_values()
{
	CNCConfig config;
	config.Read(_T("m_diameter"), &m_diameter, 12.7);
	config.Read(_T("m_x_offset"), &m_x_offset, 0);
	config.Read(_T("m_tool_length_offset"), &m_tool_length_offset, 0);
	config.Read(_T("m_orientation"), &m_orientation, 9);
}

void CCuttingToolParams::write_values_to_config()
{
	CNCConfig config;

	// We ALWAYS write the parameters into the configuration file in mm (for consistency).
	// If we're now in inches then convert the values.
	// We're in mm already.
	config.Write(_T("m_diameter"), m_diameter);
	config.Write(_T("m_x_offset"), m_x_offset);
	config.Write(_T("m_tool_length_offset"), m_tool_length_offset);
	config.Write(_T("m_orientation"), m_orientation);
}

static void on_set_diameter(double value, HeeksObj* object){((CCuttingTool*)object)->m_params.m_diameter = value;}
static void on_set_x_offset(double value, HeeksObj* object){((CCuttingTool*)object)->m_params.m_x_offset = value;}
static void on_set_tool_length_offset(double value, HeeksObj* object){((CCuttingTool*)object)->m_params.m_tool_length_offset = value;}
static void on_set_orientation(int value, HeeksObj* object){((CCuttingTool*)object)->m_params.m_orientation = value;}

void CCuttingToolParams::GetProperties(CCuttingTool* parent, std::list<Property *> *list)
{
	list->push_back(new PropertyLength(_("diameter"), m_diameter, parent, on_set_diameter));
	list->push_back(new PropertyLength(_("x_offset"), m_x_offset, parent, on_set_x_offset));
	list->push_back(new PropertyLength(_("tool_length_offset"), m_x_offset, parent, on_set_tool_length_offset));
	list->push_back(new PropertyInt(_("orientation"), m_orientation, parent, on_set_orientation));
}

void CCuttingToolParams::WriteXMLAttributes(TiXmlNode *root)
{
	TiXmlElement * element;
	element = new TiXmlElement( "params" );
	root->LinkEndChild( element );  


	element->SetDoubleAttribute("diameter", m_diameter);
	element->SetDoubleAttribute("x_offset", m_x_offset);
	element->SetDoubleAttribute("tool_length_offset", m_tool_length_offset);
	
	std::ostringstream l_ossValue;
	l_ossValue.str(""); l_ossValue << m_orientation;
	element->SetAttribute("orientation", l_ossValue.str().c_str() );
}

void CCuttingToolParams::ReadParametersFromXMLElement(TiXmlElement* pElem)
{
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

#ifdef UNICODE
	std::wostringstream ss;
#else
    std::ostringstream ss;
#endif
    ss.imbue(std::locale("C"));

	// The G10 command can be used (within EMC2) to add a tool to the tool
        // table from within a program.
        // G10 L1 P[tool number] R[radius] X[offset] Z[offset] Q[orientation]

	if (m_title.size() > 0)
	{
		ss << "#(" << m_title.c_str() << ")\n";
	} // End if - then

	ss << "tool_defn( id=" << m_tool_number << ", ";

	if (m_title.size() > 0)
	{
		ss << "name='" << m_title.c_str() << "\', ";
	} // End if - then
	else
	{
		ss << "name=None, ";
	} // End if - else

	if (m_params.m_diameter > 0)
	{
		ss << "radius=" << m_params.m_diameter / 2 /theApp.m_program->m_units << ", ";
	} // End if - then
	else
	{
		ss << "radius=None, ";
	} // End if - else

	if (m_params.m_tool_length_offset > 0)
	{
		ss << "length=" << m_params.m_tool_length_offset /theApp.m_program->m_units;
	} // End if - then
	else
	{
		ss << "length=None";
	} // End if - else
	
	ss << ")\n";

	theApp.m_program_canvas->m_textCtrl->AppendText(ss.str().c_str());
}


static void on_set_tool_number(const int value, HeeksObj* object){((CCuttingTool*)object)->m_tool_number = value;}

/**
	NOTE: The m_title member is a special case.  The HeeksObj code looks for a 'GetShortDesc()' method.  If found, it
	adds a Property called 'Object Title'.  If the value is changed, it tries to call the 'OnEditString()' method.
	That's why the m_title value is not defined here
 */
void CCuttingTool::GetProperties(std::list<Property *> *list)
{
	list->push_back(new PropertyInt(_("tool_number"), m_tool_number, this, on_set_tool_number));

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
	return owner->GetType() == ToolsType;
}

void CCuttingTool::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element = new TiXmlElement( "CuttingTool" );
	root->LinkEndChild( element );  
	element->SetAttribute("title", Ttc(m_title.c_str()));

	std::ostringstream l_ossValue;
	l_ossValue.str(""); l_ossValue << m_tool_number;
	element->SetAttribute("tool_number", l_ossValue.str().c_str() );

	m_params.WriteXMLAttributes(element);
	WriteBaseXML(element);
}

// static member function
HeeksObj* CCuttingTool::ReadFromXMLElement(TiXmlElement* element)
{

	int tool_number = 0;
	if (element->Attribute("tool_number")) tool_number = atoi(element->Attribute("tool_number"));

	wxString title(Ctt(element->Attribute("title")));
	CCuttingTool* new_object = new CCuttingTool( title.c_str(), tool_number);

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


void CCuttingTool::OnEditString(const wxChar* str){
        m_title.assign(str);
	heeksCAD->WasModified(this);
}

/**
 * Find the CuttingTool object whose tool number matches that passed in.
 */
int CCuttingTool::FindCuttingTool( const int tool_number )
{
        /* Can't make this work.  Need to figure out why */
        for (HeeksObj *ob = heeksCAD->GetFirstObject(); ob != NULL; ob = heeksCAD->GetNextObject())
        {
                if (ob->GetType() != CuttingToolType) continue;

                if (((CCuttingTool *) ob)->m_tool_number == tool_number)
                {
                        return(ob->m_id);
                } // End if - then
        } // End for

        // This is a hack but it works.  As long as we don't get more than 100 tools in the holder.
        for (int id=1; id<100; id++)
        {
                HeeksObj *ob = heeksCAD->GetIDObject( CuttingToolType, id );
                if (! ob) continue;

                if (((CCuttingTool *) ob)->m_tool_number == tool_number)
                {
                        return(ob->m_id);
                } // End if - then
        } // End for

        return(-1);

} // End FindCuttingTool() method

