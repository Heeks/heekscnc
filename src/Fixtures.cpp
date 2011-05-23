// Fixtures.cpp
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#ifndef STABLE_OPS_ONLY
#include "Fixtures.h"
#include "tinyxml/tinyxml.h"
#include "Program.h"
#include "interface/Tool.h"
#include "Operations.h"

#include <wx/stdpaths.h>

const wxBitmap &CFixtures::GetIcon()
{
	static wxBitmap* icon = NULL;
	if(icon == NULL)icon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/fixtures.png")));
	return *icon;
}

void CFixtures::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element;
	element = heeksCAD->NewXMLElement( "Fixtures" );
	heeksCAD->LinkXMLEndChild( root,  element );
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

void ImportFixturesFile( const wxChar *file_path )
{
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
    heeksCAD->OpenXMLFile( file_path, theApp.m_program->Fixtures() );
}

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

        ImportFixturesFile( previous_path.c_str() );
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


/**
    Augment my list of child CFixture objects with any found in the CFixtures (children)
    passed in.
 */
void CFixtures::CopyFrom(const HeeksObj* object)
{
    if (object->GetType() == GetType())
    {
        for (HeeksObj *child = ((HeeksObj *) object)->GetFirstChild(); child != NULL; child = ((HeeksObj *) object)->GetNextChild())
        {
            if (child->GetType() == FixtureType)
            {
                bool found = false;
                for (HeeksObj *mine = GetFirstChild(); ((mine != NULL) && (! found)); mine = GetNextChild())
                {
                    if (mine->GetType() == FixtureType)
                    {
                        if (((*(CFixture *) mine)) == (*((CFixture *) child)))
                        {
                            found = true;
                            break;
                        } // End if - then
                    } // End if - then
                } // End for
                if (! found)
                {
                    Add( child, NULL );
                } // End if - then
            } // End if - then
        } // End for
    } // End if - then
}


/**
	The fixture objects may be in the 'theApp.m_program->Fixtures()' tree (for globally applied fixtures) but
	they may also be children of operations 'theApp.m_program->Operations()'.
 */
CFixture *CFixtures::Find( const CFixture::eCoordinateSystemNumber_t coordinate_system_number )
{
    for(HeeksObj* ob = GetFirstChild(); ob; ob = GetNextChild())
    {
        if (ob->GetType() != FixtureType) continue;

        if (ob != NULL)
        {
            if (((CFixture *)ob)->m_coordinate_system_number == coordinate_system_number)
            {
                return( (CFixture *) ob );
            } // End if - then
        } // End if - then
    } // End for

	for(HeeksObj* operation = theApp.m_program->Operations()->GetFirstChild(); operation; operation = theApp.m_program->Operations()->GetNextChild())
    {
		for (HeeksObj *ob = operation->GetFirstChild(); ob != NULL; ob = operation->GetNextChild())
		{
			if (ob->GetType() != FixtureType) continue;

			if (ob != NULL)
			{
				if (((CFixture *)ob)->m_coordinate_system_number == coordinate_system_number)
				{
					return( (CFixture *) ob );
				} // End if - then
			} // End if - then
		} // End for
	} // End for

	return(NULL);

} // End Find() method


/**
	The fixture objects may be in the 'theApp.m_program->Fixtures()' tree (for globally applied fixtures) but
	they may also be children of operations 'theApp.m_program->Operations()'.
 */
int CFixtures::GetNextFixture()
{
    // Now run through and find one that's not already used.
	for (int fixture = int(CFixture::G54); fixture <= int(CFixture::G59_3); fixture++)
    {
		if (Find(CFixture::eCoordinateSystemNumber_t(fixture)) == NULL) return(fixture);
    } // End for

	return(-1);	// None available.
} // End GetNextFixture() method

#endif