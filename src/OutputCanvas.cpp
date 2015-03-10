// OutputCanvas.cpp
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "stdafx.h"
#include "OutputCanvas.h"
#include "Program.h"
#include "NCCode.h"

BEGIN_EVENT_TABLE(COutputTextCtrl, wxTextCtrl)
    EVT_MOUSE_EVENTS(COutputTextCtrl::OnMouse)
	EVT_PAINT(COutputTextCtrl::OnPaint)
END_EVENT_TABLE()

void COutputTextCtrl::OnMouse( wxMouseEvent& event )
{
	if(event.LeftUp())
	{
		wxTextPos pos = GetInsertionPoint();
		if(theApp.m_program && theApp.m_program->NCCode())
		{
			theApp.m_program->NCCode()->HighlightBlock(pos);
			heeksCAD->Repaint();
		}
	}

	event.Skip();
}

bool painting = false;
void COutputTextCtrl::OnPaint(wxPaintEvent& event)
{
	// this is here to Format the text ( which seems to be slow in Windows, if all done at once in CNCCode::SetTextCtrl )
	// OnPaint doesn't seem to get called from Linux, though
	wxPaintDC dc(this);

	if (!painting && theApp.m_program && theApp.m_program->NCCode())
	{
		painting = true;

		wxSize size = GetClientSize();
		int scrollpos = GetScrollPos(wxVERTICAL);
		wxLogDebug(_T("%i"), scrollpos);

		wxTextCoord col0, row0, col1, row1;
		HitTest(wxPoint(0,0),      &col0, &row0);
		HitTest(wxPoint(0,0)+size, &col1, &row1);

		int pos0 = XYToPosition(0, row0);
		int pos1 = XYToPosition(1, row1);

		theApp.m_program->NCCode()->FormatBlocks(this, pos0, pos1);

		SetScrollPos(wxVERTICAL, scrollpos);

		painting = false;
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

#ifdef WIN32
	// Ensure the wxTextCtrl object can accept the maximum 
	// text length allowable for this operating system.
	// (64kb on Win32) (32kb without this call on Win32)
	m_textCtrl->SetMaxLength( 0 );
#endif

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


BEGIN_EVENT_TABLE(CPrintCanvas, wxScrolledWindow)
    EVT_SIZE(CPrintCanvas::OnSize)
END_EVENT_TABLE()


CPrintCanvas::CPrintCanvas(wxWindow* parent)
        : wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                           wxHSCROLL | wxVSCROLL | wxNO_FULL_REPAINT_ON_RESIZE)
{
	m_textCtrl = new wxTextCtrl( this, 100, _T(""),	wxPoint(180,170), wxSize(200,70), wxTE_MULTILINE | wxTE_DONTWRAP | wxTE_RICH | wxTE_RICH2);

#ifdef WIN32
	// Ensure the wxTextCtrl object can accept the maximum 
	// text length allowable for this operating system.
	// (64kb on Win32) (32kb without this call on Win32)
	m_textCtrl->SetMaxLength( 0 );
#endif

	Resize();
}


void CPrintCanvas::OnSize(wxSizeEvent& event)
{
    Resize();

    event.Skip();
}



void CPrintCanvas::Resize()
{
	wxSize size = GetClientSize();
	m_textCtrl->SetSize(0, 0, size.x, size.y);
}

void CPrintCanvas::Clear()
{
	m_textCtrl->Clear();
}

