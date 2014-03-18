// ProgramDlg.h
// Copyright (c) 2014, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "interface/HeeksObjDlg.h"

class ProgramDlg : public HeeksObjDlg
{
	wxComboBox *m_cmbMachines;
	wxCheckBox *m_chkOutputNameFollowsDataName;
	wxTextCtrl *m_txtOutputFile;
	wxComboBox *m_cmbUnits;

public:
    ProgramDlg(wxWindow *parent, HeeksObj* object, const wxString& title = wxString(_T("Program")), bool top_level = true);

	// HeeksObjDlg virtual functions
	void GetDataRaw(HeeksObj* object);
	void SetFromDataRaw(HeeksObj* object);
	void SetPictureByWindow(wxWindow* w);
	void SetPicture(const wxString& name);

	void EnableControls();
	void OnOutputNameFollowsCheck( wxCommandEvent& event );
	
    DECLARE_EVENT_TABLE()
};
