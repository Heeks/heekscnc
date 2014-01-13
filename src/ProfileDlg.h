// ProfileDlg.h
// Copyright (c) 2014, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

class CProfile;
class PictureWindow;
class CLengthCtrl;
class CDoubleCtrl;
class CObjectIdsCtrl;

#include "interface/HDialogs.h"

class ProfileDlg : public HDialog
{
	CProfile* m_object;

	CObjectIdsCtrl *m_idsSketches;
	wxComboBox *m_cmbToolOnSide;
	wxComboBox *m_cmbCutMode;
	CLengthCtrl *m_lgthRollRadius;
	CLengthCtrl *m_lgthOffsetExtra;
	wxCheckBox *m_chkDoFinishingPass;
	CLengthCtrl *m_lgthClearanceHeight;
	CLengthCtrl *m_lgthRapidDownToHeight;
	CLengthCtrl *m_lgthStartDepth;
	CLengthCtrl *m_lgthFinalDepth;
	CLengthCtrl *m_lgthStepDown;
	CLengthCtrl *m_lgthZFinishDepth;
	CLengthCtrl *m_lgthZThruDepth;
	CLengthCtrl *m_lgthHFeed;
	CLengthCtrl *m_lgthVFeed;
	CDoubleCtrl *m_dblSpindleSpeed;
	wxTextCtrl *m_txtComment;
	wxCheckBox *m_chkActive;
	wxTextCtrl *m_txtTitle;
	wxComboBox *m_cmbTool;
	wxCheckBox *m_chkOnlyFinishingPass;
	CLengthCtrl *m_lgthFinishingFeedrate;
	wxComboBox *m_cmbFinishingCutMode;
	CLengthCtrl *m_lgthFinishStepDown;
	wxStaticText* m_staticFinishingFeedrate;
	wxStaticText* m_staticFinishingCutMode;
	wxStaticText* m_staticFinishStepDown;
	PictureWindow *m_picture;

	SketchOrderType m_order;

	void EnableZigZagControls();

public:
    ProfileDlg(wxWindow *parent, CProfile* object);
	void GetData(CProfile* object);
	void SetFromData(CProfile* object);
	void SetPicture();
	void SetPicture(const wxString& name, bool pocket_picture = false);
	void SetSketchOrderAndCombo();
	void EnableControls();

	void OnChildFocus(wxChildFocusEvent& event);
	void OnComboOrCheck( wxCommandEvent& event );
	void OnCheckFinishingPass( wxCommandEvent& event );

    DECLARE_EVENT_TABLE()
};
