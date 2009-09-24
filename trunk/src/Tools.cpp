// Tools.cpp
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include "Tools.h"
#include "Program.h"
#include "interface/Tool.h"
#include "tinyxml/tinyxml.h"

bool CTools::CanAdd(HeeksObj* object)
{
	return 	(((object->GetType() == CuttingToolType)) || 
		 ((object->GetType() == FixtureType)));
}


void CTools::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element;
	element = new TiXmlElement( "Tools" );
	root->LinkEndChild( element );  
	WriteBaseXML(element);
}

//static
HeeksObj* CTools::ReadFromXMLElement(TiXmlElement* pElem)
{
	CTools* new_object = new CTools;
	new_object->ReadBaseXML(pElem);
	return new_object;
}

class ExportCuttingTools: public Tool{
	// Tool's virtual functions
	const wxChar* GetTitle(){return _("Export");}
	void Run()
	{
		if (previous_path.Length() == 0) previous_path = _T("default.tooltable");

		// Prompt the user to select a file to import.
		wxFileDialog fd(heeksCAD->GetMainFrame(), _T("Select a file to export to"), _T("."), previous_path.c_str(),
				wxString(_("Known Files")) + _T(" |*.heeks;*.HEEKS;")
					+ _T("*.tool;*.TOOL;*.Tool;")
					+ _T("*.tools;*.TOOLS;*.Tools;")
					+ _T("*.tooltable;*.TOOLTABLE;*.ToolTable;"), 
					wxSAVE | wxOVERWRITE_PROMPT );

		fd.SetFilterIndex(1);
		if (fd.ShowModal() == wxID_CANCEL) return;
		previous_path = fd.GetPath().c_str();
		std::list<HeeksObj *> cutting_tools;
		for (HeeksObj *cutting_tool = theApp.m_program->Tools()->GetFirstChild();
			cutting_tool != NULL;
			cutting_tool = theApp.m_program->Tools()->GetNextChild() )
		{
			cutting_tools.push_back( cutting_tool );
		} // End for

		heeksCAD->SaveXMLFile( cutting_tools, previous_path.c_str(), false );
	}
	wxString BitmapPath(){ return _T("export");}
	wxString previous_path;
};

static ExportCuttingTools export_cutting_tools;

class ImportCuttingTools: public Tool{
	// Tool's virtual functions
	const wxChar* GetTitle(){return _("Import");}
	void Run()
	{
		if (previous_path.Length() == 0) previous_path = _T("default.tooltable");

		// Prompt the user to select a file to import.
		wxFileDialog fd(heeksCAD->GetMainFrame(), _T("Select a file to import"), _T("."), previous_path.c_str(),
				wxString(_("Known Files")) + _T(" |*.heeks;*.HEEKS;")
					+ _T("*.tool;*.TOOL;*.Tool;")
					+ _T("*.tools;*.TOOLS;*.Tools;")
					+ _T("*.tooltable;*.TOOLTABLE;*.ToolTable;"), 
					wxOPEN | wxFILE_MUST_EXIST );
		fd.SetFilterIndex(1);
		if (fd.ShowModal() == wxID_CANCEL) return;
		previous_path = fd.GetPath().c_str();

		// Delete the speed references that we've already got.  Otherwise we end
		// up with duplicates.  Do this in two passes.  Otherwise we end up
		// traversing the same list that we're modifying.

		std::list<HeeksObj *> cutting_tools;
		for (HeeksObj *cutting_tool = theApp.m_program->Tools()->GetFirstChild();
			cutting_tool != NULL;
			cutting_tool = theApp.m_program->Tools()->GetNextChild() )
		{
			cutting_tools.push_back( cutting_tool );
		} // End for

		for (std::list<HeeksObj *>::iterator l_itObject = cutting_tools.begin(); l_itObject != cutting_tools.end(); l_itObject++)
		{
			heeksCAD->Remove( *l_itObject );
		} // End for

		// And read the default speed references.
		// heeksCAD->OpenXMLFile( _T("default.speeds"), true, theApp.m_program->m_cutting_tools );
		heeksCAD->OpenXMLFile( previous_path.c_str(), theApp.m_program->Tools() );
	}
	wxString BitmapPath(){ return _T("import");}
	wxString previous_path;
};

static ImportCuttingTools import_cutting_tools;

void CTools::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
	t_list->push_back(&import_cutting_tools);
	t_list->push_back(&export_cutting_tools);

	CHeeksCNCApp::GetNewCuttingToolTools(t_list);

	ObjList::GetTools(t_list, p);
}
