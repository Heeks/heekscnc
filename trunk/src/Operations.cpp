// Operations.cpp
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include "Operations.h"
#include "Op.h"
#include "RS274X.h"
#include "tinyxml/tinyxml.h"

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
				heeksCAD->WasModified(object);
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
				heeksCAD->WasModified(object);
			}
		}
	}
	wxString BitmapPath(){ return _T("setinactive");}
};

static SetAllInactive set_all_inactive;

class ReadRS274XFile: public Tool{
	// Tool's virtual functions
	const wxChar* GetTitle(){return _("Import RS274X (Gerber) file");}
	void Run()
	{
		if (previous_path.Length() == 0) previous_path = _T("file.gbr");

		wxFileDialog fd(heeksCAD->GetMainFrame(), _T("Select a Gerber (RS274X) file to import"), _T("."), previous_path.c_str(),
				wxString(_("Gerber Files")) + _T(" |*.gbr;*.GBR;")
					+ _T("*.rs274x;*.RS274X;"),
					wxOPEN | wxFILE_MUST_EXIST );
		fd.SetFilterIndex(1);
		if (fd.ShowModal() == wxID_CANCEL) return;
		previous_path = fd.GetPath().c_str();
		RS274X gerber;
		if (!  gerber.Read( Ttc(fd.GetPath().c_str()) ))
		{
			wxMessageBox(_("Failed to import RS274X file"));
		} // End if - then
	}
	wxString BitmapPath(){ return _T("setinactive");}
	wxString previous_path;
};

static ReadRS274XFile read_rs274x_file;

void COperations::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
	object_for_tools = this;

	CHeeksCNCApp::GetNewOperationTools(t_list);

	t_list->push_back(&set_all_active);
	t_list->push_back(&set_all_inactive);
	t_list->push_back(&read_rs274x_file);

	ObjList::GetTools(t_list, p);
}
