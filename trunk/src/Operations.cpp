// Operations.cpp
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include "Operations.h"
#include "Op.h"
#include "tinyxml/tinyxml.h"
#include "Excellon.h"

#include <wx/progdlg.h>

bool COperations::CanAdd(HeeksObj* object)
{
	return 	COp::IsAnOperation(object->GetType());
}

COperations & COperations::operator= ( const COperations & rhs )
{
	if (this != &rhs)
	{
		ObjList::operator=( rhs );
	}

	return(*this);
}

COperations::COperations( const COperations & rhs ) : ObjList(rhs)
{
	*this = rhs;	// Call the assignment operator.
}

/**
	This is ALMOST the same as the assignment operator.  The difference is that
	this method augments its local list of operations with those passed in rather
	than replacing them.
 */
void COperations::CopyFrom(const HeeksObj *object)
{
	COperations *rhs = (COperations*)object;
	for (HeeksObj *child = rhs->GetFirstChild(); child != NULL; child = rhs->GetNextChild())
	{
		child->SetID( heeksCAD->GetNextID(child->GetType()) );
		Add(child, NULL);
	} // End for
}

void COperations::ReloadPointers()
{
	ObjList::ReloadPointers();
}

void COperations::glCommands(bool select, bool marked, bool no_color)
{
	ObjList::glCommands(select, marked, no_color);
}


void COperations::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element;
	element = new TiXmlElement( "Operations" );
	root->LinkEndChild( element );
	WriteBaseXML(element);
}

//static
HeeksObj* COperations::ReadFromXMLElement(TiXmlElement* pElem)
{
	COperations* new_object = new COperations;
	new_object->ReadBaseXML(pElem);
	return new_object;
}

static COperations* object_for_tools = NULL;

class SetAllActive: public Tool{
	// Tool's virtual functions
	const wxChar* GetTitle(){return _("Set All Operations Active");}
	void Run()
	{
		for(HeeksObj* object = object_for_tools->GetFirstChild(); object; object = object_for_tools->GetNextChild())
		{
			if(COp::IsAnOperation(object->GetType()))
			{
				((COp*)object)->m_active = true;
				heeksCAD->Changed();
			}
		}
	}
	wxString BitmapPath(){ return _T("setactive");}
};

static SetAllActive set_all_active;

class SetAllInactive: public Tool{
	// Tool's virtual functions
	const wxChar* GetTitle(){return _("Set All Operations Inactive");}
	void Run()
	{
		for(HeeksObj* object = object_for_tools->GetFirstChild(); object; object = object_for_tools->GetNextChild())
		{
			if(COp::IsAnOperation(object->GetType()))
			{
				((COp*)object)->m_active = false;
				heeksCAD->Changed();
			}
		}
	}
	wxString BitmapPath(){ return _T("setinactive");}
};

static SetAllInactive set_all_inactive;

void COperations::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
	object_for_tools = this;

	CHeeksCNCApp::GetNewOperationTools(t_list);

	t_list->push_back(&set_all_active);
	t_list->push_back(&set_all_inactive);

	ObjList::GetTools(t_list, p);
}
