// PocketDlg.h
// Copyright (c) 2010, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

class CPocket;
class PictureWindow;
class CLengthCtrl;
class CDoubleCtrl;
class CObjectIdsCtrl;

#include "interface/HDialogs.h"

class PocketDlg : public HDialog
{
	CObjectIdsCtrl *m_idsSketches;
	CLengthCtrl *m_lgthStepOver;
	CLengthCtrl *m_lgthMaterialAllowance;
	wxComboBox *m_cmbStartingPlace;
	wxComboBox *m_cmbCutMode;
	wxComboBox *m_cmbEntryMove;
	wxComboBox *m_cmbTool;
	wxCheckBox *m_chkKeepToolDown;
	wxCheckBox *m_chkUseZigZag;
	CDoubleCtrl *m_dblZigAngle;
	wxCheckBox *m_chkZigUnidirectional;
	CLengthCtrl *m_lgthClearanceHeight;
	CLengthCtrl *m_lgthRapidDownToHeight;
	CLengthCtrl *m_lgthStartDepth;
	CLengthCtrl *m_lgthFinalDepth;
	CLengthCtrl *m_lgthStepDown;
	CLengthCtrl *m_lgthHFeed;
	CLengthCtrl *m_lgthVFeed;
	CDoubleCtrl *m_dblSpindleSpeed;
	wxTextCtrl *m_txtComment;
	wxCheckBox *m_chkActive;
	wxTextCtrl *m_txtTitle;
	PictureWindow *m_picture;
	CLengthCtrl *m_lgthZFinishDepth;
	CLengthCtrl *m_lgthZThruDepth;

	void EnableZigZagControls();

public:
    PocketDlg(wxWindow *parent, CPocket* object);
	void GetData(CPocket* object);
	void SetFromData(CPocket* object);
	void SetPicture();
	void SetPicture(const wxString& name);

	void OnChildFocus(wxChildFocusEvent& event);
	void OnComboStartingPlace( wxCommandEvent& event );
	void OnComboCutMode( wxCommandEvent& event );
	void OnCheckKeepToolDown(wxCommandEvent& event);
	void OnCheckUseZigZag(wxCommandEvent& event);
	void OnComboTool(wxCommandEvent& event);
	void OnCheckZigUnidirectional(wxCommandEvent& event);

    DECLARE_EVENT_TABLE()
};
