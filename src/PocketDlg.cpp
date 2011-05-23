// PocketDlg.cpp
// Copyright (c) 2010, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include "PocketDlg.h"
#include "interface/PictureFrame.h"
#include "interface/NiceTextCtrl.h"
#include "Pocket.h"

enum
{
    ID_SKETCHES = 100,
	ID_STEP_OVER,
	ID_MATERIAL_ALLOWANCE,
	ID_STARTING_PLACE,
	ID_KEEP_TOOL_DOWN,
	ID_USE_ZIG_ZAG,
	ID_ZIG_ANGLE,
	ID_ZIG_UNIDIRECTIONAL,
	ID_ABS_MODE,
	ID_CLEARANCE_HEIGHT,
	ID_RAPID_SAFETY_SPACE,
	ID_START_DEPTH,
	ID_FINAL_DEPTH,
	ID_STEP_DOWN,
	ID_HFEED,
	ID_VFEED,
	ID_SPINDLE_SPEED,
	ID_COMMENT,
	ID_ACTIVE,
	ID_TITLE,
	ID_TOOL,
	ID_DESCENT_STRATGEY,
};

BEGIN_EVENT_TABLE(PocketDlg, HDialog)
    EVT_CHILD_FOCUS(PocketDlg::OnChildFocus)
    EVT_COMBOBOX(ID_STARTING_PLACE,PocketDlg::OnComboStartingPlace)
    EVT_CHECKBOX(ID_KEEP_TOOL_DOWN, PocketDlg::OnCheckKeepToolDown)
    EVT_CHECKBOX(ID_USE_ZIG_ZAG, PocketDlg::OnCheckUseZigZag)
    EVT_CHECKBOX(ID_ZIG_UNIDIRECTIONAL, PocketDlg::OnCheckZigUnidirectional)
    EVT_COMBOBOX(ID_TOOL,PocketDlg::OnComboTool)
END_EVENT_TABLE()

wxBitmap* PocketDlg::m_general_bitmap = NULL;
wxBitmap* PocketDlg::m_step_over_bitmap = NULL;
wxBitmap* PocketDlg::m_material_allowance_bitmap = NULL;
wxBitmap* PocketDlg::m_starting_center_bitmap = NULL;
wxBitmap* PocketDlg::m_starting_boundary_bitmap = NULL;
wxBitmap* PocketDlg::m_tool_down_bitmap = NULL;
wxBitmap* PocketDlg::m_not_tool_down_bitmap = NULL;
wxBitmap* PocketDlg::m_use_zig_zag_bitmap = NULL;
wxBitmap* PocketDlg::m_zig_angle_bitmap = NULL;
wxBitmap* PocketDlg::m_zig_unidirectional_bitmap = NULL;
wxBitmap* PocketDlg::m_clearance_height_bitmap = NULL;
wxBitmap* PocketDlg::m_rapid_down_to_bitmap = NULL;
wxBitmap* PocketDlg::m_start_depth_bitmap = NULL;
wxBitmap* PocketDlg::m_final_depth_bitmap = NULL;
wxBitmap* PocketDlg::m_step_down_bitmap = NULL;
wxBitmap* PocketDlg::m_entry_move_bitmap = NULL;

static std::vector< std::pair< int, wxString > > tools_for_combo;

PocketDlg::PocketDlg(wxWindow *parent, CPocket* object)
             : HDialog(parent, wxID_ANY, wxString(_T("Pocket Operation")))
{
	m_ignore_event_functions = true;
    wxBoxSizer *sizerMain = new wxBoxSizer(wxHORIZONTAL);

	// add left sizer
    wxBoxSizer *sizerLeft = new wxBoxSizer(wxVERTICAL);
    sizerMain->Add( sizerLeft, 0, wxALL, control_border );

	// add right sizer
    wxBoxSizer *sizerRight = new wxBoxSizer(wxVERTICAL);
    sizerMain->Add( sizerRight, 0, wxALL, control_border );

	// add picture to right side
	m_picture = new PictureWindow(this, wxSize(300, 200));
	wxBoxSizer *pictureSizer = new wxBoxSizer(wxVERTICAL);
	pictureSizer->Add(m_picture, 1, wxGROW);
    sizerRight->Add( pictureSizer, 0, wxALL, control_border );

	// add some of the controls to the right side
	AddLabelAndControl(sizerRight, _("horizontal feedrate"), m_lgthHFeed = new CLengthCtrl(this, ID_HFEED));
	AddLabelAndControl(sizerRight, _("vertical feedrate"), m_lgthVFeed = new CLengthCtrl(this, ID_VFEED));
	AddLabelAndControl(sizerRight, _("spindle speed"), m_dblSpindleSpeed = new CDoubleCtrl(this, ID_SPINDLE_SPEED));

	AddLabelAndControl(sizerRight, _("comment"), m_txtComment = new wxTextCtrl(this, ID_COMMENT));
	sizerRight->Add( m_chkActive = new wxCheckBox( this, ID_ACTIVE, _("active") ), 0, wxALL, control_border );
	AddLabelAndControl(sizerRight, _("title"), m_txtTitle = new wxTextCtrl(this, ID_TITLE));

	// add OK and Cancel to right side
    wxBoxSizer *sizerOKCancel = MakeOkAndCancel(wxHORIZONTAL);
	sizerRight->Add( sizerOKCancel, 0, wxALL | wxALIGN_RIGHT | wxALIGN_BOTTOM, control_border );

	// add all the controls to the left side
	AddLabelAndControl(sizerLeft, _("sketches"), m_idsSketches = new CObjectIdsCtrl(this, ID_SKETCHES));
	AddLabelAndControl(sizerLeft, _("step over"), m_lgthStepOver = new CLengthCtrl(this, ID_STEP_OVER));
	AddLabelAndControl(sizerLeft, _("material allowance"), m_lgthMaterialAllowance = new CLengthCtrl(this, ID_MATERIAL_ALLOWANCE));

	wxString starting_place_choices[] = {_("boundary"), _("center")};
	AddLabelAndControl(sizerLeft, _("starting place"), m_cmbStartingPlace = new wxComboBox(this, ID_STARTING_PLACE, _T(""), wxDefaultPosition, wxDefaultSize, 2, starting_place_choices));

	wxString entry_move_choices[] = {_("Plunge"), _("Ramp"), _("Helical")};
	AddLabelAndControl(sizerLeft, _("entry move"), m_cmbEntryMove = new wxComboBox(this, ID_DESCENT_STRATGEY, _T(""), wxDefaultPosition, wxDefaultSize, 3, entry_move_choices));

	tools_for_combo = CTool::FindAllTools();

	wxArrayString tools;
	for(unsigned int i = 0; i<tools_for_combo.size(); i++)tools.Add(tools_for_combo[i].second);
	AddLabelAndControl(sizerLeft, _("Tool"), m_cmbTool = new wxComboBox(this, ID_TOOL, _T(""), wxDefaultPosition, wxDefaultSize, tools));

	sizerLeft->Add( m_chkUseZigZag = new wxCheckBox( this, ID_USE_ZIG_ZAG, _("use zig zag") ), 0, wxALL, control_border );
	sizerLeft->Add( m_chkKeepToolDown = new wxCheckBox( this, ID_KEEP_TOOL_DOWN, _("keep tool down") ), 0, wxALL, control_border );
	AddLabelAndControl(sizerLeft, _("zig zag angle"), m_dblZigAngle = new CDoubleCtrl(this, ID_ZIG_ANGLE));
	sizerLeft->Add( m_chkZigUnidirectional = new wxCheckBox( this, ID_ZIG_UNIDIRECTIONAL, _("zig unidirectional") ), 0, wxALL, control_border );

	wxString abs_mode_choices[] = {_("absolute"), _("incremental")};
	AddLabelAndControl(sizerLeft, _("absolute mode"), m_cmbAbsMode = new wxComboBox(this, ID_ABS_MODE, _T(""), wxDefaultPosition, wxDefaultSize, 2, abs_mode_choices));

	AddLabelAndControl(sizerLeft, _("clearance height"), m_lgthClearanceHeight = new CLengthCtrl(this, ID_CLEARANCE_HEIGHT));
	AddLabelAndControl(sizerLeft, _("rapid safety space"), m_lgthRapidDownToHeight = new CLengthCtrl(this, ID_RAPID_SAFETY_SPACE));
	AddLabelAndControl(sizerLeft, _("start depth"), m_lgthStartDepth = new CLengthCtrl(this, ID_START_DEPTH));
	AddLabelAndControl(sizerLeft, _("final depth"), m_lgthFinalDepth = new CLengthCtrl(this, ID_FINAL_DEPTH));
	AddLabelAndControl(sizerLeft, _("step down"), m_lgthStepDown = new CLengthCtrl(this, ID_STEP_DOWN));

	SetFromData(object);

    SetSizer( sizerMain );
    sizerMain->SetSizeHints(this);
	sizerMain->Fit(this);

    m_idsSketches->SetFocus();

	m_ignore_event_functions = false;

	SetPicture();
}

void PocketDlg::GetData(CPocket* object)
{
	if(m_ignore_event_functions)return;
	m_ignore_event_functions = true;
	object->m_sketches.clear();
#ifdef OP_SKETCHES_AS_CHILDREN
	m_idsSketches->GetAddChildren(object, SketchType);
#else
	m_idsSketches->GetIDList(object->m_sketches);
#endif
	object->m_pocket_params.m_step_over = m_lgthStepOver->GetValue();
	object->m_pocket_params.m_material_allowance = m_lgthMaterialAllowance->GetValue();
	object->m_pocket_params.m_starting_place = m_cmbStartingPlace->GetValue() ? 1:0;
	if ( m_cmbEntryMove->GetValue().CmpNoCase(_("Plunge")) == 0) {
		object->m_pocket_params.m_entry_move = CPocketParams::ePlunge;
	}
	else if ( m_cmbEntryMove->GetValue().CmpNoCase(_("Ramp")) == 0) {
		object->m_pocket_params.m_entry_move = CPocketParams::eRamp;
	}
	else if ( m_cmbEntryMove->GetValue().CmpNoCase(_("Helical")) == 0) {
		object->m_pocket_params.m_entry_move = CPocketParams::eHelical;
	}
	object->m_pocket_params.m_keep_tool_down_if_poss = m_chkKeepToolDown->GetValue();
	object->m_pocket_params.m_use_zig_zag = m_chkUseZigZag->GetValue();
	if(object->m_pocket_params.m_use_zig_zag)object->m_pocket_params.m_zig_angle = m_dblZigAngle->GetValue();
	object->m_depth_op_params.m_abs_mode = (m_cmbAbsMode->GetValue().CmpNoCase(_("incremental")) == 0) ? CDepthOpParams::eIncremental : CDepthOpParams::eAbsolute;
	object->m_depth_op_params.m_clearance_height = m_lgthClearanceHeight->GetValue();
	object->m_depth_op_params.m_rapid_safety_space = m_lgthRapidDownToHeight->GetValue();
	object->m_depth_op_params.m_start_depth = m_lgthStartDepth->GetValue();
	object->m_depth_op_params.m_final_depth = m_lgthFinalDepth->GetValue();
	object->m_depth_op_params.m_step_down = m_lgthStepDown->GetValue();
	object->m_speed_op_params.m_horizontal_feed_rate = m_lgthHFeed->GetValue();
	object->m_speed_op_params.m_vertical_feed_rate = m_lgthVFeed->GetValue();
	object->m_speed_op_params.m_spindle_speed = m_dblSpindleSpeed->GetValue();
	object->m_comment = m_txtComment->GetValue();
	object->m_active = m_chkActive->GetValue();
	
	// get the tool number
	object->m_tool_number = 0;
	if(m_cmbTool->GetSelection() >= 0)object->m_tool_number = tools_for_combo[m_cmbTool->GetSelection()].first;

	object->m_title = m_txtTitle->GetValue();
	m_ignore_event_functions = false;
}

void PocketDlg::SetFromData(CPocket* object)
{
	m_ignore_event_functions = true;
#ifdef OP_SKETCHES_AS_CHILDREN
	m_idsSketches->SetFromChildren(object, SketchType);
#else
	m_idsSketches->SetFromIDList(object->m_sketches);
#endif
	m_lgthStepOver->SetValue(object->m_pocket_params.m_step_over);
	m_lgthMaterialAllowance->SetValue(object->m_pocket_params.m_material_allowance);
	m_cmbStartingPlace->SetValue((object->m_pocket_params.m_starting_place == 0) ? _("boundary") : _("center"));
	switch (object->m_pocket_params.m_entry_move) {
	case CPocketParams::ePlunge:
		m_cmbEntryMove->SetValue(_("Plunge"));
		break;
	case CPocketParams::eRamp:
		m_cmbEntryMove->SetValue(_("Ramp"));
		break;
	case CPocketParams::eHelical:
		m_cmbEntryMove->SetValue(_("Helical"));
		break;
	default: ;
	}

	// set the tool combo to the correct tool
	for(unsigned int i = 0; i < tools_for_combo.size(); i++)if(tools_for_combo[i].first == object->m_tool_number){m_cmbTool->SetSelection(i); break;}

	m_chkKeepToolDown->SetValue(object->m_pocket_params.m_keep_tool_down_if_poss);
	m_chkUseZigZag->SetValue(object->m_pocket_params.m_use_zig_zag);
	if(object->m_pocket_params.m_use_zig_zag)m_dblZigAngle->SetValue(object->m_pocket_params.m_zig_angle);
	m_cmbAbsMode->SetValue((object->m_depth_op_params.m_abs_mode == CDepthOpParams::eAbsolute) ? _("absolute") : _("incremental"));
	m_lgthClearanceHeight->SetValue(object->m_depth_op_params.m_clearance_height);
	m_lgthRapidDownToHeight->SetValue(object->m_depth_op_params.m_rapid_safety_space);
	m_lgthStartDepth->SetValue(object->m_depth_op_params.m_start_depth);
	m_lgthFinalDepth->SetValue(object->m_depth_op_params.m_final_depth);
	m_lgthStepDown->SetValue(object->m_depth_op_params.m_step_down);
	m_lgthHFeed->SetValue(object->m_speed_op_params.m_horizontal_feed_rate);
	m_lgthVFeed->SetValue(object->m_speed_op_params.m_vertical_feed_rate);
	m_dblSpindleSpeed->SetValue(object->m_speed_op_params.m_spindle_speed);
	m_txtComment->SetValue(object->m_comment);
	m_chkActive->SetValue(object->m_active);
	m_txtTitle->SetValue(object->m_title);
	m_ignore_event_functions = false;
}

void PocketDlg::SetPicture(wxBitmap** bitmap, const wxString& name)
{
	m_picture->SetPicture(bitmap, theApp.GetResFolder() + _T("/bitmaps/pocket/") + name + _T(".png"), wxBITMAP_TYPE_PNG);
}

void PocketDlg::SetPicture()
{
	wxWindow* w = FindFocus();

	if(w == m_lgthStepOver)SetPicture(&m_step_over_bitmap, _T("step over"));
	else if(w == m_lgthMaterialAllowance)SetPicture(&m_material_allowance_bitmap, _T("material allowance"));
	else if(w == m_cmbStartingPlace)
	{
		if(m_cmbStartingPlace->GetValue() == _("boundary"))SetPicture(&m_starting_boundary_bitmap, _T("starting boundary"));
		else SetPicture(&m_starting_center_bitmap, _T("starting center"));
	}
	else if(w == m_chkKeepToolDown)
	{
		if(m_chkKeepToolDown->IsChecked())SetPicture(&m_tool_down_bitmap, _T("tool down"));
		else SetPicture(&m_not_tool_down_bitmap, _T("not tool down"));
	}
	else if(w == m_chkUseZigZag)
	{
		if(m_chkUseZigZag->IsChecked())SetPicture(&m_use_zig_zag_bitmap, _T("use zig zag"));
		else SetPicture(&m_general_bitmap, _T("general"));
	}
	else if(w == m_dblZigAngle)SetPicture(&m_zig_angle_bitmap, _T("zig angle"));
	else if(w == m_chkZigUnidirectional)
	{
		if(m_chkZigUnidirectional->IsChecked())SetPicture(&m_zig_unidirectional_bitmap, _T("zig unidirectional"));
		else SetPicture(&m_general_bitmap, _T("general"));
	}
	else if(w == m_lgthClearanceHeight)SetPicture(&m_clearance_height_bitmap, _T("clearance height"));
	else if(w == m_lgthRapidDownToHeight)SetPicture(&m_rapid_down_to_bitmap, _T("rapid down height"));
	else if(w == m_lgthStartDepth)SetPicture(&m_start_depth_bitmap, _T("start depth"));
	else if(w == m_lgthFinalDepth)SetPicture(&m_final_depth_bitmap, _T("final depth"));
	else if(w == m_lgthStepDown)SetPicture(&m_step_down_bitmap, _T("step down"));
	else SetPicture(&m_general_bitmap, _T("general"));
}

void PocketDlg::OnChildFocus(wxChildFocusEvent& event)
{
	if(m_ignore_event_functions)return;
	if(event.GetWindow())
	{
		SetPicture();
	}
}

void PocketDlg::OnComboStartingPlace( wxCommandEvent& event )
{
	if(m_ignore_event_functions)return;
	SetPicture();
}

void PocketDlg::OnComboTool( wxCommandEvent& event )
{
//	if(m_ignore_event_functions)return;
	//SetPicture();
}

void PocketDlg::OnCheckKeepToolDown(wxCommandEvent& event)
{
	if(m_ignore_event_functions)return;
	SetPicture();
}

void PocketDlg::OnCheckUseZigZag(wxCommandEvent& event)
{
	if(m_ignore_event_functions)return;
	SetPicture();
}

void PocketDlg::OnCheckZigUnidirectional(wxCommandEvent& event)
{
	if(m_ignore_event_functions)return;
	SetPicture();
}
