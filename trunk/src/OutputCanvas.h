// OutputCanvas.h

#pragma once

class COutputTextCtrl: public wxTextCtrl
{
public:
    COutputTextCtrl(wxWindow *parent, wxWindowID id, const wxString &value, const wxPoint &pos, const wxSize &size, int style = 0): wxTextCtrl(parent, id, value, pos, size, style){}
};

class COutputCanvas: public wxScrolledWindow
{
private:
    void Resize();

public:
    COutputTextCtrl *m_textCtrl;

    COutputCanvas(wxWindow* parent);
	virtual ~COutputCanvas(){}

    void OnSize(wxSizeEvent& event);
 
    DECLARE_NO_COPY_CLASS(COutputCanvas)
    DECLARE_EVENT_TABLE()
};

