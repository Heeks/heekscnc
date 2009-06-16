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
#include "interface/Tool.h"
#include "tinyxml/tinyxml.h"
#include "HeeksCNCTypes.h"
#include "CuttingTool.h"

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

	if (element->Attribute("cutting_tool_number") != NULL)
	{
		m_cutting_tool_number = atoi(element->Attribute("cutting_tool_number"));
	} // End if - then

	HeeksObj::ReadBaseXML(element);
}

static void on_set_comment(const wxChar* value, HeeksObj* object){((COp*)object)->m_comment = value;}
static void on_set_active(bool value, HeeksObj* object){((COp*)object)->m_active = value;heeksCAD->WasModified(object);}
static void on_set_execution_order(int value, HeeksObj* object){((COp*)object)->m_execution_order = value;heeksCAD->WasModified(object);}

static void on_set_cutting_tool_number(int value, HeeksObj* object)
{
        // If they've set a negative or zero value then they must want to disassociate this
        // from a tool.  This just means they won't get prompted to change tools in the GCode.
        if (value <= 0)
        {
                ((COp *)object)->m_cutting_tool_number = value;
		heeksCAD->WasModified(object);
                return;
        }

        // Look through all objects to find a CuttingTool object whose tool number
        // matches this one.  If none are found then let the operator know.

        int id = 0;
        if ((id=CCuttingTool::FindCuttingTool( value )) > 0)
        {
                HeeksObj* ob = heeksCAD->GetIDObject( CuttingToolType, id );
                if ((ob != NULL) && (ob->GetType() == CuttingToolType))
                {
                	((COp *)object)->m_cutting_tool_number = value;
			heeksCAD->WasModified(object);
                        return;
                } // End if - then
        } // End for

        wxMessageBox(_T("This operation has not been assigned a cutting tool yet. Set the tool number to zero (0) if no tool table functionality is required for this operation. Otherwise, define a CuttingTool object with its own tool number before referring this operation to it."));
} // End on_set_cutting_tool_number() routine


void COp::GetProperties(std::list<Property *> *list)
{
	list->push_back(new PropertyString(_("comment"), m_comment, this, on_set_comment));
	list->push_back(new PropertyCheck(_("active"), m_active, this, on_set_active));
	list->push_back(new PropertyInt(_("execution_order"), m_execution_order, this, on_set_execution_order));
	list->push_back(new PropertyInt(_("cutting_tool_number"), m_cutting_tool_number, this, on_set_cutting_tool_number));

	HeeksObj::GetProperties(list);
}

void COp::AppendTextToProgram()
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
			return true;
		default:
			return false;		
	}
}

void COp::OnEditString(const wxChar* str){
	m_title.assign(str);
	heeksCAD->WasModified(this);
}
