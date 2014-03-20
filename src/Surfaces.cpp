// Surfaces.cpp
// Copyright (c) 2013, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include "Surfaces.h"
#include "Program.h"
#include "../../interface/PropertyChoice.h"
#include "Surface.h"
#include "CNCConfig.h"
#include "../../tinyxml/tinyxml.h"
#include <wx/stdpaths.h>

bool CSurfaces::CanAdd(HeeksObj* object)
{
	return 	((object != NULL) && (object->GetType() == SurfaceType));
}


HeeksObj *CSurfaces::MakeACopy(void) const
{
    return(new CSurfaces(*this));  // Call the copy constructor.
}


CSurfaces::CSurfaces()
{
    CNCConfig config;
}


CSurfaces::CSurfaces( const CSurfaces & rhs ) : ObjList(rhs)
{
}

CSurfaces & CSurfaces::operator= ( const CSurfaces & rhs )
{
    if (this != &rhs)
    {
        ObjList::operator=( rhs );
    }
    return(*this);
}


const wxBitmap &CSurfaces::GetIcon()
{
	static wxBitmap* icon = NULL;
	if(icon == NULL)icon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/surfaces.png")));
	return *icon;
}

void CSurfaces::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element;
	element = heeksCAD->NewXMLElement( "Surfaces" );
	heeksCAD->LinkXMLEndChild( root,  element );
	WriteBaseXML(element);
}

//static
HeeksObj* CSurfaces::ReadFromXMLElement(TiXmlElement* pElem)
{
	CSurfaces* new_object = new CSurfaces;
	new_object->ReadBaseXML(pElem);
	return new_object;
}

void CSurfaces::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
	CHeeksCNCApp::GetNewSurfaceTools(t_list);

	ObjList::GetTools(t_list, p);
}
