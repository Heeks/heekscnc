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


static void on_set_cutting_tool_material(int zero_based_choice, HeeksObj* object)
{
	if (zero_based_choice < 0) return;	// An error has occured.

	CSpeedReference *pSpeedReference = (CSpeedReference *) object;

	pSpeedReference->m_cutting_tool_material = zero_based_choice;
	pSpeedReference->ResetTitle();
	heeksCAD->RefreshProperties();
}

static void on_set_brinell_hardness_of_raw_material(double value, HeeksObj* object){((CSpeedReference*)object)->m_brinell_hardness_of_raw_material = value; ((CSpeedReference*)object)->ResetTitle(); }

/**
	This is either expressed in metres per minute (when m_units = 1) or feet per minute (when m_units = 25.4).  These
	are not the normal conversion for the m_units parameter but these seem to be the two standards by which these
	surface speeds are specified.
 */
static void on_set_surface_speed(double value, HeeksObj* object)
{
	if (theApp.m_program->m_units == 1.0)
	{
		// We're in metric already.  Leave it as they've entered it.
		((CSpeedReference*)object)->m_surface_speed = value;
	} // End if - then
	else
	{
		// We're using imperial measurements.  Convert from 'feet per minute' into
		// 'metres per minute' for a consistent internal representation.

		double metres_per_foot = (25.4 * 12.0) / 1000.0;
		((CSpeedReference*)object)->m_surface_speed = double(value * metres_per_foot);
	} // End if - else

	((CSpeedReference*)object)->ResetTitle();
}

static void on_set_material_name(const wxChar *value, HeeksObj* object){((CSpeedReference*)object)->m_material_name = value; ((CSpeedReference*)object)->ResetTitle(); }


const wxBitmap &CSpeedReference::GetIcon()
{
	static wxBitmap* icon = NULL;
	if(icon == NULL)icon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/speeds.png")));
	return *icon;
}

void CSpeedReference::GetProperties(std::list<Property *> *list)
{
	{
		CCuttingToolParams::MaterialsList_t materials = CCuttingToolParams::GetMaterialsList();

		int choice = -1;
		std::list< wxString > choices;
		for (CCuttingToolParams::MaterialsList_t::size_type i=0; i<materials.size(); i++)
		{
			choices.push_back(materials[i].second);
			if (m_cutting_tool_material == materials[i].first) choice = int(i);

		} // End for
		list->push_back(new PropertyChoice(_("Material"), choices, choice, this, on_set_cutting_tool_material));
	}

	list->push_back(new PropertyDouble(_("Brinell hardness of raw material"), m_brinell_hardness_of_raw_material, this, on_set_brinell_hardness_of_raw_material));

	if (theApp.m_program->m_units == 1.0)
	{
		// We're in metric.
		list->push_back(new PropertyDouble(_("Surface Speed (m/min)"), m_surface_speed, this, on_set_surface_speed));
	} // End if - then
	else
	{
		// We're using imperial measurements.
		double feet_per_metre = 1000 / (25.4 * 12.0);
		list->push_back(new PropertyDouble(_("Surface Speed (ft/min)"), (m_surface_speed * feet_per_metre), this, on_set_surface_speed));
	} // End if - else

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
	element->SetAttribute("surface_speed", m_surface_speed );
	element->SetAttribute("cutting_tool_material", int(m_cutting_tool_material) );
	element->SetAttribute("brinell_hardness_of_raw_material", m_brinell_hardness_of_raw_material );
	element->SetAttribute("raw_material_name", Ttc(m_material_name.c_str()) );

	WriteBaseXML(element);
}

// static member function
HeeksObj* CSpeedReference::ReadFromXMLElement(TiXmlElement* element)
{
	double brinell_hardness_of_raw_material = 15.0;
	if (element->Attribute("brinell_hardness_of_raw_material"))
		 element->Attribute("brinell_hardness_of_raw_material", &brinell_hardness_of_raw_material);

	double surface_speed = 600.0;
	if (element->Attribute("surface_speed")) element->Attribute("surface_speed", &surface_speed);

	wxString material_name;
	if (element->Attribute("raw_material_name")) material_name = Ctt(element->Attribute("raw_material_name"));

	int cutting_tool_material = int(CCuttingToolParams::eCarbide);
	if (element->Attribute("cutting_tool_material")) element->Attribute("cutting_tool_material", &cutting_tool_material);

	wxString title(Ctt(element->Attribute("title")));
	CSpeedReference* new_object = new CSpeedReference( 	material_name,
								cutting_tool_material,
								brinell_hardness_of_raw_material,
								surface_speed );
	new_object->ReadBaseXML(element);
	return new_object;
}


void CSpeedReference::OnEditString(const wxChar* str){
        m_title.assign(str);
	heeksCAD->Changed();
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


bool CSpeedReference::operator== ( const CSpeedReference & rhs ) const
{
    if (m_cutting_tool_material != rhs.m_cutting_tool_material) return(false);
	if (m_material_name != rhs.m_material_name) return(false);
	if (m_brinell_hardness_of_raw_material != rhs.m_brinell_hardness_of_raw_material) return(false);
	if (m_surface_speed != rhs.m_surface_speed) return(false);

	return(true);
}



