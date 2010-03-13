// CounterBore.cpp
/*
 * Copyright (c) 2009, Dan Heeks, Perttu Ahola
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "stdafx.h"
#include "CounterBore.h"
#include "CNCConfig.h"
#include "ProgramCanvas.h"
#include "interface/HeeksObj.h"
#include "interface/PropertyInt.h"
#include "interface/PropertyDouble.h"
#include "interface/PropertyLength.h"
#include "interface/PropertyChoice.h"
#include "tinyxml/tinyxml.h"
#include "CuttingTool.h"
#include "Drilling.h"
#include "CNCPoint.h"

#include <sstream>
#include <algorithm>
#include <vector>

extern CHeeksCADInterface* heeksCAD;


void CCounterBoreParams::set_initial_values( const int cutting_tool_number )
{
	CNCConfig config(ConfigScope());

	config.Read(_T("m_diameter"), &m_diameter, (25.4 / 10));	// One tenth of an inch
	config.Read(_T("m_sort_locations"), &m_sort_locations, 1);

	if (cutting_tool_number > 0)
	{
		CCuttingTool *pCuttingTool = CCuttingTool::Find( cutting_tool_number );
		if (pCuttingTool != NULL)
		{
			std::pair< double, double > depth_and_diameter = CCounterBore::SelectSizeForHead( pCuttingTool->m_params.m_diameter );
			m_diameter = depth_and_diameter.second;
		} // End if - then
	} // End if - then
}

void CCounterBoreParams::write_values_to_config()
{
	// We always want to store the parameters in mm and convert them back later on.

	CNCConfig config(ConfigScope());
	config.Write(_T("m_diameter"), m_diameter);
	config.Write(_T("m_sort_locations"), m_sort_locations);
}


static void on_set_diameter(double value, HeeksObj* object){((CCounterBore*)object)->m_params.m_diameter = value;}
static void on_set_sort_locations(int value, HeeksObj* object){((CCounterBore*)object)->m_params.m_sort_locations = value;}


void CCounterBoreParams::GetProperties(CCounterBore* parent, std::list<Property *> *list)
{
	list->push_back(new PropertyLength(_("diameter"), m_diameter, parent, on_set_diameter));

	{ // Begin choice scope
		std::list< wxString > choices;

		choices.push_back(_("Respect existing order"));	// Must be 'false' (0)
		choices.push_back(_("True"));			// Must be 'true' (non-zero)

		int choice = int(m_sort_locations);
		list->push_back(new PropertyChoice(_("sort_locations"), choices, choice, parent, on_set_sort_locations));
	} // End choice scope

}

void CCounterBoreParams::WriteXMLAttributes(TiXmlNode *root)
{
	TiXmlElement * element;
	element = new TiXmlElement( "params" );
	root->LinkEndChild( element );
	element->SetDoubleAttribute("diameter", m_diameter);

	std::ostringstream l_ossValue;
	l_ossValue.str(""); l_ossValue << m_sort_locations;
	element->SetAttribute("sort_locations", l_ossValue.str().c_str());
}

void CCounterBoreParams::ReadParametersFromXMLElement(TiXmlElement* pElem)
{
	if (pElem->Attribute("diameter")) m_diameter = atof(pElem->Attribute("diameter"));
	if (pElem->Attribute("sort_locations")) m_sort_locations = atoi(pElem->Attribute("sort_locations"));
}


static double drawing_units( const double value )
{
	return(value / theApp.m_program->m_units);
}

/**
	Before we call this routine, we're sure that the tool's diameter is <= counterbore's diameter.  To
	this end, we need to spiral down to each successive cutting depth and then spiral out to the
	outside diameter.  Repeat these operations until we've cut the full depth and the full width.
 */
void CCounterBore::GenerateGCodeForOneLocation( const CNCPoint & location, const CCuttingTool *pCuttingTool ) const
{

#ifdef UNICODE
	std::wostringstream ss;
#else
    std::ostringstream ss;
#endif
    ss.imbue(std::locale("C"));

	double cutting_depth = m_depth_op_params.m_start_depth;
	double final_depth   = m_depth_op_params.m_final_depth;

	CNCPoint point( location );
	CNCPoint centre( point );

    ss << "comment('Begin Counterbore operation for ID " << m_id << "')\n";

	// Rapid to above the starting point (up at clearance height)
	point.SetZ( m_depth_op_params.m_clearance_height );
	ss << "rapid( x=" << point.X(true) << ", y=" << point.Y(true) << ", z=" << point.Z(true) << ")\n";

	// Feed (slowly) to the starting point at the centre of the material
	point.SetZ(m_depth_op_params.m_start_depth);
	ss << "feed( x=" << point.X(true) << ", y=" << point.Y(true) << ", z=" << point.Z(true) << ")\n";

	double tolerance = heeksCAD->GetTolerance();
	double max_radius_of_spiral = (m_params.m_diameter / 2.0) - pCuttingTool->CuttingRadius(false);

	while ((cutting_depth - final_depth) > tolerance)
	{

		double step_down = m_depth_op_params.m_step_down;
		if ((cutting_depth - step_down) < final_depth)
		{
			step_down = cutting_depth - final_depth;	// last pass
		}

		// We want to spiral down to the next cutting depth over a half circle distance.
		// The width of the spiral will depend on the diameter of the tool and the diameter of
		// the hole.  We don't want it to be more than the tool's radius but we also don't
		// want it to be wider than the hole.

		double radius_of_spiral = pCuttingTool->CuttingRadius(false) * 0.75;
		if (radius_of_spiral > max_radius_of_spiral)
		{
			// Reduce the radius of the spiral so that we don't run outside the hole.

			radius_of_spiral = max_radius_of_spiral;
		} // End if - then

        ss << "comment('Spiral down half a circle until we get to the cutting depth of " << (cutting_depth - step_down) << "')\n";

		// Now spiral down using the width_of_spiral to the cutting_depth.
		// Move to 12 O'Clock.
		ss << "feed( x=" << centre.X(true) << ", "
					"y=" << drawing_units(centre.Y() + radius_of_spiral) << ", "
					"z=" << drawing_units(cutting_depth) << ")\n";
		point.SetX( centre.X(false) );
		point.SetY( centre.Y(false) + radius_of_spiral );

		// First quadrant (12 O'Clock to 9 O'Clock)
		ss << "arc_ccw( x=" << drawing_units(centre.X(false) - radius_of_spiral) << ", " <<
					"y=" << centre.Y(true) << ", " <<
					"z=" << drawing_units(cutting_depth - (0.5 * step_down)) << ", " <<
					"i=" << drawing_units(centre.X(false) - point.X()) << ", " <<
					"j=" << drawing_units(centre.Y(false) - point.Y()) << ")\n";
		point.SetX( centre.X(false) - radius_of_spiral );
		point.SetY( centre.Y(false) );

		// Second quadrant (9 O'Clock to 6 O'Clock)
		ss << "arc_ccw( x=" << centre.X(true) << ", " <<
					"y=" << drawing_units(centre.Y(false) - radius_of_spiral) << ", " <<
					"z=" << drawing_units(cutting_depth - (1.0 * step_down)) << ", " <<	// full depth now
					"i=" << drawing_units(centre.X(false) - point.X(false)) << ", " <<
					"j=" << drawing_units(centre.Y(false) - point.Y(false)) << ")\n";
		point.SetX( centre.X(false) );
		point.SetY( centre.Y(false) - radius_of_spiral );

		// Third quadrant (6 O'Clock to 3 O'Clock)
		ss << "arc_ccw( x=" << drawing_units(centre.X(false) + radius_of_spiral) << ", " <<
					"y=" << centre.Y(true) << ", " <<
					"z=" << drawing_units(cutting_depth - (1.0 * step_down)) << ", " <<	// full depth now
					"i=" << drawing_units(centre.X(false) - point.X(false)) << ", " <<
					"j=" << drawing_units(centre.Y(false) - point.Y(false)) << ")\n";
		point.SetX( centre.X(false) + radius_of_spiral );
		point.SetY( centre.Y(false) );

		// Fourth quadrant (3 O'Clock to 12 O'Clock)
		ss << "arc_ccw( x=" << centre.X(true) << ", " <<
					"y=" << drawing_units(centre.Y(false) + radius_of_spiral) << ", " <<
					"z=" << drawing_units(cutting_depth - (1.0 * step_down)) << ", " <<	// full depth now
					"i=" << drawing_units(centre.X(false) - point.X(false)) << ", " <<
					"j=" << drawing_units(centre.Y(false) - point.Y(false)) << ")\n";
		point.SetX( centre.X(false) );
		point.SetY( centre.Y(false) + radius_of_spiral );

		do {
			radius_of_spiral += (pCuttingTool->CuttingRadius(false) * 0.75);
			if (radius_of_spiral > max_radius_of_spiral)
			{
				// Reduce the radius of the spiral so that we don't run outside the hole.
				radius_of_spiral = max_radius_of_spiral;
			} // End if - then

			ss << "comment('Now spiral outwards to the counterbore perimeter of " << ((radius_of_spiral + pCuttingTool->CuttingRadius(true)) * 2.0) << "')\n";

			// Move to 12 O'Clock.
			ss << "feed( x=" << centre.X(true) << ", "
						"y=" << drawing_units(centre.Y(false) + radius_of_spiral) << ", "
						"z=" << drawing_units(cutting_depth - (1.0 * step_down)) << ")\n";
			point.SetX( centre.X(false) );
			point.SetY( centre.Y(false) + radius_of_spiral );

			// First quadrant (12 O'Clock to 9 O'Clock)
			ss << "arc_ccw( x=" << drawing_units(centre.X(false) - radius_of_spiral) << ", " <<
						"y=" << centre.Y(true) << ", " <<
						"z=" << drawing_units(cutting_depth - (1.0 * step_down)) << ", " <<	// full depth
						"i=" << drawing_units(centre.X(false) - point.X(false)) << ", " <<
						"j=" << drawing_units(centre.Y(false) - point.Y(false)) << ")\n";
			point.SetX( centre.X(false) - radius_of_spiral );
			point.SetY( centre.Y(false) );

			// Second quadrant (9 O'Clock to 6 O'Clock)
			ss << "arc_ccw( x=" << centre.X(true) << ", " <<
						"y=" << drawing_units(centre.Y(false) - radius_of_spiral) << ", " <<
						"z=" << drawing_units(cutting_depth - (1.0 * step_down)) << ", " <<	// full depth now
						"i=" << drawing_units(centre.X(false) - point.X(false)) << ", " <<
						"j=" << drawing_units(centre.Y(false) - point.Y(false)) << ")\n";
			point.SetX( centre.X(false) );
			point.SetY( centre.Y(false) - radius_of_spiral );

			// Third quadrant (6 O'Clock to 3 O'Clock)
			ss << "arc_ccw( x=" << drawing_units(centre.X(false) + radius_of_spiral) << ", " <<
						"y=" << centre.Y(true) << ", " <<
						"z=" << drawing_units(cutting_depth - (1.0 * step_down)) << ", " <<	// full depth now
						"i=" << drawing_units(centre.X(false) - point.X(false)) << ", " <<
						"j=" << drawing_units(centre.Y(false) - point.Y(false)) << ")\n";
			point.SetX( centre.X(false) + radius_of_spiral );
			point.SetY( centre.Y(false) );

			// Fourth quadrant (3 O'Clock to 12 O'Clock)
			ss << "arc_ccw( x=" << centre.X(true) << ", " <<
						"y=" << drawing_units(centre.Y(false) + radius_of_spiral) << ", " <<
						"z=" << drawing_units(cutting_depth - (1.0 * step_down)) << ", " <<	// full depth now
						"i=" << drawing_units(centre.X(false) - point.X(false)) << ", " <<
						"j=" << drawing_units(centre.Y(false) - point.Y(false)) << ")\n";
			point.SetX( centre.X(false) );
			point.SetY( centre.Y(false) + radius_of_spiral );
		} while ((max_radius_of_spiral - radius_of_spiral) > tolerance);

		if (((cutting_depth - final_depth) < step_down) && ((cutting_depth - final_depth) > tolerance))
		{
			// Last pass at this depth.
			cutting_depth = final_depth;
		} // End if - then
		else
		{
			cutting_depth -= step_down;
		} // End if - else
	} // End while

	// Rapid back to the centre of the hole (at the bottom) so we don't hit the
	// sides on the way up.
	ss << "rapid( x=" << centre.X(true) << ", y=" << centre.Y(true) << ", z=" << drawing_units(final_depth) << ")\n";

	// Rapid to above the starting point (up at clearance height)
	point.SetZ( m_depth_op_params.m_clearance_height );
	ss << "rapid( x=" << centre.X(true) << ", y=" << centre.Y(true) << ", z=" << point.Z(true) << ")\n";

    ss << "comment('End Counterbore operation for ID " << m_id << "')\n";

	theApp.m_program_canvas->m_textCtrl->AppendText(ss.str().c_str());

} // End GenerateGCodeForOneLocation() method



/**
	This method is called when the CAD operator presses the Python button.  This method generates
	Python source code whose job will be to generate RS-274 GCode.  It's done in two steps so that
	the Python code can be configured to generate GCode suitable for various CNC interpreters.
 */
void CCounterBore::AppendTextToProgram(const CFixture *pFixture)
{
	CDepthOp::AppendTextToProgram(pFixture);

#ifdef UNICODE
	std::wostringstream ss;
#else
    std::ostringstream ss;
#endif
    ss.imbue(std::locale("C"));

	if (m_cutting_tool_number > 0)
	{
		CCuttingTool *pCuttingTool = CCuttingTool::Find( m_cutting_tool_number );
		if (pCuttingTool != NULL)
		{
			if ((pCuttingTool->CuttingRadius() * 2.0) >= m_params.m_diameter)
			{
				std::ostringstream l_ossMsg;
				l_ossMsg << "Error: Tool diameter (" << pCuttingTool->m_params.m_diameter << ") "
					 << ">= hole diameter (" << m_params.m_diameter << ") "
					 << "in counter bore operation.  "
					 << "Skipping this counter bore operation (ID=" << m_id << ")";
				wxMessageBox(Ctt(l_ossMsg.str().c_str()));
				return;
			} // End if - then

			std::vector<CNCPoint> locations = FindAllLocations( m_symbols, NULL );
			for (std::vector<CNCPoint>::const_iterator l_itLocation = locations.begin(); l_itLocation != locations.end(); l_itLocation++)
			{
				CNCPoint point( pFixture->Adjustment(*l_itLocation) );
				GenerateGCodeForOneLocation( point, pCuttingTool );
			} // End for

			theApp.m_program_canvas->m_textCtrl->AppendText(ss.str().c_str());
		} // End if - then
		else
		{
			std::ostringstream l_ossMsg;
			l_ossMsg << "Warning: Counter bore refers to a cutting tool "
				 << "that can't be found in the model.  "
				 << "Skipping this counter bore operation (ID=" << m_id << ")";
			wxMessageBox(Ctt(l_ossMsg.str().c_str()));
		} // End if - else
	} // End if - then
	else
	{
		std::ostringstream l_ossMsg;
		l_ossMsg << "Warning: Counter bore operations MUST refer to a cutting tool.  "
			 << "Skipping this counter bore operation (ID=" << m_id << ")";
		wxMessageBox(Ctt(l_ossMsg.str().c_str()));
	} // End if - else

}


/**
	This routine generates a list of coordinates around the circumference of a circle.  It's just used
	to generate data suitable for OpenGL calls to paint a circle.  This graphics is transient but will
	help represent what the GCode will be doing when it's generated.
 */
std::list< CNCPoint > CCounterBore::PointsAround( const CNCPoint & origin, const double radius, const unsigned int numPoints ) const
{
	std::list<CNCPoint> results;

	double alpha = 3.1415926 * 2 / numPoints;

	unsigned int i = 0;
	while( i++ < numPoints )
	{
		double theta = alpha * i;
		CNCPoint pointOnCircle( cos( theta ) * radius, sin( theta ) * radius, 0 );

		pointOnCircle += origin;

		results.push_back(pointOnCircle);
	} // End while

	return(results);

} // End PointsAround() routine


/**
	This is the Graphics Library Commands (from the OpenGL set).  This method calls the OpenGL
	routines to paint the drill action in the graphics window.  The graphics is transient.

	Part of its job is to re-paint the elements that this CCounterBore object refers to so that
	we know what CAD objects this CNC operation is referring to.
 */
void CCounterBore::glCommands(bool select, bool marked, bool no_color)
{
	if(marked && !no_color)
	{
		// For all graphical symbols that this module refers to, highlight them as well.
		for (Symbols_t::const_iterator l_itSymbol = m_symbols.begin(); l_itSymbol != m_symbols.end(); l_itSymbol++)
		{
			HeeksObj* object = heeksCAD->GetIDObject(l_itSymbol->first, l_itSymbol->second);

			// If we found something, ask its CAD code to draw itself highlighted.
			if(object)object->glCommands(false, true, false);
		} // End for

		// For all coordinates that relate to these reference objects, draw the graphics that represents
		// both a drilling hole and a counterbore.

		std::vector<CNCPoint> locations = FindAllLocations( m_symbols, NULL );
		for (std::vector<CNCPoint>::const_iterator l_itLocation = locations.begin(); l_itLocation != locations.end(); l_itLocation++)
		{
			std::list< CNCPoint > circle = PointsAround( *l_itLocation, m_params.m_diameter / 2, 10 );

			glBegin(GL_LINE_STRIP);
			// Once around the top circle.
			for (std::list<CNCPoint>::const_iterator l_itPoint = circle.begin(); l_itPoint != circle.end(); l_itPoint++)
			{
				GLdouble start[3], end[3];

				start[0] = l_itPoint->X();
				start[1] = l_itPoint->Y();
				start[2] = m_depth_op_params.m_start_depth;

				l_itPoint++;

				if (l_itPoint != circle.end())
				{
					end[0] = l_itPoint->X();
					end[1] = l_itPoint->Y();
					end[2] = m_depth_op_params.m_start_depth;

					glVertex3dv( start );
					glVertex3dv( end );
				} // End if - then
			} // End for

			// Once around the bottom circle.
			for (std::list<CNCPoint>::const_iterator l_itPoint = circle.begin(); l_itPoint != circle.end(); l_itPoint++)
			{
				GLdouble start[3], end[3];

				start[0] = l_itPoint->X();
				start[1] = l_itPoint->Y();
				start[2] = m_depth_op_params.m_final_depth;

				l_itPoint++;

				if (l_itPoint != circle.end())
				{
					end[0] = l_itPoint->X();
					end[1] = l_itPoint->Y();
					end[2] = m_depth_op_params.m_final_depth;

					glVertex3dv( start );
					glVertex3dv( end );
				} // End if - then
			} // End for

			// And once to join the two circles together.
			for (std::list<CNCPoint>::const_iterator l_itPoint = circle.begin(); l_itPoint != circle.end(); l_itPoint++)
			{
				GLdouble start[3], end[3];

				start[0] = l_itPoint->X();
				start[1] = l_itPoint->Y();
				start[2] = m_depth_op_params.m_start_depth;

				end[0] = l_itPoint->X();
				end[1] = l_itPoint->Y();
				end[2] = m_depth_op_params.m_final_depth;

				glVertex3dv( start );
				glVertex3dv( end );
				glVertex3dv( start );
			} // End for
			glEnd();
		} // End for
	} // End if - then
}


void CCounterBore::GetProperties(std::list<Property *> *list)
{
	m_params.GetProperties(this, list);
	CDepthOp::GetProperties(list);
}

HeeksObj *CCounterBore::MakeACopy(void)const
{
	return new CCounterBore(*this);
}

void CCounterBore::CopyFrom(const HeeksObj* object)
{
	operator=(*((CCounterBore*)object));
}

bool CCounterBore::CanAddTo(HeeksObj* owner)
{
	return owner->GetType() == OperationsType;
}

void CCounterBore::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element = new TiXmlElement( "CounterBore" );
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
HeeksObj* CCounterBore::ReadFromXMLElement(TiXmlElement* element)
{
	CCounterBore* new_object = new CCounterBore;

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
 * 	add the intersection points to the result set.
 *
 *	If any of the selected objects are DrillingType objects then see if they refer
 *	to CuttingTool objects.  If so, remember which ones.  We may want to see what
 *	size holes were drilled so that we can make an intellegent selection for the
 *	socket head.
 */
std::vector<CNCPoint> CCounterBore::FindAllLocations( const CCounterBore::Symbols_t & symbols, std::list<int> *pToolNumbersReferenced ) const
{
	std::vector<CNCPoint> locations;

	// Look to find all intersections between all selected objects.  At all these locations, create
        // a drilling cycle.

        for (CCounterBore::Symbols_t::const_iterator lhs = symbols.begin(); lhs != symbols.end(); lhs++)
        {
		bool l_bIntersectionsFound = false;	// If it's a circle and it doesn't
							// intersect anything else, we want to know
							// about it.

		if (lhs->first == PointType)
		{
			HeeksObj *obj = heeksCAD->GetIDObject( lhs->first, lhs->second );
			if (obj != NULL)
			{
				double pos[3];
				obj->GetStartPoint(pos);
				if (std::find( locations.begin(), locations.end(), CNCPoint( pos[0], pos[1], pos[2] ) ) == locations.end())
				{
					locations.push_back( CNCPoint( pos[0], pos[1], pos[2] ) );
				} // End if - then
				continue;	// No need to intersect a point with anything.
			} // End if - then
		} // End if - then

		if (lhs->first == DrillingType)
		{
			// Ask the Drilling object what reference points it uses.


			HeeksObj *lhsPtr = heeksCAD->GetIDObject( lhs->first, lhs->second );
			// Ask the Drilling object what reference points it uses.
			if (lhsPtr != NULL)
			{
				if ((((COp *) lhsPtr)->m_cutting_tool_number > 0) && (pToolNumbersReferenced != NULL))
				{
					pToolNumbersReferenced->push_back( ((COp *) lhsPtr)->m_cutting_tool_number );
				} // End if - then

				std::vector<CNCPoint> holes = ((CDrilling *)lhsPtr)->FindAllLocations();
				for (std::vector<CNCPoint>::const_iterator l_itHole = holes.begin(); l_itHole != holes.end(); l_itHole++)
				{
					if (std::find( locations.begin(), locations.end(), *l_itHole ) == locations.end())
					{
						locations.push_back( *l_itHole );
					} // End if - then
				} // End for
			} // End if - then
		} // End if - then

                for (CCounterBore::Symbols_t::const_iterator rhs = symbols.begin(); rhs != symbols.end(); rhs++)
                {
                        if (lhs == rhs) continue;
			if (lhs->first == PointType) continue;	// No need to intersect a point type.

			// Avoid repeated calls to the intersection code where possible.
                        std::list<double> results;
                        HeeksObj *lhsPtr = heeksCAD->GetIDObject( lhs->first, lhs->second );
                        HeeksObj *rhsPtr = heeksCAD->GetIDObject( rhs->first, rhs->second );

			if (lhsPtr == NULL) continue;
			if (rhsPtr == NULL) continue;

                        if (lhsPtr->Intersects( rhsPtr, &results ))
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
				if (lhsPtr != NULL)
				{
					double pos[3];
					if (heeksCAD->GetArcCentre( lhsPtr, pos ))
					{
						if (std::find( locations.begin(), locations.end(), CNCPoint( pos ) ) == locations.end())
						{
							locations.push_back( CNCPoint( pos ) );
						} // End if - then
					} // End if - then
				} // End if - then
			} // End if - then
		} // End if - then
        } // End for

	if (m_params.m_sort_locations)
	{
		// This counter bore object has the 'sort' option turned on.  Take the first point (because we don't know any better) and
		// sort the points in order of distance from each preceding point.  It may not be the most efficient arrangement
		// but it's better than random.  If we were really eager we would allow the starting point to be based on the
		// previous NC operation's ending point.
		//
		for (std::vector<CNCPoint>::iterator l_itPoint = locations.begin(); l_itPoint != locations.end(); l_itPoint++)
		{
			if (l_itPoint == locations.begin())
			{
				// It's the first point.  Reference this to zero so that the order makes some sense.  It would
				// be nice, eventually, to have this first reference point be the last point produced by the
				// previous NC operation.  i.e. where the last operation left off, we should start drilling close
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


// Return depth and diameter in that order.
std::pair< double, double > CCounterBore::SelectSizeForHead( const double drill_hole_diameter )
{
	// Just bluf it for now.  We will implement a lookup table eventually.

	return( std::make_pair( drill_hole_diameter * 1, drill_hole_diameter * 1.7 )  );

} // End SelectSizeForHead() method


/**
    This method returns TRUE if the type of symbol is suitable for reference as a source of location
 */
/* static */ bool CCounterBore::ValidType( const int object_type )
{
    switch (object_type)
    {
        case PointType:
        case CircleType:
        case SketchType:
        case DrillingType:
        case LineType:
            return(true);

        default:
            return(false);
    }
}

void CCounterBore::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
    CDepthOp::GetTools( t_list, p );
}



