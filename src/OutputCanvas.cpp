// OutputCanvas.cpp

#include "stdafx.h"
#include "OutputCanvas.h"
#include "Program.h"
#include "NCCode.h"
#include "PythonStuff.h"
#include <wx/file.h>

BEGIN_EVENT_TABLE(COutputTextCtrl, wxTextCtrl)
    EVT_MOUSE_EVENTS(COutputTextCtrl::OnMouse)
END_EVENT_TABLE()


void COutputTextCtrl::OnMouse( wxMouseEvent& event )
{
	if(event.LeftUp())
	{
		wxTextPos pos = GetInsertionPoint();
		if(theApp.m_program && theApp.m_program->m_nc_code)
		{
			theApp.m_program->m_nc_code->HighlightBlock(pos);
			heeksCAD->Repaint();
		}
	}

	event.Skip();
}



BEGIN_EVENT_TABLE(COutputCanvas, wxScrolledWindow)
    EVT_SIZE(COutputCanvas::OnSize)
END_EVENT_TABLE()

static void OnOpen(wxCommandEvent& event)
{
	wxString ext_str(_T("*.*")); // to do, use the machine's NC extension
	wxString wildcard_string = wxString(_("NC files")) + _T(" |") + ext_str;
    wxFileDialog dialog(theApp.m_output_canvas, _("Open NC file"), wxEmptyString, wxEmptyString, wildcard_string);
    dialog.CentreOnParent();

    if (dialog.ShowModal() == wxID_OK)
    {
		HeeksPyBackplot(dialog.GetPath().c_str());
	}
}

static void OnSave(wxCommandEvent& event)
{
	wxString ext_str(_T("*.tap")); // to do, use the machine's NC extension
	wxString wildcard_string = wxString(_("NC files")) + _T(" |") + ext_str;
	wxFileDialog fd(theApp.m_output_canvas, _("Save NC file"), wxEmptyString, wxEmptyString, wildcard_string, wxSAVE|wxOVERWRITE_PROMPT);
	fd.SetFilterIndex(1);
	if (fd.ShowModal() == wxID_OK)
	{
		wxString nc_file_str = fd.GetPath().c_str();
		wxFile ofs(nc_file_str.c_str(), wxFile::write);
		if(!ofs.IsOpened())
		{
			wxMessageBox(wxString(_("Couldn't open file")) + _T(" - ") + nc_file_str);
			return;
		}

		ofs.Write(theApp.m_output_canvas->m_textCtrl->GetValue());
		HeeksPyBackplot(nc_file_str);
	}
}

COutputCanvas::COutputCanvas(wxWindow* parent)
        : wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                           wxHSCROLL | wxVSCROLL | wxNO_FULL_REPAINT_ON_RESIZE)
{
	m_textCtrl = new COutputTextCtrl( this, 100, _T(""),	wxPoint(180,170), wxSize(200,70), wxTE_MULTILINE | wxTE_DONTWRAP | wxTE_RICH | wxTE_RICH2);

	// make a tool bar
	m_toolBar = new wxToolBar(this, -1, wxDefaultPosition, wxDefaultSize, wxTB_NODIVIDER | wxTB_FLAT);
	m_toolBar->SetToolBitmapSize(wxSize(32, 32));

	// add toolbar buttons
	heeksCAD->AddToolBarButton(m_toolBar, _T("Open"), wxBitmap(theApp.GetDllFolder() + _T("/bitmaps/open.png"), wxBITMAP_TYPE_PNG), _T("Open an NC file"), OnOpen);
	heeksCAD->AddToolBarButton(m_toolBar, _T("Save"), wxBitmap(theApp.GetDllFolder() + _T("/bitmaps/save.png"), wxBITMAP_TYPE_PNG), _T("Save the NC file"), OnSave);

	m_toolBar->Realize();

	Resize();
}


void COutputCanvas::OnSize(wxSizeEvent& event)
{
    Resize();

    event.Skip();
}

void COutputCanvas::Resize()
{
	wxSize size = GetClientSize();
	wxSize toolbar_size = m_toolBar->GetClientSize();
	m_textCtrl->SetSize(0, 0, size.x, size.y - 39 );
	m_toolBar->SetSize(0, size.y - 39 , size.x, 39 );
}

void COutputCanvas::Clear()
{
	m_textCtrl->Clear();
}

