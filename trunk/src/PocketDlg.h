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
	static wxBitmap* m_general_bitmap;
	static wxBitmap* m_step_over_bitmap;
	static wxBitmap* m_material_allowance_bitmap;
	static wxBitmap* m_starting_center_bitmap;
	static wxBitmap* m_starting_boundary_bitmap;
	static wxBitmap* m_tool_down_bitmap;
	static wxBitmap* m_not_tool_down_bitmap;
	static wxBitmap* m_use_zig_zag_bitmap;
	static wxBitmap* m_zig_angle_bitmap;
	static wxBitmap* m_not_use_zig_zag_bitmap;
	static wxBitmap* m_clearance_height_bitmap;
	static wxBitmap* m_rapid_down_to_bitmap;
	static wxBitmap* m_start_depth_bitmap;
	static wxBitmap* m_final_depth_bitmap;
	static wxBitmap* m_step_down_bitmap;
	static wxBitmap* m_entry_move_bitmap;

	CObjectIdsCtrl *m_idsSketches;
	CLengthCtrl *m_lgthStepOver;
	CLengthCtrl *m_lgthMaterialAllowance;
	wxComboBox *m_cmbStartingPlace;
	wxComboBox *m_cmbEntryMove;
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

public:
    PocketDlg(wxWindow *parent, CPocket* object);
	void GetData(CPocket* object);
	void SetFromData(CPocket* object);
	void SetPicture();
	void SetPicture(wxBitmap** bitmap, const wxString& name);

	void OnChildFocus(wxChildFocusEvent& event);
	void OnComboStartingPlace( wxCommandEvent& event );
	void OnCheckKeepToolDown(wxCommandEvent& event);
	void OnCheckUseZigZag(wxCommandEvent& event);
	void OnComboTool(wxCommandEvent& event);


    DECLARE_EVENT_TABLE()
};
