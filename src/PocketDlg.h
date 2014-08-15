// PocketDlg.h
// Copyright (c) 2010, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

class CPocket;
class PictureWindow;
class CLengthCtrl;
class CDoubleCtrl;
class CObjectIdsCtrl;

#include "SketchOpDlg.h"

class PocketDlg : public SketchOpDlg
{
	enum
	{
		ID_STARTING_PLACE = ID_SKETCH_ENUM_MAX,
		ID_CUT_MODE,
		ID_KEEP_TOOL_DOWN,
		ID_USE_ZIG_ZAG,
		ID_ZIG_UNIDIRECTIONAL,
	};

	CLengthCtrl *m_lgthStepOver;
	CLengthCtrl *m_lgthMaterialAllowance;
	wxComboBox *m_cmbStartingPlace;
	wxComboBox *m_cmbCutMode;
	wxComboBox *m_cmbEntryMove;
	wxCheckBox *m_chkKeepToolDown;
	wxCheckBox *m_chkUseZigZag;
	CDoubleCtrl *m_dblZigAngle;
	wxCheckBox *m_chkZigUnidirectional;

	void EnableZigZagControls();

public:
	PocketDlg(wxWindow *parent, CPocket* object, const wxString& title = wxString(_("Pocket Operation")), bool top_level = true);

	static bool Do(CPocket* object);

	// HeeksObjDlg virtual functions
	void GetDataRaw(HeeksObj* object);
	void SetFromDataRaw(HeeksObj* object);
	void SetPictureByWindow(wxWindow* w);
	void SetPicture(const wxString& name);

	void OnCheckUseZigZag(wxCommandEvent& event);
	void OnHelp( wxCommandEvent& event );

	DECLARE_EVENT_TABLE()
};

