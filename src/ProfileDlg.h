// ProfileDlg.h
// Copyright (c) 2014, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

class CProfile;
class PictureWindow;
class CLengthCtrl;
class CDoubleCtrl;
class CObjectIdsCtrl;

#include "DepthOpDlg.h"

class ProfileDlg : public DepthOpDlg
{
	HTypeObjectDropDown *m_cmbSketch;
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
	wxButton *m_btnSketchPick;

	SketchOrderType m_order;

	void EnableControls();

public:
    ProfileDlg(wxWindow *parent, CProfile* object, const wxString& title = wxString(_T("Profile Operation")), bool top_level = true);

	static bool Do(CProfile* object);

	// HeeksObjDlg virtual functions
	void GetDataRaw(HeeksObj* object);
	void SetFromDataRaw(HeeksObj* object);
	void SetPictureByWindow(wxWindow* w);
	void SetPicture(const wxString& name);

	void SetSketchOrderAndCombo();
	void OnCheckFinishingPass( wxCommandEvent& event );
	void OnSketchPick( wxCommandEvent& event );

    DECLARE_EVENT_TABLE()
};
