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

	ResetSpeeds(cutting_tool_number);	// NOTE: The speed MUST be set BEFORE the feedrates
	ResetFeeds(cutting_tool_number);
}

void CSpeedOpParams::ResetFeeds(const int cutting_tool_number)
{
	if ((theApp.m_program) &&
	    (theApp.m_program->SpeedReferences()) &&
	    (theApp.m_program->SpeedReferences()->m_estimate_when_possible))
	{

		// Use the 'feeds and speeds' class along with the cutting tool properties to
		// help set some logical values for the spindle speed.

		if ((cutting_tool_number > 0) && (CCuttingTool::Find( cutting_tool_number ) != NULL))
		{
			CCuttingTool *pCuttingTool = CCuttingTool::Find( cutting_tool_number );
			if (pCuttingTool != NULL)
			{
				if (pCuttingTool->m_params.m_max_advance_per_revolution > 0) 
				{
					// Spindle speed is in revolutions per minute.
					double advance_per_rev = pCuttingTool->m_params.m_max_advance_per_revolution;
					double feed_rate_mm_per_minute = m_spindle_speed * advance_per_rev;

					// Now we need to decide whether we assign this value to the vertical
					// or horozontal (or both) feed rates.  Use the cutting tool type to
					// decide on the typical usage.

					switch (pCuttingTool->m_params.m_type)
					{
						case CCuttingToolParams::eDrill:
						case CCuttingToolParams::eCentreDrill:
							m_vertical_feed_rate = feed_rate_mm_per_minute;
							break;
				
						case CCuttingToolParams::eChamfer:
						case CCuttingToolParams::eTurningTool:
							// Spread it across both horizontal and vertical
							m_vertical_feed_rate = (1.0/sqrt(2.0)) * feed_rate_mm_per_minute;
							m_horizontal_feed_rate = (1.0/sqrt(2.0)) * feed_rate_mm_per_minute;
							break;
				
						case CCuttingToolParams::eEndmill:
						case CCuttingToolParams::eBallEndMill:
						case CCuttingToolParams::eSlotCutter:
						default:
							m_horizontal_feed_rate = feed_rate_mm_per_minute;
							break;
					} // End switch
				} // End if - then
			} // End if - then
		} // End if - then
	} // End if - then
} // End ResetFeeds() method

void CSpeedOpParams::ResetSpeeds(const int cutting_tool_number)
{
	if ((theApp.m_program) &&
	    (theApp.m_program->SpeedReferences()) &&
	    (theApp.m_program->SpeedReferences()->m_estimate_when_possible))
	{

		// Use the 'feeds and speeds' class along with the cutting tool properties to
		// help set some logical values for the spindle speed.

		if ((cutting_tool_number > 0) && (CCuttingTool::Find( cutting_tool_number ) != NULL))
		{
			wxString material_name = theApp.m_program->m_raw_material.m_material_name;
			double hardness = theApp.m_program->m_raw_material.m_brinell_hardness;
			double surface_speed = CSpeedReferences::GetSurfaceSpeed( material_name,
										CCuttingTool::CutterMaterial( cutting_tool_number ),
										hardness );
			if (surface_speed > 0)
			{
				CCuttingTool *pCuttingTool = CCuttingTool::Find( cutting_tool_number );
				if (pCuttingTool != NULL)
				{
					if (pCuttingTool->m_params.m_diameter > 0) 
					{
						m_spindle_speed = (surface_speed * 1000.0) / (PI * pCuttingTool->m_params.m_diameter);
						m_spindle_speed = floor(m_spindle_speed);	// Round down to integer

						// Now wait one minute.  If the chosen machine can't turn the spindle that
						// fast then we'd better reduce our speed to match its maximum.

						if ((theApp.m_program != NULL) &&
						    (theApp.m_program->m_machine.m_max_spindle_speed > 0.0) &&
						    (theApp.m_program->m_machine.m_max_spindle_speed < m_spindle_speed))
						{
							// Reduce the speed to match the machine's maximum setting.
						    	m_spindle_speed = theApp.m_program->m_machine.m_max_spindle_speed;
						} // End if - then
					} // End if - then
				} // End if - then
			} // End if - then
		} // End if - then
	} // End if - then
} // End ResetSpeeds() method

void CSpeedOpParams::write_values_to_config()
{
	CNCConfig config;
	config.Write(_T("SpeedOpHorizFeed"), m_horizontal_feed_rate);
	config.Write(_T("SpeedOpVertFeed"), m_vertical_feed_rate);
	config.Write(_T("SpeedOpSpindleSpeed"), m_spindle_speed);
}

static void on_set_horizontal_feed_rate(double value, HeeksObj* object)
{
	((CSpeedOp*)object)->m_speed_op_params.m_horizontal_feed_rate = value;
	((CSpeedOp*)object)->m_speed_op_params.write_values_to_config();
}

static void on_set_vertical_feed_rate(double value, HeeksObj* object)
{
	((CSpeedOp*)object)->m_speed_op_params.m_vertical_feed_rate = value;
	((CSpeedOp*)object)->m_speed_op_params.write_values_to_config();
}

static void on_set_spindle_speed(double value, HeeksObj* object)
{
	((CSpeedOp*)object)->m_speed_op_params.m_spindle_speed = value;
	((CSpeedOp*)object)->m_speed_op_params.write_values_to_config();
}

void CSpeedOpParams::GetProperties(CSpeedOp* parent, std::list<Property *> *list)
{
	list->push_back(new PropertyLength(_("horizontal feed rate"), m_horizontal_feed_rate, parent, on_set_horizontal_feed_rate));
	list->push_back(new PropertyLength(_("vertical feed rate"), m_vertical_feed_rate, parent, on_set_vertical_feed_rate));
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
	theApp.m_program_canvas->AppendText(m_speed_op_params.m_horizontal_feed_rate / theApp.m_program->m_units);
	theApp.m_program_canvas->AppendText(_T(", "));

	theApp.m_program_canvas->AppendText(m_speed_op_params.m_vertical_feed_rate / theApp.m_program->m_units);
	theApp.m_program_canvas->AppendText(_T(")\n"));

	theApp.m_program_canvas->AppendText(_T("flush_nc()\n"));
}

