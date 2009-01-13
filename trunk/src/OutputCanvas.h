// OutputCanvas.h

#pragma once

class COutputCanvas: public wxScrolledWindow
{
private:
    void Resize();

public:
    wxTextCtrl *m_textCtrl;

    COutputCanvas(wxWindow* parent);
	virtual ~COutputCanvas(){}

	void Clear();

    void OnSize(wxSizeEvent& event);
 
    DECLARE_NO_COPY_CLASS(COutputCanvas)
    DECLARE_EVENT_TABLE()
};

