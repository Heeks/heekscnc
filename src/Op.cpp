// Op.cpp
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "stdafx.h"
#include "Op.h"
#include "ProgramCanvas.h"
#include "interface/PropertyString.h"
#include "interface/PropertyCheck.h"
#include "interface/Tool.h"
#include "tinyxml/tinyxml.h"
#include "HeeksCNCTypes.h"

void COp::WriteBaseXML(TiXmlElement *element)
{
	if(m_comment.Len() > 0)element->SetAttribute("comment", Ttc(m_comment.c_str()));
	if(!m_active)element->SetAttribute("active", 0);

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

	HeeksObj::ReadBaseXML(element);
}

static void on_set_comment(const wxChar* value, HeeksObj* object){((COp*)object)->m_comment = value;}
static void on_set_active(bool value, HeeksObj* object){((COp*)object)->m_active = value;}

void COp::GetProperties(std::list<Property *> *list)
{
	list->push_back(new PropertyString(_("comment"), m_comment, this, on_set_comment));
	list->push_back(new PropertyCheck(_("active"), m_active, this, on_set_active));

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
			return true;
		default:
			return false;		
	}
}

