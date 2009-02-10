// ProgramCanvas.cpp

#include "stdafx.h"
#include <wx/config.h>
#include <wx/confbase.h>
#include <wx/fileconf.h>
#include "ProgramCanvas.h"
#include "Program.h"
#include "OutputCanvas.h"
#include "../../interface/Tool.h"
#include "../../interface/InputMode.h"
#include "../../interface/LeftAndRight.h"
#include "../../interface/PropertyInt.h"
#include "../../interface/PropertyDouble.h"
#include "../../interface/PropertyChoice.h"

BEGIN_EVENT_TABLE(CProgramCanvas, wxScrolledWindow)
    EVT_SIZE(CProgramCanvas::OnSize)
END_EVENT_TABLE()

//	if(heeksCAD->PickPosition(_T("Pick finish position"), pos)){

CProgramCanvas::CProgramCanvas(wxWindow* parent)
        : wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                           wxHSCROLL | wxVSCROLL | wxNO_FULL_REPAINT_ON_RESIZE)
{
	m_textCtrl = new wxTextCtrl( this, 100, _T(""),	wxPoint(180,170), wxSize(200,70), wxTE_MULTILINE | wxTE_DONTWRAP);

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
	m_textCtrl->SetSize(0, 0, size.x, size.y);
}

void CProgramCanvas::Clear()
{
	m_textCtrl->Clear();
}
