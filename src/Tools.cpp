// Tools.cpp
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include "Tools.h"
#include "Program.h"
#include "interface/Tool.h"
#include "interface/PropertyChoice.h"
#include "CTool.h"
#include "CNCConfig.h"
#include "tinyxml/tinyxml.h"
#include <wx/stdpaths.h>

bool CTools::CanAdd(HeeksObj* object)
{
	return 	((object != NULL) && (object->GetType() == ToolType));
}


HeeksObj *CTools::MakeACopy(void) const
{
    return(new CTools(*this));  // Call the copy constructor.
}


CTools::CTools()
{
    CNCConfig config;
	config.Read(_T("title_format"), (int *) (&m_title_format), int(eGuageReplacesSize) );
}


CTools::CTools( const CTools & rhs ) : ObjList(rhs)
{
    m_title_format = rhs.m_title_format;
}

CTools & CTools::operator= ( const CTools & rhs )
{
    if (this != &rhs)
    {
        ObjList::operator=( rhs );
        m_title_format = rhs.m_title_format;
    }
    return(*this);
}


const wxBitmap &CTools::GetIcon()
{
	static wxBitmap* icon = NULL;
	if(icon == NULL)icon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/tools.png")));
	return *icon;
}

/**
	We need to copy the tools from the CTools object passed in into our own
	list.  We don't want to duplicate tools that are already in our local tool table.
	If we import a tool, we need to make sure the tool number is unique within the
	whole tool table.  If we need to renumber a tool during this import, we need to
	also update any associated Operations objects that refer to this tool number
	so that they now point to the new tool number.
 */
void CTools::CopyFrom(const HeeksObj* object)
{
    /*
    if (object->GetType() == GetType())
    {
        for (HeeksObj *tool = object->GetFirstChild(); tool != NULL; tool = object->GetNextChild())
        {

        }
    }
    */
}

void CTools::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element;
	element = heeksCAD->NewXMLElement( "Tools" );
	heeksCAD->LinkXMLEndChild( root,  element );
	WriteBaseXML(element);
}

//static
HeeksObj* CTools::ReadFromXMLElement(TiXmlElement* pElem)
{
	CTools* new_object = new CTools;
	new_object->ReadBaseXML(pElem);
	return new_object;
}

class ExportTools: public Tool{
	bool m_for_default;

	// Tool's virtual functions
	const wxChar* GetTitle(){return m_for_default ? _("Save As Default"):_("Export");}
	void Run()
	{
#if wxCHECK_VERSION(3, 0, 0)
		wxStandardPaths& standard_paths = wxStandardPaths::Get();
#else
		wxStandardPaths standard_paths;
#endif
		if (previous_path.Length() == 0) previous_path = _T("default.tooltable");

		if(m_for_default)
		{
			previous_path = theApp.GetDllFolder() + _T("/") + previous_path;
		}
		else
		{
			// Prompt the user to select a file to import.
			wxFileDialog fd(heeksCAD->GetMainFrame(), _T("Select a file to export to"),
				standard_paths.GetUserConfigDir().c_str(), previous_path.c_str(),
				wxString(_("Known Files")) + _T(" |*.heeks;*.HEEKS;")
				+ _T("*.tool;*.TOOL;*.Tool;")
				+ _T("*.tools;*.TOOLS;*.Tools;")
				+ _T("*.tooltable;*.TOOLTABLE;*.ToolTable;"),
				wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

			fd.SetFilterIndex(1);
			if (fd.ShowModal() == wxID_CANCEL) return;
			previous_path = fd.GetPath().c_str();
		}

		std::list<HeeksObj *> tools;
		for (HeeksObj *tool = theApp.m_program->Tools()->GetFirstChild();
			tool != NULL;
			tool = theApp.m_program->Tools()->GetNextChild() )
		{
			tools.push_back( tool );
		} // End for

		heeksCAD->SaveXMLFile( tools, previous_path.c_str(), false );
	}
	wxString BitmapPath(){ return _T("export");}
	wxString previous_path;

public:
	ExportTools(bool for_default = false)
	{
		m_for_default = for_default;
	}
};

static ExportTools export_tools;
static ExportTools save_default_tools(true);

void ImportToolsFile( const wxChar *file_path )
{
    // Delete the speed references that we've already got.  Otherwise we end
    // up with duplicates.  Do this in two passes.  Otherwise we end up
    // traversing the same list that we're modifying.

    std::list<HeeksObj *> tools;
    for (HeeksObj *tool = theApp.m_program->Tools()->GetFirstChild();
        tool != NULL;
        tool = theApp.m_program->Tools()->GetNextChild() )
    {
        tools.push_back( tool );
    } // End for

    for (std::list<HeeksObj *>::iterator l_itObject = tools.begin(); l_itObject != tools.end(); l_itObject++)
    {
        heeksCAD->Remove( *l_itObject );
    } // End for

    // And read the default speed references.
    // heeksCAD->OpenXMLFile( _T("default.speeds"), true, theApp.m_program->m_tools );
    heeksCAD->OpenXMLFile( file_path, theApp.m_program->Tools() );
}

class ImportTools: public Tool{
	bool m_for_default;

	// Tool's virtual functions
	const wxChar* GetTitle(){return m_for_default ? _("Restore Default Tools"):_("Import");}
	void Run()
	{
#if wxCHECK_VERSION(3, 0, 0)
		wxStandardPaths& standard_paths = wxStandardPaths::Get();
#else
		wxStandardPaths standard_paths;
#endif
		if (previous_path.Length() == 0) previous_path = _T("default.tooltable");

		if(m_for_default)
		{
			previous_path = theApp.GetDllFolder() + _T("/") + previous_path;
		}
		else
		{
			// Prompt the user to select a file to import.
			wxFileDialog fd(heeksCAD->GetMainFrame(), _T("Select a file to import"),
				standard_paths.GetUserConfigDir().c_str(), previous_path.c_str(),
				wxString(_("Known Files")) + _T(" |*.heeks;*.HEEKS;")
				+ _T("*.tool;*.TOOL;*.Tool;")
				+ _T("*.tools;*.TOOLS;*.Tools;")
				+ _T("*.tooltable;*.TOOLTABLE;*.ToolTable;"),
				wxFD_OPEN | wxFD_FILE_MUST_EXIST );
			fd.SetFilterIndex(1);
			if (fd.ShowModal() == wxID_CANCEL) return;
			previous_path = fd.GetPath().c_str();
		}

        ImportToolsFile( previous_path.c_str() );
	}
	wxString BitmapPath(){ return _T("import");}
	wxString previous_path;

public:
	ImportTools(bool for_default = false)
	{
		m_for_default = for_default;
	}
};

static ImportTools import_tools;
static ImportTools import_default_tools(true);

void CTools::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
	t_list->push_back(&save_default_tools);
	t_list->push_back(&import_default_tools);
	t_list->push_back(&import_tools);
	t_list->push_back(&export_tools);

	CHeeksCNCApp::GetNewToolTools(t_list);

	ObjList::GetTools(t_list, p);
}

static void on_set_title_format(int value, HeeksObj* object, bool from_undo_redo)
{
	((CTools *)object)->m_title_format = CTools::TitleFormat_t(value);

	CNCConfig config;
	config.Write(_T("title_format"), (int)(((CTools *)object)->m_title_format));
}

void CTools::GetProperties(std::list<Property *> *list)
{
	{
		std::list< wxString > choices;
		choices.push_back( _("Guage number replaces size") );
		choices.push_back( _("Include guage number and size") );

		list->push_back ( new PropertyChoice ( _("Title Format"),  choices, m_title_format, this, on_set_title_format ) );
	}
	HeeksObj::GetProperties(list);
}


