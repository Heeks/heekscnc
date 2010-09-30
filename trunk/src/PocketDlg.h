// PocketDlg.h
// Copyright (c) 2010, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

class CPocket;
class PictureWindow;
class CLengthCtrl;
class CDoubleCtrl;
class CObjectIdsCtrl;

class PocketDlg : public wxDialog
{
	wxBitmap m_general_bitmap;
	wxBitmap m_step_over_bitmap;
	wxBitmap m_material_allowance_bitmap;
	wxBitmap m_starting_center_bitmap;
	wxBitmap m_starting_boundary_bitmap;
	wxBitmap m_tool_down_bitmap;
	wxBitmap m_not_tool_down_bitmap;
	wxBitmap m_use_zig_zag_bitmap;
	wxBitmap m_zig_angle_bitmap;
	wxBitmap m_not_use_zig_zag_bitmap;
	wxBitmap m_clearnce_height_bitmap;
	wxBitmap m_rapid_down_to_bitmap;
	wxBitmap m_start_depth_bitmap;
	wxBitmap m_final_depth_bitmap;
	wxBitmap m_step_down_bitmap;

	CObjectIdsCtrl *m_idsSketches;
	CLengthCtrl *m_lgthStepOver;
	CLengthCtrl *m_lgthMaterialAllowance;
	wxComboBox *m_cmbStartingPlace;
	wxComboBox *m_cmbTool;
	wxCheckBox *m_chkKeepToolDown;
	wxCheckBox *m_chkUseZigZag;
	CDoubleCtrl *m_dblZigAngle;
	wxComboBox *m_cmbAbsMode;
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

	bool m_ignore_event_functions;

	void AddLabelAndControl(wxBoxSizer* sizer, const wxString& label, wxWindow* control);

public:
    PocketDlg(wxWindow *parent, CPocket* object);
	void GetData(CPocket* object);
	void SetFromData(CPocket* object);
	void SetPicture();

	void OnChildFocus(wxChildFocusEvent& event);
	void OnComboStartingPlace( wxCommandEvent& event );
	void OnCheckKeepToolDown(wxCommandEvent& event);
	void OnCheckUseZigZag(wxCommandEvent& event);
	void OnComboTool(wxCommandEvent& event);


    DECLARE_EVENT_TABLE()
};
