// CuttingRate.cpp
/*
 * Copyright (c) 2009, Dan Heeks, Perttu Ahola
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "stdafx.h"
#include <math.h>
#include "CuttingRate.h"
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


static void on_set_brinell_hardness_of_raw_material(int zero_based_choice, HeeksObj* object)
{
	if (zero_based_choice < 0) return;	// An error has occured.

	std::set<double> all_values = CSpeedReferences::GetAllHardnessValues();
	for (std::set<double>::iterator l_itHardness = all_values.begin(); l_itHardness != all_values.end(); l_itHardness++)
	{
		if (std::distance( all_values.begin(), l_itHardness) == zero_based_choice)
		{
			((CCuttingRate*)object)->m_brinell_hardness_of_raw_material = *l_itHardness;
			break;
		} // End if - then
	} // End for
	((CCuttingRate*)object)->ResetTitle(); 
}

const wxBitmap &CCuttingRate::GetIcon()
{
	static wxBitmap* icon = NULL;
	if(icon == NULL)icon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/cutting_rate.png")));
	return *icon;
}

static void on_set_max_material_removal_rate(double value, HeeksObj* object){((CCuttingRate*)object)->m_max_material_removal_rate = value; ((CCuttingRate*)object)->ResetTitle(); }

void CCuttingRate::GetProperties(std::list<Property *> *list)
{
	int choice = -1;
	std::list<wxString> choices;
	std::set<double> all_values = CSpeedReferences::GetAllHardnessValues();
	for (std::set<double>::iterator l_itHardness = all_values.begin(); l_itHardness != all_values.end(); l_itHardness++)
	{
		std::ostringstream l_ossValue;
		l_ossValue << *l_itHardness;
		choices.push_back( Ctt(l_ossValue.str().c_str()) );
		if (m_brinell_hardness_of_raw_material == *l_itHardness) choice = std::distance( all_values.begin(), l_itHardness );
	} // End for

	list->push_back(new PropertyChoice(_("Brinell hardness of raw material"), choices, choice, this, on_set_brinell_hardness_of_raw_material));

	if (theApp.m_program->m_units == 1.0)
	{
		// We're set to metric.  Just present the internal value.
		list->push_back(new PropertyDouble(_("Maximum Material Removal Rate (mm^3/min)"), m_max_material_removal_rate, this, on_set_max_material_removal_rate));
	} // End if - then
	else
	{
		// We're set to imperial.  Convert the internal (metric) value for presentation.

		double cubic_mm_per_cubic_inch = 25.4 * 25.4 * 25.4;
		list->push_back(new PropertyDouble(_("Maximum Material Removal Rate (inches^3/min)"), m_max_material_removal_rate / cubic_mm_per_cubic_inch, this, on_set_max_material_removal_rate));
	} // End if - else

	HeeksObj::GetProperties(list);
}


HeeksObj *CCuttingRate::MakeACopy(void)const
{
	return new CCuttingRate(*this);
}

void CCuttingRate::CopyFrom(const HeeksObj* object)
{
	operator=(*((CCuttingRate*)object));
}

bool CCuttingRate::CanAddTo(HeeksObj* owner)
{
	return owner->GetType() == SpeedReferencesType;
}

void CCuttingRate::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element = new TiXmlElement( "CuttingRate" );
	root->LinkEndChild( element );  
	element->SetAttribute("title", Ttc(m_title.c_str()));

	std::ostringstream l_ossValue;

	l_ossValue.str(""); l_ossValue << m_brinell_hardness_of_raw_material;
	element->SetAttribute("brinell_hardness_of_raw_material", l_ossValue.str().c_str() );

	l_ossValue.str(""); l_ossValue << m_max_material_removal_rate;
	element->SetAttribute("max_material_removal_rate", l_ossValue.str().c_str() );

	WriteBaseXML(element);
}

// static member function
HeeksObj* CCuttingRate::ReadFromXMLElement(TiXmlElement* element)
{
	double brinell_hardness_of_raw_material = 15.0;
	if (element->Attribute("brinell_hardness_of_raw_material")) 
		brinell_hardness_of_raw_material = atof(element->Attribute("brinell_hardness_of_raw_material"));

	double max_material_removal_rate = 1.0;
	if (element->Attribute("max_material_removal_rate")) 
		max_material_removal_rate = atof(element->Attribute("max_material_removal_rate"));

	wxString title(Ctt(element->Attribute("title")));
	CCuttingRate* new_object = new CCuttingRate( 	brinell_hardness_of_raw_material,
							max_material_removal_rate );
	new_object->ReadBaseXML(element);
	return new_object;
}


void CCuttingRate::OnEditString(const wxChar* str){
        m_title.assign(str);
	heeksCAD->Changed();
}

void CCuttingRate::ResetTitle()
{
#ifdef UNICODE
	std::wostringstream l_ossTitle;
#else
	std::ostringstream l_ossTitle;
#endif

	l_ossTitle << "Brinell (" << m_brinell_hardness_of_raw_material << ") at ";
	if (theApp.m_program->m_units == 1.0)
	{
		// We're set to metric.  Just present the internal value.
		l_ossTitle << m_max_material_removal_rate << " (mm^3/min)";
	} // End if - then
	else
	{
		// We're set to imperial.  Convert the internal (metric) value for presentation.

		double cubic_mm_per_cubic_inch = 25.4 * 25.4 * 25.4;
		l_ossTitle << m_max_material_removal_rate / cubic_mm_per_cubic_inch << " (inches^3/min)";
	} // End if - else

	OnEditString(l_ossTitle.str().c_str());
} // End ResetTitle() method




