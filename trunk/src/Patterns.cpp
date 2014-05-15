// Patterns.cpp
// Copyright (c) 2013, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include "Patterns.h"
#include "Program.h"
#include "interface/PropertyChoice.h"
#include "Pattern.h"
#include "CNCConfig.h"
#include "tinyxml/tinyxml.h"
#include <wx/stdpaths.h>

bool CPatterns::CanAdd(HeeksObj* object)
{
	return 	((object != NULL) && (object->GetType() == PatternType));
}


HeeksObj *CPatterns::MakeACopy(void) const
{
    return(new CPatterns(*this));  // Call the copy constructor.
}


CPatterns::CPatterns()
{
    CNCConfig config;
}


CPatterns::CPatterns( const CPatterns & rhs ) : ObjList(rhs)
{
}

CPatterns & CPatterns::operator= ( const CPatterns & rhs )
{
    if (this != &rhs)
    {
        ObjList::operator=( rhs );
    }
    return(*this);
}


const wxBitmap &CPatterns::GetIcon()
{
	static wxBitmap* icon = NULL;
	if(icon == NULL)icon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/patterns.png")));
	return *icon;
}

void CPatterns::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element;
	element = heeksCAD->NewXMLElement( "Patterns" );
	heeksCAD->LinkXMLEndChild( root,  element );
	WriteBaseXML(element);
}

//static
HeeksObj* CPatterns::ReadFromXMLElement(TiXmlElement* pElem)
{
	CPatterns* new_object = new CPatterns;
	new_object->ReadBaseXML(pElem);
	return new_object;
}

void CPatterns::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
	CHeeksCNCApp::GetNewPatternTools(t_list);

	ObjList::GetTools(t_list, p);
}
