
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
	((CProgram *)object)->m_raw_material.Set(value);
	heeksCAD->WasModified(object);
}

static void on_set_brinell_hardness(double value, HeeksObj *object)
{
	((CProgram *)object)->m_raw_material.Set(value);
	heeksCAD->WasModified(object);
} // End on_set_brinell_hardness() routine


/**
	\class CRawMaterial
	\ingroup classes
	\brief Defines material hardness of the raw material being machined.  This value helps
		to determine recommended feed and speed settings.
 */
CRawMaterial::BrinellHardnessTable_t	CRawMaterial::m_brinell_hardness_table;

CRawMaterial::CRawMaterial( const double brinell_hardness /* = HARDNESS_OF_ALUMINIUM */ ) : m_brinell_hardness(brinell_hardness) 
{
       	// Setup a default lookup table.  We may want to make this configurable (one day)
	m_brinell_hardness_table.insert( std::make_pair( 1.6, wxString(_T("Softwood")) ) );
	m_brinell_hardness_table.insert( std::make_pair( 5.0, wxString(_T("Hardwood")) ) );
	m_brinell_hardness_table.insert( std::make_pair( HARDNESS_OF_ALUMINIUM, wxString(_T("Aluminium")) ) );
	m_brinell_hardness_table.insert( std::make_pair( 35.0, wxString(_T("Copper")) ) );
	m_brinell_hardness_table.insert( std::make_pair( 120.0, wxString(_T("Mild Steel")) ) );
	m_brinell_hardness_table.insert( std::make_pair( 200.0, wxString(_T("Stainless Steel")) ) );
	m_brinell_hardness_table.insert( std::make_pair( 1550.0, wxString(_T("Glass")) ) );
	m_brinell_hardness_table.insert( std::make_pair( 1700.0, wxString(_T("Hardenned Tool Steel")) ) );
}

/**
	This method is called with the zero-based offset through the choices which is taken, in turn,
	from the offset through the m_brinell_hardness_table.
 */
bool CRawMaterial::Set( const int choice_offset )
{
	std::vector<std::pair<double, wxString> > table;
	BrinellHardnessTable_t copy;

	std::copy( m_brinell_hardness_table.begin(), m_brinell_hardness_table.end(), std::inserter(copy,copy.end()));

	if (copy.find(m_brinell_hardness) == copy.end())
	{
		// We don't have an exact match.  Add one.
		copy.insert( std::make_pair( m_brinell_hardness, Name() ) );
	} // End if - then

	std::copy( copy.begin(), copy.end(), std::inserter(table,table.end()));

	if (choice_offset < int(table.size())) 
	{
		m_brinell_hardness = table[choice_offset].first;
		return(true);
	} // End if - then

	return(false);	// Not found
} // End Set() method

bool CRawMaterial::Set( const wxString & material_name )
{
	for (BrinellHardnessTable_t::const_iterator l_itMaterial = m_brinell_hardness_table.begin();
		l_itMaterial != m_brinell_hardness_table.end(); l_itMaterial++)
	{
		if (l_itMaterial->second == material_name)
		{
			m_brinell_hardness = l_itMaterial->first;
			return(true);	// Found.
		} // End if - then
	} // End for

	return(false);	// not found.
} // End Set() method

double CRawMaterial::Hardness() const
{
	return(m_brinell_hardness);
} // End Hardness() method

void CRawMaterial::Set( const double brinell_hardness )
{
	m_brinell_hardness = brinell_hardness;
} // End Set() method

wxString CRawMaterial::Name() const
{
	// See if the hardness value matches a name in our table.  If not, derive one
	// by looking at the hardness values and their names that we do have.
#ifdef UNICODE
	std::wostringstream l_ossName;
#else
	std::ostringstream l_ossName;
#endif


	double previous_hardness = -1;
	wxString previous_name = _T("");

	for (BrinellHardnessTable_t::const_iterator l_itMaterial = m_brinell_hardness_table.begin();
		l_itMaterial != m_brinell_hardness_table.end(); l_itMaterial++)
	{

		if (l_itMaterial->first == m_brinell_hardness) return(l_itMaterial->second);
		if (l_itMaterial->first > m_brinell_hardness)
		{
			// We've come to a material that is harder than our current value.
			// Create a name that comes somewhere between the previous one and
			// this one.

			if (previous_hardness < 0.0)
			{
				// We're softer than the softest one in our list.

				l_ossName << "Softer than " << l_itMaterial->second.c_str();
				return(l_ossName.str().c_str());
			} // End if - then

			// See if it's closer to the last one or this one.  i.e. is it
			// 'harder than ' (the last one) or 'softer than' (this one).

			double distance_back = m_brinell_hardness - previous_hardness;
			double distance_forward = l_itMaterial->first - m_brinell_hardness;

			if (distance_back < distance_forward)
			{
				l_ossName << "Harder than " << previous_name.c_str();
				return(l_ossName.str().c_str());
			} // End if - then
			else
			{
				l_ossName << "Softer than " << l_itMaterial->second.c_str();
				return(l_ossName.str().c_str());
			} // End if - else
		} // End if - then

		previous_hardness = l_itMaterial->first;
		previous_name = l_itMaterial->second;
	} // End for

	// It must be harder than anything in our list.
	l_ossName << "Harder than " << m_brinell_hardness_table.rbegin()->second.c_str();
	return(l_ossName.str().c_str());
} // End Name() method.

void CRawMaterial::GetProperties(CProgram *parent, std::list<Property *> *list)
{
	std::list< wxString > choices;
	int choice;
	BrinellHardnessTable_t copy;

	std::copy( m_brinell_hardness_table.begin(), m_brinell_hardness_table.end(),
			std::inserter( copy, copy.begin() ) );

	if (copy.find(m_brinell_hardness) == copy.end())
	{
		// We don't have an exact match.  Add one.
		copy.insert( std::make_pair( m_brinell_hardness, Name() ) );
	} // End if - then

	for (BrinellHardnessTable_t::const_iterator l_itMaterial = copy.begin();
			l_itMaterial != copy.end(); l_itMaterial++)
	{
		choices.push_back(l_itMaterial->second.c_str());
		if (l_itMaterial->first == m_brinell_hardness) choice = choices.size()-1;	// zero-based.
	} // End for
	
	list->push_back(new PropertyChoice(_("Raw Material"), choices, choice, parent, on_set_raw_material));
	list->push_back(new PropertyDouble(_("Brinell Hardness of raw material"), m_brinell_hardness, parent, on_set_brinell_hardness));
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

	ss << "comment(Feeds and Speeds set for machining " << Name() << ")\n";
	theApp.m_program_canvas->m_textCtrl->AppendText(ss.str().c_str());

}


