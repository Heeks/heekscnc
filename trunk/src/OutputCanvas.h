// OutputCanvas.h

#pragma once

class COutputTextCtrl: public wxTextCtrl
{
public:
    COutputTextCtrl(wxWindow *parent, wxWindowID id, const wxString &value, const wxPoint &pos, const wxSize &size, int style = 0): wxTextCtrl(parent, id, value, pos, size, style){}

    void OnMouse( wxMouseEvent& event );

    DECLARE_NO_COPY_CLASS(COutputTextCtrl)
    DECLARE_EVENT_TABLE()
};

class COutputCanvas: public wxScrolledWindow
{
private:
    void Resize();

	wxToolBar *m_toolBar;

public:
    COutputTextCtrl *m_textCtrl;

    COutputCanvas(wxWindow* parent);
	virtual ~COutputCanvas(){}

	void Clear();

    void OnSize(wxSizeEvent& event);
 
    DECLARE_NO_COPY_CLASS(COutputCanvas)
    DECLARE_EVENT_TABLE()
};

