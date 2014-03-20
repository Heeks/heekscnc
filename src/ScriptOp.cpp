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
#include "../../interface/HeeksObj.h"
#include "../../interface/PropertyList.h"
#include "../../interface/PropertyCheck.h"
#include "../../tinyxml/tinyxml.h"
#include "PythonStuff.h"
#include "ScriptOpDlg.h"

#include <sstream>
#include <iomanip>

CScriptOp::CScriptOp( const CScriptOp & rhs ) : COp(rhs)
{
	m_str = rhs.m_str;
	m_user_icon = rhs.m_user_icon;
	m_user_icon_name = rhs.m_user_icon_name;
}

CScriptOp & CScriptOp::operator= ( const CScriptOp & rhs )
{
	if (this != &rhs)
	{
		COp::operator=( rhs );
		m_str = rhs.m_str;
		m_user_icon = rhs.m_user_icon;
		m_user_icon_name = rhs.m_user_icon_name;
	}

	return(*this);
}

static std::map<wxString, wxBitmap> icon_map;

static const wxBitmap &UserIcon(const wxString& str)
{
	std::map<wxString, wxBitmap>::iterator FindIt = icon_map.find(str);
	if(FindIt == icon_map.end())
	{
		icon_map.insert(std::make_pair(str, wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/") + str + _T(".png")))));
		FindIt = icon_map.find(str);
	}
	return FindIt->second;
}

const wxBitmap &CScriptOp::GetIcon()
{
	if(!m_active)return GetInactiveIcon();
	if(m_user_icon)
	{
		return UserIcon(this->m_user_icon_name);
	}
	static wxBitmap* icon = NULL;
	if(icon == NULL)icon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/scriptop.png")));
	return *icon;
}

Python CScriptOp::AppendTextToProgram()
{
	Python python;

	if(m_comment.Len() > 0)
	{
	  python << _T("comment(") << PythonString(m_comment) << _T(")\n");
	}

	python << m_str.c_str();

	if(python.Last() != _T('\n'))python.Append(_T('\n'));

	return python;
} // End AppendTextToProgram() method

void CScriptOp::GetProperties(std::list<Property *> *list)
{
    COp::GetProperties(list);
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
	TiXmlElement * element = heeksCAD->NewXMLElement( "ScriptOp" );
	heeksCAD->LinkXMLEndChild( root,  element );
	element->SetAttribute( "script", Ttc(m_str.c_str()));

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

static bool OnEdit(HeeksObj* object)
{
	ScriptOpDlg dlg(heeksCAD->GetMainFrame(), (CScriptOp*)object);
	if(dlg.ShowModal() == wxID_OK)
	{
		dlg.GetData((CScriptOp*)object);
		return true;
	}
	return false;
}

void CScriptOp::GetOnEdit(bool(**callback)(HeeksObj*))
{
	*callback = OnEdit;
}
