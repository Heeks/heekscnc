// ProgramCanvas.h

#pragma once

class CProgramTextCtrl: public wxTextCtrl
{
public:
    CProgramTextCtrl(wxWindow *parent, wxWindowID id, const wxString &value, const wxPoint &pos, const wxSize &size, int style = 0): wxTextCtrl(parent, id, value, pos, size, style){}
};

class CProgramCanvas: public wxScrolledWindow
{
private:
    void Resize();

	wxToolBar *m_toolBar;

public:
    CProgramTextCtrl *m_textCtrl;

    CProgramCanvas(wxWindow* parent);
	virtual ~CProgramCanvas(){}

    void OnSize(wxSizeEvent& event);
 
    DECLARE_NO_COPY_CLASS(CProgramCanvas)
    DECLARE_EVENT_TABLE()
};

