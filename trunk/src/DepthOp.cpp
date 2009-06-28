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
#include "tinyxml/tinyxml.h"
#include "interface/Tool.h"
#include "CuttingTool.h"


CDepthOpParams::CDepthOpParams()
{
	m_tool_number = 0;    
	m_tool_diameter = 0.0;
	m_clearance_height = 0.0;
	m_start_depth = 0.0;
	m_step_down = 0.0;
	m_final_depth = 0.0;
	m_rapid_down_to_height = 0.0;
	m_horizontal_feed_rate = 0.0;
	m_vertical_feed_rate = 0.0;
	m_spindle_speed = 0.0;
}

void CDepthOpParams::set_initial_values( const int cutting_tool_number )
{
	CNCConfig config;
    	config.Read(_T("DepthFixtureOffset"), &m_workplane,1);
	config.Read(_T("DepthOpToolNumber"), &m_tool_number, 1);
	config.Read(_T("DepthOpToolDiameter"), &m_tool_diameter, 3.0);
	config.Read(_T("DepthOpClearanceHeight"), &m_clearance_height, 5.0);
	config.Read(_T("DepthOpStartDepth"), &m_start_depth, 0.0);
	config.Read(_T("DepthOpStepDown"), &m_step_down, 1.0);
	config.Read(_T("DepthOpFinalDepth"), &m_final_depth, -1.0);
	config.Read(_T("DepthOpRapidDown"), &m_rapid_down_to_height, 2.0);
	config.Read(_T("DepthOpHorizFeed"), &m_horizontal_feed_rate, 100.0);
	config.Read(_T("DepthOpVertFeed"), &m_vertical_feed_rate, 100.0);
	config.Read(_T("DepthOpSpindleSpeed"), &m_spindle_speed, 7000);

	if ((m_workplane < 1) || (m_workplane > 9)) m_workplane = 1;	// Default to the G54 (first) coordinate system.

	if (cutting_tool_number > 0)
	{
		m_tool_number = cutting_tool_number;

		if (CCuttingTool::FindCuttingTool( m_tool_number ) > 0)
		{
			HeeksObj *ob = heeksCAD->GetIDObject( CuttingToolType, CCuttingTool::FindCuttingTool( m_tool_number ) );
			if (ob != NULL)
			{
				m_tool_diameter = ((CCuttingTool *) ob)->m_params.m_diameter;
			} // End if - then
		} // End if - then
	} // End if - then
}

void CDepthOpParams::write_values_to_config()
{
	CNCConfig config;
	config.Write(_T("DepthFixtureOffset"), m_workplane);
	config.Write(_T("DepthOpToolNumber"), m_tool_number);
	config.Write(_T("DepthOpToolDiameter"), m_tool_diameter);
	config.Write(_T("DepthOpClearanceHeight"), m_clearance_height);
	config.Write(_T("DepthOpStartDepth"), m_start_depth);
	config.Write(_T("DepthOpStepDown"), m_step_down);
	config.Write(_T("DepthOpFinalDepth"), m_final_depth);
	config.Write(_T("DepthOpRapidDown"), m_rapid_down_to_height);
	config.Write(_T("DepthOpHorizFeed"), m_horizontal_feed_rate);
	config.Write(_T("DepthOpVertFeed"), m_vertical_feed_rate);
	config.Write(_T("DepthOpSpindleSpeed"), m_spindle_speed);
}

static void on_set_workplane(int value, HeeksObj* object){
	if ((value < 1) || (value > 9))
	{
		wxMessageBox( _T("Fixture offset must be between 1 and 9.  Aborting change") );
		return;
	} // End if - then

	((CDepthOp*)object)->m_depth_op_params.m_workplane = value;
} // End on_set_workplane() routine


static void on_set_tool_number(int value, HeeksObj* object){((CDepthOp*)object)->m_depth_op_params.m_tool_number = value;}
static void on_set_tool_diameter(double value, HeeksObj* object){((CDepthOp*)object)->m_depth_op_params.m_tool_diameter = value;}
static void on_set_clearance_height(double value, HeeksObj* object){((CDepthOp*)object)->m_depth_op_params.m_clearance_height = value;}
static void on_set_step_down(double value, HeeksObj* object){((CDepthOp*)object)->m_depth_op_params.m_step_down = value;}
static void on_set_start_depth(double value, HeeksObj* object){((CDepthOp*)object)->m_depth_op_params.m_start_depth = value;}
static void on_set_final_depth(double value, HeeksObj* object){((CDepthOp*)object)->m_depth_op_params.m_final_depth = value;}
static void on_set_rapid_down_to_height(double value, HeeksObj* object){((CDepthOp*)object)->m_depth_op_params.m_rapid_down_to_height = value;}
static void on_set_horizontal_feed_rate(double value, HeeksObj* object){((CDepthOp*)object)->m_depth_op_params.m_horizontal_feed_rate = value;}
static void on_set_vertical_feed_rate(double value, HeeksObj* object){((CDepthOp*)object)->m_depth_op_params.m_vertical_feed_rate = value;}
static void on_set_spindle_speed(double value, HeeksObj* object){((CDepthOp*)object)->m_depth_op_params.m_spindle_speed = value;}

void CDepthOpParams::GetProperties(CDepthOp* parent, std::list<Property *> *list)
{
	list->push_back(new PropertyInt(_("fixture offset"), m_workplane, parent, on_set_workplane));
	list->push_back(new PropertyInt(_("tool number"), m_tool_number, parent, on_set_tool_number));
	list->push_back(new PropertyLength(_("tool diameter"), m_tool_diameter, parent, on_set_tool_diameter));
	list->push_back(new PropertyLength(_("clearance height"), m_clearance_height, parent, on_set_clearance_height));
	list->push_back(new PropertyLength(_("step down"), m_step_down, parent, on_set_step_down));
	list->push_back(new PropertyLength(_("start depth"), m_start_depth, parent, on_set_start_depth));
	list->push_back(new PropertyLength(_("final depth"), m_final_depth, parent, on_set_final_depth));
	list->push_back(new PropertyLength(_("rapid down to height"), m_rapid_down_to_height, parent, on_set_rapid_down_to_height));
	list->push_back(new PropertyDouble(_("horizontal feed rate"), m_horizontal_feed_rate, parent, on_set_horizontal_feed_rate));
	list->push_back(new PropertyDouble(_("vertical feed rate"), m_vertical_feed_rate, parent, on_set_vertical_feed_rate));
	list->push_back(new PropertyDouble(_("spindle speed"), m_spindle_speed, parent, on_set_spindle_speed));
}

void CDepthOpParams::WriteXMLAttributes(TiXmlNode* pElem)
{
	TiXmlElement * element = new TiXmlElement( "depthop" );
	pElem->LinkEndChild( element ); 
	element->SetAttribute("fixture_offset", m_workplane); 
	element->SetAttribute("tooln", m_tool_number);
	element->SetDoubleAttribute("toold", m_tool_diameter);
	element->SetDoubleAttribute("clear", m_clearance_height);
	element->SetDoubleAttribute("down", m_step_down);
	element->SetDoubleAttribute("startdepth", m_start_depth);
	element->SetDoubleAttribute("depth", m_final_depth);
	element->SetDoubleAttribute("r", m_rapid_down_to_height);
	element->SetDoubleAttribute("hfeed", m_horizontal_feed_rate);
	element->SetDoubleAttribute("vfeed", m_vertical_feed_rate);
	element->SetDoubleAttribute("spin", m_spindle_speed);
}

void CDepthOpParams::ReadFromXMLElement(TiXmlElement* pElem)
{
	TiXmlElement* depthop = TiXmlHandle(pElem).FirstChildElement("depthop").Element();
	if(depthop)
	{

		depthop->Attribute("fixture_offset", &m_workplane);
		depthop->Attribute("tooln", &m_tool_number);
		depthop->Attribute("toold", &m_tool_diameter);
		depthop->Attribute("clear", &m_clearance_height);
		depthop->Attribute("down", &m_step_down);
		depthop->Attribute("startdepth", &m_start_depth);
		depthop->Attribute("depth", &m_final_depth);
		depthop->Attribute("r", &m_rapid_down_to_height);
		depthop->Attribute("hfeed", &m_horizontal_feed_rate);
		depthop->Attribute("vfeed", &m_vertical_feed_rate);
		depthop->Attribute("spin", &m_spindle_speed);
	}
}

void CDepthOp::WriteBaseXML(TiXmlElement *element)
{
	m_depth_op_params.WriteXMLAttributes(element);
	COp::WriteBaseXML(element);
}

void CDepthOp::ReadBaseXML(TiXmlElement* element)
{
	m_depth_op_params.ReadFromXMLElement(element);
	COp::ReadBaseXML(element);
}

void CDepthOp::GetProperties(std::list<Property *> *list)
{
	m_depth_op_params.GetProperties(this, list);
	COp::GetProperties(list);
}

void CDepthOp::AppendTextToProgram()
{
	COp::AppendTextToProgram();

	theApp.m_program_canvas->AppendText(_T("clearance = float("));
	theApp.m_program_canvas->AppendText(m_depth_op_params.m_clearance_height / theApp.m_program->m_units);
	theApp.m_program_canvas->AppendText(_T(")\n"));

	theApp.m_program_canvas->AppendText(_T("rapid_down_to_height = float("));
	theApp.m_program_canvas->AppendText(m_depth_op_params.m_rapid_down_to_height / theApp.m_program->m_units);
	theApp.m_program_canvas->AppendText(_T(")\n"));

	theApp.m_program_canvas->AppendText(_T("start_depth = float("));
	theApp.m_program_canvas->AppendText(m_depth_op_params.m_start_depth / theApp.m_program->m_units);
	theApp.m_program_canvas->AppendText(_T(")\n"));

	theApp.m_program_canvas->AppendText(_T("step_down = float("));
	theApp.m_program_canvas->AppendText(m_depth_op_params.m_step_down / theApp.m_program->m_units);
	theApp.m_program_canvas->AppendText(_T(")\n"));

	theApp.m_program_canvas->AppendText(_T("final_depth = float("));
	theApp.m_program_canvas->AppendText(m_depth_op_params.m_final_depth / theApp.m_program->m_units);
	theApp.m_program_canvas->AppendText(_T(")\n"));

	theApp.m_program_canvas->AppendText(_T("tool_diameter = float("));
	theApp.m_program_canvas->AppendText(m_depth_op_params.m_tool_diameter / theApp.m_program->m_units);
	theApp.m_program_canvas->AppendText(_T(")\n"));

	if (m_depth_op_params.m_spindle_speed != 0)
	{
		theApp.m_program_canvas->AppendText(_T("spindle("));
		theApp.m_program_canvas->AppendText(m_depth_op_params.m_spindle_speed);
		theApp.m_program_canvas->AppendText(_T(")\n"));
	} // End if - then

	theApp.m_program_canvas->AppendText(_T("feedrate_hv("));
	theApp.m_program_canvas->AppendText(m_depth_op_params.m_horizontal_feed_rate);
	theApp.m_program_canvas->AppendText(_T(", "));

	theApp.m_program_canvas->AppendText(m_depth_op_params.m_vertical_feed_rate);
	theApp.m_program_canvas->AppendText(_T(")\n"));

	if (m_depth_op_params.m_tool_number > 0)
	{
		theApp.m_program_canvas->AppendText(_T("tool_change("));
		theApp.m_program_canvas->AppendText(m_depth_op_params.m_tool_number);
		theApp.m_program_canvas->AppendText(_T(")\n"));
	} // End if - then

	if (m_depth_op_params.m_workplane >= 1)
	{
		theApp.m_program_canvas->AppendText(_T("workplane("));
		theApp.m_program_canvas->AppendText(m_depth_op_params.m_workplane);
		theApp.m_program_canvas->AppendText(_T(")\n"));
	} // End if - then

	theApp.m_program_canvas->AppendText(_T("flush_nc()\n"));
}

