// ProgramCanvas.h

#pragma once

class CProgramCanvas: public wxScrolledWindow
{
private:
    void Resize();

	wxToolBar *m_toolBar;

public:
    wxTextCtrl *m_textCtrl;

    CProgramCanvas(wxWindow* parent);
	virtual ~CProgramCanvas(){}

	void Clear();

    void OnSize(wxSizeEvent& event);
 
    DECLARE_NO_COPY_CLASS(CProgramCanvas)
    DECLARE_EVENT_TABLE()
};

