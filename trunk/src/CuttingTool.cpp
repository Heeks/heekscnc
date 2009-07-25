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
#include "interface/HeeksColor.h"
#include "interface/PropertyInt.h"
#include "interface/PropertyDouble.h"
#include "interface/PropertyLength.h"
#include "interface/PropertyChoice.h"
#include "interface/PropertyString.h"
#include "tinyxml/tinyxml.h"

#include <sstream>
#include <string>
#include <algorithm>

#include <gp_Pnt.hxx>

#include <TopoDS.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Solid.hxx>
#include <TopoDS_Compound.hxx>

#include <TopExp_Explorer.hxx>

#include <BRep_Tool.hxx>
#include <BRepLib.hxx>

#include <GCE2d_MakeSegment.hxx>

#include <Handle_Geom_TrimmedCurve.hxx>
#include <Handle_Geom_CylindricalSurface.hxx>
#include <Handle_Geom2d_TrimmedCurve.hxx>
#include <Handle_Geom2d_Ellipse.hxx>

#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeSolid.hxx>
#include <BRepBuilderAPI_Transform.hxx>

#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeRevolution.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>

#include <BRepPrim_Cylinder.hxx>
#include <BRepPrim_Cone.hxx>

#include <BRepFilletAPI_MakeFillet.hxx>

#include <BRepOffsetAPI_MakeThickSolid.hxx>
#include <BRepOffsetAPI_ThruSections.hxx>

#include <GC_MakeArcOfCircle.hxx>
#include <GC_MakeSegment.hxx>

#include <BRepAlgoAPI_Fuse.hxx>

#include <Geom_Surface.hxx>
#include <Geom_Plane.hxx>

extern CHeeksCADInterface* heeksCAD;


static void ResetParametersToReasonableValues(HeeksObj* object);

void CCuttingToolParams::set_initial_values()
{
	CNCConfig config;
	config.Read(_T("m_material"), &m_material, int(eCarbide));
	config.Read(_T("m_diameter"), &m_diameter, 12.7);
	config.Read(_T("m_tool_length_offset"), &m_tool_length_offset, (10 * m_diameter));
	config.Read(_T("m_max_advance_per_revolution"), &m_max_advance_per_revolution, 0.12 );	// mm

	config.Read(_T("m_type"), (int *) &m_type, eDrill);
	config.Read(_T("m_flat_radius"), &m_flat_radius, 0);
	config.Read(_T("m_corner_radius"), &m_corner_radius, 0);
	config.Read(_T("m_cutting_edge_angle"), &m_cutting_edge_angle, 59);
	config.Read(_T("m_cutting_edge_height"), &m_cutting_edge_height, 4 * m_diameter/ theApp.m_program->m_units);

	// The following are all turning tool parameters
	config.Read(_T("m_orientation"), &m_orientation, 6);
	config.Read(_T("m_x_offset"), &m_x_offset, 0);
	config.Read(_T("m_front_angle"), &m_front_angle, 95);
	config.Read(_T("m_tool_angle"), &m_tool_angle, 60);
	config.Read(_T("m_back_angle"), &m_back_angle, 25);


}

void CCuttingToolParams::write_values_to_config()
{
	CNCConfig config;

	// We ALWAYS write the parameters into the configuration file in mm (for consistency).
	// If we're now in inches then convert the values.
	// We're in mm already.
	config.Write(_T("m_material"), m_material);
	config.Write(_T("m_diameter"), m_diameter);
	config.Write(_T("m_x_offset"), m_x_offset);
	config.Write(_T("m_tool_length_offset"), m_tool_length_offset);
	config.Write(_T("m_orientation"), m_orientation);
	config.Write(_T("m_max_advance_per_revolution"), m_max_advance_per_revolution );

	config.Write(_T("m_type"), m_type);
	config.Write(_T("m_flat_radius"), m_flat_radius);
	config.Write(_T("m_corner_radius"), m_corner_radius);
	config.Write(_T("m_cutting_edge_angle"), m_cutting_edge_angle);
	config.Write(_T("m_cutting_edge_height"), m_cutting_edge_height);

	config.Write(_T("m_front_angle"), m_front_angle);
	config.Write(_T("m_tool_angle"), m_tool_angle);
	config.Write(_T("m_back_angle"), m_back_angle);
}

static void on_set_diameter(double value, HeeksObj* object)
{
	((CCuttingTool*)object)->m_params.m_diameter = value;
	ResetParametersToReasonableValues(object);
} // End on_set_diameter() routine

static void on_set_max_advance_per_revolution(double value, HeeksObj* object){((CCuttingTool*)object)->m_params.m_max_advance_per_revolution = value;}
static void on_set_x_offset(double value, HeeksObj* object){((CCuttingTool*)object)->m_params.m_x_offset = value;}
static void on_set_tool_length_offset(double value, HeeksObj* object){((CCuttingTool*)object)->m_params.m_tool_length_offset = value;}
static void on_set_orientation(int zero_based_choice, HeeksObj* object)
{
	if (zero_based_choice < 0) return;	// An error has occured

	if ((zero_based_choice >= 0) && (zero_based_choice <= 9))
	{
		((CCuttingTool*)object)->m_params.m_orientation = zero_based_choice;
	} // End if - then
	else
	{
		wxMessageBox(_T("Orientation values must be between 0 and 9 inclusive.  Aborting value change"));
	} // End if - else
}

static void on_set_material(int zero_based_choice, HeeksObj* object)
{
	if (zero_based_choice < 0) return;	// An error has occured.

	if ((zero_based_choice >= CCuttingToolParams::eHighSpeedSteel) && (zero_based_choice <= CCuttingToolParams::eCarbide))
	{
		((CCuttingTool*)object)->m_params.m_material = zero_based_choice;
		ResetParametersToReasonableValues(object);
		heeksCAD->RefreshProperties();
	} // End if - then
	else
	{
		wxMessageBox(_T("Cutting Tool material must be between 0 and 1. Aborting value change"));
	} // End if - else
}

static void on_set_front_angle(double value, HeeksObj* object){((CCuttingTool*)object)->m_params.m_front_angle = value;}
static void on_set_tool_angle(double value, HeeksObj* object){((CCuttingTool*)object)->m_params.m_tool_angle = value;}
static void on_set_back_angle(double value, HeeksObj* object){((CCuttingTool*)object)->m_params.m_back_angle = value;}

static void on_set_type(int zero_based_choice, HeeksObj* object)
{
	if (zero_based_choice < 0) return;	// An error has occured.

	((CCuttingTool*)object)->m_params.m_type = CCuttingToolParams::eCuttingToolType(zero_based_choice);
	ResetParametersToReasonableValues(object);
	heeksCAD->RefreshProperties();
} // End on_set_type() routine



static double degrees_to_radians( const double degrees )
{
	return( (degrees / 360) * 2 * PI );
} // End degrees_to_radians() routine


static void ResetParametersToReasonableValues(HeeksObj* object)
{
#ifdef UNICODE
	std::wostringstream l_ossChange;
#else
    std::ostringstream l_ossChange;
#endif

	if (((CCuttingTool*)object)->m_params.m_type != CCuttingToolParams::eTurningTool)
	{
		if (((CCuttingTool*)object)->m_params.m_tool_length_offset != (5 * ((CCuttingTool*)object)->m_params.m_diameter))
		{
			((CCuttingTool*)object)->m_params.m_tool_length_offset = (5 * ((CCuttingTool*)object)->m_params.m_diameter);
			l_ossChange << "Resetting tool length to " << (((CCuttingTool*)object)->m_params.m_tool_length_offset / theApp.m_program->m_units) << "\n";
		} // End if - then
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

				if (((CCuttingTool*)object)->m_params.m_cutting_edge_height != ((CCuttingTool*)object)->m_params.m_diameter/theApp.m_program->m_units * 3.0)
				{
					l_ossChange << "Changing cutting edge height to " << ((CCuttingTool*)object)->m_params.m_diameter / theApp.m_program->m_units * 3.0 << "\n";
					((CCuttingTool*)object)->m_params.m_cutting_edge_height = ((CCuttingTool*)object)->m_params.m_diameter/ theApp.m_program->m_units * 3.0;
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
					((CCuttingTool*)object)->m_params.m_cutting_edge_height = ((CCuttingTool*)object)->m_params.m_diameter / theApp.m_program->m_units* 1.0;
				} // End if - then

				l_ossChange << ((CCuttingTool*) object)->ResetTitle().c_str();
				break;

		case CCuttingToolParams::eEndmill:
				if (((CCuttingTool*)object)->m_params.m_corner_radius != 0) l_ossChange << "Changing corner radius to zero\n";
				((CCuttingTool*)object)->m_params.m_corner_radius = 0;

				if (((CCuttingTool*)object)->m_params.m_flat_radius != ( ((CCuttingTool*)object)->m_params.m_diameter / 2) )
				{
					l_ossChange << "Changing flat radius to " << ((CCuttingTool*)object)->m_params.m_diameter / 2 /theApp.m_program->m_units << "\n";
					((CCuttingTool*)object)->m_params.m_flat_radius = ((CCuttingTool*)object)->m_params.m_diameter / 2;
				} // End if - then

				if (((CCuttingTool*)object)->m_params.m_cutting_edge_angle != 0) l_ossChange << "Changing cutting edge angle to zero degrees\n";
				((CCuttingTool*)object)->m_params.m_cutting_edge_angle = 0;

				if (((CCuttingTool*)object)->m_params.m_cutting_edge_height != ((CCuttingTool*)object)->m_params.m_diameter/ theApp.m_program->m_units * 3.0)
				{
					l_ossChange << "Changing cutting edge height to " << ((CCuttingTool*)object)->m_params.m_diameter / theApp.m_program->m_units * 3.0 << "\n";
					((CCuttingTool*)object)->m_params.m_cutting_edge_height = ((CCuttingTool*)object)->m_params.m_diameter/ theApp.m_program->m_units * 3.0;
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
					((CCuttingTool*)object)->m_params.m_cutting_edge_height = ((CCuttingTool*)object)->m_params.m_diameter / theApp.m_program->m_units* 3.0;
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
					((CCuttingTool*)object)->m_params.m_cutting_edge_height = ((CCuttingTool*)object)->m_params.m_diameter / theApp.m_program->m_units* 3.0;
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

				height = (((CCuttingTool*)object)->m_params.m_diameter / 2.0) * tan( degrees_to_radians(90.0 - ((CCuttingTool*)object)->m_params.m_cutting_edge_angle));
				if (((CCuttingTool*)object)->m_params.m_cutting_edge_height != height)
				{
					l_ossChange << "Changing cutting edge height to " << height / theApp.m_program->m_units << "\n";
					((CCuttingTool*)object)->m_params.m_cutting_edge_height = height;
				} // End if - then

				l_ossChange << ((CCuttingTool*) object)->ResetTitle().c_str();
				break;

		case CCuttingToolParams::eTurningTool:
				// No special constraints for this.
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
	{
		CCuttingToolParams::MaterialsList_t materials = CCuttingToolParams::GetMaterialsList();

		int choice = -1;
		std::list< wxString > choices;
		for (CCuttingToolParams::MaterialsList_t::size_type i=0; i<materials.size(); i++)
		{
			choices.push_back(materials[i].second);
			if (m_material == materials[i].first) choice = int(i);
			
		} // End for
		list->push_back(new PropertyChoice(_("Material"), choices, choice, parent, on_set_material));
	}


	{
		CCuttingToolParams::CuttingToolTypesList_t cutting_tool_types = CCuttingToolParams::GetCuttingToolTypesList();

		int choice = -1;
		std::list< wxString > choices;
		for (CCuttingToolParams::CuttingToolTypesList_t::size_type i=0; i<cutting_tool_types.size(); i++)
		{
			choices.push_back(cutting_tool_types[i].second);
			if (m_type == cutting_tool_types[i].first) choice = int(i);
			
		} // End for
		list->push_back(new PropertyChoice(_("Type"), choices, choice, parent, on_set_type));
	}

	list->push_back(new PropertyLength(_("max_advance_per_revolution"), m_max_advance_per_revolution, parent, on_set_max_advance_per_revolution));

	if (m_type == eTurningTool)
	{
		// We're using lathe (turning) tools

		list->push_back(new PropertyLength(_("x_offset"), m_x_offset, parent, on_set_x_offset));
		list->push_back(new PropertyDouble(_("front_angle"), m_front_angle, parent, on_set_front_angle));
		list->push_back(new PropertyDouble(_("tool_angle"), m_tool_angle, parent, on_set_tool_angle));
		list->push_back(new PropertyDouble(_("back_angle"), m_back_angle, parent, on_set_back_angle));

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
	} // End if - then
	else
	{
		// We're using milling/drilling tools
		list->push_back(new PropertyLength(_("diameter"), m_diameter, parent, on_set_diameter));
		list->push_back(new PropertyLength(_("tool_length_offset"), m_tool_length_offset, parent, on_set_tool_length_offset));

		list->push_back(new PropertyLength(_("flat_radius"), m_flat_radius, parent, on_set_flat_radius));
		list->push_back(new PropertyLength(_("corner_radius"), m_corner_radius, parent, on_set_corner_radius));
		list->push_back(new PropertyDouble(_("cutting_edge_angle"), m_cutting_edge_angle, parent, on_set_cutting_edge_angle));
		list->push_back(new PropertyDouble(_("cutting_edge_height"), m_cutting_edge_height, parent, on_set_cutting_edge_height));
	} // End if - else

	

}

void CCuttingToolParams::WriteXMLAttributes(TiXmlNode *root)
{
	TiXmlElement * element;
	element = new TiXmlElement( "params" );
	root->LinkEndChild( element );  

	element->SetDoubleAttribute("diameter", m_diameter);
	element->SetDoubleAttribute("x_offset", m_x_offset);
	element->SetDoubleAttribute("tool_length_offset", m_tool_length_offset);
	element->SetDoubleAttribute("max_advance_per_revolution", m_max_advance_per_revolution);

	std::ostringstream l_ossValue;
	l_ossValue.str(""); l_ossValue << m_material;
	element->SetAttribute("material", l_ossValue.str().c_str() );

	l_ossValue.str(""); l_ossValue << m_orientation;
	element->SetAttribute("orientation", l_ossValue.str().c_str() );

	l_ossValue.str(""); l_ossValue << int(m_type);
	element->SetAttribute("type", l_ossValue.str().c_str() );

	element->SetDoubleAttribute("corner_radius", m_corner_radius);
	element->SetDoubleAttribute("flat_radius", m_flat_radius);
	element->SetDoubleAttribute("cutting_edge_angle", m_cutting_edge_angle);
	element->SetDoubleAttribute("cutting_edge_height", m_cutting_edge_height);

	element->SetDoubleAttribute("front_angle", m_front_angle);
	element->SetDoubleAttribute("tool_angle", m_tool_angle);
	element->SetDoubleAttribute("back_angle", m_back_angle);
}

void CCuttingToolParams::ReadParametersFromXMLElement(TiXmlElement* pElem)
{
	if (pElem->Attribute("diameter")) m_diameter = atof(pElem->Attribute("diameter"));
	if (pElem->Attribute("max_advance_per_revolution")) m_max_advance_per_revolution = atof(pElem->Attribute("max_advance_per_revolution"));
	if (pElem->Attribute("x_offset")) m_x_offset = atof(pElem->Attribute("x_offset"));
	if (pElem->Attribute("tool_length_offset")) m_tool_length_offset = atof(pElem->Attribute("tool_length_offset"));
	if (pElem->Attribute("material")) m_material = atoi(pElem->Attribute("material"));
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
		m_cutting_edge_height = m_diameter/ theApp.m_program->m_units * 4.0;
	} // End if - else

	if (pElem->Attribute("front_angle")) m_front_angle = atof(pElem->Attribute("front_angle"));
	if (pElem->Attribute("tool_angle")) m_tool_angle = atof(pElem->Attribute("tool_angle"));
	if (pElem->Attribute("back_angle")) m_back_angle = atof(pElem->Attribute("back_angle"));
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

CCuttingTool *CCuttingTool::Find( const int tool_number )
{
	int id = FindCuttingTool( tool_number );
	if (id <= 0) return(NULL);
	return((CCuttingTool *) heeksCAD->GetIDObject( CuttingToolType, id ));
} // End Find() method


/**
 * Find the CuttingTool object whose tool number matches that passed in.
 */
int CCuttingTool::FindCuttingTool( const int tool_number )
{
	if ((theApp.m_program) && (theApp.m_program->m_tools))
	{
		HeeksObj* tool_list = theApp.m_program->m_tools;

		for(HeeksObj* ob = tool_list->GetFirstChild(); ob; ob = tool_list->GetNextChild())
		{
			if (ob->GetType() != CuttingToolType) continue;
			if ((ob != NULL) && (((CCuttingTool *) ob)->m_tool_number == tool_number))
			{
				return(ob->m_id);
			} // End if - then
		} // End for
	} // End if - then

	return(-1);

} // End FindCuttingTool() method


std::vector< std::pair< int, wxString > > CCuttingTool::FindAllCuttingTools()
{
	std::vector< std::pair< int, wxString > > tools;

	// Always add a value of zero to allow for an absense of cutting tool use.
	tools.push_back( std::make_pair(0, _T("No Cutting Tool") ) );

	if ((theApp.m_program) && (theApp.m_program->m_tools))
	{
		HeeksObj* tool_list = theApp.m_program->m_tools;

		for(HeeksObj* ob = tool_list->GetFirstChild(); ob; ob = tool_list->GetNextChild())
		{
			if (ob->GetType() != CuttingToolType) continue;

			CCuttingTool *pCuttingTool = (CCuttingTool *)ob;
			if (ob != NULL)
			{
				tools.push_back( std::make_pair( pCuttingTool->m_tool_number, pCuttingTool->GetShortString() ) );
			} // End if - then
		} // End for
	} // End if - then

	return(tools);

} // End FindAllCuttingTools() method



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

	if (m_params.m_type != CCuttingToolParams::eTurningTool)
	{	
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
	} // End if - then

	switch (m_params.m_material)
	{
		case CCuttingToolParams::eHighSpeedSteel: l_ossName << "HSS ";
							  break;

		case CCuttingToolParams::eCarbide:	l_ossName << "Carbide ";
							break;
	} // End switch

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
							break;

                case CCuttingToolParams::eTurningTool:	l_ossName << "Turning Tool";
							break;

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
	     ((m_title.Find( _T("Chamfering Bit") ) != -1) && (m_title.Find(_T("degree")) != -1)) ||
	     ((m_title.Find( _T("Turning Tool") ) != -1)) )
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
		/*
		try {
			TopoDS_Shape tool_shape = GetShape();
			HeeksObj *pToolSolid = heeksCAD->NewSolid( *((TopoDS_Solid *) &tool_shape), NULL, HeeksColor(234, 123, 89) );
			pToolSolid->glCommands( true, false, true );
			delete pToolSolid;
		} // End try
		catch(Standard_DomainError) { }
		catch(...)  { }
		*/
	} // End if - then

} // End glCommands() method



/**
	This method produces a "Topology Data Structure - Shape" based on the parameters
	describing the tool's dimensions.  We will (probably) use this, along with the
	NCCode paths to find out what devastation this tool will make when run through
	the program.  We will then (again probably) intersect the resulting solid with
	things like fixture solids to see if we're going to break anything.  We could
	also intersect it with a solid that represents the raw material (i.e before it
	has been machined).  This would give us an idea of what we'd end up with if
	we ran the GCode program.

	Finally, we might use this to 'apply' a GCode operation to existing geometry.  eg:
	where a drilling cycle is defined, the result of drilling a hole from one
	location for a certain depth will be a hole in that green solid down there.  We
	may want to do this so as to use the edges (or other side-effects) to define
	subsequent NC operations.  (just typing out loud)

	NOTE: The shape is always drawn with the tool's tip at the origin.
	It is always drawn along the Z axis.  The calling routine may move and rotate the drawn
	shape if need be but this method returns a standard straight up and down version.
 */
TopoDS_Shape CCuttingTool::GetShape() const
{
   try {
	gp_Dir orientation(0,0,1);	// This method always draws it up and down.  Leave it
					// for other methods to rotate the resultant shape if
					// they need to.
	gp_Pnt tool_tip_location(0,0,0);	// Always from the origin in this method.

	double diameter = m_params.m_diameter;
	if (diameter < 0.01) diameter = 2;

	double tool_length_offset = m_params.m_tool_length_offset;
	if (tool_length_offset <  diameter) tool_length_offset = 10 * diameter;

	double cutting_edge_height = m_params.m_cutting_edge_height;
	if (cutting_edge_height < (2 * diameter)) cutting_edge_height = (2 * diameter)/ theApp.m_program->m_units;

	switch (m_params.m_type)
	{
		case CCuttingToolParams::eCentreDrill:
		{
			// First a cylinder to represent the shaft.
			double tool_tip_length = (diameter / 2) * tan( degrees_to_radians(90.0 - m_params.m_cutting_edge_angle));
			double non_cutting_shaft_length = tool_length_offset/ theApp.m_program->m_units - tool_tip_length/ theApp.m_program->m_units - cutting_edge_height/ theApp.m_program->m_units;

			gp_Pnt shaft_start_location( tool_tip_location );
			shaft_start_location.SetZ( tool_tip_location.Z() + tool_tip_length/ theApp.m_program->m_units + cutting_edge_height/ theApp.m_program->m_units );
			gp_Ax2 shaft_position_and_orientation( shaft_start_location, orientation );
			BRepPrimAPI_MakeCylinder shaft( shaft_position_and_orientation, (diameter / 2) * 2.0, non_cutting_shaft_length );

			gp_Pnt cutting_shaft_start_location( tool_tip_location );
			cutting_shaft_start_location.SetZ( tool_tip_location.Z() + tool_tip_length );
			gp_Ax2 cutting_shaft_position_and_orientation( cutting_shaft_start_location, orientation );
			BRepPrimAPI_MakeCylinder cutting_shaft( cutting_shaft_position_and_orientation, diameter / 2, cutting_edge_height/ theApp.m_program->m_units );

			// And a cone for the tip.
			gp_Ax2 tip_position_and_orientation( cutting_shaft_start_location, gp_Dir(0,0,-1) );
			BRepPrimAPI_MakeCone tool_tip( tip_position_and_orientation,
							diameter/2,
							m_params.m_flat_radius,
							tool_tip_length);

			TopoDS_Compound cutting_tool_shape;
			BRep_Builder aBuilder;
			aBuilder.MakeCompound (cutting_tool_shape);
			aBuilder.Add (cutting_tool_shape, shaft.Shape());
			aBuilder.Add (cutting_tool_shape, cutting_shaft.Shape());
			aBuilder.Add (cutting_tool_shape, tool_tip.Shape());
			return cutting_tool_shape;
		}

		case CCuttingToolParams::eDrill:
		{
			// First a cylinder to represent the shaft.
			double tool_tip_length = (diameter / 2) * tan( degrees_to_radians(90 - m_params.m_cutting_edge_angle));
			double shaft_length = tool_length_offset - tool_tip_length;

			gp_Pnt shaft_start_location( tool_tip_location );
			shaft_start_location.SetZ( tool_tip_location.Z() + tool_tip_length );

			gp_Ax2 shaft_position_and_orientation( shaft_start_location, orientation );

			BRepPrimAPI_MakeCylinder shaft( shaft_position_and_orientation, diameter / 2, shaft_length );

			// And a cone for the tip.
			gp_Ax2 tip_position_and_orientation( shaft_start_location, gp_Dir(0,0,-1) );

			BRepPrimAPI_MakeCone tool_tip( tip_position_and_orientation,
							diameter/2,
							m_params.m_flat_radius,
							tool_tip_length);

			TopoDS_Compound cutting_tool_shape;
			BRep_Builder aBuilder;
			aBuilder.MakeCompound (cutting_tool_shape);
			aBuilder.Add (cutting_tool_shape, shaft.Shape());
			aBuilder.Add (cutting_tool_shape, tool_tip.Shape());
			return cutting_tool_shape;
		}

		case CCuttingToolParams::eChamfer:
		{
			// First a cylinder to represent the shaft.
			double tool_tip_length_a = (diameter / 2) * tan( degrees_to_radians(90.0 - m_params.m_cutting_edge_angle));
			double tool_tip_length_b = (m_params.m_flat_radius)  * tan( degrees_to_radians(90.0 - m_params.m_cutting_edge_angle));
			double tool_tip_length = tool_tip_length_a - tool_tip_length_b;

			double shaft_length = tool_length_offset - tool_tip_length;

			gp_Pnt shaft_start_location( tool_tip_location );
			shaft_start_location.SetZ( tool_tip_location.Z() + tool_tip_length );

			gp_Ax2 shaft_position_and_orientation( shaft_start_location, orientation );

			BRepPrimAPI_MakeCylinder shaft( shaft_position_and_orientation, (diameter / 2) * 0.5, shaft_length );

			// And a cone for the tip.
			// double cutting_edge_angle_in_radians = ((m_params.m_cutting_edge_angle / 2) / 360) * (2 * PI);
			gp_Ax2 tip_position_and_orientation( shaft_start_location, gp_Dir(0,0,-1) );

			BRepPrimAPI_MakeCone tool_tip( tip_position_and_orientation,
							diameter/2,
							m_params.m_flat_radius,
							tool_tip_length);

			TopoDS_Compound cutting_tool_shape;
			BRep_Builder aBuilder;
			aBuilder.MakeCompound (cutting_tool_shape);
			aBuilder.Add (cutting_tool_shape, shaft.Shape());
			aBuilder.Add (cutting_tool_shape, tool_tip.Shape());
			return cutting_tool_shape;
		}

		case CCuttingToolParams::eBallEndMill:
		{
			// First a cylinder to represent the shaft.
			double shaft_length = tool_length_offset - m_params.m_corner_radius;

			gp_Pnt shaft_start_location( tool_tip_location );
			shaft_start_location.SetZ( tool_tip_location.Z() + m_params.m_corner_radius );

			gp_Ax2 shaft_position_and_orientation( shaft_start_location, orientation );

			BRepPrimAPI_MakeCylinder shaft( shaft_position_and_orientation, diameter / 2, shaft_length );
			BRepPrimAPI_MakeSphere ball( shaft_start_location, m_params.m_corner_radius );

			TopoDS_Compound cutting_tool_shape;
			BRep_Builder aBuilder;
			aBuilder.MakeCompound (cutting_tool_shape);
			aBuilder.Add (cutting_tool_shape, shaft.Shape());
			aBuilder.Add (cutting_tool_shape, ball.Shape());
			return cutting_tool_shape;
		}

		case CCuttingToolParams::eTurningTool:
		{
			// First draw the cutting tip.
			double triangle_radius = 8.0;	// mm
			double cutting_tip_thickness = 3.0;	// mm

			gp_Trsf rotation;

			gp_Pnt p1(0.0, triangle_radius, 0.0);
			rotation.SetRotation( gp_Ax1( gp_Pnt(0,0,0), gp_Dir(0,0,1)), degrees_to_radians(0) );
			p1.Transform(rotation);

			rotation.SetRotation( gp_Ax1( gp_Pnt(0,0,0), gp_Dir(0,0,1)), degrees_to_radians(+1 * ((360 - m_params.m_tool_angle)/2)) );
			gp_Pnt p2(p1);
			p2.Transform(rotation);

			rotation.SetRotation( gp_Ax1( gp_Pnt(0,0,0), gp_Dir(0,0,1)), degrees_to_radians(-1 * ((360 - m_params.m_tool_angle)/2)) );
			gp_Pnt p3(p1);
			p3.Transform(rotation);

			Handle(Geom_TrimmedCurve) line1 = GC_MakeSegment(p1,p2);
			Handle(Geom_TrimmedCurve) line2 = GC_MakeSegment(p2,p3);
			Handle(Geom_TrimmedCurve) line3 = GC_MakeSegment(p3,p1);

			TopoDS_Edge edge1 = BRepBuilderAPI_MakeEdge( line1 );
			TopoDS_Edge edge2 = BRepBuilderAPI_MakeEdge( line2 );
			TopoDS_Edge edge3 = BRepBuilderAPI_MakeEdge( line3 );

			TopoDS_Wire wire = BRepBuilderAPI_MakeWire( edge1, edge2, edge3 );
			TopoDS_Face face = BRepBuilderAPI_MakeFace( wire );
			gp_Vec vec( 0,0, cutting_tip_thickness );
			TopoDS_Shape cutting_tip = BRepPrimAPI_MakePrism( face, vec );

			// Now make the supporting shaft
			gp_Pnt p4(p3); p4.SetZ( p3.Z() + cutting_tip_thickness );
			gp_Pnt p5(p2); p5.SetZ( p2.Z() + cutting_tip_thickness );

			Handle(Geom_TrimmedCurve) shaft_line1 = GC_MakeSegment(p2,p3);
			Handle(Geom_TrimmedCurve) shaft_line2 = GC_MakeSegment(p3,p4);
			Handle(Geom_TrimmedCurve) shaft_line3 = GC_MakeSegment(p4,p5);
			Handle(Geom_TrimmedCurve) shaft_line4 = GC_MakeSegment(p5,p2);

			TopoDS_Edge shaft_edge1 = BRepBuilderAPI_MakeEdge( shaft_line1 );
			TopoDS_Edge shaft_edge2 = BRepBuilderAPI_MakeEdge( shaft_line2 );
			TopoDS_Edge shaft_edge3 = BRepBuilderAPI_MakeEdge( shaft_line3 );
			TopoDS_Edge shaft_edge4 = BRepBuilderAPI_MakeEdge( shaft_line4 );

			TopoDS_Wire shaft_wire = BRepBuilderAPI_MakeWire( shaft_edge1, shaft_edge2, shaft_edge3, shaft_edge4 );
			TopoDS_Face shaft_face = BRepBuilderAPI_MakeFace( shaft_wire );
			gp_Vec shaft_vec( 0, (-1 * tool_length_offset), 0 );
			TopoDS_Shape shaft = BRepPrimAPI_MakePrism( shaft_face, shaft_vec );
	
			// Aggregate the shaft and cutting tip	
			TopoDS_Compound cutting_tool_compound;
			BRep_Builder aBuilder;
			aBuilder.MakeCompound (cutting_tool_compound);
			aBuilder.Add (cutting_tool_compound, cutting_tip);
			aBuilder.Add (cutting_tool_compound, shaft);

			// Now orient the tool as per its settings.
			gp_Trsf tool_holder_orientation;
			gp_Trsf orient_for_lathe_use;
			TopoDS_Shape cutting_tool_shape = cutting_tool_compound;

			switch (m_params.m_orientation)
			{
				case 1: // South East
					tool_holder_orientation.SetRotation( gp_Ax1( gp_Pnt(0,0,0), gp_Dir(0,0,1)), degrees_to_radians(45 - 90) );
					break;

				case 2: // South West
					tool_holder_orientation.SetRotation( gp_Ax1( gp_Pnt(0,0,0), gp_Dir(0,0,1)), degrees_to_radians(123 - 90) );
					break;

				case 3: // North West
					tool_holder_orientation.SetRotation( gp_Ax1( gp_Pnt(0,0,0), gp_Dir(0,0,1)), degrees_to_radians(225 - 90) );
					break;

				case 4: // North East
					tool_holder_orientation.SetRotation( gp_Ax1( gp_Pnt(0,0,0), gp_Dir(0,0,1)), degrees_to_radians(-45 - 90) );
					break;

				case 5: // East
					tool_holder_orientation.SetRotation( gp_Ax1( gp_Pnt(0,0,0), gp_Dir(0,0,1)), degrees_to_radians(0 - 90) );
					break;

				case 6: // South
					tool_holder_orientation.SetRotation( gp_Ax1( gp_Pnt(0,0,0), gp_Dir(0,0,1)), degrees_to_radians(+90 - 90) );
					break;

				case 7: // West
					tool_holder_orientation.SetRotation( gp_Ax1( gp_Pnt(0,0,0), gp_Dir(0,0,1)), degrees_to_radians(180 - 90) );
					break;

				case 8: // North
					tool_holder_orientation.SetRotation( gp_Ax1( gp_Pnt(0,0,0), gp_Dir(0,0,1)), degrees_to_radians(-90 - 90) );
					break;

				case 9: // Boring (straight along Y axis)
				default:
					tool_holder_orientation.SetRotation( gp_Ax1( gp_Pnt(0,0,0), gp_Dir(1,0,0)), degrees_to_radians(90.0) );
					break;

			} // End switch

			// Rotate from drawing orientation (for easy mathematics in this code) to tool holder orientation.
			cutting_tool_shape = BRepBuilderAPI_Transform( cutting_tool_compound, tool_holder_orientation, false );

			// Rotate to use axes typically used for lathe work.
			// i.e. z axis along the bed (from head stock to tail stock as z increases)
			// and x across the bed.
			orient_for_lathe_use.SetRotation( gp_Ax1( gp_Pnt(0,0,0), gp_Dir(0,1,0) ), degrees_to_radians(-90.0) );
			cutting_tool_shape = BRepBuilderAPI_Transform( cutting_tool_shape, orient_for_lathe_use, false );

			orient_for_lathe_use.SetRotation( gp_Ax1( gp_Pnt(0,0,0), gp_Dir(0,0,1) ), degrees_to_radians(90.0) );
			cutting_tool_shape = BRepBuilderAPI_Transform( cutting_tool_shape, orient_for_lathe_use, false );

			return(cutting_tool_shape);
		}

		case CCuttingToolParams::eEndmill:
		case CCuttingToolParams::eSlotCutter:
		default:
		{
			// First a cylinder to represent the shaft.
			double shaft_length = tool_length_offset;
			gp_Pnt shaft_start_location( tool_tip_location );

			gp_Ax2 shaft_position_and_orientation( shaft_start_location, orientation );

			BRepPrimAPI_MakeCylinder shaft( shaft_position_and_orientation, diameter / 2, shaft_length );

			TopoDS_Compound cutting_tool_shape;
			BRep_Builder aBuilder;
			aBuilder.MakeCompound (cutting_tool_shape);
			aBuilder.Add (cutting_tool_shape, shaft.Shape());
			return cutting_tool_shape;
		}
	} // End switch
   } // End try
   // These are due to poor parameter settings resulting in negative lengths and the like.  I need
   // to work through the various parameters to either ensure they're correct or don't try
   // to construct a shape.
   catch (Standard_ConstructionError)
   {
	// printf("Construction error thrown while generating tool shape\n");
	throw;	// Re-throw the exception.
   } // End catch
   catch (Standard_DomainError)
   {
	// printf("Domain error thrown while generating tool shape\n");
	throw;	// Re-throw the exception.
   } // End catch
} // End GetShape() method


/**
	The CuttingRadius is almost always the same as half the cutting tool's diameter.
	The exception to this is if it's a chamfering bit.  In this case we
	want to use the flat_radius plus a little bit.  i.e. if we're chamfering the edge
	then we want to use the part of the cutting surface just a little way from
	the flat radius.  If it has a flat radius of zero (i.e. it has a pointed end)
	then it will be a small number.  If it is a carbide tipped bit then the
	flat radius will allow for the area below the bit that doesn't cut.  In this
	case we want to cut around the middle of the carbide tip.  In this case
	the carbide tip should represent the full cutting edge height.  We can
	use this method to make all these adjustments based on the cutting tool's
	geometry and return a reasonable value.

	If express_in_drawing_units is true then we need to divide by the drawing
	units value.  We use metric (mm) internally and convert to inches only
	if we need to and only as the last step in the process.  By default, return
	the value in internal (metric) units.
 */
double CCuttingTool::CuttingRadius( const bool express_in_drawing_units /* = false */ ) const
{
	double radius;

	switch (m_params.m_type)
	{
		case CCuttingToolParams::eChamfer:
			{
				// We want to decide where, along the cutting edge, we want
				// to machine.  Let's start with 1/3 of the way from the inside
				// cutting edge so that, as we plunge it into the material, it
				// cuts towards the outside.  We don't want to run right on
				// the edge because we don't want to break the top off.

				// one third from the centre-most point.
				double proportion_near_centre = 0.3;
				radius = (((m_params.m_diameter/2) - m_params.m_flat_radius) * proportion_near_centre) + m_params.m_flat_radius;
			}
			break;

		case CCuttingToolParams::eDrill:
		case CCuttingToolParams::eCentreDrill:
		case CCuttingToolParams::eEndmill:
		case CCuttingToolParams::eSlotCutter:
		case CCuttingToolParams::eBallEndMill:
		case CCuttingToolParams::eTurningTool:
		default:
			radius = m_params.m_diameter/2;
	} // End switch

	if (express_in_drawing_units) return(radius / theApp.m_program->m_units);
	else return(radius);

} // End CuttingRadius() method


CCuttingToolParams::eCuttingToolType CCuttingTool::CutterType( const int tool_number )
{
	if (tool_number <= 0) return(CCuttingToolParams::eUndefinedToolType);

	CCuttingTool *pCuttingTool = CCuttingTool::Find( tool_number );
	if (pCuttingTool == NULL) return(CCuttingToolParams::eUndefinedToolType);
	
	return(pCuttingTool->m_params.m_type);
} // End of CutterType() method


CCuttingToolParams::eMaterial_t CCuttingTool::CutterMaterial( const int tool_number )
{
	if (tool_number <= 0) return(CCuttingToolParams::eUndefinedMaterialType);

	CCuttingTool *pCuttingTool = CCuttingTool::Find( tool_number );
	if (pCuttingTool == NULL) return(CCuttingToolParams::eUndefinedMaterialType);
	
	return(CCuttingToolParams::eMaterial_t(pCuttingTool->m_params.m_material));
} // End of CutterType() method



