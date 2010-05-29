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
#include "MachineState.h"


CDepthOpParams::CDepthOpParams()
{
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
	*this = rhs;	// Call the assignment operator.
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

void CDepthOpParams::GetProperties(CDepthOp* parent, std::list<Property *> *list)
{
	list->push_back(new PropertyLength(_("clearance height"), m_clearance_height, parent, on_set_clearance_height));
	list->push_back(new PropertyLength(_("step down"), m_step_down, parent, on_set_step_down));
	list->push_back(new PropertyLength(_("start depth"), m_start_depth, parent, on_set_start_depth));
	list->push_back(new PropertyLength(_("final depth"), m_final_depth, parent, on_set_final_depth));
	list->push_back(new PropertyLength(_("rapid down to height"), m_rapid_down_to_height, parent, on_set_rapid_down_to_height));
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
}

void CDepthOpParams::ReadFromXMLElement(TiXmlElement* pElem)
{
	TiXmlElement* depthop = TiXmlHandle(pElem).FirstChildElement("depthop").Element();
	if(depthop)
	{
		depthop->Attribute("clear", &m_clearance_height);
		depthop->Attribute("down", &m_step_down);
		depthop->Attribute("startdepth", &m_start_depth);
		depthop->Attribute("depth", &m_final_depth);
		depthop->Attribute("r", &m_rapid_down_to_height);

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
}

void CDepthOp::ReadDefaultValues()
{
	CSpeedOp::ReadDefaultValues();

	CNCConfig config(ConfigScope());
	config.Read(_T("ClearanceHeight"), &m_depth_op_params.m_clearance_height, 5.0);
	config.Read(_T("StartDepth"), &m_depth_op_params.m_start_depth, 0.0);
	config.Read(_T("StepDown"), &m_depth_op_params.m_step_down, 1.0);
	config.Read(_T("FinalDepth"), &m_depth_op_params.m_final_depth, -1.0);
	config.Read(_T("RapidDown"), &m_depth_op_params.m_rapid_down_to_height, 2.0);
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
	if (m_cutting_tool_number > 0)
	{
		CCuttingTool *pCuttingTool = CCuttingTool::Find( m_cutting_tool_number );
		if (pCuttingTool != NULL)
		{
			if ((pCuttingTool->m_params.m_type == CCuttingToolParams::eChamfer) &&
			    (pCuttingTool->m_params.m_cutting_edge_angle > 0))
			{
				m_depth_op_params.m_final_depth = m_depth_op_params.m_start_depth - (default_chamfer_width * tan( degrees_to_radians( 90.0 - pCuttingTool->m_params.m_cutting_edge_angle ) ));
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

	CCuttingTool *pCuttingTool = CCuttingTool::Find( m_cutting_tool_number );
	if (pCuttingTool != NULL)
	{
		python << _T("tool_diameter = float(") << (pCuttingTool->CuttingRadius(true) * 2.0) << _T(")\n");
	} // End if - then

	return(python);
}


std::list<wxString> CDepthOp::DesignRulesAdjustment(const bool apply_changes)
{

	std::list<wxString> changes;

	CCuttingTool *pCuttingTool = CCuttingTool::Find( m_cutting_tool_number );
	if (pCuttingTool == NULL)
	{
#ifdef UNICODE
		std::wostringstream l_ossChange;
#else
		std::ostringstream l_ossChange;
#endif

		l_ossChange << _("WARNING") << ": " << _("Depth Operation") << " (id=" << m_id << ") " << _("does not have a cutting tool assigned") << ". " << _("It can not produce GCode without a cutting tool assignment") << ".\n";
		changes.push_back(l_ossChange.str().c_str());
	} // End if - then
	else
	{
		double cutting_depth = m_depth_op_params.m_start_depth - m_depth_op_params.m_final_depth;
		if (cutting_depth > pCuttingTool->m_params.m_cutting_edge_height)
		{
#ifdef UNICODE
			std::wostringstream l_ossChange;
#else
			std::ostringstream l_ossChange;
#endif

			l_ossChange << _("WARNING") << ": " << _("Depth Operation") << " (id=" << m_id << ") " << _("is set to cut deeper than the assigned cutting tool will allow") << ".\n";
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

