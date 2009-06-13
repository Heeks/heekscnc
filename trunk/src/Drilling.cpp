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
#include "interface/PropertyChoice.h"
#include "tinyxml/tinyxml.h"
#include "CuttingTool.h"

#include <sstream>

extern CHeeksCADInterface* heeksCAD;


void CDrillingParams::set_initial_values( const CDrilling::Symbols_t & cuttingTools )
{
	CNCConfig config;
	config.Read(_T("m_standoff"), &m_standoff, 5.0);
	config.Read(_T("m_dwell"), &m_dwell, 1);
	config.Read(_T("m_depth"), &m_depth, 21);
	config.Read(_T("m_peck_depth"), &m_peck_depth, 3);

	config.Read(_T("m_cutting_tool_number"), &m_cutting_tool_number, 0);

	// Override this value if we've been given one.
	if (cuttingTools.size() > 0)
	{
		HeeksObj* object = heeksCAD->GetIDObject(CuttingToolType, cuttingTools.begin()->second );
		if (object != NULL)
		{
			m_cutting_tool_number = ((CCuttingTool *) object)->m_params.m_tool_number;
		} // End if - then
		else
		{
			std::wostringstream l_ossMsg;
			l_ossMsg << "Could not find existing cutting tool with an id='" << cuttingTools.begin()->second << "'";
			wxMessageBox(l_ossMsg.str().c_str());
		} // End if - else
	} // End if - then
}

void CDrillingParams::write_values_to_config()
{
	CNCConfig config;
	config.Write(_T("m_standoff"), m_standoff);
	config.Write(_T("m_dwell"), m_dwell);
	config.Write(_T("m_depth"), m_depth);
	config.Write(_T("m_peck_depth"), m_peck_depth);
	config.Write(_T("m_cutting_tool_number"), m_cutting_tool_number);
}

static void on_set_standoff(double value, HeeksObj* object){((CDrilling*)object)->m_params.m_standoff = value;}
static void on_set_dwell(double value, HeeksObj* object){((CDrilling*)object)->m_params.m_dwell = value;}
static void on_set_depth(double value, HeeksObj* object){((CDrilling*)object)->m_params.m_depth = value;}
static void on_set_peck_depth(double value, HeeksObj* object){((CDrilling*)object)->m_params.m_peck_depth = value;}
static void on_set_cutting_tool_number(int value, HeeksObj* object)
	{
		((CDrilling*)object)->m_params.m_cutting_tool_number = value;

		HeeksObj* cuttingTool = heeksCAD->GetIDObject(CuttingToolType, value);
		if (cuttingTool != NULL)
		{
			wxMessageBox(_T("That's just fine."));
		}
		else
		{
			wxMessageBox(_T("This tool number has not been defined as a cutting tool yet."));
		} // End if - else
	} // End on_set_cutting_tool_number() routine



void CDrillingParams::GetProperties(CDrilling* parent, std::list<Property *> *list)
{
	list->push_back(new PropertyDouble(_("standoff"), m_standoff, parent, on_set_standoff));
	list->push_back(new PropertyDouble(_("dwell"), m_dwell, parent, on_set_dwell));
	list->push_back(new PropertyDouble(_("depth"), m_depth, parent, on_set_depth));
	list->push_back(new PropertyDouble(_("peck_depth"), m_peck_depth, parent, on_set_peck_depth));
	list->push_back(new PropertyInt(_("cutting_tool_number"), m_cutting_tool_number, parent, on_set_cutting_tool_number));
}

void CDrillingParams::WriteXMLAttributes(TiXmlNode *root)
{
	TiXmlElement * element;
	element = new TiXmlElement( "params" );
	root->LinkEndChild( element );  
	element->SetDoubleAttribute("standoff", m_standoff);
	element->SetDoubleAttribute("dwell", m_dwell);
	element->SetDoubleAttribute("depth", m_depth);
	element->SetDoubleAttribute("peck_depth", m_peck_depth);

	std::ostringstream l_ossValue;
	l_ossValue.str(""); l_ossValue << m_cutting_tool_number;
	element->SetAttribute("cutting_tool_number", l_ossValue.str().c_str() );
}

void CDrillingParams::ReadParametersFromXMLElement(TiXmlElement* pElem)
{
	if (pElem->Attribute("standoff")) m_standoff = atof(pElem->Attribute("standoff"));
	if (pElem->Attribute("dwell")) m_dwell = atof(pElem->Attribute("dwell"));
	if (pElem->Attribute("depth")) m_depth = atof(pElem->Attribute("depth"));
	if (pElem->Attribute("peck_depth")) m_peck_depth = atof(pElem->Attribute("peck_depth"));
	if (pElem->Attribute("cutting_tool_number")) m_cutting_tool_number = atof(pElem->Attribute("cutting_tool_number"));
}

/**
	This method is called when the CAD operator presses the Python button.  This method generates
	Python source code whose job will be to generate RS-274 GCode.  It's done in two steps so that
	the Python code can be configured to generate GCode suitable for various CNC interpreters.
 */
void CDrilling::AppendTextToProgram()
{
	COp::AppendTextToProgram();

#ifdef UNICODE
	std::wostringstream ss;
#else
    std::ostringstream ss;
#endif
    ss.imbue(std::locale("C"));

	if (m_symbols.size() > 0)
	{
		// Select the right tool.

		ss << "tool_change( id=" << m_params.m_cutting_tool_number << ")\n";
	} // End if - then
	
	std::list<HeeksObj*> points;
	for(Symbols_t::iterator l_itSymbol = m_symbols.begin(); l_itSymbol != m_symbols.end(); l_itSymbol++)
	{
		SymbolType_t symbol_type = l_itSymbol->first;
		SymbolId_t symbol_id = l_itSymbol->second;

		if (symbol_type == PointType)
		{
			HeeksObj* object = heeksCAD->GetIDObject(symbol_type, symbol_id);
			double pos[3];

			object->GetStartPoint(pos);
			ss << "drill("
				<< "x=" << pos[0] << ", "
				<< "y=" << pos[1] << ", "
				<< "z=" << pos[2] << ", "
				<< "depth=" << m_params.m_depth << ", "
				<< "standoff=" << m_params.m_standoff << ", "
				<< "dwell=" << m_params.m_dwell << ", "
				<< "peck_depth=" << m_params.m_peck_depth // << ", "
				<< ")\n";
		} // End if - then
	} // End for

	theApp.m_program_canvas->m_textCtrl->AppendText(ss.str().c_str());
}


/**
	This routine generates a list of coordinates around the circumference of a circle.  It's just used
	to generate data suitable for OpenGL calls to paint a circle.  This graphics is transient but will
	help represent what the GCode will be doing when it's generated.
 */
std::list< CDrilling::Point3d > CDrilling::PointsAround( const CDrilling::Point3d & origin, const double radius, const unsigned int numPoints ) const
{
	std::list<CDrilling::Point3d> results;

	double alpha = 3.1415926 * 2 / numPoints;

	unsigned int i = 0;
	while( i++ < numPoints )
	{
		double theta = alpha * i;
		CDrilling::Point3d pointOnCircle( cos( theta ) * radius, sin( theta ) * radius, 0 );

		pointOnCircle.x += origin.x;
		pointOnCircle.y += origin.y;
		pointOnCircle.z += origin.z;

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

std::list< CDrilling::Point3d > CDrilling::DrillBitVertices( const CDrilling::Point3d & origin, const double radius, const double length ) const
{
	std::list<CDrilling::Point3d> top, spiral, bottom, countersink, result;

	double flutePitch = 5.0;	// 5mm of depth per spiral of the drill bit's flute.
	double countersinkDepth = 3;	// this is the depth of the countersink cone at the end of the drill bit.
	unsigned int numPoints = 20;	// number of points in one circle (360 degrees) i.e. how smooth do we want the graphics
	const double pi = 3.1415926;
	double alpha = 2 * pi / numPoints;

	// Get a circle at the top of the dill bit's path
	top = PointsAround( origin, radius, numPoints );
	top.push_back( *(top.begin()) );	// Close the circle

	double depthPerItteration;
	countersinkDepth = length / 10;	// 10% of the full length instead.

	unsigned int l_iNumItterations = numPoints * (length / flutePitch);
	depthPerItteration = (length - countersinkDepth) / l_iNumItterations;

	// Now generate the spirals.
	
	unsigned int i = 0;
	while( i++ < l_iNumItterations )
	{
		double theta = alpha * i;
		CDrilling::Point3d pointOnCircle( cos( theta ) * radius, sin( theta ) * radius, 0 );

		pointOnCircle.x += origin.x;
		pointOnCircle.y += origin.y;
		pointOnCircle.z += origin.z;
		
		// And spiral down as we go.
		pointOnCircle.z -= (depthPerItteration * i);

		spiral.push_back(pointOnCircle);
	} // End while

	// And now the countersink at the bottom of the drill bit.
	i = 0;
	while( i++ < numPoints )
	{
		double theta = alpha * i;
		CDrilling::Point3d topEdge( cos( theta ) * radius, sin( theta ) * radius, 0 );

		// This is at the top edge of the countersink
		topEdge.x += origin.x;
		topEdge.y += origin.y;
		topEdge.z = origin.z - (length - countersinkDepth);
		spiral.push_back(topEdge);

		// And now at the very point of the countersink
		CDrilling::Point3d veryTip( origin );
		veryTip.x = origin.x;
		veryTip.y = origin.y;
		veryTip.z = (origin.z - length);

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
	if(marked && !no_color)
	{
		for (Symbols_t::const_iterator l_itSymbol = m_symbols.begin(); l_itSymbol != m_symbols.end(); l_itSymbol++)
		{
			HeeksObj* object = heeksCAD->GetIDObject(l_itSymbol->first, l_itSymbol->second);

			// If we found something, ask its CAD code to draw itself highlighted.
			if(object)object->glCommands(false, true, false);

			double l_dHoleDiameter = 10;	// Default.
			if (m_params.m_cutting_tool_number > 0)
			{
				HeeksObj* cuttingTool = heeksCAD->GetIDObject( CuttingToolType, m_params.m_cutting_tool_number );
				if (cuttingTool != NULL)
				{
					l_dHoleDiameter = ((CCuttingTool *) cuttingTool)->m_params.m_diameter;
				} // End if - then
			} // End if - then

			// If it's a point then draw a Drill Cycle picture so that we can imagine what's going to happen.
			if (l_itSymbol->first == PointType)
			{ 
				GLdouble start[3], end[3];

				object->GetStartPoint(start);
				object->GetStartPoint(end);
		
				end[2] -= m_params.m_depth;
					
				glBegin(GL_LINE_STRIP);
				glVertex3dv( start );
				glVertex3dv( end );
				glEnd();

				std::list< CDrilling::Point3d > pointsAroundCircle = DrillBitVertices( 	CDrilling::Point3d( start[0], start[1], start[2] ), 
													l_dHoleDiameter / 2, 
													m_params.m_depth);

				glBegin(GL_LINE_STRIP);
				CDrilling::Point3d previous = *(pointsAroundCircle.begin());
				for (std::list< CDrilling::Point3d >::const_iterator l_itPoint = pointsAroundCircle.begin();
					l_itPoint != pointsAroundCircle.end();
					l_itPoint++)
				{
					
					glVertex3d( l_itPoint->x, l_itPoint->y, l_itPoint->z );
				}
				glEnd();
			} // End if - then
		} // End for
	} // End if - then
}


void CDrilling::GetProperties(std::list<Property *> *list)
{
	m_params.GetProperties(this, list);
	HeeksObj::GetProperties(list);
}

HeeksObj *CDrilling::MakeACopy(void)const
{
	return new CDrilling(*this);
}

void CDrilling::CopyFrom(const HeeksObj* object)
{
	operator=(*((CDrilling*)object));
}

bool CDrilling::CanAddTo(HeeksObj* owner)
{
	return owner->GetType() == OperationsType;
}

void CDrilling::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element = new TiXmlElement( "Drilling" );
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
HeeksObj* CDrilling::ReadFromXMLElement(TiXmlElement* element)
{
	CDrilling* new_object = new CDrilling;

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
		}
	}

	new_object->ReadBaseXML(element);

	return new_object;
}


