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
#include "interface/PropertyList.h"
#include "interface/PropertyCheck.h"
#include "tinyxml/tinyxml.h"
#include "interface/Tool.h"
#include "CuttingTool.h"

CSpeedOpParams::CSpeedOpParams()
{
	m_horizontal_feed_rate = 0.0;
	m_vertical_feed_rate = 0.0;
	m_spindle_speed = 0.0;
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
				if ((pCuttingTool->m_params.m_type == CCuttingToolParams::eDrill) ||
				    (pCuttingTool->m_params.m_type == CCuttingToolParams::eCentreDrill))
				{
					// There is a 'rule of thumb' that Stanley Dornfeld put forward in
					// an EMC2 reference.  It states that;
					// "...feed rate one hundred times the drill's decimal diameter
					// at three thousand RPM".
					// The diameter is expressed in inches.  eg: a 1/8 inch (0.125 inches)
					// drill bit could be fed at 12.5 inches per minute using an RPM of
					// 3000.  OR since 300 is one tenth of 3000 the feed would be 1.2 inches
					// per minute.
					//
					// We will use this rule of thumb in preference to the material removal
					// rate as the material removal rate is really for milling operations
					// moving sideways through the material.

					// Let the spindle speed be used (as it has already been determined
					// based on the raw material, the cutting tool's material and the
					// tool's diameter.)  We just need to compare this spindle speed with
					// the magic '3000' value to find out what proportion of the 'rule of thumb'
					// we're using.

					double proportion = m_spindle_speed / 3000.0;
					double drill_diameter_in_inches = pCuttingTool->m_params.m_diameter / 25.4;
					double feed_rate_inches_per_minute = 100.0 * drill_diameter_in_inches * proportion;
					m_vertical_feed_rate = feed_rate_inches_per_minute * 25.4;	// mm per minute
					m_horizontal_feed_rate = 0.0;	// We're going straight down with a drill bit.
				} // End if - then
				else if (pCuttingTool->m_params.m_max_advance_per_revolution > 0) 
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
							m_horizontal_feed_rate = 0.0;	// We're going straight down with a drill bit.
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

static void on_set_horizontal_feed_rate(double value, HeeksObj* object)
{
	((CSpeedOp*)object)->m_speed_op_params.m_horizontal_feed_rate = value;
	((CSpeedOp*)object)->WriteDefaultValues();
}

static void on_set_vertical_feed_rate(double value, HeeksObj* object)
{
	((CSpeedOp*)object)->m_speed_op_params.m_vertical_feed_rate = value;
	((CSpeedOp*)object)->WriteDefaultValues();
}

static void on_set_spindle_speed(double value, HeeksObj* object)
{
	((CSpeedOp*)object)->m_speed_op_params.m_spindle_speed = value;
	((CSpeedOp*)object)->WriteDefaultValues();
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

// static
bool CSpeedOp::m_auto_set_speeds_feeds = false;

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

void CSpeedOp::WriteDefaultValues()
{
	COp::WriteDefaultValues();

	CNCConfig config;
	config.Write(_T("SpeedOpHorizFeed"), m_speed_op_params.m_horizontal_feed_rate);
	config.Write(_T("SpeedOpVertFeed"), m_speed_op_params.m_vertical_feed_rate);
	config.Write(_T("SpeedOpSpindleSpeed"), m_speed_op_params.m_spindle_speed);
}

void CSpeedOp::ReadDefaultValues()
{
	COp::ReadDefaultValues();

	CNCConfig config;
	config.Read(_T("SpeedOpHorizFeed"), &m_speed_op_params.m_horizontal_feed_rate, 100.0);
	config.Read(_T("SpeedOpVertFeed"), &m_speed_op_params.m_vertical_feed_rate, 100.0);
	config.Read(_T("SpeedOpSpindleSpeed"), &m_speed_op_params.m_spindle_speed, 7000);

	if(m_auto_set_speeds_feeds)
	{
		m_speed_op_params.ResetSpeeds(m_cutting_tool_number);	// NOTE: The speed MUST be set BEFORE the feedrates
		m_speed_op_params.ResetFeeds(m_cutting_tool_number);
	}
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

static void on_set_auto_speeds(bool value, HeeksObj* object){CSpeedOp::m_auto_set_speeds_feeds = value;}

// static
void CSpeedOp::GetOptions(std::list<Property *> *list)
{
	PropertyList* speeds_options = new PropertyList(_("speeds"));
	speeds_options->m_list.push_back ( new PropertyCheck ( _("auto set speeds for new operation"), m_auto_set_speeds_feeds, NULL, on_set_auto_speeds ) );
	list->push_back(speeds_options);
}

// static
void CSpeedOp::ReadFromConfig()
{
	CNCConfig config;
	config.Read(_T("SpeedOpAutoSetSpeeds"), &m_auto_set_speeds_feeds,true);
}

// static
void CSpeedOp::WriteToConfig()
{
	CNCConfig config;
	config.Write(_T("SpeedOpAutoSetSpeeds"), m_auto_set_speeds_feeds);
}
