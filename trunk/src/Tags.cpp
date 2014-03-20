// Tags.cpp
// Copyright (c) 2010, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include "Tags.h"
#include "Tag.h"
#include "../../tinyxml/tinyxml.h"
#include "../../interface/Tool.h"

bool CTags::CanAdd(HeeksObj* object)
{
	return ((object != NULL) && (object->GetType() == TagType));
}

CTags & CTags::operator= ( const CTags & rhs )
{
	if (this != &rhs)
	{
		ObjList::operator=( rhs );
	}

	return(*this);
}

CTags::CTags( const CTags & rhs ) : ObjList(rhs)
{
}

const wxBitmap &CTags::GetIcon()
{
	static wxBitmap* icon = NULL;
	if(icon == NULL)icon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/tags.png")));
	return *icon;
}

static CTags* object_for_tools = NULL;

class AddTagTool: public Tool
{
public:
	// Tool's virtual functions
	const wxChar* GetTitle(){return _("Add Tag");}

	void Run()
	{
      if (object_for_tools) {
		CTag* new_object = new CTag();
		heeksCAD->AddUndoably(new_object, object_for_tools);
		heeksCAD->ClearMarkedList();
		heeksCAD->Mark(new_object);
      }
	}
	wxString BitmapPath(){ return _T("addtag");}
};

static AddTagTool add_tag_tool;

void CTags::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
	object_for_tools = this;
	t_list->push_back(&add_tag_tool);
}

void CTags::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element;
	element = heeksCAD->NewXMLElement( "Tags" );
	heeksCAD->LinkXMLEndChild( root,  element );
	WriteBaseXML(element);
}

//static
HeeksObj* CTags::ReadFromXMLElement(TiXmlElement* pElem)
{
	CTags* new_object = new CTags;
	new_object->ReadBaseXML(pElem);
	return new_object;
}
