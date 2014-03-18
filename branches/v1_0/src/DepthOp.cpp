// DepthOp.cpp
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "stdafx.h"
#include "DepthOp.h"
#include "CNCConfig.h"
#include "ProgramCanvas.h"
#include "Program.h"
#include "interface/PropertyInt.h"
#include "interface/PropertyDouble.h"
#include "interface/PropertyLength.h"
#include "interface/PropertyString.h"
#include "tinyxml/tinyxml.h"
#include "interface/Tool.h"
#include "CTool.h"


CDepthOpParams::CDepthOpParams()
{
	m_clearance_height = 0.0;
	m_start_depth = 0.0;
	m_step_down = 0.0;
	m_z_finish_depth = 0.0;
	m_z_thru_depth = 0.0;
	m_final_depth = 0.0;
	m_rapid_safety_space = 0.0;

}

CDepthOp & CDepthOp::operator= ( const CDepthOp & rhs )
{
	if (this != &rhs)
	{
		CSpeedOp::operator=( rhs );
		m_depth_op_params = rhs.m_depth_op_params;
	}

	return(*this);
}

CDepthOp::CDepthOp( const CDepthOp & rhs ) : CSpeedOp(rhs)
{
	m_depth_op_params = rhs.m_depth_op_params;
}

void CDepthOp::ReloadPointers()
{
	CSpeedOp::ReloadPointers();
}

static double degrees_to_radians( const double degrees )
{
	return( (degrees / 360.0) * (2 * M_PI) );
} // End degrees_to_radians() routine

/**
	Set the starting depth to match the Z values on the sketches.

	If we've selected a chamfering bit then set the final depth such
	that a 1 mm chamfer is applied.  These are only starting points but
	we should make them as convenient as possible.
 */
static void on_set_clearance_height(double value, HeeksObj* object)
{
	((CDepthOp*)object)->m_depth_op_params.m_clearance_height = value;
	((CDepthOp*)object)->WriteDefaultValues();
}

static void on_set_step_down(double value, HeeksObj* object)
{
	((CDepthOp*)object)->m_depth_op_params.m_step_down = value;
	((CDepthOp*)object)->WriteDefaultValues();
}

static void on_set_z_finish_depth(double value, HeeksObj* object)
{
	((CDepthOp*)object)->m_depth_op_params.m_z_finish_depth = value;
	((CDepthOp*)object)->WriteDefaultValues();
}

static void on_set_z_thru_depth(double value, HeeksObj* object)
{
	((CDepthOp*)object)->m_depth_op_params.m_z_thru_depth = value;
	((CDepthOp*)object)->WriteDefaultValues();
}

static void on_set_user_depths(const wxChar* value, HeeksObj* object)
{
	((CDepthOp*)object)->m_depth_op_params.m_user_depths.assign(value);
	((CDepthOp*)object)->WriteDefaultValues();
}

static void on_set_start_depth(double value, HeeksObj* object)
{
	((CDepthOp*)object)->m_depth_op_params.m_start_depth = value;
	((CDepthOp*)object)->WriteDefaultValues();
}

static void on_set_final_depth(double value, HeeksObj* object)
{
	((CDepthOp*)object)->m_depth_op_params.m_final_depth = value;
	((CDepthOp*)object)->WriteDefaultValues();
}

static void on_set_rapid_safety_space(double value, HeeksObj* object)
{
	((CDepthOp*)object)->m_depth_op_params.m_rapid_safety_space = value;
	((CDepthOp*)object)->WriteDefaultValues();
}

void CDepthOpParams::GetProperties(CDepthOp* parent, std::list<Property *> *list)
{
	list->push_back(new PropertyLength(_("clearance height"), m_clearance_height, parent, on_set_clearance_height));

	if(CTool::IsMillingToolType(CTool::FindToolType(parent->m_tool_number)))
	{
		list->push_back(new PropertyLength(_("rapid safety space"), m_rapid_safety_space, parent, on_set_rapid_safety_space));
		list->push_back(new PropertyLength(_("start depth"), m_start_depth, parent, on_set_start_depth));
		list->push_back(new PropertyLength(_("final depth"), m_final_depth, parent, on_set_final_depth));
		list->push_back(new PropertyLength(_("max step down"), m_step_down, parent, on_set_step_down));
		list->push_back(new PropertyLength(_("z finish depth"), m_z_finish_depth, parent, on_set_z_finish_depth));
		list->push_back(new PropertyLength(_("z thru depth"), m_z_thru_depth, parent, on_set_z_thru_depth));
		list->push_back(new PropertyString(_("user depths"), m_user_depths, parent, on_set_user_depths));
	}
}

void CDepthOpParams::WriteXMLAttributes(TiXmlNode* pElem)
{
	TiXmlElement * element = heeksCAD->NewXMLElement( "depthop" );
	heeksCAD->LinkXMLEndChild( pElem,  element );
	element->SetDoubleAttribute( "clear", m_clearance_height);
	element->SetDoubleAttribute( "down", m_step_down);
	if(m_z_finish_depth > 0.0000001)element->SetDoubleAttribute( "zfinish", m_z_finish_depth);
	if(m_z_thru_depth > 0.0000001)element->SetDoubleAttribute( "zthru", m_z_thru_depth);
	element->SetAttribute("userdepths", m_user_depths.utf8_str());
	element->SetDoubleAttribute( "startdepth", m_start_depth);
	element->SetDoubleAttribute( "depth", m_final_depth);
	element->SetDoubleAttribute( "r", m_rapid_safety_space);
}

void CDepthOpParams::ReadFromXMLElement(TiXmlElement* pElem)
{
	TiXmlElement* element = heeksCAD->FirstNamedXMLChildElement(pElem, "depthop");
	if(element)
	{
		element->Attribute("clear", &m_clearance_height);
		element->Attribute("down", &m_step_down);
		element->Attribute("zfinish", &m_z_finish_depth);
		element->Attribute("zthru", &m_z_thru_depth);
		m_user_depths.assign(Ctt(element->Attribute("userdepths")));
		element->Attribute("startdepth", &m_start_depth);
		element->Attribute("depth", &m_final_depth);
		element->Attribute("r", &m_rapid_safety_space);
		heeksCAD->RemoveXMLChild(pElem, element);	// We don't want to interpret this again when
										// the ObjList::ReadBaseXML() method gets to it.
	}
}

void CDepthOp::glCommands(bool select, bool marked, bool no_color)
{
	CSpeedOp::glCommands(select, marked, no_color);
}

void CDepthOp::WriteBaseXML(TiXmlElement *element)
{
	m_depth_op_params.WriteXMLAttributes(element);
	CSpeedOp::WriteBaseXML(element);
}

void CDepthOp::ReadBaseXML(TiXmlElement* element)
{
	m_depth_op_params.ReadFromXMLElement(element);
	CSpeedOp::ReadBaseXML(element);
}

void CDepthOp::GetProperties(std::list<Property *> *list)
{
	m_depth_op_params.GetProperties(this, list);
	CSpeedOp::GetProperties(list);
}

void CDepthOp::WriteDefaultValues()
{
	CSpeedOp::WriteDefaultValues();

	CNCConfig config;
	config.Write(_T("ClearanceHeight"), m_depth_op_params.m_clearance_height);
	config.Write(_T("StartDepth"), m_depth_op_params.m_start_depth);
	config.Write(_T("StepDown"), m_depth_op_params.m_step_down);
	config.Write(_T("ZFinish"), m_depth_op_params.m_z_finish_depth);
	config.Write(_T("ZThru"), m_depth_op_params.m_z_thru_depth);
	config.Write(_T("UserDepths"), m_depth_op_params.m_user_depths);
	config.Write(_T("FinalDepth"), m_depth_op_params.m_final_depth);
	config.Write(_T("RapidDown"), m_depth_op_params.m_rapid_safety_space);
}

void CDepthOp::ReadDefaultValues()
{
	CSpeedOp::ReadDefaultValues();

	CNCConfig config;

	config.Read(_T("ClearanceHeight"), &m_depth_op_params.m_clearance_height, 5.0);
	config.Read(_T("StartDepth"), &m_depth_op_params.m_start_depth, 0.0);
	config.Read(_T("StepDown"), &m_depth_op_params.m_step_down, 1.0);
	config.Read(_T("ZFinish"), &m_depth_op_params.m_z_finish_depth, 0.0);
	config.Read(_T("ZThru"), &m_depth_op_params.m_z_thru_depth, 0.0);
	config.Read(_T("UserDepths"), &m_depth_op_params.m_user_depths, _T(""));
	config.Read(_T("FinalDepth"), &m_depth_op_params.m_final_depth, -1.0);
	config.Read(_T("RapidDown"), &m_depth_op_params.m_rapid_safety_space, 2.0);
}

Python CDepthOp::AppendTextToProgram()
{
	Python python;

    python << CSpeedOp::AppendTextToProgram();

	python << _T("depthparams = depth_params(");
	python << _T("float(") << m_depth_op_params.m_clearance_height / theApp.m_program->m_units << _T(")");
	python << _T(", float(") << m_depth_op_params.m_rapid_safety_space / theApp.m_program->m_units << _T(")");
    python << _T(", float(") << m_depth_op_params.m_start_depth / theApp.m_program->m_units << _T(")");
    python << _T(", float(") << m_depth_op_params.m_step_down / theApp.m_program->m_units << _T(")");
    python << _T(", float(") << m_depth_op_params.m_z_finish_depth / theApp.m_program->m_units << _T(")");
    python << _T(", float(") << m_depth_op_params.m_z_thru_depth / theApp.m_program->m_units << _T(")");
    python << _T(", float(") << m_depth_op_params.m_final_depth / theApp.m_program->m_units << _T(")");
	if(m_depth_op_params.m_user_depths.Len() == 0) python << _T(", None");
    else python << _T(", [") << m_depth_op_params.m_user_depths << _T("]");
	python << _T(")\n");

	CTool *pTool = CTool::Find( m_tool_number );
	if (pTool != NULL)
	{
		python << _T("tool_diameter = float(") << (pTool->CuttingRadius(true) * 2.0) << _T(")\n");
		python << _T("cutting_edge_angle = float(") << pTool->m_params.m_cutting_edge_angle<< _T(")\n");

	} // End if - then

	return(python);
}

void CDepthOp::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
    CSpeedOp::GetTools( t_list, p );
}

bool CDepthOpParams::operator== ( const CDepthOpParams & rhs ) const
{
	if (m_clearance_height != rhs.m_clearance_height) return(false);
	if (m_start_depth != rhs.m_start_depth) return(false);
	if (m_step_down != rhs.m_step_down) return(false);
	if (m_z_finish_depth != rhs.m_z_finish_depth) return(false);
	if (m_z_thru_depth != rhs.m_z_thru_depth) return(false);
	if (m_final_depth != rhs.m_final_depth) return(false);
	if (m_rapid_safety_space != rhs.m_rapid_safety_space) return(false);

	return(true);
}

bool CDepthOp::operator== ( const CDepthOp & rhs ) const
{
	if (m_depth_op_params != rhs.m_depth_op_params) return(false);
	return(CSpeedOp::operator==(rhs));
}
