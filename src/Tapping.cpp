// Tapping.cpp
/*
 * Copyright (c) 2010 Michael Haberler, Dan Heeks, Perttu Ahola
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "stdafx.h"
#include "Tapping.h"
#include "CNCConfig.h"
#include "ProgramCanvas.h"
#include "interface/HeeksObj.h"
#include "interface/PropertyInt.h"
#include "interface/PropertyDouble.h"
#include "interface/PropertyLength.h"
#include "interface/PropertyChoice.h"
#include "tinyxml/tinyxml.h"
#include "Operations.h"
#include "CTool.h"
#include "Profile.h"
#include "Fixture.h"
#include "CNCPoint.h"
#include "MachineState.h"
#include "Program.h"

#include <sstream>
#include <iomanip>
#include <vector>
#include <algorithm>

extern CHeeksCADInterface* heeksCAD;


void CTappingParams::set_initial_values( const double depth, const int tool_number )
{
	CNCConfig config(ConfigScope());

	config.Read(_T("m_standoff"), &m_standoff, (25.4 / 4));	// Quarter of an inch
	config.Read(_T("m_dwell"), &m_dwell, 1);
	config.Read(_T("m_depth"), &m_depth, 25.4/2);		// half an inch
	config.Read(_T("m_sort_tapping_locations"), &m_sort_tapping_locations, 1);
	config.Read(_T("m_tap_mode"), &m_tap_mode, 0);    // 0 -> rigid tap mode, 1 -> chuck tapping

	if (depth > 0)
	{
		// We've found the depth we want used.  Assign it now.
		m_depth = depth;
	} // End if - then
}

void CTappingParams::write_values_to_config()
{
	// We always want to store the parameters in mm and convert them back later on.

	CNCConfig config(ConfigScope());

	// These values are in mm.
	config.Write(_T("m_standoff"), m_standoff);
	config.Write(_T("m_dwell"), m_dwell);
	config.Write(_T("m_depth"), m_depth);
	config.Write(_T("m_sort_tapping_locations"), m_sort_tapping_locations);
	config.Write(_T("m_tap_mode"), m_tap_mode);
}

static void on_set_mode(int value, HeeksObj* object)
{
	((CTapping*)object)->m_params.m_tap_mode = value;
	((CTapping*)object)->m_params.write_values_to_config();
}


static void on_set_standoff(double value, HeeksObj* object)
{
	((CTapping*)object)->m_params.m_standoff = value;
	((CTapping*)object)->m_params.write_values_to_config();
}

static void on_set_dwell(double value, HeeksObj* object)
{
	((CTapping*)object)->m_params.m_dwell = value;
	((CTapping*)object)->m_params.write_values_to_config();
}

static void on_set_depth(double value, HeeksObj* object)
{
	((CTapping*)object)->m_params.m_depth = value;
	((CTapping*)object)->m_params.write_values_to_config();
}


static void on_set_sort_tapping_locations(int value, HeeksObj* object)
{
	((CTapping*)object)->m_params.m_sort_tapping_locations = value;
	((CTapping*)object)->m_params.write_values_to_config();
}


void CTappingParams::GetProperties(CTapping* parent, std::list<Property *> *list)
{
	{ // Begin choice scope
		std::list< wxString > choices;

		choices.push_back(_("rigid tapping"));	// Must be 0
		choices.push_back(_("chuck tapping"));	// Must be 1

		int choice = int(m_tap_mode);
		list->push_back(new PropertyChoice(_("tap_mode"), choices, choice, parent, on_set_mode));

	} // End choice scope

	list->push_back(new PropertyLength(_("standoff"), m_standoff, parent, on_set_standoff));
	list->push_back(new PropertyDouble(_("dwell"), m_dwell, parent, on_set_dwell));
	list->push_back(new PropertyLength(_("depth"), m_depth, parent, on_set_depth));
	//	list->push_back(new PropertyLength(_("pitch"), m_pitch, parent, on_set_pitch));
	{ // Begin choice scope
		std::list< wxString > choices;

		choices.push_back(_("Respect existing order"));	// Must be 'false' (0)
		choices.push_back(_("True"));			// Must be 'true' (non-zero)

		int choice = int(m_sort_tapping_locations);
		list->push_back(new PropertyChoice(_("sort_tapping_locations"), choices, choice, parent, on_set_sort_tapping_locations));

	} // End choice scope
}

void CTappingParams::WriteXMLAttributes(TiXmlNode *root)
{
	TiXmlElement * element;
	element = heeksCAD->NewXMLElement( "params" );
	heeksCAD->LinkXMLEndChild( root,  element );

	element->SetDoubleAttribute( "standoff", m_standoff);
	element->SetDoubleAttribute( "dwell", m_dwell);
	element->SetDoubleAttribute( "depth", m_depth);
	element->SetAttribute( "sort_tapping_locations", m_sort_tapping_locations);
	element->SetAttribute( "tap_mode", m_tap_mode);
}

void CTappingParams::ReadParametersFromXMLElement(TiXmlElement* pElem)
{
	if (pElem->Attribute("standoff")) pElem->Attribute("standoff", &m_standoff);
	if (pElem->Attribute("dwell")) pElem->Attribute("dwell", &m_dwell);
	if (pElem->Attribute("depth")) pElem->Attribute("depth", &m_depth);
	if (pElem->Attribute("sort_tapping_locations")) pElem->Attribute("sort_tapping_locations", &m_sort_tapping_locations);
	if (pElem->Attribute("tap_mode")) pElem->Attribute("tap_mode", &m_tap_mode);
}

const wxBitmap &CTapping::GetIcon()
{
	if(!m_active)return GetInactiveIcon();
	static wxBitmap* icon = NULL;
	if(icon == NULL)icon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/tapping.png")));
	return *icon;
}

/**
	This method is called when the CAD operator presses the Python button.  This method generates
	Python source code whose job will be to generate RS-274 GCode.  It's done in two steps so that
	the Python code can be configured to generate GCode suitable for various CNC interpreters.
 */
Python CTapping::AppendTextToProgram( CMachineState *pMachineState )
{
	Python python;
	double pitch = 0.0;
	int direction = 0; // default to right hand

	if (m_tool_number > 0)
	  {
	    HeeksObj* Tool = heeksCAD->GetIDObject( ToolType, m_tool_number );
	    if (Tool != NULL)
	      {
		pitch = ((CTool *) Tool)->m_params.m_pitch;
		direction = ((CTool *) Tool)->m_params.m_direction;
	      } // End if - then
	  } // End if - then

	python << CSpeedOp::AppendTextToProgram( pMachineState );   // Set any private fixtures and change tools (if necessary)

	std::vector<CNCPoint> locations = CDrilling::FindAllLocations(this, pMachineState->Location(), m_params.m_sort_tapping_locations != 0, NULL);
	for (std::vector<CNCPoint>::const_iterator l_itLocation = locations.begin(); l_itLocation != locations.end(); l_itLocation++)
	{
		gp_Pnt point = pMachineState->Fixture().Adjustment( *l_itLocation );

		python << _T("tap(")
		       << _T("x=") << point.X()/theApp.m_program->m_units << _T(", ")
		       << _T("y=") << point.Y()/theApp.m_program->m_units << _T(", ")
		       << _T("z=") << point.Z()/theApp.m_program->m_units << _T(", ")
		       << _T("tap_mode=") << m_params.m_tap_mode << _T(", ")
		       << _T("depth=") << m_params.m_depth/theApp.m_program->m_units << _T(", ")
		       << _T("standoff=") << m_params.m_standoff/theApp.m_program->m_units << _T(", ")
		       << _T("direction=") << direction << _T(", ") // needed for ISO G84/G74
		       << _T("pitch=") << pitch/theApp.m_program->m_units // _T(", ")
		       << _T(")\n");

	        pMachineState->Location(point); // Remember where we are.
	} // End for

	python << _T("end_canned_cycle()\n");

	return(python);
}


/**
	This routine generates a list of coordinates around the circumference of a circle.  It's just used
	to generate data suitable for OpenGL calls to paint a circle.  This graphics is transient but will
	help represent what the GCode will be doing when it's generated.
 */
std::list< CNCPoint > CTapping::PointsAround(
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
	Generate a list of vertices that represent the hole that will be taped.  Let it be a circle at the top, a
	spiral down the length and a countersunk base.

	This method is only called by the glCommands() method.  This means that the graphics is transient.

	TODO: Handle tapping in any rotational angle. At the moment it only handles tapping 'down' along the 'z' axis
 */

std::list< CNCPoint > CTapping::TapBitVertices( const CNCPoint & origin, const double radius, const double length ) const
{
	std::list<CNCPoint> top, spiral, bottom, countersink, result;

	double flutePitch = 5.0;	// 5mm of depth per spiral of the tap bit's flute.
	double countersinkDepth = -1 * radius * tan(31.0); // this is the depth of the countersink cone at the end of the tap bit. (for a typical 118 degree bevel)
	unsigned int numPoints = 20;	// number of points in one circle (360 degrees) i.e. how smooth do we want the graphics
	const double pi = 3.1415926;
	double alpha = 2 * pi / numPoints;

	// Get a circle at the top of the dill bit's path
	top = PointsAround( origin, radius, numPoints );
	top.push_back( *(top.begin()) );	// Close the circle

	double depthPerItteration;
	countersinkDepth = -1 * radius * tan(31.0);	// For a typical (118 degree bevel on the tap bit tip)

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

	// And now the countersink at the bottom of the tap bit.
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

} // End TapBitVertices() routine


/**
	This is the Graphics Library Commands (from the OpenGL set).  This method calls the OpenGL
	routines to paint the tap action in the graphics window.  The graphics is transient.

	Part of its job is to re-paint the elements that this CTapping object refers to so that
	we know what CAD objects this CNC operation is referring to.
 */
void CTapping::glCommands(bool select, bool marked, bool no_color)
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

		std::vector<CNCPoint> locations = CDrilling::FindAllLocations(this);

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

			std::list< CNCPoint > pointsAroundCircle = TapBitVertices( 	*l_itLocation,
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


void CTapping::GetProperties(std::list<Property *> *list)
{
	m_params.GetProperties(this, list);
	CSpeedOp::GetProperties(list);
}

HeeksObj *CTapping::MakeACopy(void)const
{
	return new CTapping(*this);
}

void CTapping::CopyFrom(const HeeksObj* object)
{
	if (object->GetType() == GetType())
	{
		operator=(*((CTapping*)object));
	}
}

CTapping::CTapping(	const Symbols_t &symbols,
        const int tool_number,
        const double depth )
    : CSpeedOp(GetTypeString(), tool_number, TappingType), m_symbols(symbols)
{
    m_params.set_initial_values(depth, tool_number);
    for (Symbols_t::iterator itSymbol = m_symbols.begin(); itSymbol != m_symbols.end(); itSymbol++)
    {
        HeeksObj *obj = heeksCAD->GetIDObject(itSymbol->first, itSymbol->second);
        if (obj != NULL)
        {
            Add(obj, NULL);
        }
    } // End for
    m_symbols.clear();	// We don't want to convert them twice.
}


CTapping::CTapping( const CTapping & rhs ) : CSpeedOp( rhs )
{
	std::copy( rhs.m_symbols.begin(), rhs.m_symbols.end(), std::inserter( m_symbols, m_symbols.begin() ));
    m_params = rhs.m_params;
}

CTapping & CTapping::operator= ( const CTapping & rhs )
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

bool CTapping::CanAddTo(HeeksObj* owner)
{
    if (owner == NULL) return(false);

	int type = owner->GetType();

	if (type == OperationsType) return(true);
	if (type == CounterBoreType) return(true);

	return(false);
}

bool CTapping::CanAdd(HeeksObj* object)
{
	return(CTapping::ValidType(object->GetType()));
}

void CTapping::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element = heeksCAD->NewXMLElement( "Tapping" );
	heeksCAD->LinkXMLEndChild( root,  element );
	m_params.WriteXMLAttributes(element);

	TiXmlElement * symbols;
	symbols = heeksCAD->NewXMLElement( "symbols" );
	heeksCAD->LinkXMLEndChild( element, symbols );

	WriteBaseXML(element);
}

// static member function
HeeksObj* CTapping::ReadFromXMLElement(TiXmlElement* element)
{
	CTapping* new_object = new CTapping;

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

					// We need to convert these type/id pairs into HeeksObj pointers but we want them to
					// come from the right source.  If we're importing data then they need to come from the
					// data we're importing.  If we're updating the main model then the main tree
					// will do.  We don't want to just use heeksCAD->GetIDObject() here as it will always
					// look in the main tree.  Perhaps we can force a recursive 'ReloadPointers()' call
					// so that these values are reset when necessary.  Eventually we will be storing the
					// child elements as real XML elements rather than just references.  Until time passes
					// a little longer, we need to support this type/id version.  Otherwise old HeeksCNC files
					// won't read in correctly.
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
	The old version of the CTapping object stored references to graphics as type/id pairs
	that get read into the m_symbols list.  The new version stores these graphics references
	as child elements (based on ObjList).  If we read in an old-format file then the m_symbols
	list will have data in it for which we don't have children.  This routine converts
	these type/id pairs into the HeeksObj pointers as children.
 */
void CTapping::ReloadPointers()
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

	CSpeedOp::ReloadPointers();
}



/**
	This method adjusts any parameters that don't make sense.  It should report a list
	of changes in the list of strings.
 */
std::list<wxString> CTapping::DesignRulesAdjustment(const bool apply_changes)
{
	std::list<wxString> changes;

	// Check that we're using a tapping tool and see if it's one of the standard sizes.
	if (m_tool_number > 0)
	{
		CTool *pTool = (CTool *) CTool::Find( m_tool_number );
		if (pTool != NULL)
		{
		    if (pTool->m_params.m_type != CToolParams::eTapTool)
		    {
		        changes.push_back(_("The tapping operation has not selected a tapping tool to use\n"));
		    }
		    else
		    {
		        // It is a tapping tool.  Check to see if the diameter and pitch combination match a standard size.

		        std::list<wxString> tool_check = pTool->DesignRulesAdjustment(apply_changes);
		        for (std::list<wxString>::iterator itChange = tool_check.begin(); itChange != tool_check.end(); itChange++)
		        {
		            changes.push_back(*itChange);
		        }
			
			// see wether tapping direction and spindle direction match
			if (pTool->m_params.m_direction && (m_speed_op_params.m_spindle_speed > 0)) 
			{
			    changes.push_back(_("Left-hand tapping needs a counterclockwise spindle rotation (negative spindle_speed)\n"));
			    if (apply_changes)
			    {
			        changes.push_back(_("Adjusting spindle rotation to counterclockwise.\n"));
				m_speed_op_params.m_spindle_speed  = - m_speed_op_params.m_spindle_speed;
			    } // End if - then
			}
			if (!pTool->m_params.m_direction && (m_speed_op_params.m_spindle_speed < 0) )
			{
			    changes.push_back(_("right-hand tapping needs clockwise spindle rotation (positive spindle_speed)\n"));
			    if (apply_changes)
			    {
				changes.push_back(_("Adjusting spindle rotation to clockwise.\n"));
				m_speed_op_params.m_spindle_speed  = - m_speed_op_params.m_spindle_speed;
			    } // End if - then
			}
		    }
		}
	}


	// Make some special checks if we're using a chamfering bit.
	if (m_tool_number > 0)
	{
		CTool *pTap = (CTool *) CTool::Find( m_tool_number );
		if (pTap != NULL)
		{
			std::vector<CNCPoint> these_locations = CDrilling::FindAllLocations(this);

			if (pTap->m_params.m_type == CToolParams::eTapTool)
			{
				// We need to make sure that the depth of the hole we're drilling is at least
				// as deep as the depth of our tapping operation.

				for (HeeksObj *obj = theApp.m_program->Operations()->GetFirstChild();
					obj != NULL;
					obj = theApp.m_program->Operations()->GetNextChild())
				{
					if (obj->GetType() == DrillingType)
					{
					    CDrilling *pDrilling = (CDrilling *) obj;

						// Make sure we're looking at a hole taped with something
						// more than a centre tap.
						CToolParams::eToolType type = CTool::CutterType( pDrilling->m_tool_number );
						if (	(type == CToolParams::eDrill) ||
							(type == CToolParams::eEndmill) ||
							(type == CToolParams::eSlotCutter) ||
							(type == CToolParams::eBallEndMill))
						{
							// See if any of the other drilling locations line up
							// with our tapping locations.  If so, we must be
							// tapping a previously drilled hole.

							std::vector<CNCPoint> previous_locations = CDrilling::FindAllLocations((CTapping *)obj);
							std::vector<CNCPoint> common_locations;
							std::set_intersection( previous_locations.begin(), previous_locations.end(),
										these_locations.begin(), these_locations.end(),
										std::inserter( common_locations, common_locations.begin() ));
							if (common_locations.size() > 0)
							{
								// We're here.  We must be tapping a hole we've
								// drilled previously.  Check the depths.

								if (pDrilling->m_params.m_depth < m_params.m_depth)
								{
								    wxString change;
								    change << _("ID ") << this->m_id << _(" The tapping operation's depth is greater than the previously drilled hole\n");
								    changes.push_back(change);
								}
							} // End if - then

						} // End if - then
					} // End if - then
				} // End for
			} // End if - then
		} // End if - then
	} // End if - then

	if (m_tool_number > 0)
	{
		// Make sure the hole depth isn't greater than the tool's cutting depth.
		CTool *pTap = (CTool *) CTool::Find( m_tool_number );
		if ((pTap != NULL) && (pTap->m_params.m_cutting_edge_height < m_params.m_depth))
		{
			// The drill bit we've chosen can't cut as deep as we've setup to go.

			if (apply_changes)
			{
#ifdef UNICODE
				std::wostringstream l_ossChange;
#else
				std::ostringstream l_ossChange;
#endif

				l_ossChange << _("Adjusting depth of tapping cycle") << " id='" << m_id << "' " << _("from") << " '"
					<< m_params.m_depth / theApp.m_program->m_units << "' " << _("to") << " "
					<< pTap->m_params.m_cutting_edge_height / theApp.m_program->m_units << "\n";
				changes.push_back(l_ossChange.str().c_str());

				m_params.m_depth = pTap->m_params.m_cutting_edge_height;
			} // End if - then
			else
			{
#ifdef UNICODE
				std::wostringstream l_ossChange;
#else
				std::ostringstream l_ossChange;
#endif

				l_ossChange << _("WARNING") << ": " << _("Tapping") << " (id=" << m_id << ").  " << _("Can't tap hole") << " " << m_params.m_depth / theApp.m_program->m_units << " when the tapping bit's cutting length is only " << pTap->m_params.m_cutting_edge_height << " long\n";
				changes.push_back(l_ossChange.str().c_str());
			} // End if - else
		} // End if - then
	} // End if - then


	return(changes);

} // End DesignRulesAdjustment() method


/**
    This method returns TRUE if the type of symbol is suitable for reference as a source of location
 */
/* static */ bool CTapping::ValidType( const int object_type )
{
    switch (object_type)
    {
        case PointType:
        case CircleType:
        case SketchType:
        case DrillingType:
        case ProfileType:
        case PocketType:
		case FixtureType:
            return(true);

        default:
            return(false);
    }
}


void CTapping::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
    CSpeedOp::GetTools( t_list, p );
}


bool CTappingParams::operator==( const CTappingParams & rhs) const
{
	if (m_standoff != rhs.m_standoff) return(false);
	if (m_dwell != rhs.m_dwell) return(false);
	if (m_depth != rhs.m_depth) return(false);
	if (m_sort_tapping_locations != rhs.m_sort_tapping_locations) return(false);
	if (m_tap_mode != rhs.m_tap_mode) return(false);

	return(true);
}


bool CTapping::operator==( const CTapping & rhs ) const
{
	if (m_params != rhs.m_params) return(false);

	return(CSpeedOp::operator==(rhs));
}
