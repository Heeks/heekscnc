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
#include "CTool.h"
#include "MachineState.h"


CDepthOpParams::CDepthOpParams()
{
	m_abs_mode = eAbsolute;
	m_clearance_height = 0.0;
	m_start_depth = 0.0;
	m_step_down = 0.0;
	m_final_depth = 0.0;
	m_rapid_down_to_height = 0.0;
	
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
	return( (degrees / 360.0) * (2 * PI) );
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

static void on_set_rapid_down_to_height(double value, HeeksObj* object)
{
	((CDepthOp*)object)->m_depth_op_params.m_rapid_down_to_height = value;
	((CDepthOp*)object)->WriteDefaultValues();
}

static void on_set_abs_mode(int value, HeeksObj* object) {

	((CDepthOp*)object)->m_depth_op_params.m_abs_mode = (CDepthOpParams::eAbsMode)value;
	((CDepthOp*)object)->WriteDefaultValues();
	
}


void CDepthOpParams::GetProperties(CDepthOp* parent, std::list<Property *> *list)
{
	{
		std::list< wxString > choices;
		choices.push_back(_("Absolute"));
		choices.push_back(_("Incremental"));
		list->push_back(new PropertyChoice(_("ABS/INCR mode"), choices, m_abs_mode, parent, on_set_abs_mode));
	}
		list->push_back(new PropertyLength(_("clearance height"), m_clearance_height, parent, on_set_clearance_height));
		list->push_back(new PropertyLength(_("rapid down to height"), m_rapid_down_to_height, parent, on_set_rapid_down_to_height));
	
	//My initial thought was that extrusion operatons would always start at z=0 and end at z=top of object.  I'm now thinking it might be desireable to preserve this as an option.
	//It might be good to run an operation that prints the bottom half of the object, pauses to allow insertion of something.  Then another operation could print the top half.
	
		list->push_back(new PropertyLength(_("start depth"), m_start_depth, parent, on_set_start_depth));
		list->push_back(new PropertyLength(_("final depth"), m_final_depth, parent, on_set_final_depth));
	   
	   //Step down doesn't make much sense for extrusion.  The amount the z axis steps up or down is equal to the layer thickness of the slice which
	   //is determined by the thickness of an extruded filament.  Step up is very important since it is directly related to the resolution of the final 
	   //produce.  
	   	list->push_back(new PropertyLength(_("step down"), m_step_down, parent, on_set_step_down));
		

}

void CDepthOpParams::WriteXMLAttributes(TiXmlNode* pElem)
{
	TiXmlElement * element = new TiXmlElement( "depthop" );
	pElem->LinkEndChild( element );
	element->SetDoubleAttribute("clear", m_clearance_height);
	element->SetDoubleAttribute("down", m_step_down);
	element->SetDoubleAttribute("startdepth", m_start_depth);
	element->SetDoubleAttribute("depth", m_final_depth);
	element->SetDoubleAttribute("r", m_rapid_down_to_height);
	element->SetAttribute("abs_mode", m_abs_mode);
}

void CDepthOpParams::ReadFromXMLElement(TiXmlElement* pElem)
{
	TiXmlElement* depthop = TiXmlHandle(pElem).FirstChildElement("depthop").Element();
	if(depthop)
	{	int int_for_enum;
		depthop->Attribute("clear", &m_clearance_height);
		depthop->Attribute("down", &m_step_down);
		depthop->Attribute("startdepth", &m_start_depth);
		depthop->Attribute("depth", &m_final_depth);
		depthop->Attribute("r", &m_rapid_down_to_height);
		if(pElem->Attribute("abs_mode", &int_for_enum))m_abs_mode = (eAbsMode)int_for_enum;
		pElem->RemoveChild(depthop);	// We don't want to interpret this again when
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

	CNCConfig config(GetTypeString());
	config.Write(_T("ClearanceHeight"), m_depth_op_params.m_clearance_height);
	config.Write(_T("StartDepth"), m_depth_op_params.m_start_depth);
	config.Write(_T("StepDown"), m_depth_op_params.m_step_down);
	config.Write(_T("FinalDepth"), m_depth_op_params.m_final_depth);
	config.Write(_T("RapidDown"), m_depth_op_params.m_rapid_down_to_height);
	config.Write(_T("ABSMode"), m_depth_op_params.m_abs_mode);
}

void CDepthOp::ReadDefaultValues()
{
	CSpeedOp::ReadDefaultValues();

	CNCConfig config(GetTypeString());
	config.Read(_T("ClearanceHeight"), &m_depth_op_params.m_clearance_height, 5.0);
	config.Read(_T("StartDepth"), &m_depth_op_params.m_start_depth, 0.0);
	config.Read(_T("StepDown"), &m_depth_op_params.m_step_down, 1.0);
	config.Read(_T("FinalDepth"), &m_depth_op_params.m_final_depth, -1.0);
	config.Read(_T("RapidDown"), &m_depth_op_params.m_rapid_down_to_height, 2.0);
	int int_mode = m_depth_op_params.m_abs_mode;
	config.Read(_T("ABSMode"), &int_mode, CDepthOpParams::eAbsolute);
	m_depth_op_params.m_abs_mode = (CDepthOpParams::eAbsMode)int_mode;
}

void CDepthOp::SetDepthsFromSketchesAndTool(const std::list<int> *sketches)
{
	std::list<HeeksObj *> objects;

	if (sketches != NULL)
	{
		for (std::list<int>::const_iterator l_itSketch = sketches->begin(); l_itSketch != sketches->end(); l_itSketch++)
		{
			HeeksObj *pSketch = heeksCAD->GetIDObject( SketchType, *l_itSketch );
			if (pSketch != NULL)
			{
				objects.push_back(pSketch);
			}
		}
	}

	SetDepthsFromSketchesAndTool( objects );
}

void CDepthOp::SetDepthsFromSketchesAndTool(const std::list<HeeksObj *> sketches)
{
	for (std::list<HeeksObj *>::const_iterator l_itSketch = sketches.begin(); l_itSketch != sketches.end(); l_itSketch++)
	{
		double default_depth = 1.0;	// mm
		HeeksObj *pSketch = *l_itSketch;
		if (pSketch != NULL)
		{
			CBox bounding_box;
			pSketch->GetBox( bounding_box );

			if (l_itSketch == sketches.begin())
			{
				// This is the first cab off the rank.

				m_depth_op_params.m_start_depth = bounding_box.MaxZ();
				m_depth_op_params.m_final_depth = m_depth_op_params.m_start_depth - default_depth;
			} // End if - then
			else
			{
				// We've seen some before.  If this one is higher up then use
				// that instead.

				if (m_depth_op_params.m_start_depth < bounding_box.MaxZ())
				{
					m_depth_op_params.m_start_depth = bounding_box.MaxZ();
				} // End if - then

				if (m_depth_op_params.m_final_depth > bounding_box.MinZ())
				{
					m_depth_op_params.m_final_depth = bounding_box.MinZ() - default_depth;
				} // End if - then
			} // End if - else
		} // End if - then
	} // End for

	// If we've chosen a chamfering bit, calculate the depth required to give a 1 mm wide
	// chamfer.  It's as good as any width to start with.  If it's not a chamfering bit
	// then we can't even guess as to what the operator wants.

	const double default_chamfer_width = 1.0;	// mm
	if (m_tool_number > 0)
	{
		CTool *pTool = CTool::Find( m_tool_number );
		if (pTool != NULL)
		{
			if ((pTool->m_params.m_type == CToolParams::eChamfer) &&
			    (pTool->m_params.m_cutting_edge_angle > 0))
			{
				m_depth_op_params.m_final_depth = m_depth_op_params.m_start_depth - (default_chamfer_width * tan( degrees_to_radians( 90.0 - pTool->m_params.m_cutting_edge_angle ) ));
			} // End if - then
		} // End if - then
	} // End if - then
}

Python CDepthOp::AppendTextToProgram(CMachineState *pMachineState)
{
	Python python;

    python << CSpeedOp::AppendTextToProgram(pMachineState);

	python << _T("clearance = float(") << m_depth_op_params.m_clearance_height / theApp.m_program->m_units << _T(")\n");
	python << _T("rapid_down_to_height = float(") << m_depth_op_params.m_rapid_down_to_height / theApp.m_program->m_units << _T(")\n");
    python << _T("start_depth = float(") << m_depth_op_params.m_start_depth / theApp.m_program->m_units << _T(")\n");
    python << _T("step_down = float(") << m_depth_op_params.m_step_down / theApp.m_program->m_units << _T(")\n");
    python << _T("final_depth = float(") << m_depth_op_params.m_final_depth / theApp.m_program->m_units << _T(")\n");

	CTool *pTool = CTool::Find( m_tool_number );
	if (pTool != NULL)
	{
		python << _T("tool_diameter = float(") << (pTool->CuttingRadius(true) * 2.0) << _T(")\n");
	} // End if - then

	if(m_depth_op_params.m_abs_mode == CDepthOpParams::eAbsolute){
		python << _T("#absolute() mode\n");
	} 
	else
	{
		python << _T("rapid(z=clearance)\n");
		python << _T("incremental()\n");
	}// End if else - then

	return(python);
}


std::list<wxString> CDepthOp::DesignRulesAdjustment(const bool apply_changes)
{

	std::list<wxString> changes;

	CTool *pTool = CTool::Find( m_tool_number );
	if (pTool == NULL)
	{
#ifdef UNICODE
		std::wostringstream l_ossChange;
#else
		std::ostringstream l_ossChange;
#endif

		l_ossChange << _("WARNING") << ": " << _("Depth Operation") << " (id=" << m_id << ") " << _("does not have a tool assigned") << ". " << _("It can not produce GCode without a tool assignment") << ".\n";
		changes.push_back(l_ossChange.str().c_str());
	} // End if - then
	else
	{
		double cutting_depth = m_depth_op_params.m_start_depth - m_depth_op_params.m_final_depth;
		if (cutting_depth > pTool->m_params.m_cutting_edge_height)
		{
#ifdef UNICODE
			std::wostringstream l_ossChange;
#else
			std::ostringstream l_ossChange;
#endif

			l_ossChange << _("WARNING") << ": " << _("Depth Operation") << " (id=" << m_id << ") " << _("is set to cut deeper than the assigned tool will allow") << ".\n";
			changes.push_back(l_ossChange.str().c_str());
		} // End if - then
	} // End if - else

	if (m_depth_op_params.m_start_depth <= m_depth_op_params.m_final_depth)
	{
#ifdef UNICODE
		std::wostringstream l_ossChange;
#else
		std::ostringstream l_ossChange;
#endif
		l_ossChange << _("WARNING") << ": " << _("Depth Operation") << " (id=" << m_id << ") " << _("has poor start and final depths") << ". " << _("Can't change this setting automatically") << ".\n";
		changes.push_back(l_ossChange.str().c_str());
	} // End if - then

	if (m_depth_op_params.m_start_depth > m_depth_op_params.m_clearance_height)
	{
#ifdef UNICODE
		std::wostringstream l_ossChange;
#else
		std::ostringstream l_ossChange;
#endif

		l_ossChange << _("WARNING") << ": " << _("Depth Operation") << " (id=" << m_id << ") " << _("Clearance height is below start depth") << ".\n";
		changes.push_back(l_ossChange.str().c_str());

		if (apply_changes)
		{
			l_ossChange << _("Depth Operation") << " (id=" << m_id << ").  " << _("Raising clearance height up to start depth (+5 mm)") << "\n";
			m_depth_op_params.m_clearance_height = m_depth_op_params.m_start_depth + 5;
		} // End if - then
	} // End if - then

    if (m_depth_op_params.m_step_down < 0)
    {
        wxString change;

        change << _("The step-down value for pocket (id=") << m_id << _(") must be positive");
        changes.push_back(change);

        if (apply_changes)
        {
            m_depth_op_params.m_step_down *= -1.0;
        }
    }

	return(changes);

} // End DesignRulesAdjustment() method

void CDepthOp::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
    CSpeedOp::GetTools( t_list, p );
}

std::list<double> CDepthOp::GetDepths() const
{
    std::list<double> depths;

    if (m_depth_op_params.m_start_depth <= m_depth_op_params.m_final_depth)
    {
        // Invalid depth values defined.
        return(depths); // Empty list.
    }

    for (double depth=m_depth_op_params.m_start_depth - m_depth_op_params.m_step_down;
                depth > m_depth_op_params.m_final_depth;
                depth -= m_depth_op_params.m_step_down)
    {
        depths.push_back(depth);
    }

    if ((depths.size() == 0) || (*(depths.rbegin()) > m_depth_op_params.m_final_depth))
    {
        depths.push_back( m_depth_op_params.m_final_depth );
    }

    return(depths);
}



bool CDepthOpParams::operator== ( const CDepthOpParams & rhs ) const
{
	if (m_clearance_height != rhs.m_clearance_height) return(false);
	if (m_start_depth != rhs.m_start_depth) return(false);
	if (m_step_down != rhs.m_step_down) return(false);
	if (m_final_depth != rhs.m_final_depth) return(false);
	if (m_rapid_down_to_height != rhs.m_rapid_down_to_height) return(false);

	return(true);
}

bool CDepthOp::operator== ( const CDepthOp & rhs ) const
{
	if (m_depth_op_params != rhs.m_depth_op_params) return(false);
	return(CSpeedOp::operator==(rhs));
}

