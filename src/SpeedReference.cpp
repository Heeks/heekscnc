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
		((CSpeedReference*)object)->ResetTitle();
		heeksCAD->RefreshProperties();
}

static void on_set_brinell_hardness_of_raw_material(double value, HeeksObj* object){((CSpeedReference*)object)->m_brinell_hardness_of_raw_material = value; ((CSpeedReference*)object)->ResetTitle(); }
static void on_set_surface_speed(double value, HeeksObj* object){((CSpeedReference*)object)->m_surface_speed = value; ((CSpeedReference*)object)->ResetTitle(); }
static void on_set_material_name(const wxChar *value, HeeksObj* object){((CSpeedReference*)object)->m_material_name = value; ((CSpeedReference*)object)->ResetTitle(); }


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
	list->push_back(new PropertyDouble(_("Surface Speed (m/min)"), m_surface_speed, this, on_set_surface_speed));
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
	l_ossValue.str(""); l_ossValue << m_surface_speed;
	element->SetAttribute("surface_speed", l_ossValue.str().c_str() );

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

	double surface_speed = 600.0;
	if (element->Attribute("surface_speed")) 
		surface_speed = atof(element->Attribute("surface_speed"));

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
								surface_speed );
	new_object->ReadBaseXML(element);

	return new_object;
}


void CSpeedReference::OnEditString(const wxChar* str){
        m_title.assign(str);
	heeksCAD->WasModified(this);
}

void CSpeedReference::ResetTitle()
{
#ifdef UNICODE
	std::wostringstream l_ossTitle;
#else
	std::ostringstream l_ossTitle;
#endif

	CCuttingToolParams::MaterialsList_t materials = CCuttingToolParams::GetMaterialsList();
	std::map< int, wxString > materials_map;
	std::copy( materials.begin(), materials.end(), std::inserter( materials_map, materials_map.begin() ) );

	l_ossTitle << m_material_name.c_str() << " (" << m_brinell_hardness_of_raw_material << ") with " << materials_map[m_cutting_tool_material].c_str();

	OnEditString(l_ossTitle.str().c_str());
} // End ResetTitle() method



