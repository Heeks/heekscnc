// CTool.cpp
/*
 * Copyright (c) 2009, Dan Heeks, Perttu Ahola
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include <stdafx.h>
#include <math.h>
#include "CTool.h"
#include "CNCConfig.h"
#include "ProgramCanvas.h"
#include "../../interface/HeeksObj.h"
#include "../../interface/HeeksColor.h"
#include "../../interface/Tool.h"
#include "../../interface/PropertyInt.h"
#include "../../interface/PropertyDouble.h"
#include "../../interface/PropertyLength.h"
#include "../../interface/PropertyChoice.h"
#include "../../interface/PropertyString.h"
#include "../../tinyxml/tinyxml.h"
#include "CNCPoint.h"
#include "PythonStuff.h"
#include "Program.h"
#include "Surface.h"

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
#include <BRepPrimAPI_MakeBox.hxx>
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

#include "Tools.h"
#include "CToolDlg.h"

extern CHeeksCADInterface* heeksCAD;

#ifdef HEEKSCNC
#define PROGRAM theApp.m_program
#define TOOLS (theApp.m_program ? theApp.m_program->Tools() : NULL)
#else
#define PROGRAM heeksCNC->GetProgram()
#define TOOLS heeksCNC->GetTools()
#endif

void CToolParams::set_initial_values()
{
	CNCConfig config;
	config.Read(_T("m_material"), &m_material, int(eCarbide));
	config.Read(_T("m_diameter"), &m_diameter, 12.7);
	config.Read(_T("m_tool_length_offset"), &m_tool_length_offset, (10 * m_diameter));
	config.Read(_T("m_automatically_generate_title"), &m_automatically_generate_title, 1 );
	config.Read(_T("m_type"), (int *) &m_type, eDrill);
	config.Read(_T("m_flat_radius"), &m_flat_radius, 0);
	config.Read(_T("m_corner_radius"), &m_corner_radius, 0);
	config.Read(_T("m_cutting_edge_angle"), &m_cutting_edge_angle, 59);
	config.Read(_T("m_cutting_edge_height"), &m_cutting_edge_height, 4 * m_diameter);
}

void CToolParams::write_values_to_config()
{
	CNCConfig config;

	config.Write(_T("m_material"), m_material);
	config.Write(_T("m_diameter"), m_diameter);
	config.Write(_T("m_tool_length_offset"), m_tool_length_offset);
	config.Write(_T("m_automatically_generate_title"), m_automatically_generate_title );
	config.Write(_T("m_type"), (int)m_type);
	config.Write(_T("m_flat_radius"), m_flat_radius);
	config.Write(_T("m_corner_radius"), m_corner_radius);
	config.Write(_T("m_cutting_edge_angle"), m_cutting_edge_angle);
	config.Write(_T("m_cutting_edge_height"), m_cutting_edge_height);
}

void CTool::SetDiameter( const double diameter )
{
	m_params.m_diameter = diameter;
	switch (m_params.m_type)
	{
	    case CToolParams::eChamfer:
	    {
            // Recalculate the cutting edge length based on this new diameter
            // and the cutting angle.
            double opposite = (m_params.m_diameter / 2.0) - m_params.m_flat_radius;
            double angle = m_params.m_cutting_edge_angle / 360.0 * 2 * M_PI;
            m_params.m_cutting_edge_height = opposite / tan(angle);
	    }
	    break;

	    case CToolParams::eEndmill:
	    case CToolParams::eSlotCutter:
		case CToolParams::eEngravingTool:
            m_params.m_flat_radius = diameter / 2.0;
            break;

	    default:
	    break;
	}

	ResetTitle();
	KillGLLists();
	heeksCAD->Repaint();
} // End SetDiameter() method

static void on_set_diameter(double value, HeeksObj* object)
{
	((CTool*)object)->SetDiameter( value );
} // End on_set_diameter() routine

static void on_set_tool_length_offset(double value, HeeksObj* object)
{
	((CTool*)object)->m_params.m_tool_length_offset = value;
	object->KillGLLists();
	heeksCAD->Repaint();
}

static void on_set_material(int zero_based_choice, HeeksObj* object, bool from_undo_redo)
{
	((CTool*)object)->m_params.m_material = zero_based_choice;
	((CTool*)object)->ResetTitle();
	object->KillGLLists();
	heeksCAD->Repaint();
}

typedef std::pair< CToolParams::eToolType, wxString > ToolTypeDescription_t;
typedef std::vector<ToolTypeDescription_t > ToolTypesList_t;
static ToolTypesList_t tool_types_for_on_set_type;

static void on_set_type(int zero_based_choice, HeeksObj* object, bool from_undo_redo)
{
	if (zero_based_choice < 0) return;	// An error has occured.

	((CTool*)object)->m_params.m_type = tool_types_for_on_set_type[zero_based_choice].first;
	((CTool*)object)->ResetTitle();
	heeksCAD->RefreshProperties();
	object->KillGLLists();
	heeksCAD->Repaint();
} // End on_set_type() routine

static void on_set_automatically_generate_title(int zero_based_choice, HeeksObj* object, bool from_undo_redo)
{
	if (zero_based_choice < 0) return;	// An error has occured.

	((CTool*)object)->m_params.m_automatically_generate_title = zero_based_choice;
	((CTool*)object)->ResetTitle();

} // End on_set_type() routine

static double degrees_to_radians( const double degrees )
{
	return( (degrees / 360) * 2 * M_PI );
} // End degrees_to_radians() routine

static void on_set_corner_radius(double value, HeeksObj* object)
{
	((CTool*)object)->m_params.m_corner_radius = value;
	object->KillGLLists();
	heeksCAD->Repaint();
}

static void on_set_flat_radius(double value, HeeksObj* object)
{
	if (value > (((CTool*)object)->m_params.m_diameter / 2.0))
	{
		wxMessageBox(_T("Flat radius cannot be larger than the tool's diameter"));
		return;
	}

	((CTool*)object)->m_params.m_flat_radius = value;

	switch(((CTool*)object)->m_params.m_type)
	{
		case CToolParams::eChamfer:
		case CToolParams::eEngravingTool:
		{
			// Recalculate the cutting edge length based on this new diameter
			// and the cutting angle.

			double opposite = ((CTool*)object)->m_params.m_diameter - ((CTool*)object)->m_params.m_flat_radius;
			double angle = ((CTool*)object)->m_params.m_cutting_edge_angle / 360.0 * 2 * M_PI;

			((CTool*)object)->m_params.m_cutting_edge_height = opposite / tan(angle);
		}
		break;
	}

	object->KillGLLists();
	heeksCAD->Repaint();
}

static void on_set_cutting_edge_angle(double value, HeeksObj* object)
{
	if (value < 0)
	{
		wxMessageBox(_T("Cutting edge angle must be zero or positive."));
		return;
	}

	((CTool*)object)->m_params.m_cutting_edge_angle = value;

	switch(((CTool*)object)->m_params.m_type)
	{
		case CToolParams::eChamfer:
		case CToolParams::eEngravingTool:
		{
			// Recalculate the cutting edge length based on this new diameter
			// and the cutting angle.

			double opposite = ((CTool*)object)->m_params.m_diameter - ((CTool*)object)->m_params.m_flat_radius;
			double angle = ((CTool*)object)->m_params.m_cutting_edge_angle / 360.0 * 2 * M_PI;

			((CTool*)object)->m_params.m_cutting_edge_height = opposite / tan(angle);
		}
		break;
	}

	((CTool*)object)->ResetTitle();
	object->KillGLLists();
	heeksCAD->Repaint();
}

static void on_set_cutting_edge_height(double value, HeeksObj* object)
{
	if (value <= 0)
	{
		wxMessageBox(_T("Cutting edge height must be positive."));
		return;
	}

	if (((CTool*)object)->m_params.m_type == CToolParams::eChamfer)
	{
		wxMessageBox(_T("Cutting edge height is generated from diameter, flat radius and cutting edge angle for chamfering bits."));
		return;
	}

	((CTool*)object)->m_params.m_cutting_edge_height = value;
	object->KillGLLists();
	heeksCAD->Repaint();
}

static ToolTypesList_t GetToolTypesList()
{
	ToolTypesList_t types_list;

	types_list.push_back( ToolTypeDescription_t( CToolParams::eDrill, wxString(_("Drill Bit")) ));
	types_list.push_back( ToolTypeDescription_t( CToolParams::eCentreDrill, wxString(_("Centre Drill Bit")) ));
	types_list.push_back( ToolTypeDescription_t( CToolParams::eEndmill, wxString(_("End Mill")) ));
	types_list.push_back( ToolTypeDescription_t( CToolParams::eSlotCutter, wxString(_("Slot Cutter")) ));
	types_list.push_back( ToolTypeDescription_t( CToolParams::eBallEndMill, wxString(_("Ball End Mill")) ));
	types_list.push_back( ToolTypeDescription_t( CToolParams::eChamfer, wxString(_("Chamfer")) ));
	return(types_list);
} // End GetToolTypesList() method

void CToolParams::GetProperties(CTool* parent, std::list<Property *> *list)
{
	{
		int choice = m_automatically_generate_title;
		std::list< wxString > choices;
		choices.push_back( wxString(_("Leave manually assigned title")) );	// option 0 (false)
		choices.push_back( wxString(_("Automatically generate title")) );	// option 1 (true)

		list->push_back(new PropertyChoice(_("Automatic Title"), choices, choice, parent, on_set_automatically_generate_title));
	}

	{
		std::list< wxString > choices;
		choices.push_back(_("High Speed Steel"));
		choices.push_back(_("Carbide"));
		list->push_back(new PropertyChoice(_("Material"), choices, m_material, parent, on_set_material));

	}

	{
		tool_types_for_on_set_type = GetToolTypesList();

		int choice = -1;
		std::list< wxString > choices;
		for (ToolTypesList_t::size_type i=0; i<tool_types_for_on_set_type.size(); i++)
		{
			choices.push_back(tool_types_for_on_set_type[i].second);
			if (m_type == tool_types_for_on_set_type[i].first) choice = int(i);

		} // End for
		list->push_back(new PropertyChoice(_("Type"), choices, choice, parent, on_set_type));
	}

	{
		// We're using milling/drilling tools
		list->push_back(new PropertyLength(_("diameter"), m_diameter, parent, on_set_diameter));
		list->push_back(new PropertyLength(_("tool_length_offset"), m_tool_length_offset, parent, on_set_tool_length_offset));
		list->push_back(new PropertyLength(_("flat_radius"), m_flat_radius, parent, on_set_flat_radius));
		list->push_back(new PropertyLength(_("corner_radius"), m_corner_radius, parent, on_set_corner_radius));
		list->push_back(new PropertyDouble(_("cutting_edge_angle"), m_cutting_edge_angle, parent, on_set_cutting_edge_angle));
		list->push_back(new PropertyLength(_("cutting_edge_height"), m_cutting_edge_height, parent, on_set_cutting_edge_height));
	}
}

#define XML_STRING_DRILL "drill"
#define XML_STRING_CENTRE_DRILL "centre_drill"
#define XML_STRING_END_MILL "end_mill"
#define XML_STRING_SLOT_CUTTER "slot_cutter"
#define XML_STRING_BALL_END_MILL "ball_end_mill"
#define XML_STRING_CHAMFER "chamfer"
#define XML_STRING_ENGRAVER "engraver"

CToolParams::eToolType GetToolTypeFromString(const char* str)
{
	if(!strcasecmp(str, XML_STRING_DRILL))return CToolParams::eDrill;
	if(!strcasecmp(str, XML_STRING_CENTRE_DRILL))return CToolParams::eCentreDrill;
	if(!strcasecmp(str, XML_STRING_END_MILL))return CToolParams::eEndmill;
	if(!strcasecmp(str, XML_STRING_SLOT_CUTTER))return CToolParams::eSlotCutter;
	if(!strcasecmp(str, XML_STRING_BALL_END_MILL))return CToolParams::eBallEndMill;
	if(!strcasecmp(str, XML_STRING_CHAMFER))return CToolParams::eChamfer;
	if(!strcasecmp(str, XML_STRING_ENGRAVER))return CToolParams::eEngravingTool;
	return CToolParams::eUndefinedToolType;
}

CToolParams::eToolType GetToolTypeFromOldInt(int t)
{
	switch(t)
	{
	case 0:
		return CToolParams::eDrill;
	case 1:
		return CToolParams::eCentreDrill;
	case 2:
		return CToolParams::eEndmill;
	case 3:
		return CToolParams::eSlotCutter;
	case 4:
		return CToolParams::eBallEndMill;
	case 5:
		return CToolParams::eChamfer;
	default:
		return CToolParams::eUndefinedToolType;
	}
}

const char* GetToolTypeXMLString(CToolParams::eToolType t)
{
	switch(t)
	{
	case CToolParams::eDrill:
		return XML_STRING_DRILL;
	case CToolParams::eCentreDrill:
		return XML_STRING_CENTRE_DRILL;
	case CToolParams::eEndmill:
		return XML_STRING_END_MILL;
	case CToolParams::eSlotCutter:
		return XML_STRING_SLOT_CUTTER;
	case CToolParams::eBallEndMill:
		return XML_STRING_BALL_END_MILL;
	case CToolParams::eChamfer:
		return XML_STRING_CHAMFER;
	case CToolParams::eEngravingTool:
		return XML_STRING_ENGRAVER;
	default:
		return "";
	}
}

void CToolParams::WriteXMLAttributes(TiXmlNode *root)
{
	TiXmlElement * element;
	element = heeksCAD->NewXMLElement( "params" );
	heeksCAD->LinkXMLEndChild( root,  element );

	element->SetDoubleAttribute( "diameter", m_diameter);
	element->SetDoubleAttribute( "tool_length_offset", m_tool_length_offset);
	element->SetAttribute( "automatically_generate_title", m_automatically_generate_title );
	element->SetAttribute( "material", m_material );
	element->SetAttribute( "type", GetToolTypeXMLString(m_type) );
	element->SetDoubleAttribute( "corner_radius", m_corner_radius);
	element->SetDoubleAttribute( "flat_radius", m_flat_radius);
	element->SetDoubleAttribute( "cutting_edge_angle", m_cutting_edge_angle);
	element->SetDoubleAttribute( "cutting_edge_height", m_cutting_edge_height);
}

void CToolParams::ReadParametersFromXMLElement(TiXmlElement* pElem)
{
	if (pElem->Attribute("diameter")) pElem->Attribute("diameter", &m_diameter);
	if (pElem->Attribute("tool_length_offset")) pElem->Attribute("tool_length_offset", &m_tool_length_offset);
	if (pElem->Attribute("automatically_generate_title")) pElem->Attribute("automatically_generate_title", &m_automatically_generate_title);
	if (pElem->Attribute("material")) pElem->Attribute("material", &m_material);
	if (const char* value = pElem->Attribute("type")) { m_type = GetToolTypeFromString(value); }
	if (pElem->Attribute("corner_radius")) pElem->Attribute("corner_radius", &m_corner_radius);
	if (pElem->Attribute("flat_radius")) pElem->Attribute("flat_radius", &m_flat_radius);
	if (pElem->Attribute("cutting_edge_angle")) pElem->Attribute("cutting_edge_angle", &m_cutting_edge_angle);
	if (pElem->Attribute("cutting_edge_height")) pElem->Attribute("cutting_edge_height", &m_cutting_edge_height);
}

CTool::~CTool()
{
	DeleteSolid();
}




/**
	This method is called when the CAD operator presses the Python button.  This method generates
	Python source code whose job will be to generate RS-274 GCode.  It's done in two steps so that
	the Python code can be configured to generate GCode suitable for various CNC interpreters.
 */
Python CTool::AppendTextToProgram()
{
	Python python;

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
		python << _T("#(") << m_title.c_str() << _T(")\n");
	} // End if - then

	python << _T("tool_defn( ") << m_tool_number << _T(", ");

	if (m_title.size() > 0)
	{
		python << PythonString(m_title).c_str() << _T(", ");
	} // End if - then
	else
	{
		python << _T("None, ");
	} // End if - else

	// write all the other parameters as a dictionary
	python << _T("{");
	python << _T("'corner radius':") << this->m_params.m_corner_radius;
	python << _T(", ");
	python << _T("'cutting edge angle':") << this->m_params.m_cutting_edge_angle;
	python << _T(", ");
	python << _T("'cutting edge height':") << this->m_params.m_cutting_edge_height;
	python << _T(", ");
	python << _T("'diameter':") << this->m_params.m_diameter;
	python << _T(", ");
	python << _T("'flat radius':") << this->m_params.m_flat_radius;
	python << _T(", ");
	python << _T("'material':") << this->m_params.m_material;
	python << _T(", ");
	python << _T("'tool length offset':") << this->m_params.m_tool_length_offset;
	python << _T(", ");
	python << _T("'type':") << this->m_params.m_type;
	python << _T(", ");
	python << _T("'name':'") << this->GetMeaningfulName(theApp.m_program->m_units) << _T("'");
	python << _T("})\n");

	return(python);
}


static void on_set_tool_number(const int value, HeeksObj* object){((CTool*)object)->m_tool_number = value;}

/**
	NOTE: The m_title member is a special case.  The HeeksObj code looks for a 'GetShortString()' method.  If found, it
	adds a Property called 'Object Title'.  If the value is changed, it tries to call the 'OnEditString()' method.
	That's why the m_title value is not defined here
 */
void CTool::GetProperties(std::list<Property *> *list)
{
	list->push_back(new PropertyInt(_("tool_number"), m_tool_number, this, on_set_tool_number));

	m_params.GetProperties(this, list);
	HeeksObj::GetProperties(list);
}


HeeksObj *CTool::MakeACopy(void)const
{
	HeeksObj *duplicate = new CTool(*this);
	((CTool *) duplicate)->m_pToolSolid = NULL;	// We didn't duplicate this so reset the pointer.
	return(duplicate);
}

CTool::CTool( const CTool & rhs )
{
    m_pToolSolid = NULL;
    *this = rhs;    // Call the assignment operator.
}

CTool & CTool::operator= ( const CTool & rhs )
{
    if (this != &rhs)
    {
        m_params = rhs.m_params;
        m_title = rhs.m_title;
        m_tool_number = rhs.m_tool_number;

        if (m_pToolSolid)
        {
            delete m_pToolSolid;
            m_pToolSolid = NULL;
        }

        HeeksObj::operator=( rhs );
    }

    return(*this);
}


void CTool::CopyFrom(const HeeksObj* object)
{
	operator=(*((CTool*)object));
	m_pToolSolid = NULL;	// We didn't duplicate this so reset the pointer.
}

bool CTool::CanAddTo(HeeksObj* owner)
{
	return ((owner != NULL) && (owner->GetType() == ToolsType));
}

const wxBitmap &CTool::GetIcon()
{
	switch(m_params.m_type){
		case CToolParams::eDrill:
			{
				static wxBitmap* drillIcon = NULL;
				if(drillIcon == NULL)drillIcon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/drill.png")));
				return *drillIcon;
			}
		case CToolParams::eCentreDrill:
			{
				static wxBitmap* centreDrillIcon = NULL;
				if(centreDrillIcon == NULL)centreDrillIcon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/centredrill.png")));
				return *centreDrillIcon;
			}
		case CToolParams::eEndmill:
			{
				static wxBitmap* endmillIcon = NULL;
				if(endmillIcon == NULL)endmillIcon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/endmill.png")));
				return *endmillIcon;
			}
		case CToolParams::eSlotCutter:
			{
				static wxBitmap* slotCutterIcon = NULL;
				if(slotCutterIcon == NULL)slotCutterIcon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/slotdrill.png")));
				return *slotCutterIcon;
			}
		case CToolParams::eBallEndMill:
			{
				static wxBitmap* ballEndMillIcon = NULL;
				if(ballEndMillIcon == NULL)ballEndMillIcon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons//ballmill.png")));
				return *ballEndMillIcon;
			}
		case CToolParams::eChamfer:
			{
				static wxBitmap* chamferIcon = NULL;
				if(chamferIcon == NULL)chamferIcon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/chamfmill.png")));
				return *chamferIcon;
			}
		default:
			{
				static wxBitmap* toolIcon = NULL;
				if(toolIcon == NULL)toolIcon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/tool.png")));
				return *toolIcon;
			}
	}
}

void CTool::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element = heeksCAD->NewXMLElement( "Tool" );
	heeksCAD->LinkXMLEndChild( root,  element );
	element->SetAttribute( "title", m_title.utf8_str());
	element->SetAttribute( "tool_number", m_tool_number );
	m_params.WriteXMLAttributes(element);
	WriteBaseXML(element);
}

// static member function
HeeksObj* CTool::ReadFromXMLElement(TiXmlElement* element)
{

	int tool_number = 0;
	if (element->Attribute("tool_number")) element->Attribute("tool_number", &tool_number);

	wxString title(Ctt(element->Attribute("title")));
	CTool* new_object = new CTool( title.c_str(), CToolParams::eDrill, tool_number);

	// read point and circle ids
	for(TiXmlElement* pElem = heeksCAD->FirstXMLChildElement( element ) ; pElem; pElem = pElem->NextSiblingElement())
	{
		std::string name(pElem->Value());
		if(name == "params"){
			new_object->m_params.ReadParametersFromXMLElement(pElem);
			if(new_object->m_params.m_automatically_generate_title == 0)new_object->m_title.assign(title);
		}
	}

	new_object->ReadBaseXML(element);
	new_object->ResetTitle();	// reset title to match design units.
	return new_object;
}


void CTool::OnEditString(const wxChar* str)
{
    m_title.assign(str);
	m_params.m_automatically_generate_title = false;	// It's been manually edited.  Leave it alone now.
	// to do, make undoable properties
}

CTool *CTool::Find( const int tool_number )
{
	int id = FindTool( tool_number );
	if (id <= 0) return(NULL);
	return((CTool *) heeksCAD->GetIDObject( ToolType, id ));
} // End Find() method

CTool::ToolNumber_t CTool::FindFirstByType( const CToolParams::eToolType type )
{

	if (TOOLS)
	{
		HeeksObj* tool_list = TOOLS;
		for(HeeksObj* ob = tool_list->GetFirstChild(); ob; ob = tool_list->GetNextChild())
		{
			if (ob->GetType() != ToolType) continue;
			if ((ob != NULL) && (((CTool *) ob)->m_params.m_type == type))
			{
				return(((CTool *)ob)->m_tool_number);
			} // End if - then
		} // End for
	} // End if - then

	return(-1);
}


/**
 * Find the Tool object whose tool number matches that passed in.
 */
int CTool::FindTool( const int tool_number )
{

	if (TOOLS)
	{
		HeeksObj* tool_list = TOOLS;

		for(HeeksObj* ob = tool_list->GetFirstChild(); ob; ob = tool_list->GetNextChild())
		{
			if (ob->GetType() != ToolType) continue;
			if ((ob != NULL) && (((CTool *) ob)->m_tool_number == tool_number))
			{
				return(ob->m_id);
			} // End if - then
		} // End for
	} // End if - then

	return(-1);

} // End FindTool() method

//static
CToolParams::eToolType CTool::FindToolType( const int tool_number )
{
	CTool* pTool = Find(tool_number);
	if(pTool)return pTool->m_params.m_type;
	return CToolParams::eUndefinedToolType;
}

// static
bool CTool::IsMillingToolType( CToolParams::eToolType type )
{
	switch(type)
	{
	case CToolParams::eEndmill:
	case CToolParams::eSlotCutter:
	case CToolParams::eBallEndMill:
	case CToolParams::eDrill:
	case CToolParams::eCentreDrill:
	case CToolParams::eChamfer:
		return true;
	default:
		return false;
	}
}


/**
	Find a fraction that represents this floating point number.  We use this
	purely for readability purposes.  It only looks accurate to the nearest 1/64th

	eg: 0.125 -> "1/8"
	    1.125 -> "1 1/8"
	    0.109375 -> "7/64"
 */
/* static */ wxString CTool::FractionalRepresentation( const double original_value, const int max_denominator /* = 64 */ )
{

    wxString result;
	double _value(original_value);
	double near_enough = 0.00001;

	if (floor(_value) > 0)
	{
		result << floor(_value) << _T(" ");
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
				result << numerator << _T("/") << denominator;
				return(result);
			} // End if - then
		} // End for
	} // End for


	result = _T("");	// It's not a recognisable fraction.  Return nothing to indicate such.
	return(result);
} // End FractionalRepresentation() method


/**
 * This method uses the various attributes of the tool to produce a meaningful name.
 * eg: with diameter = 6, units = 1 (mm) and type = 'drill' the name would be '6mm Drill Bit".  The
 * idea is to produce a m_title value that is representative of the tool.  This will
 * make selection in the program list easier.
 */
wxString CTool::GetMeaningfulName(double units) const
{
#ifdef UNICODE
	std::wostringstream l_ossName;
#else
    std::ostringstream l_ossName;
#endif

	{
		if (units == 1.0)
		{
			// We're using metric.  Leave the diameter as a floating point number.  It just looks more natural.
			l_ossName << m_params.m_diameter / units << " mm ";
		} // End if - then
		else
		{
			// We're using inches.  Find a fractional representation if one matches.
			wxString fraction = FractionalRepresentation(m_params.m_diameter / units);

			if (fraction.Len() > 0)
			{
                l_ossName << fraction.c_str() << " inch ";
			}
			else
			{
		        l_ossName << m_params.m_diameter / units << " inch ";
			}
		} // End if - else
	} // End if - then

	switch (m_params.m_type)
	{
		case CToolParams::eDrill:	l_ossName << (_("Drill Bit"));
			break;

		case CToolParams::eCentreDrill:	l_ossName << (_("Centre Drill Bit"));
			break;

        case CToolParams::eEndmill:	l_ossName << (_("End Mill"));
			break;

        case CToolParams::eSlotCutter:	l_ossName << (_("Slot Cutter"));
			break;

        case CToolParams::eBallEndMill:	l_ossName << (_("Ball End Mill"));
			break;

        case CToolParams::eChamfer:	l_ossName.str(_T(""));	// Remove all that we've already prepared.
			l_ossName << m_params.m_cutting_edge_angle << (_T(" degreee "));
					l_ossName << (_("Chamfering Bit"));
			break;
		default:
			break;
	} // End switch

	return( l_ossName.str().c_str() );
} // End GetMeaningfulName() method


/**
	Reset the m_title value with a meaningful name ONLY if it does not look like it was
	automatically generated in the first place.  If someone has reset it manually then leave it alone.

	Return a verbose description of what we've done (if anything) so that we can pop up a
	warning message to the operator letting them know.
 */
wxString CTool::ResetTitle()
{
	if (m_params.m_automatically_generate_title)
	{
		// It has the default title.  Give it a name that makes sense.
		m_title = GetMeaningfulName(heeksCAD->GetViewUnits());
		// to do, make undoable properties

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
        routines to paint the tool in the graphics window.

	We want to draw an outline of the tool in 2 dimensions so that the operator
	gets a feel for what the various tool parameter values mean.
 */
void CTool::glCommands(bool select, bool marked, bool no_color)
{
	// draw this tool only if it is selected
	if(heeksCAD->ObjectMarked(this) && !no_color)
	{
		if(!m_pToolSolid)
		{
			try {
				TopoDS_Shape tool_shape = GetShape();
				m_pToolSolid = heeksCAD->NewSolid( *((TopoDS_Solid *) &tool_shape), NULL, HeeksColor(234, 123, 89) );
			} // End try
			catch(Standard_DomainError) { }
			catch(...)  { }
		}

		if(m_pToolSolid)m_pToolSolid->glCommands( true, false, true );
	} // End if - then

} // End glCommands() method

void CTool::KillGLLists(void)
{
	DeleteSolid();
}

void CTool::DeleteSolid()
{
	if(m_pToolSolid)delete m_pToolSolid;
	m_pToolSolid = NULL;
}

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
TopoDS_Shape CTool::GetShape() const
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
	if (cutting_edge_height < (2 * diameter)) cutting_edge_height = 2 * diameter;

	switch (m_params.m_type)
	{
		case CToolParams::eCentreDrill:
		{
			// First a cylinder to represent the shaft.
			double tool_tip_length = (diameter / 2) * tan( degrees_to_radians(90.0 - m_params.m_cutting_edge_angle));
			double non_cutting_shaft_length = tool_length_offset - tool_tip_length - cutting_edge_height;

			gp_Pnt shaft_start_location( tool_tip_location );
			shaft_start_location.SetZ( tool_tip_location.Z() + tool_tip_length + cutting_edge_height );
			gp_Ax2 shaft_position_and_orientation( shaft_start_location, orientation );
			BRepPrimAPI_MakeCylinder shaft( shaft_position_and_orientation, (diameter / 2) * 2.0, non_cutting_shaft_length );

			gp_Pnt cutting_shaft_start_location( tool_tip_location );
			cutting_shaft_start_location.SetZ( tool_tip_location.Z() + tool_tip_length );
			gp_Ax2 cutting_shaft_position_and_orientation( cutting_shaft_start_location, orientation );
			BRepPrimAPI_MakeCylinder cutting_shaft( cutting_shaft_position_and_orientation, diameter / 2, cutting_edge_height );

			// And a cone for the tip.
			gp_Ax2 tip_position_and_orientation( cutting_shaft_start_location, gp_Dir(0,0,-1) );
			BRepPrimAPI_MakeCone tool_tip( tip_position_and_orientation,
							diameter/2,
							m_params.m_flat_radius,
							tool_tip_length);

			TopoDS_Shape shafts = BRepAlgoAPI_Fuse(shaft.Shape() , cutting_shaft.Shape() );
			TopoDS_Shape tool_shape = BRepAlgoAPI_Fuse(shafts , tool_tip.Shape() );
			return tool_shape;
		}

		case CToolParams::eDrill:
		{
			// First a cylinder to represent the shaft.
			double tool_tip_length = (diameter / 2) * tan( degrees_to_radians(90 - m_params.m_cutting_edge_angle));
			double shaft_length = tool_length_offset - tool_tip_length;

			gp_Pnt shaft_start_location( tool_tip_location );
			shaft_start_location.SetZ( tool_tip_location.Z() + tool_tip_length );

			gp_Ax2 shaft_position_and_orientation( shaft_start_location, orientation );

			BRepPrimAPI_MakeCylinder shaft( shaft_position_and_orientation, (diameter / 2), shaft_length );

			// And a cone for the tip.
			gp_Ax2 tip_position_and_orientation( shaft_start_location, gp_Dir(0,0,-1) );

			BRepPrimAPI_MakeCone tool_tip( tip_position_and_orientation,
							diameter/2,
							m_params.m_flat_radius,
							tool_tip_length);

			TopoDS_Shape tool_shape = BRepAlgoAPI_Fuse(shaft.Shape() , tool_tip.Shape() );
			return tool_shape;
		}

		case CToolParams::eChamfer:
		case CToolParams::eEngravingTool:
		{
			// First a cylinder to represent the shaft.
			if(m_params.m_cutting_edge_angle > 0.00001)
			{
			double tool_tip_length_a = (diameter / 2) / tan( degrees_to_radians(m_params.m_cutting_edge_angle));
			double tool_tip_length_b = (m_params.m_flat_radius)  / tan( degrees_to_radians(m_params.m_cutting_edge_angle));
			double tool_tip_length = tool_tip_length_a - tool_tip_length_b;

			double shaft_length = tool_length_offset - tool_tip_length;

			gp_Pnt shaft_start_location( tool_tip_location );
			shaft_start_location.SetZ( tool_tip_location.Z() + tool_tip_length );

			gp_Ax2 shaft_position_and_orientation( shaft_start_location, orientation );

			BRepPrimAPI_MakeCylinder shaft( shaft_position_and_orientation, diameter / 2, shaft_length );

			// And a cone for the tip.
			// double cutting_edge_angle_in_radians = ((m_params.m_cutting_edge_angle / 2) / 360) * (2 * M_PI);
			gp_Ax2 tip_position_and_orientation( shaft_start_location, gp_Dir(0,0,-1) );

			BRepPrimAPI_MakeCone tool_tip( tip_position_and_orientation,
							diameter/2,
							m_params.m_flat_radius,
							tool_tip_length);

			TopoDS_Shape tool_shape = BRepAlgoAPI_Fuse(shaft.Shape() , tool_tip.Shape() );
			return tool_shape;
			}
			else
			{
			// First a cylinder to represent the shaft.
			double shaft_length = tool_length_offset;
			gp_Pnt shaft_start_location( tool_tip_location );

			gp_Ax2 shaft_position_and_orientation( shaft_start_location, orientation );

			BRepPrimAPI_MakeCylinder shaft( shaft_position_and_orientation, diameter / 2, shaft_length );
			return(shaft.Shape());
			}
		}

		case CToolParams::eBallEndMill:
		{
			// First a cylinder to represent the shaft.
			double shaft_length = tool_length_offset - m_params.m_corner_radius;

			gp_Pnt shaft_start_location( tool_tip_location );
			shaft_start_location.SetZ( tool_tip_location.Z() + m_params.m_corner_radius );

			gp_Ax2 shaft_position_and_orientation( shaft_start_location, orientation );

			BRepPrimAPI_MakeCylinder shaft( shaft_position_and_orientation, diameter / 2, shaft_length );
			BRepPrimAPI_MakeSphere ball( shaft_start_location, diameter / 2 );

			// TopoDS_Compound tool_shape;
			TopoDS_Shape tool_shape;
			tool_shape = BRepAlgoAPI_Fuse(shaft.Shape() , ball.Shape() );
			return tool_shape;
		}

		default:
		{
			// First a cylinder to represent the shaft.
			double shaft_length = tool_length_offset;
			gp_Pnt shaft_start_location( tool_tip_location );

			gp_Ax2 shaft_position_and_orientation( shaft_start_location, orientation );

			BRepPrimAPI_MakeCylinder shaft( shaft_position_and_orientation, diameter / 2, shaft_length );
			return(shaft.Shape());
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

Python CTool::OCLDefinition(CSurface* surface) const
{
	Python python;

	switch (m_params.m_type)
	{
		case CToolParams::eBallEndMill:
			python << _T("ocl.BallCutter(float(") << m_params.m_diameter + surface->m_material_allowance * 2 << _T("), 1000)\n");
			break;

		case CToolParams::eChamfer:
		case CToolParams::eEngravingTool:
			python << _T("ocl.CylConeCutter(float(") << m_params.m_flat_radius * 2 + surface->m_material_allowance << _T("), float(") << m_params.m_diameter + surface->m_material_allowance * 2 << _T("), float(") << m_params.m_cutting_edge_angle * M_PI/360 << _T("))\n");
			break;

		default:
			if(this->m_params.m_corner_radius > 0.000000001)
			{
				python << _T("ocl.BullCutter(float(") << m_params.m_diameter + surface->m_material_allowance * 2 << _T("), float(") << m_params.m_corner_radius << _T("), 1000)\n");
			}
			else
			{
				python << _T("ocl.CylCutter(float(") << m_params.m_diameter + surface->m_material_allowance * 2 << _T("), 1000)\n");
			}
			break;
	} // End switch

	return python;

} 

Python CTool::VoxelcutDefinition()const
{
	Python python;

	python << _T("Tool([[Span(Point(");

	double height_above_cutting_edge = 30.0;
	double r = m_params.m_diameter/2;
	double h = m_params.m_cutting_edge_height;
	double cr = m_params.m_corner_radius;

	switch (m_params.m_type)
	{
	case CToolParams::eDrill:
		case CToolParams::eCentreDrill:
		case CToolParams::eChamfer:
		case CToolParams::eEngravingTool:
			{
				double max_cutting_height = 0.0;
				double radius_at_cutting_height = r;
				if(m_params.m_cutting_edge_angle < 0.01 || (r < m_params.m_flat_radius))
				{
					python << _T("float(") << m_params.m_flat_radius << _T("), 0), Vertex(Point(float(") << m_params.m_flat_radius << _T("), float(") << h << _T("))), False), GRAY], ");
					python << _T("[Span(Point(float(") << m_params.m_flat_radius << _T("), ") << h << _T("), Vertex(Point(float(") << m_params.m_flat_radius << _T("), float(") << h + height_above_cutting_edge << _T("))), False), RED]])");
				}
				else
				{
					double rad_diff = r - m_params.m_flat_radius;
					max_cutting_height = rad_diff / tan(m_params.m_cutting_edge_angle * 0.0174532925199432);
					radius_at_cutting_height = (h/max_cutting_height) * rad_diff + m_params.m_flat_radius;
					if(max_cutting_height > h)
					{
						python << _T("float(") << m_params.m_flat_radius << _T("), 0), Vertex(Point(float(") << radius_at_cutting_height << _T("), float(") << h << _T("))), False), GRAY],");
						python << _T("[Span(Point(float(") << radius_at_cutting_height << _T("), float(") << h << _T(")), Vertex(Point(float(") << r << _T("), float(") << max_cutting_height << _T("))), False), GRAY], ");
						python << _T("[Span(Point(float(") << r << _T("), float(") << max_cutting_height << _T(")), Vertex(Point(float(") << r << _T("), float(") << max_cutting_height + height_above_cutting_edge << _T("))), False), RED]])");
					}
					else
					{
						python << _T("float(") << m_params.m_flat_radius << _T("), 0), Vertex(Point(float(") << r << _T("), float(") << max_cutting_height << _T("))), False), GRAY],");
						python << _T("[Span(Point(float(") << r << _T("), float(") << max_cutting_height << _T(")), Vertex(Point(float(") << r << _T("), float(") << h << _T("))), False), GRAY], ");
						python << _T("[Span(Point(float(") << r << _T("), float(") << h << _T(")), Vertex(Point(float(") << r << _T("), float(") << h + height_above_cutting_edge << _T("))), False), RED]])");
					}
				}
			}
			break;
		case CToolParams::eBallEndMill:
			{
				if(h >= r)
				{
					python << _T("0, 0), Vertex(1, Point(float(") << r << _T("), float(") << r << _T(")), Point(0, float(") << r << _T("))), False), GRAY],");
					python << _T("[Span(Point(float(") << r << _T("), float(") << r << _T(")), Vertex(Point(float(") << r << _T("), float(") << h << _T("))), False), GRAY], ");
					python << _T("[Span(Point(float(") << r << _T("), float(") << h << _T(")), Vertex(Point(float(") << r << _T("), float(") << h + height_above_cutting_edge << _T("))), False), RED]])");
				}
				else
				{
					double x = sqrt(r*r - (r-h) * (r-h));

					python << _T("0, 0), Vertex(1, Point(float(") << x << _T("), float(") << h << _T(")), Point(0, float(") << r << _T("))), False), GRAY],");
					python << _T("[Span(Point(float(") << x << _T("), float(") << h << _T(")), Vertex(1, Point(float(") << r << _T("), float(") << r << _T(")), Point(0, float(") << r << _T("))), False), RED], ");
					python << _T("[Span(Point(float(") << r << _T("), float(") << r << _T(")), Vertex(Point(float(") << r << _T("), float(") << r + height_above_cutting_edge << _T("))), False), RED]])");
				}
			}
			break;
		default:
			if(cr > r)cr = r;
			if(cr > 0.0001)
			{
				if(h >= cr)
				{
					python << _T("float(") << r-cr << _T("), 0), Vertex(1, Point(float(") << r << _T("), float(") << cr << _T(")), Point(float(") << r-cr << _T("), float(") << cr << _T("))), False), GRAY],");
					python << _T("[Span(Point(float(") << r << _T("), float(") << r << _T(")), Vertex(Point(float(") << r << _T("), float(") << h << _T("))), False), GRAY], ");
					python << _T("[Span(Point(float(") << r << _T("), float(") << h << _T(")), Vertex(Point(float(") << r << _T("), float(") << h + height_above_cutting_edge << _T("))), False), RED]])");
				}
				else
				{
					double x = (r - cr) + sqrt(cr*cr - (cr-h) * (cr-h));

					python << _T("float(") << r-cr << _T("), 0), Vertex(1, Point(float(") << x << _T("), float(") << h << _T(")), Point(0, float(") << cr << _T("))), False), GRAY],");
					python << _T("[Span(Point(float(") << x << _T("), float(") << h << _T(")), Vertex(1, Point(float(") << r << _T("), float(") << cr << _T(")), Point(0, float(") << cr << _T("))), False), RED], ");
					python << _T("[Span(Point(float(") << r << _T("), float(") << cr << _T(")), Vertex(Point(float(") << r << _T("), float(") << cr + height_above_cutting_edge << _T("))), False), RED]])");
				}
			}
			else
			{
				python << _T("float(") << r << _T("), 0), Vertex(Point(float(") << r << _T("), float(") << h << _T("))), False), GRAY],");
				python << _T("[Span(Point(float(") << r << _T("), float(") << h << _T(")), Vertex(Point(float(") << r << _T("), float(") << h + height_above_cutting_edge << _T("))), False), RED]])");
			}
			break;
	} // End switch

	return python;
}

TopoDS_Face CTool::GetSideProfile() const
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
	if (cutting_edge_height < (2 * diameter)) cutting_edge_height = 2 * diameter;

    gp_Pnt top_left( tool_tip_location );
    gp_Pnt top_right( tool_tip_location );
    gp_Pnt bottom_left( tool_tip_location );
    gp_Pnt bottom_right( tool_tip_location );

    top_left.SetY( tool_tip_location.Y() - CuttingRadius());
    bottom_left.SetY( tool_tip_location.Y() - CuttingRadius());

    top_right.SetY( tool_tip_location.Y() + CuttingRadius());
    bottom_right.SetY( tool_tip_location.Y() + CuttingRadius());

    top_left.SetZ( tool_tip_location.Z() + cutting_edge_height);
    bottom_left.SetZ( tool_tip_location.Z() );

    top_right.SetZ( tool_tip_location.Z() + cutting_edge_height);
    bottom_right.SetZ( tool_tip_location.Z() );

    Handle(Geom_TrimmedCurve) seg1 = GC_MakeSegment(top_left, bottom_left);
    Handle(Geom_TrimmedCurve) seg2 = GC_MakeSegment(bottom_left, bottom_right);
    Handle(Geom_TrimmedCurve) seg3 = GC_MakeSegment(bottom_right, top_right);
    Handle(Geom_TrimmedCurve) seg4 = GC_MakeSegment(top_right, top_left);

    TopoDS_Edge edge1 = BRepBuilderAPI_MakeEdge(seg1);
    TopoDS_Edge edge2 = BRepBuilderAPI_MakeEdge(seg2);
    TopoDS_Edge edge3 = BRepBuilderAPI_MakeEdge(seg3);
    TopoDS_Edge edge4 = BRepBuilderAPI_MakeEdge(seg4);

    BRepBuilderAPI_MakeWire wire_maker;

    wire_maker.Add(edge1);
    wire_maker.Add(edge2);
    wire_maker.Add(edge3);
    wire_maker.Add(edge4);

    TopoDS_Face face = BRepBuilderAPI_MakeFace(wire_maker.Wire());
    return(face);
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
} // End GetSideProfile() method




/**
	The CuttingRadius is almost always the same as half the tool's diameter.
	The exception to this is if it's a chamfering bit.  In this case we
	want to use the flat_radius plus a little bit.  i.e. if we're chamfering the edge
	then we want to use the part of the cutting surface just a little way from
	the flat radius.  If it has a flat radius of zero (i.e. it has a pointed end)
	then it will be a small number.  If it is a carbide tipped bit then the
	flat radius will allow for the area below the bit that doesn't cut.  In this
	case we want to cut around the middle of the carbide tip.  In this case
	the carbide tip should represent the full cutting edge height.  We can
	use this method to make all these adjustments based on the tool's
	geometry and return a reasonable value.

	If express_in_program_units is true then we need to divide by the program
	units value.  We use metric (mm) internally and convert to inches only
	if we need to and only as the last step in the process.  By default, return
	the value in internal (metric) units.

	If the depth value is passed in as a positive number then the radius is given
	for the corresponding depth (from the bottom-most tip of the tool).  This is
	only relevant for chamfering (angled) bits.
 */
double CTool::CuttingRadius( const bool express_in_program_units /* = false */, const double depth /* = -1 */ ) const
{
	double radius;

	switch (m_params.m_type)
	{
		case CToolParams::eChamfer:
			{
			    if (depth < 0.0)
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
			    else
			    {
			        radius = m_params.m_flat_radius + (depth * tan((m_params.m_cutting_edge_angle / 360.0 * 2 * M_PI)));
			        if (radius > (m_params.m_diameter / 2.0))
			        {
			            // The angle and depth would have us cutting larger than our largest diameter.
			            radius = (m_params.m_diameter / 2.0);
			        }
			    }
			}
			break;

		case CToolParams::eEngravingTool:
			{
	            radius = m_params.m_flat_radius;
			}
			break;

		default:
			radius = m_params.m_diameter/2;
	} // End switch

	if (express_in_program_units) return(radius / PROGRAM->m_units);
	else return(radius);

} // End CuttingRadius() method


CToolParams::eToolType CTool::CutterType( const int tool_number )
{
	if (tool_number <= 0) return(CToolParams::eUndefinedToolType);

	CTool *pTool = CTool::Find( tool_number );
	if (pTool == NULL) return(CToolParams::eUndefinedToolType);

	return(pTool->m_params.m_type);
} // End of CutterType() method

CToolParams::eMaterial_t CTool::CutterMaterial( const int tool_number )
{
	if (tool_number <= 0) return(CToolParams::eUndefinedMaterialType);

	CTool *pTool = CTool::Find( tool_number );
	if (pTool == NULL) return(CToolParams::eUndefinedMaterialType);

	return(CToolParams::eMaterial_t(pTool->m_params.m_material));
} // End of CutterType() method

void CTool::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
}

bool CToolParams::operator==( const CToolParams & rhs ) const
{
	if (m_material != rhs.m_material) return(false);
	if (m_diameter != rhs.m_diameter) return(false);
	if (m_tool_length_offset != rhs.m_tool_length_offset) return(false);
	if (m_corner_radius != rhs.m_corner_radius) return(false);
	if (m_flat_radius != rhs.m_flat_radius) return(false);
	if (m_cutting_edge_angle != rhs.m_cutting_edge_angle) return(false);
	if (m_cutting_edge_height != rhs.m_cutting_edge_height) return(false);
	if (m_type != rhs.m_type) return(false);
	if (m_automatically_generate_title != rhs.m_automatically_generate_title) return(false);
	return(true);
}

bool CTool::operator==( const CTool & rhs ) const
{
	if (m_params != rhs.m_params) return(false);
	if (m_title != rhs.m_title) return(false);
	if (m_tool_number != rhs.m_tool_number) return(false);

	return(true);
}

Python CTool::OpenCamLibDefinition(const unsigned int indent /* = 0 */ )const
{
	Python python;
	Python _indent;

	for (::size_t i=0; i<indent; i++)
	{
		_indent << _T("\040\040\040\040");
	}

	switch (m_params.m_type)
	{
	case CToolParams::eBallEndMill:
		python << _indent << _T("ocl.BallCutter(") << CuttingRadius(false) << _T(", 1000)");
		return(python);

	case CToolParams::eEndmill:
	case CToolParams::eSlotCutter:
		python << _indent << _T("ocl.CylCutter(") << CuttingRadius(false) << _T(", 1000)");
		return(python);

	default:
        return(python); // Avoid the compiler warnings.
	} // End switch

	return(python);
}

static bool OnEdit(HeeksObj* object)
{
	CToolDlg dlg(heeksCAD->GetMainFrame(), (CTool*)object);
	if(dlg.ShowModal() == wxID_OK)
	{
		dlg.GetData((CTool*)object);
		((CToolParams*)object)->write_values_to_config();
		return true;
	}
	return false;
}

void CTool::GetOnEdit(bool(**callback)(HeeksObj*))
{
#if 1
	*callback = OnEdit;
#else
	*callback = NULL;
#endif
}

void CTool::OnChangeViewUnits(const double units)
{
	if (m_params.m_automatically_generate_title)m_title = GetMeaningfulName(heeksCAD->GetViewUnits());
}

void CTool::WriteDefaultValues()
{
	m_params.write_values_to_config();
}

void CTool::ReadDefaultValues()
{
	m_params.set_initial_values();
}

HeeksObj* CTool::PreferredPasteTarget()
{
	return theApp.m_program->Tools();
}
