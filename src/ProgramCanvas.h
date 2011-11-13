// ProgramCanvas.h
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#pragma once

#include <wx/event.h>
#include <wx/scrolwin.h>
#include <wx/window.h>
#include <wx/textctrl.h>

class CProgramCanvas: public wxScrolledWindow
{
private:
    void Resize();

public:
    wxTextCtrl *m_textCtrl;

    CProgramCanvas(wxWindow* parent);
	virtual ~CProgramCanvas(){}

	void Clear();

    void OnSize(wxSizeEvent& event);

    void AppendText(const wxString& text);
    void AppendText(double value);
    void AppendText(int value);
    void AppendText(unsigned int value);
 
    DECLARE_NO_COPY_CLASS(CProgramCanvas)
    DECLARE_EVENT_TABLE()
};

