
#include "Program.h"
#include "HeeksCNC.h"
#include "RawMaterial.h"

#include "interface/Property.h"
#include "interface/PropertyChoice.h"
#include "interface/PropertyDouble.h"
#include "interface/HeeksObj.h"
#include "ProgramCanvas.h"

#include <wx/string.h>
#include <sstream>
#include <map>
#include <list>
#include <algorithm>
#include <vector>


static void on_set_raw_material(int value, HeeksObj* object)
{
	printf("on_set_raw_material() selected option %d\n", value );

	std::set<wxString> materials = CSpeedReferences::GetMaterials();
	std::vector<wxString> copy;

	std::copy( materials.begin(), materials.end(), std::inserter(copy,copy.end()));

	if (value <= int(copy.size()))
	{
		printf("Option %d is '%s'\n", value, copy[value-1].c_str() );

		((CProgram *)object)->m_raw_material.m_material_name = copy[value-1];
		((CProgram *)object)->m_raw_material.m_brinell_hardness = 0.0;	// Now that they've selected a material, they need to reset the hardness
	} // End if - then
	heeksCAD->WasModified(object);
}

static void on_set_brinell_hardness(int value, HeeksObj *object)
{
	printf("on_set_brinell_hardness() selected option %d\n", value );

	std::set<double> choices = CSpeedReferences::GetHardnessForMaterial( ((CProgram *)object)->m_raw_material.m_material_name );
	std::vector<double> choice_array;
	std::copy( choices.begin(), choices.end(), std::inserter( choice_array, choice_array.begin() ) );
	if (value <= int(choice_array.size()))
	{
		((CProgram *)object)->m_raw_material.m_brinell_hardness = choice_array[value-1];
		heeksCAD->WasModified(object);
	} // End if - then
} // End on_set_brinell_hardness() routine


/**
	\class CRawMaterial
	\ingroup classes
	\brief Defines material hardness of the raw material being machined.  This value helps
		to determine recommended feed and speed settings.
 */
CRawMaterial::CRawMaterial()
{
	m_brinell_hardness = 0.0;
	m_material_name = _T("Please select a material to machine");
}

double CRawMaterial::Hardness() const
{
	return(m_brinell_hardness);
} // End Hardness() method


void CRawMaterial::GetProperties(CProgram *parent, std::list<Property *> *list)
{
	{
		std::list< wxString > choices;
		int choice = 0;
		std::set< wxString > materials = theApp.m_program->m_speed_references->GetMaterials();
		std::copy( materials.begin(), materials.end(), std::inserter( choices, choices.begin() ) );
		if (std::find( choices.begin(), choices.end(), m_material_name ) != choices.end())
		{
			choice = int(std::distance( std::find( choices.begin(), choices.end(), m_material_name ), choices.begin() ));
		} // End if - then
		list->push_back(new PropertyChoice(_("Raw Material"), choices, choice, parent, on_set_raw_material));
	}

	{
		std::set<double> hardness_values = theApp.m_program->m_speed_references->GetHardnessForMaterial(m_material_name);
		std::list<wxString> choices;

		int choice = 0;
		for (std::set<double>::const_iterator l_itChoice = hardness_values.begin(); l_itChoice != hardness_values.end(); l_itChoice++)
		{
#ifdef UNICODE
			std::wostringstream l_ossChoice;
#else
			std::ostringstream l_ossChoice;
#endif

			l_ossChoice << *l_itChoice;
			if (m_brinell_hardness == *l_itChoice)
			{
				choice = int(std::distance( hardness_values.begin(), l_itChoice ));
			} // End if - then

			choices.push_back( l_ossChoice.str().c_str() );
		} // End for

		list->push_back(new PropertyChoice(_("Brinell Hardness of raw material"), choices, choice, parent, on_set_brinell_hardness));
	}
} // End GetProperties() method

void CRawMaterial::WriteBaseXML(TiXmlElement *element)
{
	element->SetDoubleAttribute("brinell_hardness", m_brinell_hardness);
} // End WriteBaseXML() method

void CRawMaterial::ReadBaseXML(TiXmlElement* element)
{
	if (element->Attribute("brinell_hardness")) m_brinell_hardness = atof(element->Attribute("brinell_hardness"));
} // End ReadBaseXML() method

/**
	This method is called when the CAD operator presses the Python button.  This method generates
	Python source code whose job will be to generate RS-274 GCode.  It's done in two steps so that
	the Python code can be configured to generate GCode suitable for various CNC interpreters.
 */
void CRawMaterial::AppendTextToProgram()
{
#ifdef UNICODE
	std::wostringstream ss;
#else
    std::ostringstream ss;
#endif
    ss.imbue(std::locale("C"));

	ss << "comment('Feeds and Speeds set for machining " << m_material_name.c_str() << "')\n";
	theApp.m_program_canvas->m_textCtrl->AppendText(ss.str().c_str());

}


