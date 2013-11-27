// Drilling.cpp
/*
 * Copyright (c) 2009, Dan Heeks, Perttu Ahola
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "stdafx.h"
#include "Drilling.h"
#include "CNCConfig.h"
#include "ProgramCanvas.h"
#include "interface/HeeksObj.h"
#include "interface/PropertyInt.h"
#include "interface/PropertyDouble.h"
#include "interface/PropertyLength.h"
#include "interface/PropertyChoice.h"
#include "interface/PropertyString.h"
#include "tinyxml/tinyxml.h"
#include "Operations.h"
#include "CTool.h"
#include "Profile.h"
#include "CNCPoint.h"
#include "MachineState.h"
#include "Program.h"

#include <sstream>
#include <iomanip>
#include <vector>
#include <algorithm>

extern CHeeksCADInterface* heeksCAD;


void CDrillingParams::set_initial_values( const double depth, const int tool_number )
{
	CNCConfig config(ConfigScope());

	config.Read(_T("m_standoff"), &m_standoff, (25.4 / 4));	// Quarter of an inch
	config.Read(_T("m_dwell"), &m_dwell, 1);
	config.Read(_T("m_depth"), &m_depth, 25.4);		// One inch
	config.Read(_T("m_peck_depth"), &m_peck_depth, (25.4 / 10));	// One tenth of an inch
	config.Read(_T("m_sort_drilling_locations"), &m_sort_drilling_locations, 1);
	config.Read(_T("m_retract_mode"), &m_retract_mode, 0);
	config.Read(_T("m_spindle_mode"), &m_spindle_mode, 0);
	config.Read(_T("m_clearance_height"), &m_clearance_height, 25.4);		// One inch

	if (depth > 0)
	{
		// We've found the depth we want used.  Assign it now.
		m_depth = depth;
	} // End if - then

	// The following is taken from the 'rule of thumb' document that Stanley Dornfeld put
	// together for drilling feeds and speeds.  It includes a statement something like;
	// "We most always peck every one half drill diameter in depth after the first peck of
	// three diameters".  From this, we will take his advice and set a default peck depth
	// that is half the drill's diameter.
	//
	// NOTE: If the peck depth is zero (or less) then the operator may have manually chosen
	// to not peck.  In this case, don't add a positive peck depth - which would force
	// a pecking cycle rather than another drilling cycle.
	if ((tool_number > 0) && (m_peck_depth > 0.0))
	{
		CTool *pTool = CTool::Find( tool_number );
		if (pTool != NULL)
		{
			m_peck_depth = pTool->m_params.m_diameter / 2.0;
		}
	}

}

void CDrillingParams::write_values_to_config()
{
	// We always want to store the parameters in mm and convert them back later on.

	CNCConfig config(ConfigScope());

	// These values are in mm.
	config.Write(_T("m_standoff"), m_standoff);
	config.Write(_T("m_dwell"), m_dwell);
	config.Write(_T("m_depth"), m_depth);
	config.Write(_T("m_peck_depth"), m_peck_depth);
	config.Write(_T("m_sort_drilling_locations"), m_sort_drilling_locations);
	config.Write(_T("m_retract_mode"), m_retract_mode);
	config.Write(_T("m_spindle_mode"), m_spindle_mode);
	config.Write(_T("m_clearance_height"), m_clearance_height);

}


static void on_set_spindle_mode(int value, HeeksObj* object, bool from_undo_redo)
{
	((CDrilling*)object)->m_params.m_spindle_mode = value;
	((CDrilling*)object)->m_params.write_values_to_config();
}

static void on_set_retract_mode(int value, HeeksObj* object, bool from_undo_redo)
{
	((CDrilling*)object)->m_params.m_retract_mode = value;
	((CDrilling*)object)->m_params.write_values_to_config();
}

static void on_set_standoff(double value, HeeksObj* object)
{
	((CDrilling*)object)->m_params.m_standoff = value;
	((CDrilling*)object)->m_params.write_values_to_config();
}

static void on_set_dwell(double value, HeeksObj* object)
{
	((CDrilling*)object)->m_params.m_dwell = value;
	((CDrilling*)object)->m_params.write_values_to_config();
}

static void on_set_depth(double value, HeeksObj* object)
{
	((CDrilling*)object)->m_params.m_depth = value;
	((CDrilling*)object)->m_params.write_values_to_config();
}

static void on_set_peck_depth(double value, HeeksObj* object)
{
	((CDrilling*)object)->m_params.m_peck_depth = value;
	((CDrilling*)object)->m_params.write_values_to_config();
}

static void on_set_sort_drilling_locations(int value, HeeksObj* object, bool from_undo_redo)
{
	((CDrilling*)object)->m_params.m_sort_drilling_locations = value;
	((CDrilling*)object)->m_params.write_values_to_config();
}

static void on_set_clearance_height(double value, HeeksObj* object)
{
	((CDrilling*)object)->m_params.ClearanceHeight( value );
	((CDrilling*)object)->m_params.write_values_to_config();
}

void CDrillingParams::GetProperties(CDrilling* parent, std::list<Property *> *list)
{
	list->push_back(new PropertyLength(_("standoff"), m_standoff, parent, on_set_standoff));

	switch(theApp.m_program->m_clearance_source)
	{
	case CProgram::eClearanceDefinedByFixture:
		list->push_back(new PropertyString(_("clearance height"), _("Defined in fixture definition"), NULL, NULL));
		break;

	case CProgram::eClearanceDefinedByMachine:
		list->push_back(new PropertyString(_("clearance height"), _("Defined in Program properties for whole machine"), NULL, NULL));
		break;

	case CProgram::eClearanceDefinedByOperation:
	default:
		list->push_back(new PropertyLength(_("clearance height"), m_clearance_height, parent, on_set_clearance_height));
	} // End switch

	list->push_back(new PropertyDouble(_("dwell"), m_dwell, parent, on_set_dwell));
	list->push_back(new PropertyLength(_("depth"), m_depth, parent, on_set_depth));
	list->push_back(new PropertyLength(_("peck_depth"), m_peck_depth, parent, on_set_peck_depth));
	{ // Begin choice scope
		std::list< wxString > choices;

		choices.push_back(_("Rapid retract"));	// Must be 'false' (0)
		choices.push_back(_("Feed retract"));	// Must be 'true' (non-zero)

		int choice = int(m_retract_mode);
		list->push_back(new PropertyChoice(_("retract_mode"), choices, choice, parent, on_set_retract_mode));
	} // End choice scope
	{ // Begin choice scope
		std::list< wxString > choices;

		choices.push_back(_("Keep running"));	// Must be 'false' (0)
		choices.push_back(_("Stop at bottom"));	// Must be 'true' (non-zero)

		int choice = int(m_spindle_mode);
		list->push_back(new PropertyChoice(_("spindle_mode"), choices, choice, parent, on_set_spindle_mode));
	} // End choice scope
	{ // Begin choice scope
		std::list< wxString > choices;

		choices.push_back(_("Respect existing order"));	// Must be 'false' (0)
		choices.push_back(_("True"));			// Must be 'true' (non-zero)

		int choice = int(m_sort_drilling_locations);
		list->push_back(new PropertyChoice(_("sort_drilling_locations"), choices, choice, parent, on_set_sort_drilling_locations));
	} // End choice scope

}

void CDrillingParams::WriteXMLAttributes(TiXmlNode *root)
{
	TiXmlElement * element;
	element = heeksCAD->NewXMLElement( "params" );
	heeksCAD->LinkXMLEndChild( root,  element );

	element->SetDoubleAttribute( "standoff", m_standoff);
	element->SetDoubleAttribute( "dwell", m_dwell);
	element->SetDoubleAttribute( "depth", m_depth);
	element->SetDoubleAttribute( "peck_depth", m_peck_depth);

	element->SetAttribute( "sort_drilling_locations", m_sort_drilling_locations);
	element->SetAttribute( "retract_mode", m_retract_mode);
	element->SetAttribute( "spindle_mode", m_spindle_mode);
	element->SetAttribute( "clearance_height", m_clearance_height);
}

void CDrillingParams::ReadParametersFromXMLElement(TiXmlElement* pElem)
{
	if (pElem->Attribute("standoff")) pElem->Attribute("standoff", &m_standoff);
	m_clearance_height = m_standoff;  // Default if the clearance_height parameter is not found.
	if (pElem->Attribute("dwell")) pElem->Attribute("dwell", &m_dwell);
	if (pElem->Attribute("depth")) pElem->Attribute("depth", &m_depth);
	if (pElem->Attribute("peck_depth")) pElem->Attribute("peck_depth", &m_peck_depth);
	if (pElem->Attribute("sort_drilling_locations")) pElem->Attribute("sort_drilling_locations", &m_sort_drilling_locations);
	if (pElem->Attribute("retract_mode")) pElem->Attribute("retract_mode", &m_retract_mode);
	if (pElem->Attribute("spindle_mode")) pElem->Attribute("spindle_mode", &m_spindle_mode);
	if (pElem->Attribute("clearance_height")) pElem->Attribute("clearance_height", &m_clearance_height);
}

const wxBitmap &CDrilling::GetIcon()
{
	if(!m_active)return GetInactiveIcon();
	static wxBitmap* icon = NULL;
	if(icon == NULL)icon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/drilling.png")));
	return *icon;
}

/**
	This method is called when the CAD operator presses the Python button.  This method generates
	Python source code whose job will be to generate RS-274 GCode.  It's done in two steps so that
	the Python code can be configured to generate GCode suitable for various CNC interpreters.
 */
Python CDrilling::AppendTextToProgram()
{
	Python python;

	python << CSpeedOp::AppendTextToProgram();   // Set any private fixtures and change tools (if necessary)

	std::vector<CNCPoint> locations = FindAllLocations();
	if(m_params.m_sort_drilling_locations != 0)CDrilling::SortLocations(locations, theApp.machine_state.Location());

	for (std::vector<CNCPoint>::const_iterator l_itLocation = locations.begin(); l_itLocation != locations.end(); l_itLocation++)
	{
		gp_Pnt point = *l_itLocation;

		python << _T("drill(")
			<< _T("x=") << point.X()/theApp.m_program->m_units << _T(", ")
			<< _T("y=") << point.Y()/theApp.m_program->m_units << _T(", ")
			<< _T("z=") << point.Z()/theApp.m_program->m_units << _T(", ")
			<< _T("depth=") << m_params.m_depth/theApp.m_program->m_units << _T(", ")
			<< _T("standoff=") << m_params.m_standoff/theApp.m_program->m_units << _T(", ")
			<< _T("dwell=") << m_params.m_dwell << _T(", ")
			<< _T("peck_depth=") << m_params.m_peck_depth/theApp.m_program->m_units << _T(", ")
			<< _T("retract_mode=") << m_params.m_retract_mode << _T(", ")
			<< _T("spindle_mode=") << m_params.m_spindle_mode << _T(", ")
			<< _T("clearance_height=") << m_params.ClearanceHeight()
			<< _T(")\n");
        theApp.machine_state.Location(point); // Remember where we are.
	} // End for

	python << _T("end_canned_cycle()\n");

	return(python);
}


/**
	This routine generates a list of coordinates around the circumference of a circle.  It's just used
	to generate data suitable for OpenGL calls to paint a circle.  This graphics is transient but will
	help represent what the GCode will be doing when it's generated.
 */
std::list< CNCPoint > CDrilling::PointsAround(
		const CNCPoint & origin,
		const double radius,
		const unsigned int numPoints ) const
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
	Generate a list of vertices that represent the hole that will be drilled.  Let it be a circle at the top, a
	spiral down the length and a countersunk base.

	This method is only called by the glCommands() method.  This means that the graphics is transient.

	TODO: Handle drilling in any rotational angle. At the moment it only handles drilling 'down' along the 'z' axis
 */

std::list< CNCPoint > CDrilling::DrillBitVertices( const CNCPoint & origin, const double radius, const double length ) const
{
	std::list<CNCPoint> top, spiral, bottom, countersink, result;

	double flutePitch = 5.0;	// 5mm of depth per spiral of the drill bit's flute.
	double countersinkDepth = -1 * radius * tan(31.0); // this is the depth of the countersink cone at the end of the drill bit. (for a typical 118 degree bevel)
	unsigned int numPoints = 20;	// number of points in one circle (360 degrees) i.e. how smooth do we want the graphics
	const double pi = 3.1415926;
	double alpha = 2 * pi / numPoints;

	// Get a circle at the top of the dill bit's path
	top = PointsAround( origin, radius, numPoints );
	top.push_back( *(top.begin()) );	// Close the circle

	double depthPerItteration;
	countersinkDepth = -1 * radius * tan(31.0);	// For a typical (118 degree bevel on the drill bit tip)

	unsigned int l_iNumItterations = numPoints * (length / flutePitch);
	depthPerItteration = (length - countersinkDepth) / l_iNumItterations;

	// Now generate the spirals.

	unsigned int i = 0;
	while( i++ < l_iNumItterations )
	{
		double theta = alpha * i;
		CNCPoint pointOnCircle( cos( theta ) * radius, sin( theta ) * radius, 0 );
		pointOnCircle += origin;

		// And spiral down as we go.
		pointOnCircle.SetZ( pointOnCircle.Z() - (depthPerItteration * i) );

		spiral.push_back(pointOnCircle);
	} // End while

	// And now the countersink at the bottom of the drill bit.
	i = 0;
	while( i++ < numPoints )
	{
		double theta = alpha * i;
		CNCPoint topEdge( cos( theta ) * radius, sin( theta ) * radius, 0 );

		// This is at the top edge of the countersink
		topEdge.SetX( topEdge.X() + origin.X() );
		topEdge.SetY( topEdge.Y() + origin.Y() );
		topEdge.SetZ( origin.Z() - (length - countersinkDepth) );
		spiral.push_back(topEdge);

		// And now at the very point of the countersink
		CNCPoint veryTip( origin );
		veryTip.SetZ( (origin.Z() - length) );

		spiral.push_back(veryTip);
		spiral.push_back(topEdge);
	} // End while

	std::copy( top.begin(), top.end(), std::inserter( result, result.begin() ) );
	std::copy( spiral.begin(), spiral.end(), std::inserter( result, result.end() ) );
	std::copy( countersink.begin(), countersink.end(), std::inserter( result, result.end() ) );

	return(result);

} // End DrillBitVertices() routine


/**
	This is the Graphics Library Commands (from the OpenGL set).  This method calls the OpenGL
	routines to paint the drill action in the graphics window.  The graphics is transient.

	Part of its job is to re-paint the elements that this CDrilling object refers to so that
	we know what CAD objects this CNC operation is referring to.
 */
void CDrilling::glCommands(bool select, bool marked, bool no_color)
{
	CSpeedOp::glCommands(select, marked, no_color);

	if(marked && !no_color)
	{
		double l_dHoleDiameter = 12.7;	// Default at half-inch (in mm)

		if (m_tool_number > 0)
		{
			HeeksObj* Tool = heeksCAD->GetIDObject( ToolType, m_tool_number );
			if (Tool != NULL)
			{
                		l_dHoleDiameter = ((CTool *) Tool)->m_params.m_diameter;
			} // End if - then
		} // End if - then

		std::vector<CNCPoint> locations = FindAllLocations();

		for (std::vector<CNCPoint>::const_iterator l_itLocation = locations.begin(); l_itLocation != locations.end(); l_itLocation++)
		{
			GLdouble start[3], end[3];

			start[0] = l_itLocation->X();
			start[1] = l_itLocation->Y();
			start[2] = l_itLocation->Z();

			end[0] = l_itLocation->X();
			end[1] = l_itLocation->Y();
			end[2] = l_itLocation->Z();

			end[2] -= m_params.m_depth;

			glBegin(GL_LINE_STRIP);
			glVertex3dv( start );
			glVertex3dv( end );
			glEnd();

			std::list< CNCPoint > pointsAroundCircle = DrillBitVertices( 	*l_itLocation,
												l_dHoleDiameter / 2,
												m_params.m_depth);

			glBegin(GL_LINE_STRIP);
			CNCPoint previous = *(pointsAroundCircle.begin());
			for (std::list< CNCPoint >::const_iterator l_itPoint = pointsAroundCircle.begin();
				l_itPoint != pointsAroundCircle.end();
				l_itPoint++)
			{

				glVertex3d( l_itPoint->X(), l_itPoint->Y(), l_itPoint->Z() );
			}
			glEnd();
		} // End for
	} // End if - then
}


void CDrilling::GetProperties(std::list<Property *> *list)
{
	m_params.GetProperties(this, list);
	CSpeedOp::GetProperties(list);
}

HeeksObj *CDrilling::MakeACopy(void)const
{
	return new CDrilling(*this);
}

void CDrilling::CopyFrom(const HeeksObj* object)
{
	if (object->GetType() == GetType())
	{
		operator=(*((CDrilling*)object));
	}
}

CDrilling::CDrilling(	const Symbols_t &symbols,
        const int tool_number,
        const double depth )
    : CSpeedOp(GetTypeString(), tool_number, DrillingType), m_symbols(symbols)
{
    m_params.set_initial_values(depth, tool_number);
}


CDrilling::CDrilling( const CDrilling & rhs ) : CSpeedOp( rhs )
{
	std::copy( rhs.m_symbols.begin(), rhs.m_symbols.end(), std::inserter( m_symbols, m_symbols.begin() ));
    m_params = rhs.m_params;
}

CDrilling & CDrilling::operator= ( const CDrilling & rhs )
{
	if (this != &rhs)
	{
		CSpeedOp::operator=(rhs);
		m_symbols.clear();
		std::copy( rhs.m_symbols.begin(), rhs.m_symbols.end(), std::inserter( m_symbols, m_symbols.begin() ));

		m_params = rhs.m_params;
	}

	return(*this);
}

bool CDrilling::CanAddTo(HeeksObj* owner)
{
    if (owner == NULL) return(false);

	int type = owner->GetType();

	if (type == OperationsType) return(true);

	return(false);
}

bool CDrilling::CanAdd(HeeksObj* object)
{
	return false;
}

void CDrilling::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element = heeksCAD->NewXMLElement( "Drilling" );
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
HeeksObj* CDrilling::ReadFromXMLElement(TiXmlElement* element)
{
	CDrilling* new_object = new CDrilling;

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
				int type, id;
				if (child->Attribute("type", &type) && child->Attribute("id", &id))
				{
					new_object->AddSymbol( type, id );
				}
			} // End for
			elements_to_remove.push_back(pElem);
		} // End if
		else if(name == "Point"){
			// older version where children were allowed
			int id;
			if(pElem->Attribute("id", &id))
			{
				new_object->AddSymbol( PointType, id);
			}
		}
	}

	for (std::list<TiXmlElement*>::iterator itElem = elements_to_remove.begin(); itElem != elements_to_remove.end(); itElem++)
	{
		heeksCAD->RemoveXMLChild( element, *itElem);
	}

	new_object->ReadBaseXML(element);

	return new_object;
}

std::vector<CNCPoint> CDrilling::FindAllLocations()
{
	std::vector<CNCPoint> locations;

	for (Symbols_t::iterator itLhs = m_symbols.begin(); itLhs != m_symbols.end(); itLhs++)
	{
	    Symbol_t &symbol = *itLhs;

		if (symbol.first == PointType)
		{
			HeeksObj* object = heeksCAD->GetIDObject(symbol.first, symbol.second);
			double pos[3];
			object->GetStartPoint(pos);

			// Copy the results in ONLY if each point doesn't already exist.
			if (std::find( locations.begin(), locations.end(), CNCPoint( pos ) ) == locations.end())
			{
				locations.push_back( CNCPoint( pos ) );
			} // End if - then

			continue;	// No need to intersect a point with anything.
		} // End if - then
	}

	return(locations);
}

//static
void CDrilling::SortLocations(std::vector<CNCPoint> &locations, const CNCPoint starting_location)
{
	for (std::vector<CNCPoint>::iterator l_itPoint = locations.begin(); l_itPoint != locations.end(); l_itPoint++)
	{
		if (l_itPoint == locations.begin())
		{
			// It's the first point.
			CNCPoint reference_location(0.0, 0.0, 0.0);
            reference_location = starting_location;

			sort_points_by_distance compare( reference_location );
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

/**
    This method returns TRUE if the type of symbol is suitable for reference as a source of location
 */
/* static */ bool CDrilling::ValidType( const int object_type )
{
    switch (object_type)
    {
        case PointType:
        case CircleType:
        case SketchType:
        case DrillingType:
        case ProfileType:
        case PocketType:
		case ILineType:
            return(true);

        default:
            return(false);
    }
}


void CDrilling::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
    CSpeedOp::GetTools( t_list, p );
}


bool CDrillingParams::operator==( const CDrillingParams & rhs) const
{
	if (m_standoff != rhs.m_standoff) return(false);
	if (m_dwell != rhs.m_dwell) return(false);
	if (m_depth != rhs.m_depth) return(false);
	if (m_peck_depth != rhs.m_peck_depth) return(false);
	if (m_sort_drilling_locations != rhs.m_sort_drilling_locations) return(false);
	if (m_retract_mode != rhs.m_retract_mode) return(false);
	if (m_spindle_mode != rhs.m_spindle_mode) return(false);
	if (m_clearance_height != rhs.m_clearance_height) return(false);

	return(true);
}


bool CDrilling::operator==( const CDrilling & rhs ) const
{
	if (m_params != rhs.m_params) return(false);

	return(CSpeedOp::operator==(rhs));
}

double CDrillingParams::ClearanceHeight() const
{
	switch (theApp.m_program->m_clearance_source)
	{
	case CProgram::eClearanceDefinedByMachine:
		return(theApp.m_program->m_machine.m_clearance_height);

	case CProgram::eClearanceDefinedByFixture:
		// We need to figure out which is the 'active' fixture and return
		// the clearance height from that fixture.
			// This should not occur.  In any case, use the clearance value from the individual operation.
			return(m_clearance_height);

	case CProgram::eClearanceDefinedByOperation:
	default:
		return(m_clearance_height);
	} // End switch
}
