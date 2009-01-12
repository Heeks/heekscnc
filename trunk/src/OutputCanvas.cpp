// OutputCanvas.cpp

#include "stdafx.h"
#include "OutputCanvas.h"

BEGIN_EVENT_TABLE(COutputCanvas, wxScrolledWindow)
    EVT_SIZE(COutputCanvas::OnSize)
END_EVENT_TABLE()



COutputCanvas::COutputCanvas(wxWindow* parent)
        : wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                           wxHSCROLL | wxVSCROLL | wxNO_FULL_REPAINT_ON_RESIZE)
{
	m_textCtrl = new COutputTextCtrl( this, 100, _T(""),
		wxPoint(180,170), wxSize(200,70), wxTE_MULTILINE | wxTE_READONLY | wxTE_DONTWRAP | wxTE_RICH);
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
    m_textCtrl->SetSize(0, 0, size.x, size.y );

#if 0
    m_textCtrl->SetDefaultStyle(wxTextAttr(*wxRED));
    m_textCtrl->AppendText("Red text\n");
    m_textCtrl->SetDefaultStyle(wxTextAttr(wxNullColour, *wxLIGHT_GREY));
    m_textCtrl->AppendText("Red on grey text\n");
    m_textCtrl->SetDefaultStyle(wxTextAttr(*wxBLUE));
    m_textCtrl->AppendText("Blue on grey text\n");
#endif
}

void COutputCanvas::Clear()
{
	m_textCtrl->Clear();
}

