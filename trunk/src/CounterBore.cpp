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
#include "CTool.h"
#include "Drilling.h"
#include "CNCPoint.h"
#include "PythonStuff.h"
#include "MachineState.h"
#include "Program.h"

#include <sstream>
#include <algorithm>
#include <vector>

extern CHeeksCADInterface* heeksCAD;


void CCounterBoreParams::set_initial_values( const int tool_number )
{
	CNCConfig config(ConfigScope());

	config.Read(_T("m_diameter"), &m_diameter, (25.4 / 10));	// One tenth of an inch
	config.Read(_T("m_sort_locations"), &m_sort_locations, 1);

	if (tool_number > 0)
	{
		CTool *pTool = CTool::Find( tool_number );
		if (pTool != NULL)
		{
			std::pair< double, double > depth_and_diameter = CCounterBore::SelectSizeForHead( pTool->m_params.m_diameter );
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
	element->SetAttribute("sort_locations", m_sort_locations);
}

void CCounterBoreParams::ReadParametersFromXMLElement(TiXmlElement* pElem)
{
	if (pElem->Attribute("diameter")) pElem->Attribute("diameter", &m_diameter);
	if (pElem->Attribute("sort_locations")) pElem->Attribute("sort_locations", &m_sort_locations);
}


static double drawing_units( const double value )
{
	return(value / theApp.m_program->m_units);
}

const wxBitmap &CCounterBore::GetIcon()
{
	if(!m_active)return GetInactiveIcon();
	static wxBitmap* icon = NULL;
	if(icon == NULL)icon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/drilling.png")));
	return *icon;
}

/**
	Before we call this routine, we're sure that the tool's diameter is <= counterbore's diameter.  To
	this end, we need to spiral down to each successive cutting depth and then spiral out to the
	outside diameter.  Repeat these operations until we've cut the full depth and the full width.
 */
Python CCounterBore::GenerateGCodeForOneLocation( const CNCPoint & location, const CTool *pTool ) const
{
	Python python;

	double cutting_depth = m_depth_op_params.m_start_depth;
	double final_depth   = m_depth_op_params.m_final_depth;

	CNCPoint point( location );
	CNCPoint centre( point );

	// Rapid to above the starting point (up at clearance height)
	point.SetZ( m_depth_op_params.m_clearance_height );
	python << _T("rapid( x=") << point.X(true) << _T(", y=") << point.Y(true) << _T(", z=") << point.Z(true) << _T(")\n");

	// Feed (slowly) to the starting point at the centre of the material
	point.SetZ(m_depth_op_params.m_start_depth);
	python << _T("feed( x=") << point.X(true) << _T(", y=") << point.Y(true) << _T(", z=") << point.Z(true) << _T(")\n");

	double tolerance = heeksCAD->GetTolerance();
	double max_radius_of_spiral = (m_params.m_diameter / 2.0) - pTool->CuttingRadius(false);

	while ((cutting_depth - final_depth) > tolerance)
	{
		python << _T("comment('Spiral down half a circle until we get to the cutting depth')\n");

		double step_down = m_depth_op_params.m_step_down;
		if ((cutting_depth - step_down) < final_depth)
		{
			step_down = cutting_depth - final_depth;	// last pass
		}

		// We want to spiral down to the next cutting depth over a half circle distance.
		// The width of the spiral will depend on the diameter of the tool and the diameter of
		// the hole.  We don't want it to be more than the tool's radius but we also don't
		// want it to be wider than the hole.

		double radius_of_spiral = pTool->CuttingRadius(false) * 0.75;
		if (radius_of_spiral > max_radius_of_spiral)
		{
			// Reduce the radius of the spiral so that we don't run outside the hole.

			radius_of_spiral = max_radius_of_spiral;
		} // End if - then

		// Now spiral down using the width_of_spiral to the cutting_depth.
		// Move to 12 O'Clock.
		python << _T("feed( x=") << centre.X(true) << _T(", ")
					_T("y=") << drawing_units(centre.Y() + radius_of_spiral) << _T(", ")
					_T("z=") << drawing_units(cutting_depth) << _T(")\n");
		point.SetX( centre.X(false) );
		point.SetY( centre.Y(false) + radius_of_spiral );

		// First quadrant (12 O'Clock to 9 O'Clock)
		python << _T("arc_ccw( x=") << drawing_units(centre.X(false) - radius_of_spiral) << _T(", ") <<
					_T("y=") << centre.Y(true) << _T(", ") <<
					_T("z=") << drawing_units(cutting_depth - (0.5 * step_down)) << _T(", ") <<
					_T("i=") << drawing_units(centre.X(false) - point.X()) << _T(", ") <<
					_T("j=") << drawing_units(centre.Y(false) - point.Y()) << _T(")\n");
		point.SetX( centre.X(false) - radius_of_spiral );
		point.SetY( centre.Y(false) );

		// Second quadrant (9 O'Clock to 6 O'Clock)
		python << _T("arc_ccw( x=") << centre.X(true) << _T(", ") <<
					_T("y=") << drawing_units(centre.Y(false) - radius_of_spiral) << _T(", ") <<
					_T("z=") << drawing_units(cutting_depth - (1.0 * step_down)) << _T(", ") <<	// full depth now
					_T("i=") << drawing_units(centre.X(false) - point.X(false)) << _T(", ") <<
					_T("j=") << drawing_units(centre.Y(false) - point.Y(false)) << _T(")\n");
		point.SetX( centre.X(false) );
		point.SetY( centre.Y(false) - radius_of_spiral );

		// Third quadrant (6 O'Clock to 3 O'Clock)
		python << _T("arc_ccw( x=") << drawing_units(centre.X(false) + radius_of_spiral) << _T(", ") <<
					_T("y=") << centre.Y(true) << _T(", ") <<
					_T("z=") << drawing_units(cutting_depth - (1.0 * step_down)) << _T(", ") <<	// full depth now
					_T("i=") << drawing_units(centre.X(false) - point.X(false)) << _T(", ") <<
					_T("j=") << drawing_units(centre.Y(false) - point.Y(false)) << _T(")\n");
		point.SetX( centre.X(false) + radius_of_spiral );
		point.SetY( centre.Y(false) );

		// Fourth quadrant (3 O'Clock to 12 O'Clock)
		python << _T("arc_ccw( x=") << centre.X(true) << _T(", ") <<
					_T("y=") << drawing_units(centre.Y(false) + radius_of_spiral) << _T(", ") <<
					_T("z=") << drawing_units(cutting_depth - (1.0 * step_down)) << _T(", ") <<	// full depth now
					_T("i=") << drawing_units(centre.X(false) - point.X(false)) << _T(", ") <<
					_T("j=") << drawing_units(centre.Y(false) - point.Y(false)) << _T(")\n");
		point.SetX( centre.X(false) );
		point.SetY( centre.Y(false) + radius_of_spiral );

		python << _T("comment('Now spiral outwards to the counterbore perimeter')\n");

		do {
			radius_of_spiral += (pTool->CuttingRadius(false) * 0.75);
			if (radius_of_spiral > max_radius_of_spiral)
			{
				// Reduce the radius of the spiral so that we don't run outside the hole.
				radius_of_spiral = max_radius_of_spiral;
			} // End if - then

			// Move to 12 O'Clock.
			python << _T("feed( x=") << centre.X(true) << _T(", ")
						_T("y=") << drawing_units(centre.Y(false) + radius_of_spiral) << _T(", ")
						_T("z=") << drawing_units(cutting_depth - (1.0 * step_down)) << _T(")\n");
			point.SetX( centre.X(false) );
			point.SetY( centre.Y(false) + radius_of_spiral );

			// First quadrant (12 O'Clock to 9 O'Clock)
			python << _T("arc_ccw( x=") << drawing_units(centre.X(false) - radius_of_spiral) << _T(", ") <<
						_T("y=") << centre.Y(true) << _T(", ") <<
						_T("z=") << drawing_units(cutting_depth - (1.0 * step_down)) << _T(", ") <<	// full depth
						_T("i=") << drawing_units(centre.X(false) - point.X(false)) << _T(", ") <<
						_T("j=") << drawing_units(centre.Y(false) - point.Y(false)) << _T(")\n");
			point.SetX( centre.X(false) - radius_of_spiral );
			point.SetY( centre.Y(false) );

			// Second quadrant (9 O'Clock to 6 O'Clock)
			python << _T("arc_ccw( x=") << centre.X(true) << _T(", ") <<
						_T("y=") << drawing_units(centre.Y(false) - radius_of_spiral) << _T(", ") <<
						_T("z=") << drawing_units(cutting_depth - (1.0 * step_down)) << _T(", ") <<	// full depth now
						_T("i=") << drawing_units(centre.X(false) - point.X(false)) << _T(", ") <<
						_T("j=") << drawing_units(centre.Y(false) - point.Y(false)) << _T(")\n");
			point.SetX( centre.X(false) );
			point.SetY( centre.Y(false) - radius_of_spiral );

			// Third quadrant (6 O'Clock to 3 O'Clock)
			python << _T("arc_ccw( x=") << drawing_units(centre.X(false) + radius_of_spiral) << _T(", ") <<
						_T("y=") << centre.Y(true) << _T(", ") <<
						_T("z=") << drawing_units(cutting_depth - (1.0 * step_down)) << _T(", ") <<	// full depth now
						_T("i=") << drawing_units(centre.X(false) - point.X(false)) << _T(", ") <<
						_T("j=") << drawing_units(centre.Y(false) - point.Y(false)) << _T(")\n");
			point.SetX( centre.X(false) + radius_of_spiral );
			point.SetY( centre.Y(false) );

			// Fourth quadrant (3 O'Clock to 12 O'Clock)
			python << _T("arc_ccw( x=") << centre.X(true) << _T(", ") <<
						_T("y=") << drawing_units(centre.Y(false) + radius_of_spiral) << _T(", ") <<
						_T("z=") << drawing_units(cutting_depth - (1.0 * step_down)) << _T(", ") <<	// full depth now
						_T("i=") << drawing_units(centre.X(false) - point.X(false)) << _T(", ") <<
						_T("j=") << drawing_units(centre.Y(false) - point.Y(false)) << _T(")\n");
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
	python << _T("rapid( x=") << centre.X(true) << _T(", y=") << centre.Y(true) << _T(", z=") << drawing_units(final_depth) << _T(")\n");

	// Rapid to above the starting point (up at clearance height)
	point.SetZ( m_depth_op_params.m_clearance_height );
	python << _T("rapid( x=") << centre.X(true) << _T(", y=") << centre.Y(true) << _T(", z=") << point.Z(true) << _T(")\n");

	return(python);

} // End GenerateGCodeForOneLocation() method



/**
	This method is called when the CAD operator presses the Python button.  This method generates
	Python source code whose job will be to generate RS-274 GCode.  It's done in two steps so that
	the Python code can be configured to generate GCode suitable for various CNC interpreters.
 */
Python CCounterBore::AppendTextToProgram(CMachineState *pMachineState)
{
	Python python;

	python << CDepthOp::AppendTextToProgram(pMachineState);

	if (m_tool_number > 0)
	{
		CTool *pTool = CTool::Find( m_tool_number );
		if (pTool != NULL)
		{
			if ((pTool->CuttingRadius() * 2.0) >= m_params.m_diameter)
			{
				wxString message;
				message << _("Error: Tool diameter (") << pTool->m_params.m_diameter << _T(") ")
					 << _(">= hole diameter (") << m_params.m_diameter << _T(") ")
					 << _("in counter bore operation.  ")
					 << _("Skipping this counter bore operation (ID=") << m_id << _T(")");
				wxMessageBox(message);
				return(python);
			} // End if - then

			std::vector<CNCPoint> locations = CDrilling::FindAllLocations( this, pMachineState->Location(), m_params.m_sort_locations, NULL );
			for (std::vector<CNCPoint>::const_iterator l_itLocation = locations.begin(); l_itLocation != locations.end(); l_itLocation++)
			{
				CNCPoint point( pMachineState->Fixture().Adjustment(*l_itLocation) );
				python << GenerateGCodeForOneLocation( point, pTool );
				pMachineState->Location(*l_itLocation); // Remember where we are.
			} // End for
		} // End if - then
		else
		{
			wxString message;
			message << _("Warning: Counter bore refers to a tool ")
				 << _("that can't be found in the model.  ")
				 << _("Skipping this counter bore operation (ID=") << m_id << _T(")");
			wxMessageBox(message);
		} // End if - else
	} // End if - then
	else
	{
		wxString message;
		message << _("Warning: Counter bore operations MUST refer to a tool.  ")
			 << _("Skipping this counter bore operation (ID=") << m_id << _T(")");
		wxMessageBox(message);
	} // End if - else

	return(python);
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
	CDepthOp::glCommands(select,marked,no_color);

	if(marked && !no_color)
	{
		// For all coordinates that relate to these reference objects, draw the graphics that represents
		// both a drilling hole and a counterbore.

		std::vector<CNCPoint> locations = CDrilling::FindAllLocations( this );
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
CCounterBore::CCounterBore( const CCounterBore & rhs ) : CDepthOp(rhs)
{
	std::copy( rhs.m_symbols.begin(), rhs.m_symbols.end(), std::inserter( m_symbols, m_symbols.begin() ) );
    m_params = rhs.m_params;
}

CCounterBore & CCounterBore::operator= ( const CCounterBore & rhs )
{
	if (this != &rhs)
	{
		CDepthOp::operator=(rhs);
		m_symbols.clear();
		std::copy( rhs.m_symbols.begin(), rhs.m_symbols.end(), std::inserter( m_symbols, m_symbols.begin() ) );

		m_params = rhs.m_params;
	}

	return(*this);
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
	if (object->GetType() == GetType())
	{
		operator=(*((CCounterBore*)object));
	}
}

bool CCounterBore::CanAddTo(HeeksObj* owner)
{
	return ((owner != NULL) && (owner->GetType() == OperationsType));
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

	std::list<TiXmlElement *> elements_to_remove;

	// read point and circle ids
	for(TiXmlElement* pElem = TiXmlHandle(element).FirstChildElement().Element(); pElem; pElem = pElem->NextSiblingElement())
	{
		std::string name(pElem->Value());
		if(name == "params"){
			new_object->m_params.ReadParametersFromXMLElement(pElem);
			elements_to_remove.push_back(pElem);
		}
		else if(name == "symbols"){
			for(TiXmlElement* child = TiXmlHandle(pElem).FirstChildElement().Element(); child; child = child->NextSiblingElement())
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
		element->RemoveChild(*itElem);
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
void CCounterBore::ReloadPointers()
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

	CDepthOp::ReloadPointers();
}





// Return depth and diameter in that order.
std::pair< double, double > CCounterBore::SelectSizeForHead( const double drill_hole_diameter )
{
	// Just bluf it for now.  We will implement a lookup table eventually.

	return( std::make_pair( drill_hole_diameter * 1, drill_hole_diameter * 1.7 )  );

} // End SelectSizeForHead() method


bool CCounterBore::CanAdd(HeeksObj* object)
{
	return((object != NULL) && (CCounterBore::ValidType(object->GetType())));
}

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
		case FixtureType:
            return(true);

        default:
            return(false);
    }
}

void CCounterBore::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
    CDepthOp::GetTools( t_list, p );
}


CCounterBore::CCounterBore(	const Symbols_t &symbols,
			const int tool_number )
		: CDepthOp(GetTypeString(), NULL, tool_number, CounterBoreType), m_symbols(symbols)
{
    for (Symbols_t::iterator symbol = m_symbols.begin(); symbol != m_symbols.end(); symbol++)
    {
        HeeksObj *object = heeksCAD->GetIDObject( symbol->first, symbol->second );
        if (object != NULL)
        {
            Add(object,NULL);
        } // End if - then
    } // End for
    m_symbols.clear();

    m_params.set_initial_values( tool_number );

    std::list<int> drillbits;
    std::vector<CNCPoint> locations = CDrilling::FindAllLocations( this, CNCPoint(0.0, 0.0, 0.0), false, &drillbits );
    if (drillbits.size() > 0)
    {
        // We found some drilling objects amongst the symbols. Use the diameter of
        // any tools they used to help decide what size cap screw we're trying to cater for.

        for (std::list<int>::const_iterator drillbit = drillbits.begin(); drillbit != drillbits.end(); drillbit++)
        {
            HeeksObj *object = heeksCAD->GetIDObject( ToolType, *drillbit );
            if (object != NULL)
            {
                std::pair< double, double > screw_size = SelectSizeForHead( ((CTool *) object)->m_params.m_diameter );
                m_depth_op_params.m_final_depth = screw_size.first;
                m_params.m_diameter = screw_size.second;
            } // End if - then
        } // End for
    } // End if - then
}

bool CCounterBoreParams::operator== ( const CCounterBoreParams & rhs ) const
{
	if (m_diameter != rhs.m_diameter) return(false);
	if (m_sort_locations != rhs.m_sort_locations) return(false);

	return(true);
}


bool CCounterBore::operator== ( const CCounterBore & rhs ) const
{
	if (m_params != rhs.m_params) return(false);

	return(CDepthOp::operator==(rhs));
}

