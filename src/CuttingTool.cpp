// CuttingTool.cpp
/*
 * Copyright (c) 2009, Dan Heeks, Perttu Ahola
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "stdafx.h"
#include <math.h>
#include "CuttingTool.h"
#include "CNCConfig.h"
#include "ProgramCanvas.h"
#include "interface/HeeksObj.h"
#include "interface/PropertyInt.h"
#include "interface/PropertyDouble.h"
#include "interface/PropertyLength.h"
#include "interface/PropertyChoice.h"
#include "interface/PropertyString.h"
#include "tinyxml/tinyxml.h"

#include <sstream>
#include <string>
#include <algorithm>

extern CHeeksCADInterface* heeksCAD;


static void ResetParametersToReasonableValues(HeeksObj* object);

void CCuttingToolParams::set_initial_values()
{
	CNCConfig config;
	config.Read(_T("m_diameter"), &m_diameter, 12.7);
	config.Read(_T("m_x_offset"), &m_x_offset, 0);
	config.Read(_T("m_tool_length_offset"), &m_tool_length_offset, (10 * m_diameter));
	config.Read(_T("m_orientation"), &m_orientation, 0);

	config.Read(_T("m_type"), (int *) &m_type, eDrill);
	config.Read(_T("m_flat_radius"), &m_flat_radius, 0);
	config.Read(_T("m_corner_radius"), &m_corner_radius, 0);
	config.Read(_T("m_cutting_edge_angle"), &m_cutting_edge_angle, 59);
	config.Read(_T("m_cutting_edge_height"), &m_cutting_edge_angle, 4 * m_diameter);
}

void CCuttingToolParams::write_values_to_config()
{
	CNCConfig config;

	// We ALWAYS write the parameters into the configuration file in mm (for consistency).
	// If we're now in inches then convert the values.
	// We're in mm already.
	config.Write(_T("m_diameter"), m_diameter);
	config.Write(_T("m_x_offset"), m_x_offset);
	config.Write(_T("m_tool_length_offset"), m_tool_length_offset);
	config.Write(_T("m_orientation"), m_orientation);

	config.Write(_T("m_type"), m_type);
	config.Write(_T("m_flat_radius"), m_flat_radius);
	config.Write(_T("m_corner_radius"), m_corner_radius);
	config.Write(_T("m_cutting_edge_angle"), m_cutting_edge_angle);
	config.Write(_T("m_cutting_edge_height"), m_cutting_edge_height);
}

static void on_set_diameter(double value, HeeksObj* object)
{
	((CCuttingTool*)object)->m_params.m_diameter = value;
	ResetParametersToReasonableValues(object);
} // End on_set_diameter() routine

static void on_set_x_offset(double value, HeeksObj* object){((CCuttingTool*)object)->m_params.m_x_offset = value;}
static void on_set_tool_length_offset(double value, HeeksObj* object){((CCuttingTool*)object)->m_params.m_tool_length_offset = value;}
static void on_set_orientation(int value, HeeksObj* object)
{
	if ((value >= 0) && (value <= 9))
	{
		((CCuttingTool*)object)->m_params.m_orientation = value;
	} // End if - then
	else
	{
		wxMessageBox(_T("Orientation values must be between 0 and 9 inclusive.  Aborting value change"));
	} // End if - else
}

static void on_set_type(int value, HeeksObj* object)
{
	((CCuttingTool*)object)->m_params.m_type = CCuttingToolParams::eCuttingToolType(value);
	ResetParametersToReasonableValues(object);
} // End on_set_type() routine


static void ResetParametersToReasonableValues(HeeksObj* object)
{
#ifdef UNICODE
	std::wostringstream l_ossChange;
#else
    std::ostringstream l_ossChange;
#endif

	if (((CCuttingTool*)object)->m_params.m_tool_length_offset != (5 * ((CCuttingTool*)object)->m_params.m_diameter))
	{
		((CCuttingTool*)object)->m_params.m_tool_length_offset = (5 * ((CCuttingTool*)object)->m_params.m_diameter);
		l_ossChange << "Resetting tool length to " << (((CCuttingTool*)object)->m_params.m_tool_length_offset / theApp.m_program->m_units) << "\n";
	} // End if - then

	double height;
	switch(((CCuttingTool*)object)->m_params.m_type)
	{
		case CCuttingToolParams::eDrill:
				if (((CCuttingTool*)object)->m_params.m_corner_radius != 0) l_ossChange << "Changing corner radius to zero\n";
				((CCuttingTool*)object)->m_params.m_corner_radius = 0;

				if (((CCuttingTool*)object)->m_params.m_flat_radius != 0) l_ossChange << "Changing flat radius to zero\n";
				((CCuttingTool*)object)->m_params.m_flat_radius = 0;

				if (((CCuttingTool*)object)->m_params.m_cutting_edge_angle != 59) l_ossChange << "Changing cutting edge angle to 59 degrees (for normal 118 degree cutting face)\n";
				((CCuttingTool*)object)->m_params.m_cutting_edge_angle = 59;

				if (((CCuttingTool*)object)->m_params.m_cutting_edge_height != ((CCuttingTool*)object)->m_params.m_diameter * 3.0)
				{
					l_ossChange << "Changing cutting edge height to " << ((CCuttingTool*)object)->m_params.m_diameter / theApp.m_program->m_units * 3.0 << "\n";
					((CCuttingTool*)object)->m_params.m_cutting_edge_height = ((CCuttingTool*)object)->m_params.m_diameter * 3.0;
				} // End if - then

				l_ossChange << ((CCuttingTool*) object)->ResetTitle().c_str();
				break;

		case CCuttingToolParams::eCentreDrill:
				if (((CCuttingTool*)object)->m_params.m_corner_radius != 0) l_ossChange << "Changing corner radius to zero\n";
				((CCuttingTool*)object)->m_params.m_corner_radius = 0;

				if (((CCuttingTool*)object)->m_params.m_flat_radius != 0) l_ossChange << "Changing flat radius to zero\n";
				((CCuttingTool*)object)->m_params.m_flat_radius = 0;

				if (((CCuttingTool*)object)->m_params.m_cutting_edge_angle != 59) l_ossChange << "Changing cutting edge angle to 59 degrees (for normal 118 degree cutting face)\n";
				((CCuttingTool*)object)->m_params.m_cutting_edge_angle = 59;

				if (((CCuttingTool*)object)->m_params.m_cutting_edge_height != ((CCuttingTool*)object)->m_params.m_diameter * 1.0)
				{
					l_ossChange << "Changing cutting edge height to " << ((CCuttingTool*)object)->m_params.m_diameter / theApp.m_program->m_units * 1.0 << "\n";
					((CCuttingTool*)object)->m_params.m_cutting_edge_height = ((CCuttingTool*)object)->m_params.m_diameter * 1.0;
				} // End if - then

				l_ossChange << ((CCuttingTool*) object)->ResetTitle().c_str();
				break;

		case CCuttingToolParams::eEndmill:
				if (((CCuttingTool*)object)->m_params.m_corner_radius != 0) l_ossChange << "Changing corner radius to zero\n";
				((CCuttingTool*)object)->m_params.m_corner_radius = 0;

				if (((CCuttingTool*)object)->m_params.m_flat_radius != ( ((CCuttingTool*)object)->m_params.m_diameter / 2) )
				{
					l_ossChange << "Changing flat radius to " << ((CCuttingTool*)object)->m_params.m_diameter / 2 << "\n";
					((CCuttingTool*)object)->m_params.m_flat_radius = ((CCuttingTool*)object)->m_params.m_diameter / 2;
				} // End if - then

				if (((CCuttingTool*)object)->m_params.m_cutting_edge_angle != 0) l_ossChange << "Changing cutting edge angle to zero degrees\n";
				((CCuttingTool*)object)->m_params.m_cutting_edge_angle = 0;

				if (((CCuttingTool*)object)->m_params.m_cutting_edge_height != ((CCuttingTool*)object)->m_params.m_diameter * 3.0)
				{
					l_ossChange << "Changing cutting edge height to " << ((CCuttingTool*)object)->m_params.m_diameter / theApp.m_program->m_units * 3.0 << "\n";
					((CCuttingTool*)object)->m_params.m_cutting_edge_height = ((CCuttingTool*)object)->m_params.m_diameter * 3.0;
				} // End if - then

				l_ossChange << ((CCuttingTool*) object)->ResetTitle().c_str();
				break;

		case CCuttingToolParams::eSlotCutter:
				if (((CCuttingTool*)object)->m_params.m_corner_radius != 0) l_ossChange << "Changing corner radius to zero\n";
				((CCuttingTool*)object)->m_params.m_corner_radius = 0;

				if (((CCuttingTool*)object)->m_params.m_flat_radius != (((CCuttingTool*)object)->m_params.m_diameter / 2)) 
				{
					l_ossChange << "Changing flat radius to " << ((((CCuttingTool*)object)->m_params.m_diameter / 2) / theApp.m_program->m_units) << "\n";
					((CCuttingTool*)object)->m_params.m_flat_radius = ((CCuttingTool*)object)->m_params.m_diameter / 2;
				} // End if- then

				if (((CCuttingTool*)object)->m_params.m_cutting_edge_angle != 0) l_ossChange << "Changing cutting edge angle to zero degrees\n";
				((CCuttingTool*)object)->m_params.m_cutting_edge_angle = 0;

				if (((CCuttingTool*)object)->m_params.m_cutting_edge_height != ((CCuttingTool*)object)->m_params.m_diameter * 3.0)
				{
					l_ossChange << "Changing cutting edge height to " << ((CCuttingTool*)object)->m_params.m_diameter / theApp.m_program->m_units * 3.0 << "\n";
					((CCuttingTool*)object)->m_params.m_cutting_edge_height = ((CCuttingTool*)object)->m_params.m_diameter * 3.0;
				} // End if - then

				l_ossChange << ((CCuttingTool*) object)->ResetTitle().c_str();
				break;

		case CCuttingToolParams::eBallEndMill:
				if (((CCuttingTool*)object)->m_params.m_corner_radius != (((CCuttingTool*)object)->m_params.m_diameter / 2)) 
				{
					l_ossChange << "Changing corner radius to " << ((((CCuttingTool*)object)->m_params.m_diameter / 2) / theApp.m_program->m_units) << "\n";
					((CCuttingTool*)object)->m_params.m_corner_radius = (((CCuttingTool*)object)->m_params.m_diameter / 2);
				} // End if - then

				if (((CCuttingTool*)object)->m_params.m_flat_radius != 0) l_ossChange << "Changing flat radius to zero\n";
				((CCuttingTool*)object)->m_params.m_flat_radius = 0;

				if (((CCuttingTool*)object)->m_params.m_cutting_edge_angle != 0) l_ossChange << "Changing cutting edge angle to zero degrees\n";
				((CCuttingTool*)object)->m_params.m_cutting_edge_angle = 0;

				if (((CCuttingTool*)object)->m_params.m_cutting_edge_height != ((CCuttingTool*)object)->m_params.m_diameter * 3.0)
				{
					l_ossChange << "Changing cutting edge height to " << ((CCuttingTool*)object)->m_params.m_diameter / theApp.m_program->m_units * 3.0 << "\n";
					((CCuttingTool*)object)->m_params.m_cutting_edge_height = ((CCuttingTool*)object)->m_params.m_diameter * 3.0;
				} // End if - then

				l_ossChange << ((CCuttingTool*) object)->ResetTitle().c_str();
				break;

		case CCuttingToolParams::eChamfer:
				if (((CCuttingTool*)object)->m_params.m_corner_radius != 0) l_ossChange << "Changing corner radius to zero\n";
				((CCuttingTool*)object)->m_params.m_corner_radius = 0;

				if (((CCuttingTool*)object)->m_params.m_flat_radius != 0) l_ossChange << "Changing flat radius to zero (this may need to be reset)\n";
				((CCuttingTool*)object)->m_params.m_flat_radius = 0;

				if (((CCuttingTool*)object)->m_params.m_cutting_edge_angle != 45) l_ossChange << "Changing cutting edge angle to 45 degrees\n";
				((CCuttingTool*)object)->m_params.m_cutting_edge_angle = 45;

				height = (((CCuttingTool*)object)->m_params.m_diameter / 2.0) * tan( 90.0 - ((CCuttingTool*)object)->m_params.m_cutting_edge_angle);
				if (((CCuttingTool*)object)->m_params.m_cutting_edge_height != height)
				{
					l_ossChange << "Changing cutting edge height to " << height / theApp.m_program->m_units << "\n";
					((CCuttingTool*)object)->m_params.m_cutting_edge_height = height;
				} // End if - then

				l_ossChange << ((CCuttingTool*) object)->ResetTitle().c_str();
				break;

		default:
				wxMessageBox(_T("That is not a valid cutting tool type. Aborting value change."));
				return;
	} // End switch

	if (l_ossChange.str().size() > 0)
	{
		wxMessageBox( wxString( l_ossChange.str().c_str() ).c_str() );
	} // End if - then
} // End on_set_type() method

static void on_set_corner_radius(double value, HeeksObj* object){((CCuttingTool*)object)->m_params.m_corner_radius = value;}
static void on_set_flat_radius(double value, HeeksObj* object){((CCuttingTool*)object)->m_params.m_flat_radius = value;}
static void on_set_cutting_edge_angle(double value, HeeksObj* object){((CCuttingTool*)object)->m_params.m_cutting_edge_angle = value;}
static void on_set_cutting_edge_height(double value, HeeksObj* object){((CCuttingTool*)object)->m_params.m_cutting_edge_height = value;}


void CCuttingToolParams::GetProperties(CCuttingTool* parent, std::list<Property *> *list)
{
	list->push_back(new PropertyLength(_("diameter"), m_diameter, parent, on_set_diameter));
	list->push_back(new PropertyLength(_("x_offset"), m_x_offset, parent, on_set_x_offset));
	list->push_back(new PropertyLength(_("tool_length_offset"), m_tool_length_offset, parent, on_set_tool_length_offset));

	{
                std::list< wxString > choices;
                choices.push_back(_("(0) Unused"));
                choices.push_back(_("(1) Turning/Back Facing"));
                choices.push_back(_("(2) Turning/Facing"));
                choices.push_back(_("(3) Boring/Facing"));
                choices.push_back(_("(4) Boring/Back Facing"));
                choices.push_back(_("(5) Back Facing"));
                choices.push_back(_("(6) Turning"));
                choices.push_back(_("(7) Facing"));
                choices.push_back(_("(8) Boring"));
                choices.push_back(_("(9) Centre"));
                int choice = int(m_orientation);
                list->push_back(new PropertyChoice(_("orientation"), choices, choice, parent, on_set_orientation));
        }

	{
                std::list< wxString > choices;
                choices.push_back(_("Drill bit"));
                choices.push_back(_("Centre Drill bit"));
                choices.push_back(_("End Mill"));
                choices.push_back(_("Slot Cutter"));
                choices.push_back(_("Ball End Mill"));
                choices.push_back(_("Chamfering bit"));
                int choice = int(m_type);
                list->push_back(new PropertyChoice(_("type"), choices, choice, parent, on_set_type));
        }

	list->push_back(new PropertyLength(_("flat_radius"), m_flat_radius, parent, on_set_flat_radius));
	list->push_back(new PropertyLength(_("corner_radius"), m_corner_radius, parent, on_set_corner_radius));
	list->push_back(new PropertyDouble(_("cutting_edge_angle"), m_cutting_edge_angle, parent, on_set_cutting_edge_angle));
	list->push_back(new PropertyDouble(_("cutting_edge_height"), m_cutting_edge_height, parent, on_set_cutting_edge_height));
}

void CCuttingToolParams::WriteXMLAttributes(TiXmlNode *root)
{
	TiXmlElement * element;
	element = new TiXmlElement( "params" );
	root->LinkEndChild( element );  

	element->SetDoubleAttribute("diameter", m_diameter);
	element->SetDoubleAttribute("x_offset", m_x_offset);
	element->SetDoubleAttribute("tool_length_offset", m_tool_length_offset);
	
	std::ostringstream l_ossValue;
	l_ossValue.str(""); l_ossValue << m_orientation;
	element->SetAttribute("orientation", l_ossValue.str().c_str() );

	l_ossValue.str(""); l_ossValue << int(m_type);
	element->SetAttribute("type", l_ossValue.str().c_str() );

	element->SetDoubleAttribute("corner_radius", m_corner_radius);
	element->SetDoubleAttribute("flat_radius", m_flat_radius);
	element->SetDoubleAttribute("cutting_edge_angle", m_cutting_edge_angle);
	element->SetDoubleAttribute("cutting_edge_height", m_cutting_edge_height);
}

void CCuttingToolParams::ReadParametersFromXMLElement(TiXmlElement* pElem)
{
	if (pElem->Attribute("diameter")) m_diameter = atof(pElem->Attribute("diameter"));
	if (pElem->Attribute("x_offset")) m_x_offset = atof(pElem->Attribute("x_offset"));
	if (pElem->Attribute("tool_length_offset")) m_tool_length_offset = atof(pElem->Attribute("tool_length_offset"));
	if (pElem->Attribute("orientation")) m_orientation = atoi(pElem->Attribute("orientation"));
	if (pElem->Attribute("type")) m_type = CCuttingToolParams::eCuttingToolType(atoi(pElem->Attribute("type")));
	if (pElem->Attribute("corner_radius")) m_corner_radius = atof(pElem->Attribute("corner_radius"));
	if (pElem->Attribute("flat_radius")) m_flat_radius = atof(pElem->Attribute("flat_radius"));
	if (pElem->Attribute("cutting_edge_angle")) m_cutting_edge_angle = atof(pElem->Attribute("cutting_edge_angle"));
	if (pElem->Attribute("cutting_edge_height"))
	{
		m_cutting_edge_height = atof(pElem->Attribute("cutting_edge_height"));
	} // End if - then
	else
	{
		m_cutting_edge_height = m_diameter * 4.0;
	} // End if - else
}

/**
	This method is called when the CAD operator presses the Python button.  This method generates
	Python source code whose job will be to generate RS-274 GCode.  It's done in two steps so that
	the Python code can be configured to generate GCode suitable for various CNC interpreters.
 */
void CCuttingTool::AppendTextToProgram()
{

#ifdef UNICODE
	std::wostringstream ss;
#else
    std::ostringstream ss;
#endif
    ss.imbue(std::locale("C"));

	// The G10 command can be used (within EMC2) to add a tool to the tool
        // table from within a program.
        // G10 L1 P[tool number] R[radius] X[offset] Z[offset] Q[orientation]
	//
	// The radius value must be expressed in MACHINE CONFIGURATION UNITS.  This may be different
	// to this model's drawing units.  The value is interpreted, at lease for EMC2, in terms
	// of the units setup for the machine's configuration (something.ini in EMC2 parlence).  At
	// the moment we don't have a MACHINE CONFIGURATION UNITS parameter so we've got a 50%
	// chance of getting it right.

	if (m_title.size() > 0)
	{
		ss << "#(" << m_title.c_str() << ")\n";
	} // End if - then

	ss << "tool_defn( id=" << m_tool_number << ", ";

	if (m_title.size() > 0)
	{
		ss << "name='" << m_title.c_str() << "\', ";
	} // End if - then
	else
	{
		ss << "name=None, ";
	} // End if - else

	if (m_params.m_diameter > 0)
	{
		ss << "radius=" << m_params.m_diameter / 2 /theApp.m_program->m_units << ", ";
	} // End if - then
	else
	{
		ss << "radius=None, ";
	} // End if - else

	if (m_params.m_tool_length_offset > 0)
	{
		ss << "length=" << m_params.m_tool_length_offset /theApp.m_program->m_units;
	} // End if - then
	else
	{
		ss << "length=None";
	} // End if - else
	
	ss << ")\n";

	theApp.m_program_canvas->m_textCtrl->AppendText(ss.str().c_str());
}


static void on_set_tool_number(const int value, HeeksObj* object){((CCuttingTool*)object)->m_tool_number = value;}

/**
	NOTE: The m_title member is a special case.  The HeeksObj code looks for a 'GetShortString()' method.  If found, it
	adds a Property called 'Object Title'.  If the value is changed, it tries to call the 'OnEditString()' method.
	That's why the m_title value is not defined here
 */
void CCuttingTool::GetProperties(std::list<Property *> *list)
{
	list->push_back(new PropertyInt(_("tool_number"), m_tool_number, this, on_set_tool_number));

	m_params.GetProperties(this, list);
	HeeksObj::GetProperties(list);
}


HeeksObj *CCuttingTool::MakeACopy(void)const
{
	return new CCuttingTool(*this);
}

void CCuttingTool::CopyFrom(const HeeksObj* object)
{
	operator=(*((CCuttingTool*)object));
}

bool CCuttingTool::CanAddTo(HeeksObj* owner)
{
	return owner->GetType() == ToolsType;
}

void CCuttingTool::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element = new TiXmlElement( "CuttingTool" );
	root->LinkEndChild( element );  
	element->SetAttribute("title", Ttc(m_title.c_str()));

	std::ostringstream l_ossValue;
	l_ossValue.str(""); l_ossValue << m_tool_number;
	element->SetAttribute("tool_number", l_ossValue.str().c_str() );

	m_params.WriteXMLAttributes(element);
	WriteBaseXML(element);
}

// static member function
HeeksObj* CCuttingTool::ReadFromXMLElement(TiXmlElement* element)
{

	int tool_number = 0;
	if (element->Attribute("tool_number")) tool_number = atoi(element->Attribute("tool_number"));

	wxString title(Ctt(element->Attribute("title")));
	CCuttingTool* new_object = new CCuttingTool( title.c_str(), tool_number);

	// read point and circle ids
	for(TiXmlElement* pElem = TiXmlHandle(element).FirstChildElement().Element(); pElem; pElem = pElem->NextSiblingElement())
	{
		std::string name(pElem->Value());
		if(name == "params"){
			new_object->m_params.ReadParametersFromXMLElement(pElem);
		}
	}

	new_object->ReadBaseXML(element);

	return new_object;
}


void CCuttingTool::OnEditString(const wxChar* str){
        m_title.assign(str);
	heeksCAD->WasModified(this);
}

/**
 * Find the CuttingTool object whose tool number matches that passed in.
 */
int CCuttingTool::FindCuttingTool( const int tool_number )
{
	CHeeksCNCApp::Symbols_t all_symbols = CHeeksCNCApp::GetAllSymbols();
	for (CHeeksCNCApp::Symbols_t::const_iterator l_itSymbol = all_symbols.begin(); l_itSymbol != all_symbols.end(); l_itSymbol++)
	{
                if (l_itSymbol->first != CuttingToolType) continue;

		HeeksObj *ob = heeksCAD->GetIDObject( l_itSymbol->first, l_itSymbol->second );
                if ((ob != NULL) && (((CCuttingTool *) ob)->m_tool_number == tool_number))
                {
                        return(ob->m_id);
                } // End if - then
        } // End for

        return(-1);

} // End FindCuttingTool() method


std::vector< std::pair< int, wxString > > CCuttingTool::FindAllCuttingTools()
{
	std::vector< std::pair< int, wxString > > tools;

	// Always add a value of zero to allow for an absense of cutting tool use.
	tools.push_back( std::make_pair(0, _T("No Cutting Tool") ) );
	
	CHeeksCNCApp::Symbols_t all_symbols = CHeeksCNCApp::GetAllSymbols();
	for (CHeeksCNCApp::Symbols_t::const_iterator l_itSymbol = all_symbols.begin(); l_itSymbol != all_symbols.end(); l_itSymbol++)
	{
                if (l_itSymbol->first != CuttingToolType) continue;

		HeeksObj *ob = heeksCAD->GetIDObject( l_itSymbol->first, l_itSymbol->second );
                if (ob != NULL)
                {
			tools.push_back( std::make_pair(((CCuttingTool *) ob)->m_tool_number, ((CCuttingTool*) ob)->GetShortString() ) );
                } // End if - then
        } // End for

        return(tools);

} // End FindCuttingTool() method



/**
	Find a fraction that represents this floating point number.  We use this
	purely for readability purposes.  It only looks accurate to the nearest 1/64th

	eg: 0.125 -> "1/8"
	    1.125 -> "1 1/8"
	    0.109375 -> "7/64"
 */
wxString CCuttingTool::FractionalRepresentation( const double original_value, const int max_denominator /* = 64 */ ) const
{
#ifdef UNICODE
	std::wostringstream l_ossValue;
#else
    std::ostringstream l_ossValue;
#endif

	double _value(original_value);
	// double near_enough = double(double(1.0) / (2.0 * double(max_denominator)));
	double near_enough = 0.00001;

	if (floor(_value) > 0)
	{
		l_ossValue << floor(_value) << " ";
		_value -= floor(_value);
	} // End if - then

	// We only want even numbers between 2 and 64 for the denominator.  The others just look 'wierd'.
	for (int denominator = 2; denominator <= max_denominator; denominator *= 2)
	{
		for (int numerator = 1; numerator < denominator; numerator++)
		{
			double fraction = double( double(numerator) / double(denominator) );
			if ( ((_value > fraction) && ((_value - fraction) < near_enough)) ||
			     ((_value < fraction) && ((fraction - _value) < near_enough)) ||
			     (_value == fraction) )
			{
				l_ossValue << numerator << "/" << denominator;
				return(l_ossValue.str().c_str());
			} // End if - then
		} // End for
	} // End for

	l_ossValue.str(_T(""));	// Delete any floor(value) data we had before.
	l_ossValue << original_value;
	return(l_ossValue.str().c_str());
} // End FractionalRepresentation() method


/**
 * This method uses the various attributes of the cutting tool to produce a meaningful name.
 * eg: with diameter = 6, units = 1 (mm) and type = 'drill' the name would be '6mm Drill Bit".  The
 * idea is to produce a m_title value that is representative of the cutting tool.  This will
 * make selection in the program list easier.
 *
 * NOTE: The ResetTitle() method looks at the m_title value for strings that are likely to
 * have come from this method.  If this method changes, the ResetTitle() method may also
 * need to change.
 */
wxString CCuttingTool::GenerateMeaningfulName() const
{
#ifdef UNICODE
	std::wostringstream l_ossName;
#else
    std::ostringstream l_ossName;
#endif
	
	if (theApp.m_program->m_units == 1)
	{
		// We're using metric.  Leave the diameter as a floating point number.  It just looks more natural.
		l_ossName << m_params.m_diameter / theApp.m_program->m_units << " mm ";
	} // End if - then
	else
	{	
		// We're using inches.  Find a fractional representation if one matches.
		l_ossName << FractionalRepresentation(m_params.m_diameter / theApp.m_program->m_units).c_str() << " inch ";
	} // End if - else

	switch (m_params.m_type)
	{
		case CCuttingToolParams::eDrill:	l_ossName << "Drill Bit";
							break;

		case CCuttingToolParams::eCentreDrill:	l_ossName << "Centre Drill Bit";
							break;

                case CCuttingToolParams::eEndmill:	l_ossName << "End Mill";
							break;

                case CCuttingToolParams::eSlotCutter:	l_ossName << "Slot Cutter";
							break;

                case CCuttingToolParams::eBallEndMill:	l_ossName << "Ball End Mill";
							break;

                case CCuttingToolParams::eChamfer:	l_ossName.str(_T(""));	// Remove all that we've already prepared.
							l_ossName << m_params.m_cutting_edge_angle << " degreee ";
                					l_ossName << "Chamfering Bit";
		default:				break;
	} // End switch

	return( l_ossName.str().c_str() );
} // End GenerateMeaningfulName() method


/**
	Reset the m_title value with a meaningful name ONLY if it does not look like it was
	automatically generated in the first place.  If someone has reset it manually then leave it alone.

	Return a verbose description of what we've done (if anything) so that we can pop up a
	warning message to the operator letting them know.
 */
wxString CCuttingTool::ResetTitle()
{
#ifdef UNICODE
	std::wostringstream l_ossUnits;
#else
    std::ostringstream l_ossUnits;
#endif

	l_ossUnits << (char *) ((theApp.m_program->m_units == 1)?" mm ":" inch ");

	if ( (m_title == GetTypeString()) ||
	     ((m_title.Find( _T("Drill Bit") ) != -1) && (m_title.Find( l_ossUnits.str().c_str() ) != -1)) ||
	     ((m_title.Find( _T("End Mill") ) != -1) && (m_title.Find( l_ossUnits.str().c_str() ) != -1)) ||
	     ((m_title.Find( _T("Slot Cutter") ) != -1) && (m_title.Find( l_ossUnits.str().c_str() ) != -1)) ||
	     ((m_title.Find( _T("Ball End Mill") ) != -1) && (m_title.Find( l_ossUnits.str().c_str() ) != -1)) ||
	     ((m_title.Find( _T("Chamfering Bit") ) != -1) && (m_title.Find(_T("degree")) != -1)) )
	{
		// It has the default title.  Give it a name that makes sense.
		m_title = GenerateMeaningfulName();
		heeksCAD->WasModified(this);

#ifdef UNICODE
		std::wostringstream l_ossChange;
#else
		std::ostringstream l_ossChange;
#endif
		l_ossChange << "Changing name to " << m_title.c_str() << "\n";
		return( l_ossChange.str().c_str() );
	} // End if - then

	// Nothing changed, nothing to report
	return(_T(""));
} // End ResetTitle() method



/**
        This is the Graphics Library Commands (from the OpenGL set).  This method calls the OpenGL
        routines to paint the cutting tool in the graphics window.  The graphics is transient.

	We want to draw an outline of the cutting tool in 2 dimensions so that the operator
	gets a feel for what the various cutting tool parameter values mean.
 */
void CCuttingTool::glCommands(bool select, bool marked, bool no_color)
{
        if(marked && !no_color)
        {
                // Draw the outline of the bit.

		double ShaftLength = m_params.m_tool_length_offset;

		if (m_params.m_cutting_edge_angle > 0)
		{
			ShaftLength -= ((m_params.m_diameter / 2) * tan( 90 - m_params.m_cutting_edge_angle));
		} // End if - then

		glBegin(GL_LINE_STRIP);
		glVertex3d( -1 * ( m_params.m_diameter / 2), -1 * (ShaftLength / 2), 0 );
		glVertex3d( -1 * ( m_params.m_diameter / 2), +1 * (ShaftLength / 2), 0 );
		glVertex3d( +1 * ( m_params.m_diameter / 2), +1 * (ShaftLength / 2), 0 );
		glVertex3d( +1 * ( m_params.m_diameter / 2), -1 * (ShaftLength / 2), 0 );
		glEnd();

		// Draw the cutting edge.
		if (m_params.m_cutting_edge_angle > 0)
		{
			// It has a taper at the bottom.
			
			glBegin(GL_LINE_STRIP);
			glVertex3d( -1 * ( m_params.m_diameter / 2), -1 * (ShaftLength / 2), 0 );
			glVertex3d( 0, -1 * (m_params.m_tool_length_offset / 2), 0 );
			glVertex3d( +1 * ( m_params.m_diameter / 2), -1 * (ShaftLength / 2), 0 );
			glEnd();
		} // End if - then
		else
		{
			// It's either a normal endmill or a ball-endmill.
			if ((m_params.m_flat_radius == 0) && (m_params.m_corner_radius == m_params.m_diameter / 2 ))
			{
				// It's a ball-endmill.

				unsigned int numPoints = 10;	
				double alpha = 3.1415926 * 2 / numPoints;

				glBegin(GL_LINE_STRIP);
				unsigned int i = (numPoints / 2) - 1;
				while( i++ < numPoints )
				{
					double theta = alpha * i;
					glVertex3d( cos( theta ) * (m_params.m_diameter / 2), (sin( theta ) * (m_params.m_diameter / 2) - (ShaftLength / 2)), 0 );
				} // End while
				glEnd();
			} // End if - then
			else
			{
				// it's a normal endmill.
				
				glBegin(GL_LINE_STRIP);
				glVertex3d( -1 * ( m_params.m_diameter / 2), -1 * (ShaftLength / 2), 0 );
				glVertex3d( +1 * ( m_params.m_diameter / 2), -1 * (ShaftLength / 2), 0 );
				glEnd();
			} // End if - else
		} // End if - else
	} // End if - then

} // End glCommands() method


