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

#include <sstream>

extern CHeeksCADInterface* heeksCAD;


void CCounterBoreParams::set_initial_values( const int cutting_tool_number )
{
	CNCConfig config;
	
	config.Read(_T("m_diameter"), &m_diameter, (25.4 / 10));	// One tenth of an inch

	if ((cutting_tool_number > 0) && (CCuttingTool::FindCuttingTool( cutting_tool_number )))
	{
		HeeksObj *ob = heeksCAD->GetIDObject( CuttingToolType, CCuttingTool::FindCuttingTool( cutting_tool_number ) );
		if (ob != NULL)
		{
			std::pair< double, double > depth_and_diameter = CCounterBore::SelectSizeForHead( ((CCuttingTool *)ob)->m_params.m_diameter );
			m_diameter = depth_and_diameter.second;
		} // End if - then
	} // End if - then
}

void CCounterBoreParams::write_values_to_config()
{
	// We always want to store the parameters in mm and convert them back later on.

	CNCConfig config;
	config.Write(_T("m_diameter"), m_diameter);
}


static void on_set_diameter(double value, HeeksObj* object){((CCounterBore*)object)->m_params.m_diameter = value;}


void CCounterBoreParams::GetProperties(CCounterBore* parent, std::list<Property *> *list)
{
	list->push_back(new PropertyLength(_("diameter"), m_diameter, parent, on_set_diameter));
}

void CCounterBoreParams::WriteXMLAttributes(TiXmlNode *root)
{
	TiXmlElement * element;
	element = new TiXmlElement( "params" );
	root->LinkEndChild( element );  
	element->SetDoubleAttribute("diameter", m_diameter);
}

void CCounterBoreParams::ReadParametersFromXMLElement(TiXmlElement* pElem)
{
	if (pElem->Attribute("diameter")) m_diameter = atof(pElem->Attribute("diameter"));
}

/**
	This method is called when the CAD operator presses the Python button.  This method generates
	Python source code whose job will be to generate RS-274 GCode.  It's done in two steps so that
	the Python code can be configured to generate GCode suitable for various CNC interpreters.
 */
void CCounterBore::AppendTextToProgram()
{
	COp::AppendTextToProgram();

#ifdef UNICODE
	std::wostringstream ss;
#else
    std::ostringstream ss;
#endif
    ss.imbue(std::locale("C"));

	if (m_cutting_tool_number > 0)
	{
		HeeksObj *cuttingTool = heeksCAD->GetIDObject( CuttingToolType, m_cutting_tool_number );
		CCuttingTool *pCuttingTool = (CCuttingTool *)cuttingTool;
		if (pCuttingTool != NULL)
		{
			if (pCuttingTool->m_params.m_diameter >= m_params.m_diameter)
			{
				std::ostringstream l_ossMsg;
				l_ossMsg << "Error: Tool diameter (" << pCuttingTool->m_params.m_diameter << ") "
					 << ">= hole diameter (" << m_params.m_diameter << ") "
					 << "in counter bore operation.  "
					 << "Skipping this counter bore operation (ID=" << m_id << ")";
				wxMessageBox(Ctt(l_ossMsg.str().c_str()));
				return;
			} // End if - then

			std::set<Point3d> locations = FindAllLocations( m_symbols, NULL );
			for (std::set<Point3d>::const_iterator l_itLocation = locations.begin(); l_itLocation != locations.end(); l_itLocation++)
			{   
                
				ss << "flush_nc()\ncircular_pocket( "
							<< "x=" << l_itLocation->x << ", "
							<< "y=" << l_itLocation->y << ", "
       		                         		<< "ToolDiameter=" << pCuttingTool->m_params.m_diameter << ", "
       		                         		<< "HoleDiameter=" << m_params.m_diameter << ", "
       		                         		<< "ClearanceHeight=" << m_depth_op_params.m_clearance_height << ", "
       		                         		<< "StartHeight=" << l_itLocation->z + m_depth_op_params.m_start_depth << ", "
       		                         		<< "MaterialTop=" << l_itLocation->z << ", "
       		                         		<< "FeedRate=" << m_depth_op_params.m_vertical_feed_rate << ", "
       		                         		<< "HoleDepth=" << m_depth_op_params.m_final_depth << ")\n";
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
std::list< CCounterBore::Point3d > CCounterBore::PointsAround( const CCounterBore::Point3d & origin, const double radius, const unsigned int numPoints ) const
{
	std::list<CCounterBore::Point3d> results;

	double alpha = 3.1415926 * 2 / numPoints;

	unsigned int i = 0;
	while( i++ < numPoints )
	{
		double theta = alpha * i;
		CCounterBore::Point3d pointOnCircle( cos( theta ) * radius, sin( theta ) * radius, 0 );

		pointOnCircle.x += origin.x;
		pointOnCircle.y += origin.y;
		pointOnCircle.z += origin.z;

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

		std::set<Point3d> locations = FindAllLocations( m_symbols, NULL );
		for (std::set<Point3d>::const_iterator l_itLocation = locations.begin(); l_itLocation != locations.end(); l_itLocation++)
		{
			std::list< CCounterBore::Point3d > circle = PointsAround( *l_itLocation, m_params.m_diameter / 2, 10 );

			glBegin(GL_LINE_STRIP);
			// Once around the top circle.
			for (std::list<CCounterBore::Point3d>::const_iterator l_itPoint = circle.begin(); l_itPoint != circle.end(); l_itPoint++)
			{
				GLdouble start[3], end[3];

				start[0] = l_itPoint->x;
				start[1] = l_itPoint->y;
				start[2] = l_itPoint->z;

				l_itPoint++;

				if (l_itPoint != circle.end())
				{
					end[0] = l_itPoint->x;
					end[1] = l_itPoint->y;
					end[2] = l_itPoint->z;
				
					glVertex3dv( start );
					glVertex3dv( end );
				} // End if - then
			} // End for

			// Once around the bottom circle.
			for (std::list<CCounterBore::Point3d>::const_iterator l_itPoint = circle.begin(); l_itPoint != circle.end(); l_itPoint++)
			{
				GLdouble start[3], end[3];

				start[0] = l_itPoint->x;
				start[1] = l_itPoint->y;
				start[2] = l_itPoint->z - m_depth_op_params.m_final_depth;

				l_itPoint++;

				if (l_itPoint != circle.end())
				{
					end[0] = l_itPoint->x;
					end[1] = l_itPoint->y;
					end[2] = l_itPoint->z - m_depth_op_params.m_final_depth;
				
					glVertex3dv( start );
					glVertex3dv( end );
				} // End if - then
			} // End for

			// And once to join the two circles together.
			for (std::list<CCounterBore::Point3d>::const_iterator l_itPoint = circle.begin(); l_itPoint != circle.end(); l_itPoint++)
			{
				GLdouble start[3], end[3];

				start[0] = l_itPoint->x;
				start[1] = l_itPoint->y;
				start[2] = l_itPoint->z;

				end[0] = l_itPoint->x;
				end[1] = l_itPoint->y;
				end[2] = l_itPoint->z - m_depth_op_params.m_final_depth;
			
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
 * 	add the intersection points to the result set.  We use std::set<Point3d> so that
 * 	we end up with no duplicate points.
 *
 *	If any of the selected objects are DrillingType objects then see if they refer
 *	to CuttingTool objects.  If so, remember which ones.  We may want to see what
 *	size holes were drilled so that we can make an intellegent selection for the
 *	socket head.
 */
std::set<CCounterBore::Point3d> CCounterBore::FindAllLocations( const CCounterBore::Symbols_t & symbols, std::list<int> *pToolNumbersReferenced )
{
	std::set<CCounterBore::Point3d> locations;

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
				locations.insert( CCounterBore::Point3d( pos[0], pos[1], pos[2] ) );
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

				// std::set<CDrilling::Point3d> holes = ((CDrilling *)lhsPtr)->FindAllLocations( ((CDrilling *)lhsPtr)->m_symbols );
				std::set<CDrilling::Point3d> holes = ((CDrilling *)lhsPtr)->FindAllLocations();
				for (std::set<CDrilling::Point3d>::const_iterator l_itHole = holes.begin(); l_itHole != holes.end(); l_itHole++)
				{
					locations.insert( CCounterBore::Point3d( l_itHole->x, l_itHole->y, l_itHole->z ) );
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
                                        CCounterBore::Point3d intersection;

                                        intersection.x = *(results.begin());
                                        results.erase(results.begin());

                                        intersection.y = *(results.begin());
                                        results.erase(results.begin());

                                        intersection.z = *(results.begin());
                                        results.erase(results.begin());

                                        locations.insert(intersection);
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
						locations.insert( CCounterBore::Point3d( pos[0], pos[1], pos[2] ) );
					} // End if - then
				} // End if - then
			} // End if - then
		} // End if - then
        } // End for

	return(locations);
} // End FindAllLocations() method


// Return depth and diameter in that order.
std::pair< double, double > CCounterBore::SelectSizeForHead( const double drill_hole_diameter )
{
	// Just bluf it for now.  We will implement a lookup table eventually.

	return( std::make_pair( drill_hole_diameter * 1, drill_hole_diameter * 1.7 )  );

} // End SelectSizeForHead() method



