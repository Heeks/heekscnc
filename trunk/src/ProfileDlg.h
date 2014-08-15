// ProfileDlg.h
// Copyright (c) 2014, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

class CProfile;
class PictureWindow;
class CLengthCtrl;
class CDoubleCtrl;
class CObjectIdsCtrl;

#include "SketchOpDlg.h"

class ProfileDlg : public SketchOpDlg
{
	enum
	{
		ID_TOOL_ON_SIDE = ID_SKETCH_ENUM_MAX,
		ID_CUT_MODE,
		ID_DO_FINISHING_PASS,
		ID_ONLY_FINISHING_PASS,
		ID_FINISH_CUT_MODE,
	};

	wxComboBox *m_cmbToolOnSide;
	wxComboBox *m_cmbCutMode;
	CLengthCtrl *m_lgthRollRadius;
	CLengthCtrl *m_lgthOffsetExtra;
	wxCheckBox *m_chkDoFinishingPass;
	wxCheckBox *m_chkOnlyFinishingPass;
	CLengthCtrl *m_lgthFinishingFeedrate;
	wxComboBox *m_cmbFinishingCutMode;
	CLengthCtrl *m_lgthFinishStepDown;
	wxStaticText* m_staticFinishingFeedrate;
	wxStaticText* m_staticFinishingCutMode;
	wxStaticText* m_staticFinishStepDown;

	SketchOrderType m_order;

	void EnableControls();

public:
	ProfileDlg(wxWindow *parent, CProfile* object, const wxString& title = wxString(_("Profile Operation")), bool top_level = true);

	static bool Do(CProfile* object);

	// HeeksObjDlg virtual functions
	void GetDataRaw(HeeksObj* object);
	void SetFromDataRaw(HeeksObj* object);
	void SetPictureByWindow(wxWindow* w);
	void SetPicture(const wxString& name);

	void SetSketchOrderAndCombo(int sketch);
	void OnCheckFinishingPass( wxCommandEvent& event );
	void OnHelp( wxCommandEvent& event );
	void OnSketchCombo( wxCommandEvent& event );

	DECLARE_EVENT_TABLE()
};

