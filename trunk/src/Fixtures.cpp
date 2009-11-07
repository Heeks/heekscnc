// Fixtures.cpp
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include "Fixtures.h"
#include "tinyxml/tinyxml.h"
#include "Program.h"
#include "interface/Tool.h"
#include <wx/stdpaths.h>

void CFixtures::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element;
	element = new TiXmlElement( "Fixtures" );
	root->LinkEndChild( element );  
	WriteBaseXML(element);
}

HeeksObj* CFixtures::ReadFromXMLElement(TiXmlElement* pElem)
{
	CFixtures* new_object = new CFixtures();
	new_object->ReadBaseXML(pElem);
	return new_object;
}



class ExportFixtures: public Tool{
	// Tool's virtual functions
	const wxChar* GetTitle(){return _("Export");}
	void Run()
	{
		wxStandardPaths standard_paths;
		if (previous_path.Length() == 0) previous_path = _T("default.fixtures");

		// Prompt the user to select a file to import.
		wxFileDialog fd(heeksCAD->GetMainFrame(), _T("Select a file to export to"), 
		standard_paths.GetUserConfigDir().c_str(), previous_path.c_str(),
				wxString(_("Known Files")) + _T(" |*.heeks;*.HEEKS;")
					+ _T("*.fixture;*.FIXTURE;*.Fixture;")
					+ _T("*.fixtures;*.FIXTURES;*.Fixtures;"),
					wxSAVE | wxOVERWRITE_PROMPT );

		fd.SetFilterIndex(1);
		if (fd.ShowModal() == wxID_CANCEL) return;
		previous_path = fd.GetPath().c_str();
		std::list<HeeksObj *> fixtures;
		for (HeeksObj *fixture = theApp.m_program->Fixtures()->GetFirstChild();
			fixture != NULL;
			fixture = theApp.m_program->Fixtures()->GetNextChild() )
		{
			fixtures.push_back( fixture );
		} // End for

		heeksCAD->SaveXMLFile( fixtures, previous_path.c_str(), false );
	}
	wxString BitmapPath(){ return _T("export");}
	wxString previous_path;
};

static ExportFixtures export_fixtures;

class ImportFixtures: public Tool{
	// Tool's virtual functions
	const wxChar* GetTitle(){return _("Import");}
	void Run()
	{
		wxStandardPaths standard_paths;
		if (previous_path.Length() == 0) previous_path = _T("default.fixtures");

		// Prompt the user to select a file to import.
		wxFileDialog fd(heeksCAD->GetMainFrame(), _T("Select a file to import"),
				standard_paths.GetUserConfigDir().c_str(), previous_path.c_str(),
				wxString(_("Known Files")) + _T(" |*.heeks;*.HEEKS;")
					+ _T("*.fixture;*.FIXTURE;*.Fixture;")
					+ _T("*.fixtures;*.FIXTURES;*.Fixtures;"),
					wxOPEN | wxFILE_MUST_EXIST );
		fd.SetFilterIndex(1);
		if (fd.ShowModal() == wxID_CANCEL) return;
		previous_path = fd.GetPath().c_str();

		// Delete the fixtures that we've already got.  Otherwise we end
		// up with duplicates.  Do this in two passes.  Otherwise we end up
		// traversing the same list that we're modifying.

		std::list<HeeksObj *> fixtures;
		for (HeeksObj *fixture = theApp.m_program->Fixtures()->GetFirstChild();
			fixture != NULL;
			fixture = theApp.m_program->Fixtures()->GetNextChild() )
		{
			fixtures.push_back( fixture );
		} // End for

		for (std::list<HeeksObj *>::iterator l_itObject = fixtures.begin(); l_itObject != fixtures.end(); l_itObject++)
		{
			heeksCAD->Remove( *l_itObject );
		} // End for

		// And read the default fixtures references.
		heeksCAD->OpenXMLFile( previous_path.c_str(), theApp.m_program->Fixtures() );
	}
	wxString BitmapPath(){ return _T("import");}
	wxString previous_path;
};

static ImportFixtures import_fixtures;

void CFixtures::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
	t_list->push_back(&import_fixtures);
	t_list->push_back(&export_fixtures);

	ObjList::GetTools(t_list, p);
}
