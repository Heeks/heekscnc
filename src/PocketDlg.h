// PocketDlg.h
// Copyright (c) 2010, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

class CPocket;
class PictureWindow;
class CLengthCtrl;
class CDoubleCtrl;
class CObjectIdsCtrl;

#include "DepthOpDlg.h"

class PocketDlg : public DepthOpDlg
{
	HTypeObjectDropDown *m_cmbSketch;
	CLengthCtrl *m_lgthStepOver;
	CLengthCtrl *m_lgthMaterialAllowance;
	wxComboBox *m_cmbStartingPlace;
	wxComboBox *m_cmbCutMode;
	wxComboBox *m_cmbEntryMove;
	wxCheckBox *m_chkKeepToolDown;
	wxCheckBox *m_chkUseZigZag;
	CDoubleCtrl *m_dblZigAngle;
	wxCheckBox *m_chkZigUnidirectional;
	wxButton *m_btnSketchPick;

	void EnableZigZagControls();

public:
    PocketDlg(wxWindow *parent, CPocket* object, const wxString& title = wxString(_T("Pocket Operation")), bool top_level = true);

	static bool Do(CPocket* object);

	// HeeksObjDlg virtual functions
	void GetDataRaw(HeeksObj* object);
	void SetFromDataRaw(HeeksObj* object);
	void SetPictureByWindow(wxWindow* w);
	void SetPicture(const wxString& name);

	void OnCheckUseZigZag(wxCommandEvent& event);
	void OnSketchPick( wxCommandEvent& event );

    DECLARE_EVENT_TABLE()
};
