// SpeedReference.cpp
/*
 * Copyright (c) 2009, Dan Heeks, Perttu Ahola
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "stdafx.h"
#include <math.h>
#include "SpeedReference.h"
#include "CNCConfig.h"
#include "ProgramCanvas.h"
#include "interface/HeeksObj.h"
#include "interface/HeeksColor.h"
#include "interface/PropertyInt.h"
#include "interface/PropertyDouble.h"
#include "interface/PropertyLength.h"
#include "interface/PropertyChoice.h"
#include "interface/PropertyString.h"
#include "tinyxml/tinyxml.h"

#include <sstream>
#include <string>
#include <algorithm>

extern CHeeksCADInterface* heeksCAD;


static void on_set_cutting_tool_material(int value, HeeksObj* object)
{
		((CSpeedReference*)object)->m_cutting_tool_material = value;
		heeksCAD->RefreshProperties();
}

static void on_set_brinell_hardness_of_raw_material(double value, HeeksObj* object){((CSpeedReference*)object)->m_brinell_hardness_of_raw_material = value;}
static void on_set_surface_feet_per_minute(double value, HeeksObj* object){((CSpeedReference*)object)->m_surface_feet_per_minute = value;}
static void on_set_material_name(const wxChar *value, HeeksObj* object){((CSpeedReference*)object)->m_material_name = value;}


void CSpeedReference::GetProperties(std::list<Property *> *list)
{
	{
		CCuttingToolParams::MaterialsList_t materials = CCuttingToolParams::GetMaterialsList();

		int choice = 0;
		std::list< wxString > choices;
		for (CCuttingToolParams::MaterialsList_t::size_type i=0; i<materials.size(); i++)
		{
			choices.push_back(materials[i].second);
			if (m_cutting_tool_material == materials[i].first) choice = int(i);
			
		} // End for
		list->push_back(new PropertyChoice(_("Material"), choices, choice, this, on_set_cutting_tool_material));
	}

	list->push_back(new PropertyDouble(_("Brinell hardness of raw material"), m_brinell_hardness_of_raw_material, this, on_set_brinell_hardness_of_raw_material));
	list->push_back(new PropertyDouble(_("Surface Feet per Minute"), m_surface_feet_per_minute, this, on_set_surface_feet_per_minute));
	list->push_back(new PropertyString(_("Raw Material Name"), m_material_name, this, on_set_material_name));
	HeeksObj::GetProperties(list);
}


HeeksObj *CSpeedReference::MakeACopy(void)const
{
	return new CSpeedReference(*this);
}

void CSpeedReference::CopyFrom(const HeeksObj* object)
{
	operator=(*((CSpeedReference*)object));
}

bool CSpeedReference::CanAddTo(HeeksObj* owner)
{
	return owner->GetType() == SpeedReferencesType;
}

void CSpeedReference::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element = new TiXmlElement( "SpeedReference" );
	root->LinkEndChild( element );  
	element->SetAttribute("title", Ttc(m_title.c_str()));

	std::ostringstream l_ossValue;
	l_ossValue.str(""); l_ossValue << m_surface_feet_per_minute;
	element->SetAttribute("surface_feet_per_minute", l_ossValue.str().c_str() );

	l_ossValue.str(""); l_ossValue << int(m_cutting_tool_material);
	element->SetAttribute("cutting_tool_material", l_ossValue.str().c_str() );

	l_ossValue.str(""); l_ossValue << m_brinell_hardness_of_raw_material;
	element->SetAttribute("brinell_hardness_of_raw_material", l_ossValue.str().c_str() );

	element->SetAttribute("raw_material_name", Ttc(m_material_name.c_str()) );

	WriteBaseXML(element);
}

// static member function
HeeksObj* CSpeedReference::ReadFromXMLElement(TiXmlElement* element)
{
	double brinell_hardness_of_raw_material = 15.0;
	if (element->Attribute("brinell_hardness_of_raw_material")) 
		brinell_hardness_of_raw_material = atof(element->Attribute("brinell_hardness_of_raw_material"));

	double surface_feet_per_minute = 600.0;
	if (element->Attribute("surface_feet_per_minute")) 
		surface_feet_per_minute = atof(element->Attribute("surface_feet_per_minute"));

	wxString raw_material_name;
	if (element->Attribute("raw_material_name")) 
		raw_material_name = Ctt(element->Attribute("raw_material_name"));

	int cutting_tool_material = int(CCuttingToolParams::eCarbide);
	if (element->Attribute("cutting_tool_material")) 
		cutting_tool_material = atoi(element->Attribute("cutting_tool_material"));

	wxString title(Ctt(element->Attribute("title")));
	CSpeedReference* new_object = new CSpeedReference( title.c_str(), 
								raw_material_name.c_str(),
								cutting_tool_material,
								brinell_hardness_of_raw_material,
								surface_feet_per_minute );
	new_object->ReadBaseXML(element);

	return new_object;
}


void CSpeedReference::OnEditString(const wxChar* str){
        m_title.assign(str);
	heeksCAD->WasModified(this);
}



