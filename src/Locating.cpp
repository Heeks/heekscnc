// Locating.cpp
/*
 * Copyright (c) 2009, Dan Heeks, Perttu Ahola
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "stdafx.h"
#include "Locating.h"
#include "CNCConfig.h"
#include "ProgramCanvas.h"
#include "interface/HeeksObj.h"
#include "interface/PropertyInt.h"
#include "interface/PropertyDouble.h"
#include "interface/PropertyLength.h"
#include "interface/PropertyChoice.h"
#include "tinyxml/tinyxml.h"
#include "CuttingTool.h"
#include "Profile.h"
#include "Fixture.h"
#include "Drilling.h"

#include <sstream>
#include <iomanip>
#include <vector>
#include <algorithm>

extern CHeeksCADInterface* heeksCAD;


void CLocatingParams::set_initial_values()
{
	CNCConfig config(ConfigScope());

	config.Read(_T("m_standoff"), &m_standoff, (25.4 / 4));	// Quarter of an inch
	config.Read(_T("m_sort_locations"), &m_sort_locations, 1);
}

void CLocatingParams::write_values_to_config()
{
	// We always want to store the parameters in mm and convert them back later on.

	CNCConfig config(ConfigScope());

	// These values are in mm.
	config.Write(_T("m_standoff"), m_standoff);
	config.Write(_T("m_sort_locations"), m_sort_locations);
}


static void on_set_standoff(double value, HeeksObj* object)
{
	((CLocating*)object)->m_params.m_standoff = value;
	((CLocating*)object)->m_params.write_values_to_config();
}

static void on_set_sort_locations(int value, HeeksObj* object)
{
	((CLocating*)object)->m_params.m_sort_locations = value;
	((CLocating*)object)->m_params.write_values_to_config();
}


void CLocatingParams::GetProperties(CLocating* parent, std::list<Property *> *list)
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

void CLocatingParams::WriteXMLAttributes(TiXmlNode *root)
{
	TiXmlElement * element;
	element = new TiXmlElement( "params" );
	root->LinkEndChild( element );

	element->SetDoubleAttribute("standoff", m_standoff);

	std::ostringstream l_ossValue;
	l_ossValue.str(""); l_ossValue << m_sort_locations;
	element->SetAttribute("sort_locations", l_ossValue.str().c_str());
}

void CLocatingParams::ReadParametersFromXMLElement(TiXmlElement* pElem)
{
	if (pElem->Attribute("standoff")) m_standoff = atof(pElem->Attribute("standoff"));
	if (pElem->Attribute("sort_locations")) m_sort_locations = atoi(pElem->Attribute("sort_locations"));
}

/**
	This method is called when the CAD operator presses the Python button.  This method generates
	Python source code whose job will be to generate RS-274 GCode.  It's done in two steps so that
	the Python code can be configured to generate GCode suitable for various CNC interpreters.
 */
void CLocating::AppendTextToProgram( const CFixture *pFixture )
{
	COp::AppendTextToProgram( pFixture );

#ifdef UNICODE
	std::wostringstream ss;
#else
    std::ostringstream ss;
#endif
    ss.imbue(std::locale("C"));
	ss<<std::setprecision(10);

	std::vector<CNCPoint> locations = FindAllLocations( m_symbols );
	for (std::vector<CNCPoint>::const_iterator l_itLocation = locations.begin(); l_itLocation != locations.end(); l_itLocation++)
	{
		CNCPoint location( *l_itLocation );

		// Move the Z location up above the workpiece.
		location.SetZ( location.Z() + m_params.m_standoff );

		// Rotate the point to align it with the fixture
		CNCPoint point( pFixture->Adjustment( location ) );

		ss << "rapid("
			<< "x=" << point.X(true) << ", "
			<< "y=" << point.Y(true) << ", "
			<< "z=" << point.Z(true) << ")\n";
		ss << "program_stop(optional=False)\n";
	} // End for

	theApp.m_program_canvas->m_textCtrl->AppendText(ss.str().c_str());
}



/**
	This is the Graphics Library Commands (from the OpenGL set).  This method calls the OpenGL
	routines to paint the drill action in the graphics window.  The graphics is transient.

	Part of its job is to re-paint the elements that this CLocating object refers to so that
	we know what CAD objects this CNC operation is referring to.
 */
void CLocating::glCommands(bool select, bool marked, bool no_color)
{
	if(marked && !no_color)
	{
		std::vector<CNCPoint> locations = FindAllLocations( m_symbols );

		for (Symbols_t::const_iterator l_itSymbol = m_symbols.begin(); l_itSymbol != m_symbols.end(); l_itSymbol++)
		{
			HeeksObj* object = heeksCAD->GetIDObject(l_itSymbol->first, l_itSymbol->second);

			// If we found something, ask its CAD code to draw itself highlighted.
			if(object)object->glCommands(false, true, false);
		} // End for
	} // End if - then
}


void CLocating::GetProperties(std::list<Property *> *list)
{
	m_params.GetProperties(this, list);
	COp::GetProperties(list);
}

HeeksObj *CLocating::MakeACopy(void)const
{
	return new CLocating(*this);
}

void CLocating::CopyFrom(const HeeksObj* object)
{
	operator=(*((CLocating*)object));
}

bool CLocating::CanAddTo(HeeksObj* owner)
{
	return owner->GetType() == OperationsType;
}

void CLocating::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element = new TiXmlElement( "Locating" );
	root->LinkEndChild( element );
	m_params.WriteXMLAttributes(element);

	TiXmlElement * symbols;
	symbols = new TiXmlElement( "symbols" );
	element->LinkEndChild( symbols );

	for (Symbols_t::const_iterator l_itSymbol = m_symbols.begin(); l_itSymbol != m_symbols.end(); l_itSymbol++)
	{
		TiXmlElement * symbol = new TiXmlElement( "symbol" );
		symbols->LinkEndChild( symbol );
		symbol->SetAttribute("type", l_itSymbol->first );
		symbol->SetAttribute("id", l_itSymbol->second );
	} // End for

	WriteBaseXML(element);
}

// static member function
HeeksObj* CLocating::ReadFromXMLElement(TiXmlElement* element)
{
	CLocating* new_object = new CLocating;

	// read point and circle ids
	for(TiXmlElement* pElem = TiXmlHandle(element).FirstChildElement().Element(); pElem; pElem = pElem->NextSiblingElement())
	{
		std::string name(pElem->Value());
		if(name == "params"){
			new_object->m_params.ReadParametersFromXMLElement(pElem);
		}
		else if(name == "symbols"){
			for(TiXmlElement* child = TiXmlHandle(pElem).FirstChildElement().Element(); child; child = child->NextSiblingElement())
			{
				if (child->Attribute("type") && child->Attribute("id"))
				{
					new_object->AddSymbol( atoi(child->Attribute("type")), atoi(child->Attribute("id")) );
				}
			} // End for
		} // End if
	}

	new_object->ReadBaseXML(element);

	return new_object;
}


/**
 * 	This method looks through the symbols in the list.  If they're PointType objects
 * 	then the object's location is added to the result set.  If it's a circle object
 * 	that doesn't intersect any other element (selected) then add its centre to
 * 	the result set.  Finally, find the intersections of all of these elements and
 * 	add the intersection points to the result vector.
 */
std::vector<CNCPoint> CLocating::FindAllLocations( const CLocating::Symbols_t & symbols ) const
{
	std::vector<CNCPoint> locations;

	// Look to find all intersections between all selected objects.  At all these locations, create
        // a locating cycle.

        for (CLocating::Symbols_t::const_iterator lhs = symbols.begin(); lhs != symbols.end(); lhs++)
        {
		bool l_bIntersectionsFound = false;	// If it's a circle and it doesn't
							// intersect anything else, we want to know
							// about it.

		if (lhs->first == PointType)
		{
			HeeksObj *lhsPtr = heeksCAD->GetIDObject( lhs->first, lhs->second );
			if (lhsPtr != NULL)
			{
				double pos[3];
				lhsPtr->GetStartPoint(pos);

				// Copy the results in ONLY if each point doesn't already exist.
				if (std::find( locations.begin(), locations.end(), CNCPoint( pos ) ) == locations.end())
				{
					locations.push_back( CNCPoint( pos ) );
				} // End if - then
			} // End if - then

			continue;	// No need to intersect a point with anything.
		} // End if - then

                for (CLocating::Symbols_t::const_iterator rhs = symbols.begin(); rhs != symbols.end(); rhs++)
                {
                        if (lhs == rhs) continue;
			if (lhs->first == PointType) continue;	// No need to intersect a point type.

                        std::list<double> results;
                        HeeksObj *lhsPtr = heeksCAD->GetIDObject( lhs->first, lhs->second );
                        HeeksObj *rhsPtr = heeksCAD->GetIDObject( rhs->first, rhs->second );

                        if ((lhsPtr != NULL) && (rhsPtr != NULL) && (lhsPtr->Intersects( rhsPtr, &results )))
                        {
				l_bIntersectionsFound = true;
                                while (((results.size() % 3) == 0) && (results.size() > 0))
                                {
                                        CNCPoint intersection;

                                        intersection.SetX( *(results.begin()) );
                                        results.erase(results.begin());

                                        intersection.SetY( *(results.begin()) );
                                        results.erase(results.begin());

                                        intersection.SetZ( *(results.begin()) );
                                        results.erase(results.begin());

					// Copy the results in ONLY if each point doesn't already exist.
					if (std::find( locations.begin(), locations.end(), intersection ) == locations.end())
					{
	                                        locations.push_back(intersection);
					} // End if - then
                                } // End while
                        } // End if - then
                } // End for

		if (! l_bIntersectionsFound)
		{
			// This element didn't intersect anything else.  If it's a circle
			// then add its centre point to the result set.

			if (lhs->first == CircleType)
			{
                        	HeeksObj *lhsPtr = heeksCAD->GetIDObject( lhs->first, lhs->second );
				double pos[3];
				if ((lhsPtr != NULL) && (heeksCAD->GetArcCentre( lhsPtr, pos )))
				{
					// Copy the results in ONLY if each point doesn't already exist.
					if (std::find( locations.begin(), locations.end(), CNCPoint( pos ) ) == locations.end())
					{
						locations.push_back( CNCPoint( pos ) );
					} // End if - then
				} // End if - then
			} // End if - then


			if (lhs->first == SketchType)
			{
                        	HeeksObj *lhsPtr = heeksCAD->GetIDObject( lhs->first, lhs->second );
				if (lhsPtr != NULL)
				{
					CBox bounding_box;
					lhsPtr->GetBox( bounding_box );
					double pos[3];
					bounding_box.Centre(pos);
					// Copy the results in ONLY if each point doesn't already exist.
					if (std::find( locations.begin(), locations.end(), CNCPoint( pos ) ) == locations.end())
					{
						locations.push_back( CNCPoint( pos ) );
					} // End if - then
				} // End if - then
			} // End if - then


			if (lhs->first == DrillingType)
			{
                        	HeeksObj *lhsPtr = heeksCAD->GetIDObject( lhs->first, lhs->second );
				if (lhsPtr != NULL)
				{
					std::vector<CNCPoint> starting_points;
					starting_points = ((CDrilling *)lhsPtr)->FindAllLocations();

					// Copy the results in ONLY if each point doesn't already exist.
					for (std::vector<CNCPoint>::const_iterator l_itPoint = starting_points.begin(); l_itPoint != starting_points.end(); l_itPoint++)
					{
						if (std::find( locations.begin(), locations.end(), CNCPoint( *l_itPoint ) ) == locations.end())
						{
							locations.push_back( *l_itPoint );
						} // End if - then
					} // End for
				} // End if - then
			} // End if - then
		} // End if - then
        } // End for

	if (m_params.m_sort_locations)
	{
		// This locating cycle has the 'sort' option turned on.  Take the first point (because we don't know any better) and
		// sort the points in order of distance from each preceding point.  It may not be the most efficient arrangement
		// but it's better than random.  If we were really eager we would allow the starting point to be based on the
		// previous NC operation's ending point.
		//
		// If the sorting option is turned off then the points need to be returned in order of the m_symbols list.  One day,
		// we will allow the operator to re-order the m_symbols list by using a drag-n-drop operation on the sub-elements
		// in the menu.  When this is done, the operator's decision as to order should be respected.  Until then, we can
		// use the 'sort' option in the locating cycle's parameters.

		for (std::vector<CNCPoint>::iterator l_itPoint = locations.begin(); l_itPoint != locations.end(); l_itPoint++)
		{
			if (l_itPoint == locations.begin())
			{
				// It's the first point.  Reference this to zero so that the order makes some sense.  It would
				// be nice, eventually, to have this first reference point be the last point produced by the
				// previous NC operation.  i.e. where the last operation left off, we should start locating close
				// by.

				sort_points_by_distance compare( CNCPoint( 0.0, 0.0, 0.0 ) );
				std::sort( locations.begin(), locations.end(), compare );
			} // End if - then
			else
			{
				// We've already begun.  Just sort based on the previous point's location.
				std::vector<CNCPoint>::iterator l_itNextPoint = l_itPoint;
				l_itNextPoint++;

				if (l_itNextPoint != locations.end())
				{
					sort_points_by_distance compare( *l_itPoint );
					std::sort( l_itNextPoint, locations.end(), compare );
				} // End if - then
			} // End if - else
		} // End for
	} // End if - then

	return(locations);
} // End FindAllLocations() method

std::vector<CNCPoint> CLocating::FindAllLocations() const
{
	return( FindAllLocations( m_symbols ) );
} // End FindAllLocations() method

/**
	This method adjusts any parameters that don't make sense.  It should report a list
	of changes in the list of strings.
 */
std::list<wxString> CLocating::DesignRulesAdjustment(const bool apply_changes)
{
	std::list<wxString> changes;
	return(changes);

} // End DesignRulesAdjustment() method


/**
    This method returns TRUE if the type of symbol is suitable for reference as a source of location
 */
/* static */ bool CLocating::ValidType( const int object_type )
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

void CLocating::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
    COp::GetTools( t_list, p );
}


