// ProgramCanvas.h

#pragma once

class CProgramTextCtrl: public wxTextCtrl
{
public:
    CProgramTextCtrl(wxWindow *parent, wxWindowID id, const wxString &value, const wxPoint &pos, const wxSize &size, int style = 0): wxTextCtrl(parent, id, value, pos, size, style){}

    // callbacks
    void OnDropFiles(wxDropFilesEvent& event);
    void OnChar(wxKeyEvent& event); // Process 'enter' if required

    void OnCut(wxCommandEvent& event);
    void OnCopy(wxCommandEvent& event);
    void OnPaste(wxCommandEvent& event);
    void OnUndo(wxCommandEvent& event);
    void OnRedo(wxCommandEvent& event);
    void OnDelete(wxCommandEvent& event);
    void OnSelectAll(wxCommandEvent& event);

	void WriteText(const wxString& text);

    DECLARE_NO_COPY_CLASS(CProgramTextCtrl)
    DECLARE_EVENT_TABLE()
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

	void Clear();

    void OnSize(wxSizeEvent& event);
 
    DECLARE_NO_COPY_CLASS(CProgramCanvas)
    DECLARE_EVENT_TABLE()
};

