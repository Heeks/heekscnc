// Surface.cpp

#include <stdafx.h>

#include "Surface.h"
#include "Surfaces.h"
#include "Program.h"
#include "CNCConfig.h"
#include "../../tinyxml/tinyxml.h"
#include "../../interface/PropertyLength.h"
#include "../../interface/PropertyCheck.h"
#include "Reselect.h"
#include "SurfaceDlg.h"

int CSurface::number_for_stl_file = 1;

CSurface::CSurface()
{
	ReadDefaultValues();
}

HeeksObj *CSurface::MakeACopy(void)const
{
	return new CSurface(*this);
}

void CSurface::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element = heeksCAD->NewXMLElement( "Surface" );
	heeksCAD->LinkXMLEndChild( root,  element );

	element->SetDoubleAttribute( "tolerance", m_tolerance);
	element->SetDoubleAttribute( "material_allowance", m_material_allowance);
	element->SetAttribute( "same_for_posns", m_same_for_each_pattern_position ? 1:0);

	// write solid ids
	for (std::list<int>::iterator It = m_solids.begin(); It != m_solids.end(); It++)
    {
		int solid = *It;

		TiXmlElement * solid_element = heeksCAD->NewXMLElement( "solid" );
		heeksCAD->LinkXMLEndChild( element, solid_element );
		solid_element->SetAttribute("id", solid);
	}

	IdNamedObj::WriteBaseXML(element);
}

// static member function
HeeksObj* CSurface::ReadFromXMLElement(TiXmlElement* element)
{
	CSurface* new_object = new CSurface;

	element->Attribute("tolerance", &new_object->m_tolerance);
	element->Attribute("material_allowance", &new_object->m_material_allowance);
	int int_for_bool = 1;
	if(element->Attribute( "same_for_posns", &int_for_bool))new_object->m_same_for_each_pattern_position = (int_for_bool != 0);

	if(const char* pstr = element->Attribute("title"))new_object->m_title = Ctt(pstr);

	// read solid ids
	for(TiXmlElement* pElem = heeksCAD->FirstXMLChildElement( element ) ; pElem; pElem = pElem->NextSiblingElement())
	{
		std::string name(pElem->Value());
		if(name == "solid"){
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

void CSurface::WriteDefaultValues()
{
	CNCConfig config;
	config.Write(wxString(GetTypeString()) + _T("Tolerance"), m_tolerance);
	config.Write(wxString(GetTypeString()) + _T("MatAllowance"), m_material_allowance);
	config.Write(wxString(GetTypeString()) + _T("SameForPositions"), m_same_for_each_pattern_position);
}

void CSurface::ReadDefaultValues()
{
	CNCConfig config;
	config.Read(wxString(GetTypeString()) + _T("Tolerance"), &m_tolerance, 0.01);
	config.Read(wxString(GetTypeString()) + _T("MatAllowance"), &m_material_allowance, 0.0);
	config.Read(wxString(GetTypeString()) + _T("SameForPositions"), &m_same_for_each_pattern_position, true);
}

static void on_set_tolerance(double value, HeeksObj* object){((CSurface*)object)->m_tolerance = value; ((CSurface*)object)->WriteDefaultValues();}
static void on_set_material_allowance(double value, HeeksObj* object){((CSurface*)object)->m_material_allowance = value; ((CSurface*)object)->WriteDefaultValues();}
static void on_set_same_for_position(bool value, HeeksObj* object){((CSurface*)object)->m_same_for_each_pattern_position = value; ((CSurface*)object)->WriteDefaultValues();}

void CSurface::GetProperties(std::list<Property *> *list)
{
	AddSolidsProperties(list, m_solids);

	list->push_back(new PropertyLength(_("tolerance"), m_tolerance, this, on_set_tolerance));
	list->push_back(new PropertyLength(_("material allowance"), m_material_allowance, this, on_set_material_allowance));
	list->push_back(new PropertyCheck(_("same for each pattern position"), m_same_for_each_pattern_position, this, on_set_same_for_position));

	IdNamedObj::GetProperties(list);
}

void CSurface::CopyFrom(const HeeksObj* object)
{
	if (object->GetType() == GetType())
	{
		operator=(*((CSurface*)object));
	}
}

bool CSurface::CanAddTo(HeeksObj* owner)
{
	return ((owner != NULL) && (owner->GetType() == SurfacesType));
}

const wxBitmap &CSurface::GetIcon()
{
	static wxBitmap* icon = NULL;
	if(icon == NULL)icon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/surface.png")));
	return *icon;
}

static bool OnEdit(HeeksObj* object)
{
	return SurfaceDlg::Do((CSurface*)object);
}

void CSurface::GetOnEdit(bool(**callback)(HeeksObj*))
{
	*callback = OnEdit;
}

HeeksObj* CSurface::PreferredPasteTarget()
{
	return theApp.m_program->Surfaces();
}
