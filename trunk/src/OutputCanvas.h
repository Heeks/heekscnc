// OutputCanvas.h
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#pragma once

class COutputTextCtrl: public wxTextCtrl
{
public:
    COutputTextCtrl(wxWindow *parent, wxWindowID id, const wxString &value, const wxPoint &pos, const wxSize &size, int style = 0): wxTextCtrl(parent, id, value, pos, size, style){}

    void OnMouse( wxMouseEvent& event );
	void OnPaint(wxPaintEvent& event);

    DECLARE_NO_COPY_CLASS(COutputTextCtrl)
    DECLARE_EVENT_TABLE()
};

class COutputCanvas: public wxScrolledWindow
{
private:
    void Resize();

public:
    COutputTextCtrl *m_textCtrl;

    COutputCanvas(wxWindow* parent);
	virtual ~COutputCanvas(){}

	void Clear();

    void OnSize(wxSizeEvent& event);
	void OnLengthExceeded(wxCommandEvent& event);
 
    DECLARE_NO_COPY_CLASS(COutputCanvas)
	DECLARE_EVENT_TABLE()
};

