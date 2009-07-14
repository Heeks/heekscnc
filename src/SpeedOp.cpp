// SpeedOp.cpp
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "stdafx.h"
#include "SpeedOp.h"
#include "CNCConfig.h"
#include "ProgramCanvas.h"
#include "Program.h"
#include "interface/PropertyInt.h"
#include "interface/PropertyDouble.h"
#include "interface/PropertyLength.h"
#include "tinyxml/tinyxml.h"
#include "interface/Tool.h"
#include "CuttingTool.h"


CSpeedOpParams::CSpeedOpParams()
{
	m_horizontal_feed_rate = 0.0;
	m_vertical_feed_rate = 0.0;
	m_spindle_speed = 0.0;
}

void CSpeedOpParams::set_initial_values( const int cutting_tool_number )
{
	CNCConfig config;
	config.Read(_T("SpeedOpHorizFeed"), &m_horizontal_feed_rate, 100.0);
	config.Read(_T("SpeedOpVertFeed"), &m_vertical_feed_rate, 100.0);
	config.Read(_T("SpeedOpSpindleSpeed"), &m_spindle_speed, 7000);
}

void CSpeedOpParams::write_values_to_config()
{
	CNCConfig config;
	config.Write(_T("SpeedOpHorizFeed"), m_horizontal_feed_rate);
	config.Write(_T("SpeedOpVertFeed"), m_vertical_feed_rate);
	config.Write(_T("SpeedOpSpindleSpeed"), m_spindle_speed);
}

static void on_set_horizontal_feed_rate(double value, HeeksObj* object){((CSpeedOp*)object)->m_speed_op_params.m_horizontal_feed_rate = value;}
static void on_set_vertical_feed_rate(double value, HeeksObj* object){((CSpeedOp*)object)->m_speed_op_params.m_vertical_feed_rate = value;}
static void on_set_spindle_speed(double value, HeeksObj* object){((CSpeedOp*)object)->m_speed_op_params.m_spindle_speed = value;}

void CSpeedOpParams::GetProperties(CSpeedOp* parent, std::list<Property *> *list)
{
	list->push_back(new PropertyDouble(_("horizontal feed rate"), m_horizontal_feed_rate, parent, on_set_horizontal_feed_rate));
	list->push_back(new PropertyDouble(_("vertical feed rate"), m_vertical_feed_rate, parent, on_set_vertical_feed_rate));
	list->push_back(new PropertyDouble(_("spindle speed"), m_spindle_speed, parent, on_set_spindle_speed));
}

void CSpeedOpParams::WriteXMLAttributes(TiXmlNode* pElem)
{
	TiXmlElement * element = new TiXmlElement( "speedop" );
	pElem->LinkEndChild( element ); 
	element->SetDoubleAttribute("hfeed", m_horizontal_feed_rate);
	element->SetDoubleAttribute("vfeed", m_vertical_feed_rate);
	element->SetDoubleAttribute("spin", m_spindle_speed);
}

void CSpeedOpParams::ReadFromXMLElement(TiXmlElement* pElem)
{
	TiXmlElement* speedop = TiXmlHandle(pElem).FirstChildElement("speedop").Element();
	if(speedop)
	{
		speedop->Attribute("hfeed", &m_horizontal_feed_rate);
		speedop->Attribute("vfeed", &m_vertical_feed_rate);
		speedop->Attribute("spin", &m_spindle_speed);
	}
}

void CSpeedOp::WriteBaseXML(TiXmlElement *element)
{
	m_speed_op_params.WriteXMLAttributes(element);
	COp::WriteBaseXML(element);
}

void CSpeedOp::ReadBaseXML(TiXmlElement* element)
{
	m_speed_op_params.ReadFromXMLElement(element);
	COp::ReadBaseXML(element);
}

void CSpeedOp::GetProperties(std::list<Property *> *list)
{
	m_speed_op_params.GetProperties(this, list);
	COp::GetProperties(list);
}

void CSpeedOp::AppendTextToProgram(const CFixture *pFixture)
{
	COp::AppendTextToProgram(pFixture);

	if (m_speed_op_params.m_spindle_speed != 0)
	{
		theApp.m_program_canvas->AppendText(_T("spindle("));
		theApp.m_program_canvas->AppendText(m_speed_op_params.m_spindle_speed);
		theApp.m_program_canvas->AppendText(_T(")\n"));
	} // End if - then

	theApp.m_program_canvas->AppendText(_T("feedrate_hv("));
	theApp.m_program_canvas->AppendText(m_speed_op_params.m_horizontal_feed_rate);
	theApp.m_program_canvas->AppendText(_T(", "));

	theApp.m_program_canvas->AppendText(m_speed_op_params.m_vertical_feed_rate);
	theApp.m_program_canvas->AppendText(_T(")\n"));

	theApp.m_program_canvas->AppendText(_T("flush_nc()\n"));
}

