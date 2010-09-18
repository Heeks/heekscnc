// Operations.cpp
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include "Operations.h"
#include "Op.h"
#include "tinyxml/tinyxml.h"
#include "Excellon.h"
#include "Probing.h"

#include <wx/progdlg.h>

bool COperations::CanAdd(HeeksObj* object)
{
	return 	((object != NULL) && (IsAnOperation(object->GetType())));
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
}

const wxBitmap &COperations::GetIcon()
{
	static wxBitmap* icon = NULL;
	if(icon == NULL)icon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/operations.png")));
	return *icon;
}

/**
	This is ALMOST the same as the assignment operator.  The difference is that
	this method augments its local list of operations with those passed in rather
	than replacing them.
 */
void COperations::CopyFrom(const HeeksObj *object)
{
    if (object->GetType() == GetType())
    {
        COperations *rhs = (COperations*)object;
        for (HeeksObj *child = rhs->GetFirstChild(); child != NULL; child = rhs->GetNextChild())
        {
            child->SetID( heeksCAD->GetNextID(child->GetType()) );
            Add(child, NULL);
        } // End for
    }
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
			if(COperations::IsAnOperation(object->GetType()))
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
			if(COperations::IsAnOperation(object->GetType()))
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

	t_list->push_back(&set_all_active);
	t_list->push_back(&set_all_inactive);

	ObjList::GetTools(t_list, p);
}

void COperations::OnChangeUnits(const double units)
{
    for(HeeksObj* object = GetFirstChild(); object != NULL; object = GetNextChild())
    {
        switch (((COp *)object)->m_operation_type)
        {
           case ProbeCentreType:
                ((CProbe_Centre *)object)->OnChangeUnits(units);
                break;

           case ProbeEdgeType:
                ((CProbe_Grid *)object)->OnChangeUnits(units);
                break;

           case ProbeGridType:
                ((CProbe_Grid *)object)->OnChangeUnits(units);
                break;
        }
    }
    heeksCAD->Changed();
}

//static
bool COperations::IsAnOperation(int object_type)
{
	switch(object_type)
	{
		case ProfileType:
		case PocketType:
		case RaftType:
		case ZigZagType:
		case WaterlineType:
		case AdaptiveType:
		case DrillingType:
		case CounterBoreType:
		case TurnRoughType:
		case LocatingType:
		case ProbeCentreType:
		case ProbeEdgeType:
		case ProbeGridType:
		case ChamferType:
		case ContourType:
		case InlayType:
		case ScriptOpType:
		case AttachOpType:
		case UnattachOpType:
			return true;
		default:
			return theApp.m_external_op_types.find(object_type) != theApp.m_external_op_types.end();
	}
}

