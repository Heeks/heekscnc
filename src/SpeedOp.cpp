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
#include "CTool.h"
#include "MachineState.h"

CSpeedOpParams::CSpeedOpParams()
{
	m_horizontal_feed_rate = 0.0;
	m_vertical_feed_rate = 0.0;
	m_spindle_speed = 0.0;
}

void CSpeedOpParams::ResetFeeds(const int tool_number)
{
	if ((theApp.m_program) &&
	    (theApp.m_program->SpeedReferences()) &&
	    (theApp.m_program->SpeedReferences()->m_estimate_when_possible))
	{

		// Use the 'feeds and speeds' class along with the tool properties to
		// help set some logical values for the spindle speed.

		if ((tool_number > 0) && (CTool::Find( tool_number ) != NULL))
		{
			CTool *pTool = CTool::Find( tool_number );
			if (pTool != NULL)
			{
				if ((pTool->m_params.m_type == CToolParams::eDrill) ||
				    (pTool->m_params.m_type == CToolParams::eCentreDrill))
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
					// based on the raw material, the tool's material and the
					// tool's diameter.)  We just need to compare this spindle speed with
					// the magic '3000' value to find out what proportion of the 'rule of thumb'
					// we're using.

				        double proportion = fabs(m_spindle_speed) / 3000.0;
					double drill_diameter_in_inches = pTool->m_params.m_diameter / 25.4;
					double feed_rate_inches_per_minute = 100.0 * drill_diameter_in_inches * proportion;
					m_vertical_feed_rate = feed_rate_inches_per_minute * 25.4;	// mm per minute
					m_horizontal_feed_rate = 0.0;	// We're going straight down with a drill bit.
				} // End if - then
				else if (pTool->m_params.m_type == CToolParams::eTapTool)
				{
				    // Make sure we set the feed rate based on both the spindle speed and the pitch.

				        m_vertical_feed_rate = fabs(m_spindle_speed) * pTool->m_params.m_pitch;
					m_horizontal_feed_rate = 0.0;	// We're going straight down with a tap.
				} // End if - then
				else if (pTool->m_params.m_max_advance_per_revolution > 0)
				{
					// Spindle speed is in revolutions per minute.
					double advance_per_rev = pTool->m_params.m_max_advance_per_revolution;
					double feed_rate_mm_per_minute = fabs(m_spindle_speed) * advance_per_rev;

					// Now we need to decide whether we assign this value to the vertical
					// or horozontal (or both) feed rates.  Use the tool type to
					// decide on the typical usage.

					switch (pTool->m_params.m_type)
					{
						case CToolParams::eDrill:
						case CToolParams::eCentreDrill:
							m_vertical_feed_rate = feed_rate_mm_per_minute;
							m_horizontal_feed_rate = 0.0;	// We're going straight down with a drill bit.
							break;

						case CToolParams::eChamfer:
						case CToolParams::eTurningTool:
							// Spread it across both horizontal and vertical
							m_vertical_feed_rate = (1.0/sqrt(2.0)) * feed_rate_mm_per_minute;
							m_horizontal_feed_rate = (1.0/sqrt(2.0)) * feed_rate_mm_per_minute;
							break;

						case CToolParams::eEndmill:
						case CToolParams::eBallEndMill:
						case CToolParams::eSlotCutter:
						default:
							m_horizontal_feed_rate = feed_rate_mm_per_minute;
							break;
					} // End switch
				} // End if - then
			} // End if - then
		} // End if - then
	} // End if - then
} // End ResetFeeds() method

void CSpeedOpParams::ResetSpeeds(const int tool_number)
{
	if ((theApp.m_program) &&
	    (theApp.m_program->SpeedReferences()) &&
	    (theApp.m_program->SpeedReferences()->m_estimate_when_possible))
	{

		// Use the 'feeds and speeds' class along with the tool properties to
		// help set some logical values for the spindle speed.

        CTool *pTool = CTool::Find( tool_number );
		if ((tool_number > 0) && (pTool != NULL))
		{
			wxString material_name = theApp.m_program->m_raw_material.m_material_name;
			double hardness = theApp.m_program->m_raw_material.m_brinell_hardness;
			double surface_speed = CSpeedReferences::GetSurfaceSpeed( material_name,
										CTool::CutterMaterial( tool_number ),
										hardness );
			if (surface_speed > 0)
			{
                if (pTool->m_params.m_diameter > 0)
                {
                    m_spindle_speed = (surface_speed * 1000.0) / (PI * pTool->m_params.m_diameter);
                    m_spindle_speed = floor(m_spindle_speed);	// Round down to integer
                } // End if - then
			} // End if - then

            if (pTool->m_params.m_type == CToolParams::eTapTool)
            {
                // We don't want to run too fast with a tap.
                m_spindle_speed = 500;  // We really should define this along with the max_spindle_speed parameter for the machine configuration.
            }

            // Now wait one minute.  If the chosen machine can't turn the spindle that
            // fast then we'd better reduce our speed to match its maximum.

            if ((theApp.m_program != NULL) &&
                (theApp.m_program->m_machine.m_max_spindle_speed > 0.0) &&
                (theApp.m_program->m_machine.m_max_spindle_speed < fabs(m_spindle_speed)))
            {
                // Reduce the speed to match the machine's maximum setting.
	        m_spindle_speed = (m_spindle_speed < 0) ? -theApp.m_program->m_machine.m_max_spindle_speed : theApp.m_program->m_machine.m_max_spindle_speed;
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
	((CSpeedOp*)object)->m_speed_op_params.ResetFeeds(((CSpeedOp*)object)->m_tool_number);
	((CSpeedOp*)object)->WriteDefaultValues();
}

void CSpeedOpParams::GetProperties(CSpeedOp* parent, std::list<Property *> *list)
{
	if(CTool::IsMillingToolType(CTool::FindToolType(parent->m_tool_number)))
	{
		list->push_back(new PropertyLength(_("horizontal feed rate"), m_horizontal_feed_rate, parent, on_set_horizontal_feed_rate));
		list->push_back(new PropertyLength(_("vertical feed rate"), m_vertical_feed_rate, parent, on_set_vertical_feed_rate));
		list->push_back(new PropertyDouble(_("spindle speed"), m_spindle_speed, parent, on_set_spindle_speed));
	}
}

void CSpeedOpParams::WriteXMLAttributes(TiXmlNode* pElem)
{
	TiXmlElement * element = heeksCAD->NewXMLElement( "speedop" );
	heeksCAD->LinkXMLEndChild( pElem,  element );
	element->SetDoubleAttribute( "hfeed", m_horizontal_feed_rate);
	element->SetDoubleAttribute( "vfeed", m_vertical_feed_rate);
	element->SetDoubleAttribute( "spin", m_spindle_speed);
}

void CSpeedOpParams::ReadFromXMLElement(TiXmlElement* pElem)
{
	TiXmlElement* speedop = heeksCAD->FirstNamedXMLChildElement(pElem, "speedop");
	if(speedop)
	{
		speedop->Attribute("hfeed", &m_horizontal_feed_rate);
		speedop->Attribute("vfeed", &m_vertical_feed_rate);
		speedop->Attribute("spin", &m_spindle_speed);

		heeksCAD->RemoveXMLChild(pElem, speedop);
	}
}

void CSpeedOp::glCommands(bool select, bool marked, bool no_color)
{
	COp::glCommands(select, marked, no_color);
}


CSpeedOp & CSpeedOp::operator= ( const CSpeedOp & rhs )
{
	if (this != &rhs)
	{
		COp::operator=(rhs);
		m_speed_op_params = rhs.m_speed_op_params;
		// static bool m_auto_set_speeds_feeds;
	}

	return(*this);
}

CSpeedOp::CSpeedOp( const CSpeedOp & rhs ) : COp(rhs)
{
	m_speed_op_params = rhs.m_speed_op_params;
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

	CNCConfig config(CSpeedOp::ConfigScope());
	config.Write(_T("SpeedOpHorizFeed"), m_speed_op_params.m_horizontal_feed_rate);
	config.Write(_T("SpeedOpVertFeed"), m_speed_op_params.m_vertical_feed_rate);
	config.Write(_T("SpeedOpSpindleSpeed"), m_speed_op_params.m_spindle_speed);
}

void CSpeedOp::ReadDefaultValues()
{
	COp::ReadDefaultValues();

	CNCConfig config(CSpeedOp::ConfigScope());
	config.Read(_T("SpeedOpHorizFeed"), &m_speed_op_params.m_horizontal_feed_rate, 100.0);
	config.Read(_T("SpeedOpVertFeed"), &m_speed_op_params.m_vertical_feed_rate, 100.0);
	config.Read(_T("SpeedOpSpindleSpeed"), &m_speed_op_params.m_spindle_speed, 7000);

	if(m_auto_set_speeds_feeds)
	{
		m_speed_op_params.ResetSpeeds(m_tool_number);	// NOTE: The speed MUST be set BEFORE the feedrates
		m_speed_op_params.ResetFeeds(m_tool_number);
	}
}

void CSpeedOp::GetProperties(std::list<Property *> *list)
{
	m_speed_op_params.GetProperties(this, list);
	COp::GetProperties(list);
}

Python CSpeedOp::AppendTextToProgram(CMachineState *pMachineState)
{
	Python python;

	python << COp::AppendTextToProgram(pMachineState);

	if (m_speed_op_params.m_spindle_speed != 0)
	{
		python << _T("spindle(") << m_speed_op_params.m_spindle_speed << _T(")\n");
	} // End if - then

	python << _T("feedrate_hv(") << m_speed_op_params.m_horizontal_feed_rate / theApp.m_program->m_units << _T(", ");
    python << m_speed_op_params.m_vertical_feed_rate / theApp.m_program->m_units << _T(")\n");
    python << _T("flush_nc()\n");

    return(python);
}

static void on_set_auto_speeds(bool value, HeeksObj* object){CSpeedOp::m_auto_set_speeds_feeds = value;}

// static
void CSpeedOp::GetOptions(std::list<Property *> *list)
{
	list->push_back ( new PropertyCheck ( _("auto set speeds for new operation"), m_auto_set_speeds_feeds, NULL, on_set_auto_speeds ) );
}

// static
void CSpeedOp::ReadFromConfig()
{
	CNCConfig config(ConfigScope());
	config.Read(_T("SpeedOpAutoSetSpeeds"), &m_auto_set_speeds_feeds,true);
}

// static
void CSpeedOp::WriteToConfig()
{
	CNCConfig config(ConfigScope());
	config.Write(_T("SpeedOpAutoSetSpeeds"), m_auto_set_speeds_feeds);
}


class ResetFeedsAndSpeeds: public Tool{
public:
    void SetObject( CSpeedOp *pThis )
    {
        m_pThis = pThis;
    }

	// Tool's virtual functions
	const wxChar* GetTitle(){return _("Reset Feeds and Speeds");}
	void Run()
	{
     	m_pThis->m_speed_op_params.ResetSpeeds( m_pThis->m_tool_number);	// NOTE: The speed MUST be set BEFORE the feedrates
		m_pThis->m_speed_op_params.ResetFeeds( m_pThis->m_tool_number);
	}
	wxString BitmapPath(){ return _T("import");}
	wxString previous_path;

private:
	CSpeedOp *m_pThis;
};

static ResetFeedsAndSpeeds reset_feeds_and_speeds;

void CSpeedOp::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
    if (m_tool_number > 0)
    {
        wxString material_name = theApp.m_program->m_raw_material.m_material_name;
        double hardness = theApp.m_program->m_raw_material.m_brinell_hardness;
        double surface_speed = CSpeedReferences::GetSurfaceSpeed( material_name,
                                        CTool::CutterMaterial( m_tool_number ),
                                            hardness );
        if (surface_speed > 0)
        {
            // We have enough information to set feeds and speeds.  Make the option available.
            reset_feeds_and_speeds.SetObject( this );
            t_list->push_back(&reset_feeds_and_speeds);
        }
    }

    COp::GetTools(t_list, p);
}

bool CSpeedOpParams::operator== ( const CSpeedOpParams & rhs ) const
{
	if (m_horizontal_feed_rate != rhs.m_horizontal_feed_rate) return(false);
	if (m_vertical_feed_rate != rhs.m_vertical_feed_rate) return(false);
	if (m_spindle_speed != rhs.m_spindle_speed) return(false);

	return(true);
}

bool CSpeedOp::operator==(const CSpeedOp & rhs) const
{
	if (m_speed_op_params != rhs.m_speed_op_params) return(false);

	return(COp::operator==(rhs));
}




