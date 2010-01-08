// Op.cpp
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "stdafx.h"
#include "Op.h"
#include "ProgramCanvas.h"
#include "interface/PropertyInt.h"
#include "interface/PropertyString.h"
#include "interface/PropertyCheck.h"
#include "interface/PropertyChoice.h"
#include "interface/Tool.h"
#include "tinyxml/tinyxml.h"
#include "HeeksCNCTypes.h"
#include "CuttingTool.h"

#include <iterator>
#include <vector>

void COp::WriteBaseXML(TiXmlElement *element)
{
	if(m_comment.Len() > 0)element->SetAttribute("comment", Ttc(m_comment.c_str()));
	if(!m_active)element->SetAttribute("active", 0);
	element->SetAttribute("title", Ttc(m_title.c_str()));
	element->SetAttribute("execution_order", m_execution_order);
	element->SetAttribute("cutting_tool_number", m_cutting_tool_number);

	HeeksObj::WriteBaseXML(element);
}

void COp::ReadBaseXML(TiXmlElement* element)
{
	const char* comment = element->Attribute("comment");
	if(comment)m_comment.assign(Ctt(comment));
	const char* active = element->Attribute("active");
	if(active)
	{
		int int_active;
		element->Attribute("active", &int_active);
		m_active = (int_active != 0);
	}
	const char* title = element->Attribute("title");
	if(title)m_title = wxString(Ctt(title));

	if (element->Attribute("execution_order") != NULL)
	{
		m_execution_order = atoi(element->Attribute("execution_order"));
	} // End if - then
	else
	{
		m_execution_order = 0;
	} // End if - else

	if (element->Attribute("cutting_tool_number") != NULL)
	{
		m_cutting_tool_number = atoi(element->Attribute("cutting_tool_number"));
	} // End if - then
	else
	{
		m_cutting_tool_number = 0;
	} // End if - else

	HeeksObj::ReadBaseXML(element);
}

static void on_set_comment(const wxChar* value, HeeksObj* object){((COp*)object)->m_comment = value;}
static void on_set_active(bool value, HeeksObj* object){((COp*)object)->m_active = value;heeksCAD->Changed();}
static void on_set_execution_order(int value, HeeksObj* object){((COp*)object)->m_execution_order = value;heeksCAD->Changed();}

static void on_set_cutting_tool_number(int zero_based_choice, HeeksObj* object)
{
	if (zero_based_choice < 0) return;	// An error has occured.

	std::vector< std::pair< int, wxString > > tools = CCuttingTool::FindAllCuttingTools();

	if ((zero_based_choice >= int(0)) && (zero_based_choice <= int(tools.size()-1)))
	{
                ((COp *)object)->m_cutting_tool_number = tools[zero_based_choice].first;	// Convert the choice offset to the tool number for that choice
	} // End if - then

	((COp*)object)->WriteDefaultValues();

} // End on_set_cutting_tool_number() routine


void COp::GetProperties(std::list<Property *> *list)
{
	list->push_back(new PropertyString(_("comment"), m_comment, this, on_set_comment));
	list->push_back(new PropertyCheck(_("active"), m_active, this, on_set_active));
	list->push_back(new PropertyInt(_("execution_order"), m_execution_order, this, on_set_execution_order));

	{
		std::vector< std::pair< int, wxString > > tools = CCuttingTool::FindAllCuttingTools();

		int choice = -1;
                std::list< wxString > choices;
		for (std::vector< std::pair< int, wxString > >::size_type i=0; i<tools.size(); i++)
		{
                	choices.push_back(tools[i].second);

			if (m_cutting_tool_number == tools[i].first)
			{
                		choice = int(i);
			} // End if - then
		} // End for

                list->push_back(new PropertyChoice(_("cutting tool"), choices, choice, this, on_set_cutting_tool_number));
        }

	HeeksObj::GetProperties(list);
}

void COp::WriteDefaultValues()
{
	CNCConfig config;
	config.Write(wxString(GetTypeString()) + _T("CuttingTool"), m_cutting_tool_number);
}

void COp::ReadDefaultValues()
{
	if (m_cutting_tool_number <= 0)
	{
		// The cutting tool number hasn't been assigned from above.  Set some reasonable
		// defaults.

		CNCConfig config;

		// assume that default.tooltable contains tools with IDs:
		// 1 drill
		// 2 centre drill
		// 3 end mill
		// 4 slot drill
		// 5 ball end mill
		// 6 chamfering bit
		// 7 turn tool

		int default_tool = 0;
		switch(GetType())
		{
		case DrillingType:
			default_tool = CCuttingTool::FindFirstByType( CCuttingToolParams::eDrill );
			if (default_tool <= 0) default_tool = CCuttingTool::FindFirstByType( CCuttingToolParams::eCentreDrill );
			break;
		case AdaptiveType:
			default_tool = CCuttingTool::FindFirstByType( CCuttingToolParams::eEndmill );
			if (default_tool <= 0) default_tool = CCuttingTool::FindFirstByType( CCuttingToolParams::eSlotCutter );
			if (default_tool <= 0) default_tool = CCuttingTool::FindFirstByType( CCuttingToolParams::eBallEndMill );
			break;
		case ProfileType:
		case PocketType:
		case CounterBoreType:
			default_tool = CCuttingTool::FindFirstByType( CCuttingToolParams::eEndmill );
			if (default_tool <= 0) default_tool = CCuttingTool::FindFirstByType( CCuttingToolParams::eSlotCutter );
			if (default_tool <= 0) default_tool = CCuttingTool::FindFirstByType( CCuttingToolParams::eBallEndMill );
			break;
		case ZigZagType:
			default_tool = CCuttingTool::FindFirstByType( CCuttingToolParams::eEndmill );
			if (default_tool <= 0) default_tool = CCuttingTool::FindFirstByType( CCuttingToolParams::eBallEndMill );
			if (default_tool <= 0) default_tool = CCuttingTool::FindFirstByType( CCuttingToolParams::eSlotCutter );
			break;
		case TurnRoughType:
			default_tool = CCuttingTool::FindFirstByType( CCuttingToolParams::eTurningTool );
			break;
		case ProbeCentreType:
		case ProbeEdgeType:
			default_tool = CCuttingTool::FindFirstByType( CCuttingToolParams::eTouchProbe );
			break;
		default:
			default_tool = CCuttingTool::FindFirstByType( CCuttingToolParams::eEndmill );
			if (default_tool <= 0) default_tool = CCuttingTool::FindFirstByType( CCuttingToolParams::eSlotCutter );
			if (default_tool <= 0) default_tool = CCuttingTool::FindFirstByType( CCuttingToolParams::eBallEndMill );
			if (default_tool <= 0) default_tool = 4;
			break;
		}
		config.Read(wxString(GetTypeString()) + _T("CuttingTool"), &m_cutting_tool_number, default_tool);
	} // End if - then
}

void COp::AppendTextToProgram(const CFixture *pFixture)
{
	if(m_comment.Len() > 0)
	{
		theApp.m_program_canvas->m_textCtrl->AppendText(wxString(_T("comment('")) + m_comment + _T("')\n"));
	}
}

//static
bool COp::IsAnOperation(int object_type)
{
	switch(object_type)
	{
		case ProfileType:
		case PocketType:
		case ZigZagType:
		case AdaptiveType:
		case DrillingType:
		case CounterBoreType:
		case TurnRoughType:
		case LocatingType:
		case ProbeCentreType:
		case ProbeEdgeType:
			return true;
		default:
			return false;
	}
}

void COp::OnEditString(const wxChar* str){
	m_title.assign(str);
	heeksCAD->Changed();
}

void COp::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
    HeeksObj::GetTools( t_list, p );
}