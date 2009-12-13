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

class ImportExcellonDrillFile: public Tool{
	// Tool's virtual functions
	const wxChar* GetTitle(){return _("Import Excellon Drill File");}
	void Run()
	{
		wxFileDialog dialog(heeksCAD->GetMainFrame(), _("Import Excellon Drill File"), 
					wxEmptyString, wxEmptyString, 
					wxString(_("Excellon Drill Files")) + _T(" |*.cnc;*.CNC;*.drill;*.DRILL;*.drl;*.DRL") );
		dialog.CentreOnParent();
		if (dialog.ShowModal() == wxID_OK)
		{
			Excellon drill;
			drill.Read( Ttc(dialog.GetPath().c_str()) );
		}

	}
	wxString BitmapPath(){ return _T("drill");}
};

static ImportExcellonDrillFile import_excellon_drill_file;

void COperations::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
	object_for_tools = this;

	CHeeksCNCApp::GetNewOperationTools(t_list);

	t_list->push_back(&set_all_active);
	t_list->push_back(&set_all_inactive);
	t_list->push_back(&import_excellon_drill_file);

	ObjList::GetTools(t_list, p);
}
