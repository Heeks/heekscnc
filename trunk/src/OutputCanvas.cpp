// OutputCanvas.cpp

#include "stdafx.h"
#include "OutputCanvas.h"
#include "Program.h"
#include "NCCode.h"

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


COutputCanvas::COutputCanvas(wxWindow* parent)
        : wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                           wxHSCROLL | wxVSCROLL | wxNO_FULL_REPAINT_ON_RESIZE)
{
	m_textCtrl = new COutputTextCtrl( this, 100, _T(""),	wxPoint(180,170), wxSize(200,70), wxTE_MULTILINE | wxTE_DONTWRAP | wxTE_RICH | wxTE_RICH2);

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
	m_textCtrl->SetSize(0, 0, size.x, size.y);
}

void COutputCanvas::Clear()
{
	m_textCtrl->Clear();
}

