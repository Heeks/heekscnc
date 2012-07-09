// Positioning.cpp
/*
 * Copyright (c) 2009, Dan Heeks, Perttu Ahola
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "stdafx.h"
#include "Positioning.h"
#include "CNCConfig.h"
#include "ProgramCanvas.h"
#include "interface/HeeksObj.h"
#include "interface/PropertyInt.h"
#include "interface/PropertyDouble.h"
#include "interface/PropertyLength.h"
#include "interface/PropertyChoice.h"
#include "tinyxml/tinyxml.h"
#include "CTool.h"
#include "Profile.h"
#include "Drilling.h"
#include "MachineState.h"

#include <sstream>
#include <iomanip>
#include <vector>
#include <algorithm>

extern CHeeksCADInterface* heeksCAD;


void CPositioningParams::set_initial_values()
{
	CNCConfig config(ConfigScope());

	config.Read(_T("m_standoff"), &m_standoff, (25.4 / 4));	// Quarter of an inch
	config.Read(_T("m_sort_locations"), &m_sort_locations, 1);
}

void CPositioningParams::write_values_to_config()
{
	// We always want to store the parameters in mm and convert them back later on.

	CNCConfig config(ConfigScope());

	// These values are in mm.
	config.Write(_T("m_standoff"), m_standoff);
	config.Write(_T("m_sort_locations"), m_sort_locations);
}


static void on_set_standoff(double value, HeeksObj* object)
{
	((CPositioning*)object)->m_params.m_standoff = value;
	((CPositioning*)object)->m_params.write_values_to_config();
}

static void on_set_sort_locations(int value, HeeksObj* object)
{
	((CPositioning*)object)->m_params.m_sort_locations = value;
	((CPositioning*)object)->m_params.write_values_to_config();
}


void CPositioningParams::GetProperties(CPositioning* parent, std::list<Property *> *list)
{
	list->push_back(new PropertyLength(_("standoff"), m_standoff, parent, on_set_standoff));
	{ // Begin choice scope
		std::list< wxString > choices;

		choices.push_back(_("Respect existing order"));	// Must be 'false' (0)
		choices.push_back(_("True"));			// Must be 'true' (non-zero)

		int choice = int(m_sort_locations);
		list->push_back(new PropertyChoice(_("sort_locations"), choices, choice, parent, on_set_sort_locations));
	} // End choice scope
}

void CPositioningParams::WriteXMLAttributes(TiXmlNode *root)
{
	TiXmlElement * element;
	element = heeksCAD->NewXMLElement( "params" );
	heeksCAD->LinkXMLEndChild( root,  element );

	element->SetDoubleAttribute( "standoff", m_standoff);
	element->SetAttribute( "sort_locations", m_sort_locations);
}

void CPositioningParams::ReadParametersFromXMLElement(TiXmlElement* pElem)
{
	if (pElem->Attribute("standoff")) pElem->Attribute("standoff", &m_standoff);
	if (pElem->Attribute("sort_locations")) pElem->Attribute("sort_locations", &m_sort_locations);
}

const wxBitmap &CPositioning::GetIcon()
{
	static wxBitmap* icon = NULL;
	if(icon == NULL)icon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/locating.png")));
	return *icon;
}

/**
	This method is called when the CAD operator presses the Python button.  This method generates
	Python source code whose job will be to generate RS-274 GCode.  It's done in two steps so that
	the Python code can be configured to generate GCode suitable for various CNC interpreters.
 */
Python CPositioning::AppendTextToProgram( CMachineState *pMachineState )
{
	Python python;

	python << COp::AppendTextToProgram( pMachineState );

	std::vector<CNCPoint> locations = CDrilling::FindAllLocations(this, pMachineState->Location(), (m_params.m_sort_locations != 0), NULL);
	for (std::vector<CNCPoint>::const_iterator l_itLocation = locations.begin(); l_itLocation != locations.end(); l_itLocation++)
	{
		CNCPoint location( *l_itLocation );

		// Move the Z location up above the workpiece.
		location.SetZ( location.Z() + m_params.m_standoff );

		// Rotate the point to align it with the fixture
		CNCPoint point( location );

		python << _T("rapid(")
			<< _T("x=") << point.X(true) << _T(", ")
			<< _T("y=") << point.Y(true) << _T(", ")
			<< _T("z=") << point.Z(true) << _T(")\n");
		python << _T("program_stop(optional=False)\n");
		pMachineState->Location(point); // Remember where we are.
	} // End for

	return(python);
}



/**
	This is the Graphics Library Commands (from the OpenGL set).  This method calls the OpenGL
	routines to paint the drill action in the graphics window.  The graphics is transient.

	Part of its job is to re-paint the elements that this CPositioning object refers to so that
	we know what CAD objects this CNC operation is referring to.
 */
void CPositioning::glCommands(bool select, bool marked, bool no_color)
{
	COp::glCommands(select, marked, no_color);
}


void CPositioning::GetProperties(std::list<Property *> *list)
{
	m_params.GetProperties(this, list);
	COp::GetProperties(list);
}

HeeksObj *CPositioning::MakeACopy(void)const
{
	return new CPositioning(*this);
}

void CPositioning::CopyFrom(const HeeksObj* object)
{
	operator=(*((CPositioning*)object));
}

bool CPositioning::CanAddTo(HeeksObj* owner)
{
	return ((owner != NULL) && (owner->GetType() == OperationsType));
}

CPositioning::CPositioning( const CPositioning & rhs ) : COp(rhs)
{
	m_symbols.clear();
	std::copy( rhs.m_symbols.begin(), rhs.m_symbols.end(), std::inserter( m_symbols, m_symbols.begin() ));
	m_params = rhs.m_params;
}

CPositioning::CPositioning(	const Symbols_t &symbols )
		: COp(GetTypeString(), 0, PositioningType), m_symbols(symbols)
{
    m_params.set_initial_values();

    for (Symbols_t::const_iterator symbol = symbols.begin(); symbol != symbols.end(); symbol++)
    {
        HeeksObj *object = heeksCAD->GetIDObject( symbol->first, symbol->second );
        if (object != NULL)
        {
            Add( object, NULL );
        } // End if - then
    } // End for

    m_symbols.clear();	// we don't want to do this twice.
}


CPositioning & CPositioning::operator= ( const CPositioning & rhs )
{
	if (this != &rhs)
	{
		COp::operator=( rhs );

		m_symbols.clear();
		std::copy( rhs.m_symbols.begin(), rhs.m_symbols.end(), std::inserter( m_symbols, m_symbols.begin() ));

		m_params = rhs.m_params;
	}

	return(*this);
}

void CPositioning::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element = heeksCAD->NewXMLElement( "Positioning" );
	heeksCAD->LinkXMLEndChild( root,  element );
	m_params.WriteXMLAttributes(element);

	TiXmlElement * symbols;
	symbols = heeksCAD->NewXMLElement( "symbols" );
	heeksCAD->LinkXMLEndChild( element, symbols );

	for (Symbols_t::const_iterator l_itSymbol = m_symbols.begin(); l_itSymbol != m_symbols.end(); l_itSymbol++)
	{
		TiXmlElement * symbol = heeksCAD->NewXMLElement( "symbol" );
		symbols->LinkEndChild( symbol );
		symbol->SetAttribute("type", l_itSymbol->first );
		symbol->SetAttribute("id", l_itSymbol->second );
	} // End for

	WriteBaseXML(element);
}

// static member function
HeeksObj* CPositioning::ReadFromXMLElement(TiXmlElement* element)
{
	CPositioning* new_object = new CPositioning;

	std::list<TiXmlElement *> elements_to_remove;

	// read point and circle ids
	for(TiXmlElement* pElem = heeksCAD->FirstXMLChildElement( element ) ; pElem; pElem = pElem->NextSiblingElement())
	{
		std::string name(pElem->Value());
		if(name == "params"){
			new_object->m_params.ReadParametersFromXMLElement(pElem);
			elements_to_remove.push_back(pElem);
		}
		else if(name == "symbols"){
			for(TiXmlElement* child = heeksCAD->FirstXMLChildElement( pElem ) ; child; child = child->NextSiblingElement())
			{
				if (child->Attribute("type") && child->Attribute("id"))
				{
					new_object->AddSymbol( atoi(child->Attribute("type")), atoi(child->Attribute("id")) );
				}
			} // End for
			elements_to_remove.push_back(pElem);
		} // End if
	}

	for (std::list<TiXmlElement*>::iterator itElem = elements_to_remove.begin(); itElem != elements_to_remove.end(); itElem++)
	{
		heeksCAD->RemoveXMLChild( element, *itElem);
	}

	new_object->ReadBaseXML(element);

	return new_object;
}


/**
	The old version of the CDrilling object stored references to graphics as type/id pairs
	that get read into the m_symbols list.  The new version stores these graphics references
	as child elements (based on ObjList).  If we read in an old-format file then the m_symbols
	list will have data in it for which we don't have children.  This routine converts
	these type/id pairs into the HeeksObj pointers as children.
 */
void CPositioning::ReloadPointers()
{
	for (Symbols_t::iterator symbol = m_symbols.begin(); symbol != m_symbols.end(); symbol++)
	{
		HeeksObj *object = heeksCAD->GetIDObject( symbol->first, symbol->second );
		if (object != NULL)
		{
			Add( object, NULL );
		}
	}

	m_symbols.clear();	// We don't want to convert them twice.

	COp::ReloadPointers();
}


/**
	This method adjusts any parameters that don't make sense.  It should report a list
	of changes in the list of strings.
 */
std::list<wxString> CPositioning::DesignRulesAdjustment(const bool apply_changes)
{
	std::list<wxString> changes;
	return(changes);

} // End DesignRulesAdjustment() method


/**
    This method returns TRUE if the type of symbol is suitable for reference as a source of location
 */
/* static */ bool CPositioning::ValidType( const int object_type )
{
    switch (object_type)
    {
        case PointType:
        case CircleType:
        case SketchType:
        case DrillingType:
            return(true);

        default:
            return(false);
    }
}

void CPositioning::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
    COp::GetTools( t_list, p );
}


bool CPositioningParams::operator== ( const CPositioningParams & rhs ) const
{
	if (m_standoff != rhs.m_standoff) return(false);
	if (m_sort_locations != rhs.m_sort_locations) return(false);

	return(true);
}

bool CPositioning::operator== ( const CPositioning & rhs ) const
{
	if (m_params != rhs.m_params) return(false);

	return(COp::operator==(rhs));
}


