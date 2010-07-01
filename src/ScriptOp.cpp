// ScriptOp.cpp
/*
 * Copyright (c) 2010, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "stdafx.h"
#include "ScriptOp.h"
#include "ProgramCanvas.h"
#include "Program.h"
#include "interface/HeeksObj.h"
#include "tinyxml/tinyxml.h"
#include "PythonStuff.h"
#include "MachineState.h"

#include <sstream>
#include <iomanip>

CScriptOp::CScriptOp( const CScriptOp & rhs ) : COp(rhs)
{
	m_str = rhs.m_str;
}

CScriptOp & CScriptOp::operator= ( const CScriptOp & rhs )
{
	if (this != &rhs)
	{
		COp::operator=( rhs );
		m_str = rhs.m_str;
	}

	return(*this);
}

const wxBitmap &CScriptOp::GetIcon()
{
	if(!m_active)return GetInactiveIcon();
	static wxBitmap* icon = NULL;
	if(icon == NULL)icon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/scriptop.png")));
	return *icon;
}

Python CScriptOp::AppendTextToProgram(CMachineState *pMachineState)
{
	Python python;

	python << m_str.c_str();

	if(python.Last() != _T('\n'))python.Append(_T('\n'));

	return python;
} // End AppendTextToProgram() method

ObjectCanvas* CScriptOp::GetDialog(wxWindow* parent)
{
	static TextCanvas* object_canvas = NULL;
	if(object_canvas == NULL)object_canvas = new TextCanvas(parent, &m_str);
	else
	{
		object_canvas->m_str = &m_str;
		object_canvas->SetWithObject(this);
	}
	return object_canvas;
}

HeeksObj *CScriptOp::MakeACopy(void)const
{
	return new CScriptOp(*this);
}

void CScriptOp::CopyFrom(const HeeksObj* object)
{
	if (object->GetType() == GetType())
	{
		operator=(*((CScriptOp*)object));
	}
}

bool CScriptOp::CanAddTo(HeeksObj* owner)
{
	return ((owner != NULL) && (owner->GetType() == OperationsType));
}

void CScriptOp::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element = new TiXmlElement( "ScriptOp" );
	root->LinkEndChild( element );
	element->SetAttribute("script", Ttc(m_str.c_str()));

	COp::WriteBaseXML(element);
}

// static member function
HeeksObj* CScriptOp::ReadFromXMLElement(TiXmlElement* element)
{
	CScriptOp* new_object = new CScriptOp;

	new_object->m_str = wxString(Ctt(element->Attribute("script")));

	// read common parameters
	new_object->ReadBaseXML(element);

	return new_object;
}

bool CScriptOp::operator==( const CScriptOp & rhs ) const
{
	if (m_str != rhs.m_str) return(false);

	return(COp::operator==(rhs));
}
