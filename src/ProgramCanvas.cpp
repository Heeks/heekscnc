// ProgramCanvas.cpp

#include "stdafx.h"
#include "ProgramCanvas.h"
#include "Program.h"
#include "OutputCanvas.h"

BEGIN_EVENT_TABLE(CProgramCanvas, wxScrolledWindow)
    EVT_SIZE(CProgramCanvas::OnSize)
END_EVENT_TABLE()

static void OnRun(wxCommandEvent& event)
{
	theApp.m_program->DestroyGLLists();
	heeksCAD->Repaint();// clear the screen
	theApp.m_output_canvas->m_textCtrl->Clear(); // clear the output window
	theApp.m_program->m_create_display_list_next_render = true;
	std::list<HeeksObj*> list;
	list.push_back(theApp.m_program);
	heeksCAD->DrawObjectsOnFront(list);
}

CProgramCanvas::CProgramCanvas(wxWindow* parent)
        : wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                           wxHSCROLL | wxVSCROLL | wxNO_FULL_REPAINT_ON_RESIZE)
{
	m_textCtrl = new CProgramTextCtrl( this, 100, _T(""),
		wxPoint(180,170), wxSize(200,70), wxTE_MULTILINE);

	// make a tool bar
	m_toolBar = new wxToolBar(this, -1, wxDefaultPosition, wxDefaultSize, wxTB_NODIVIDER | wxTB_FLAT);
	m_toolBar->SetToolBitmapSize(wxSize(32, 32));

	// add toolbar buttons
	wxString exe_folder = heeksCAD->GetExeFolder();
	heeksCAD->AddToolBarButton(m_toolBar, _T("Run"), wxBitmap(exe_folder + "/../HeeksCNC/bitmaps/run.png", wxBITMAP_TYPE_PNG), _T("Run the program"), OnRun);
	m_toolBar->Realize();

	Resize();
}


void CProgramCanvas::OnSize(wxSizeEvent& event)
{
    Resize();

    event.Skip();
}

void CProgramCanvas::Resize()
{
	wxSize size = GetClientSize();
	m_textCtrl->SetSize(0, 0, size.x - 39, size.y );
	m_toolBar->SetSize(size.x - 39 , 0, 39, size.y );
}