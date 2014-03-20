
// Op.cpp
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include <stdafx.h>
#include "Op.h"
#include "Operations.h"
#include "../../interface/PropertyInt.h"
#include "../../interface/PropertyString.h"
#include "../../interface/PropertyCheck.h"
#include "../../interface/PropertyChoice.h"
#include "../../interface/Tool.h"
#include "../../tinyxml/tinyxml.h"
#include "HeeksCNCTypes.h"
#include "CTool.h"
#include "PythonStuff.h"
#include "CNCConfig.h"
#include "Program.h"
#include "../../interface/HDialogs.h"
#include "Tools.h"

#define FIND_FIRST_TOOL CTool::FindFirstByType
#define FIND_ALL_TOOLS CTool::FindAllTools

#include <iterator>
#include <vector>

const wxBitmap& COp::GetInactiveIcon()
{
	static wxBitmap* icon = NULL;
	if(icon == NULL)icon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/noentry.png")));
	return *icon;
}

void COp::WriteBaseXML(TiXmlElement *element)
{
	if(m_comment.Len() > 0)element->SetAttribute( "comment", m_comment.utf8_str());
	element->SetAttribute( "active", m_active);
	element->SetAttribute( "tool_number", m_tool_number);
	element->SetAttribute( "pattern", m_pattern);
	element->SetAttribute( "surface", m_surface);

	IdNamedObjList::WriteBaseXML(element);
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
	else
	{
        m_active = true;
	}

	if (element->Attribute("tool_number") != NULL)
	{
		m_tool_number = atoi(element->Attribute("tool_number"));
	} // End if - then
	else
	{
	    // The older files used to call this attribute 'cutting_tool_number'.  Look for this old
	    // name if we can't find the new one.
	    if (element->Attribute("cutting_tool_number") != NULL)
        {
            m_tool_number = atoi(element->Attribute("cutting_tool_number"));
        } // End if - then
        else
        {
            m_tool_number = 0;
        } // End if - else
	} // End if - else

	element->Attribute( "pattern", &m_pattern);
	element->Attribute( "surface", &m_surface);

	IdNamedObjList::ReadBaseXML(element);
}

static void on_set_comment(const wxChar* value, HeeksObj* object){((COp*)object)->m_comment = value;}
static void on_set_active(bool value, HeeksObj* object){((COp*)object)->m_active = value;}
static void on_set_pattern(int value, HeeksObj* object){((COp*)object)->m_pattern = value;}
static void on_set_surface(int value, HeeksObj* object){((COp*)object)->m_surface = value;}

static std::vector< std::pair< int, wxString > > tools_for_GetProperties;

static void on_set_tool_number(int zero_based_choice, HeeksObj* object, bool from_undo_redo)
{
	if (zero_based_choice < 0) return;	// An error has occured.

	if ((zero_based_choice >= int(0)) && (zero_based_choice <= int(tools_for_GetProperties.size()-1)))
	{
                ((COp *)object)->m_tool_number = tools_for_GetProperties[zero_based_choice].first;	// Convert the choice offset to the tool number for that choice
	} // End if - then

	((COp*)object)->WriteDefaultValues();

} // End on_set_tool_number() routine


void COp::GetProperties(std::list<Property *> *list)
{
	list->push_back(new PropertyString(_("comment"), m_comment, this, on_set_comment));
	list->push_back(new PropertyCheck(_("active"), m_active, this, on_set_active));

	if(UsesTool()){

		HTypeObjectDropDown::GetObjectArrayString(ToolType, theApp.m_program->Tools(), tools_for_GetProperties);
		int choice = 0;
                std::list< wxString > choices;
		for (std::vector< std::pair< int, wxString > >::size_type i=0; i<tools_for_GetProperties.size(); i++)
		{
                	choices.push_back(tools_for_GetProperties[i].second);

			if (m_tool_number == tools_for_GetProperties[i].first)
			{
                		choice = int(i);
			} // End if - then
		} // End for

		list->push_back(new PropertyChoice(_("tool"), choices, choice, this, on_set_tool_number));
	}

	list->push_back(new PropertyInt(_("pattern"), m_pattern, this, on_set_pattern));
	list->push_back(new PropertyInt(_("surface"), m_surface, this, on_set_surface));

	IdNamedObjList::GetProperties(list);
}

COp & COp::operator= ( const COp & rhs )
{
	if (this != &rhs)
	{
		IdNamedObjList::operator=( rhs );

		m_comment = rhs.m_comment;
		m_active = rhs.m_active;
		m_tool_number = rhs.m_tool_number;
		m_operation_type = rhs.m_operation_type;
		m_pattern = rhs.m_pattern;
		m_surface = rhs.m_surface;
	}

	return(*this);
}

COp::COp( const COp & rhs ) : IdNamedObjList(rhs)
{
	*this = rhs;	// Call the assignment operator.
}

void COp::glCommands(bool select, bool marked, bool no_color)
{
	IdNamedObjList::glCommands(select, marked, no_color);
}


void COp::WriteDefaultValues()
{
	CNCConfig config;
	config.Write(_T("Tool"), m_tool_number);
	config.Write(_T("Pattern"), m_pattern);
	config.Write(_T("Surface"), m_surface);
}

void COp::ReadDefaultValues()
{
	CNCConfig config;
	if (m_tool_number <= 0)
	{
		// The tool number hasn't been assigned from above.  Set some reasonable
		// defaults.


		// assume that default.tooltable contains tools with IDs:
		// 1 drill
		// 2 centre drill
		// 3 end mill
		// 4 slot drill
		// 5 ball end mill
		// 6 chamfering bit
		// 7 turn tool

		int default_tool = 0;
		switch(m_operation_type)
		{
		case DrillingType:
			default_tool = FIND_FIRST_TOOL( CToolParams::eDrill );
			if (default_tool <= 0) default_tool = FIND_FIRST_TOOL( CToolParams::eCentreDrill );
			break;
		case ProfileType:
		case PocketType:
			default_tool = FIND_FIRST_TOOL( CToolParams::eEndmill );
			if (default_tool <= 0) default_tool = FIND_FIRST_TOOL( CToolParams::eSlotCutter );
			if (default_tool <= 0) default_tool = FIND_FIRST_TOOL( CToolParams::eBallEndMill );
			break;

		default:
			default_tool = FIND_FIRST_TOOL( CToolParams::eEndmill );
			if (default_tool <= 0) default_tool = FIND_FIRST_TOOL( CToolParams::eSlotCutter );
			if (default_tool <= 0) default_tool = FIND_FIRST_TOOL( CToolParams::eBallEndMill );
			if (default_tool <= 0) default_tool = 4;
			break;
		}
		config.Read(_T("Tool"), &m_tool_number, default_tool);
	} // End if - then

	config.Read(_T("Pattern"), &m_pattern, 0);
	config.Read(_T("Surface"), &m_surface, 0);
}


/**
    Change tools (if necessary) and assign any private fixtures.
 */
Python COp::AppendTextToProgram()
{
    Python python;

	if(m_comment.Len() > 0)
	{
		python << _T("comment(") << PythonString(m_comment) << _T(")\n");
	}

	if(UsesTool())
	{
		python << theApp.SetTool(m_tool_number); // Select the correct  tool.
	}

	return(python);
}

void COp::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
    IdNamedObjList::GetTools( t_list, p );
}

bool COp::operator==(const COp & rhs) const
{
	if (m_comment != rhs.m_comment) return(false);
	if (m_active != rhs.m_active) return(false);
	if (m_tool_number != rhs.m_tool_number) return(false);
	if (m_operation_type != rhs.m_operation_type) return(false);

	return(IdNamedObjList::operator==(rhs));
}

HeeksObj* COp::PreferredPasteTarget()
{
	return theApp.m_program->Operations();
}
