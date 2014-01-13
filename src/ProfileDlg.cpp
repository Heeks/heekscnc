// ProfileDlg.cpp
// Copyright (c) 2010, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include "ProfileDlg.h"
#include "interface/PictureFrame.h"
#include "interface/NiceTextCtrl.h"
#include "Profile.h"
#include "CTool.h"

enum
{
	ID_TOOL_ON_SIDE = 100,
	ID_CUT_MODE,
	ID_DO_FINISHING_PASS,
	ID_TOOL,
	ID_ONLY_FINISHING_PASS,
	ID_FINISH_CUT_MODE,
};

BEGIN_EVENT_TABLE(ProfileDlg, HDialog)
    EVT_CHILD_FOCUS(ProfileDlg::OnChildFocus)
    EVT_COMBOBOX(ID_TOOL_ON_SIDE,ProfileDlg::OnComboOrCheck)
    EVT_COMBOBOX(ID_CUT_MODE,ProfileDlg::OnComboOrCheck)
    EVT_COMBOBOX(ID_TOOL,ProfileDlg::OnComboOrCheck)
    EVT_CHECKBOX(ID_DO_FINISHING_PASS, ProfileDlg::OnCheckFinishingPass)
    EVT_CHECKBOX(ID_ONLY_FINISHING_PASS, ProfileDlg::OnComboOrCheck)
    EVT_COMBOBOX(ID_FINISH_CUT_MODE,ProfileDlg::OnComboOrCheck)
END_EVENT_TABLE()

static std::vector< std::pair< int, wxString > > tools_for_combo;

ProfileDlg::ProfileDlg(wxWindow *parent, CProfile* object)
             : HDialog(parent, wxID_ANY, wxString(_T("Profile Operation")))
{
	m_object = object;

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
	AddLabelAndControl(sizerRight, _("horizontal feedrate"), m_lgthHFeed = new CLengthCtrl(this));
	AddLabelAndControl(sizerRight, _("vertical feedrate"), m_lgthVFeed = new CLengthCtrl(this));
	AddLabelAndControl(sizerRight, _("spindle speed"), m_dblSpindleSpeed = new CDoubleCtrl(this));

	AddLabelAndControl(sizerRight, _("comment"), m_txtComment = new wxTextCtrl(this, wxID_ANY));
	sizerRight->Add( m_chkActive = new wxCheckBox( this, wxID_ANY, _("active") ), 0, wxALL, control_border );
	AddLabelAndControl(sizerRight, _("title"), m_txtTitle = new wxTextCtrl(this, wxID_ANY));

	// add OK and Cancel to right side
    wxBoxSizer *sizerOKCancel = MakeOkAndCancel(wxHORIZONTAL);
	sizerRight->Add( sizerOKCancel, 0, wxALL | wxALIGN_RIGHT | wxALIGN_BOTTOM, control_border );

	// add all the controls to the left side
	AddLabelAndControl(sizerLeft, _("sketches"), m_idsSketches = new CObjectIdsCtrl(this));
	wxString tool_on_side_choices[] = {_("left"), _("right"), _("on")};
	AddLabelAndControl(sizerLeft, _("tool on side"), m_cmbToolOnSide = new wxComboBox(this, ID_TOOL_ON_SIDE, _T(""), wxDefaultPosition, wxDefaultSize, 3, tool_on_side_choices));

	SetSketchOrderAndCombo();

	wxString cut_mode_choices[] = {_("conventional"), _("climb")};
	AddLabelAndControl(sizerLeft, _("cut mode"), m_cmbCutMode = new wxComboBox(this, ID_CUT_MODE, _T(""), wxDefaultPosition, wxDefaultSize, 2, cut_mode_choices));
	AddLabelAndControl(sizerLeft, _("roll radius"), m_lgthRollRadius = new CLengthCtrl(this));
	AddLabelAndControl(sizerLeft, _("offset extra"), m_lgthOffsetExtra = new CLengthCtrl(this));

	tools_for_combo = CTool::FindAllTools();

	wxArrayString tools;
	for(unsigned int i = 0; i<tools_for_combo.size(); i++)tools.Add(tools_for_combo[i].second);
	AddLabelAndControl(sizerLeft, _("Tool"), m_cmbTool = new wxComboBox(this, ID_TOOL, _T(""), wxDefaultPosition, wxDefaultSize, tools));

	AddLabelAndControl(sizerLeft, _("clearance height"), m_lgthClearanceHeight = new CLengthCtrl(this));
	AddLabelAndControl(sizerLeft, _("rapid safety space"), m_lgthRapidDownToHeight = new CLengthCtrl(this));
	AddLabelAndControl(sizerLeft, _("start depth"), m_lgthStartDepth = new CLengthCtrl(this));
	AddLabelAndControl(sizerLeft, _("final depth"), m_lgthFinalDepth = new CLengthCtrl(this));
	AddLabelAndControl(sizerLeft, _("step down"), m_lgthStepDown = new CLengthCtrl(this));
	AddLabelAndControl(sizerLeft, _("z finish depth"), m_lgthZFinishDepth = new CLengthCtrl(this));
	AddLabelAndControl(sizerLeft, _("z through depth"), m_lgthZThruDepth = new CLengthCtrl(this));

	sizerLeft->Add( m_chkDoFinishingPass = new wxCheckBox( this, ID_DO_FINISHING_PASS, _("do finishing pass") ), 0, wxALL, control_border );
	sizerLeft->Add( m_chkOnlyFinishingPass = new wxCheckBox( this, ID_ONLY_FINISHING_PASS, _("only finishing pass") ), 0, wxALL, control_border );
	m_staticFinishingFeedrate = AddLabelAndControl(sizerLeft, _("finishing feed rate"), m_lgthFinishingFeedrate = new CLengthCtrl(this));
	m_staticFinishingCutMode = AddLabelAndControl(sizerLeft, _("cut mode"), m_cmbFinishingCutMode = new wxComboBox(this, ID_FINISH_CUT_MODE, _T(""), wxDefaultPosition, wxDefaultSize, 2, cut_mode_choices));
	m_staticFinishStepDown = AddLabelAndControl(sizerLeft, _("finish step down"), m_lgthFinishStepDown = new CLengthCtrl(this));

	SetFromData(object);

    SetSizer( sizerMain );
    sizerMain->SetSizeHints(this);
	sizerMain->Fit(this);

    m_idsSketches->SetFocus();

	m_ignore_event_functions = false;

	SetPicture();
}

void ProfileDlg::GetData(CProfile* object)
{
	if(m_ignore_event_functions)return;
	m_ignore_event_functions = true;

	object->m_sketches.clear();
	m_idsSketches->GetIDList(object->m_sketches);
	object->m_profile_params.m_cut_mode = (m_cmbCutMode->GetValue().CmpNoCase(_("climb")) == 0) ? CProfileParams::eClimb : CProfileParams::eConventional;

	object->m_profile_params.m_auto_roll_radius = m_lgthRollRadius->GetValue();
	object->m_profile_params.m_offset_extra = m_lgthOffsetExtra->GetValue();
	object->m_profile_params.m_do_finishing_pass = m_chkDoFinishingPass->GetValue();
	object->m_depth_op_params.ClearanceHeight( m_lgthClearanceHeight->GetValue() );
	object->m_depth_op_params.m_rapid_safety_space = m_lgthRapidDownToHeight->GetValue();
	object->m_depth_op_params.m_start_depth = m_lgthStartDepth->GetValue();
	object->m_depth_op_params.m_final_depth = m_lgthFinalDepth->GetValue();
	object->m_depth_op_params.m_step_down = m_lgthStepDown->GetValue();
	object->m_depth_op_params.m_z_finish_depth = m_lgthZFinishDepth->GetValue();
	object->m_depth_op_params.m_z_thru_depth = m_lgthZThruDepth->GetValue();
	object->m_speed_op_params.m_horizontal_feed_rate = m_lgthHFeed->GetValue();
	object->m_speed_op_params.m_vertical_feed_rate = m_lgthVFeed->GetValue();
	object->m_speed_op_params.m_spindle_speed = m_dblSpindleSpeed->GetValue();
	object->m_comment = m_txtComment->GetValue();
	object->m_active = m_chkActive->GetValue();

	object->m_profile_params.m_only_finishing_pass = m_chkOnlyFinishingPass->GetValue();
	object->m_profile_params.m_finishing_h_feed_rate = m_lgthFinishingFeedrate->GetValue();
	object->m_profile_params.m_finishing_cut_mode = (m_cmbFinishingCutMode->GetValue().CmpNoCase(_("climb")) == 0) ? CProfileParams::eClimb : CProfileParams::eConventional;
	object->m_profile_params.m_finishing_step_down = m_lgthFinishStepDown->GetValue();

	// get the tool number
	object->m_tool_number = 0;
	if(m_cmbTool->GetSelection() >= 0)object->m_tool_number = tools_for_combo[m_cmbTool->GetSelection()].first;

	object->m_title = m_txtTitle->GetValue();
	m_ignore_event_functions = false;
}

void ProfileDlg::SetFromData(CProfile* object)
{
	m_ignore_event_functions = true;

	m_idsSketches->SetFromIDList(object->m_sketches);

	{
		int choice = int(CProfileParams::eOn);
		switch (object->m_profile_params.m_tool_on_side)
		{
		case CProfileParams::eRightOrInside:	choice = 1;
					break;

			case CProfileParams::eOn:	choice = 2;
					break;

			case CProfileParams::eLeftOrOutside:	choice = 0;
					break;
		} // End switch

		m_cmbToolOnSide->SetSelection(choice);

	}

	m_cmbCutMode->SetValue((object->m_profile_params.m_cut_mode == CProfileParams::eClimb) ? _("climb") : _("conventional"));

	m_lgthRollRadius->SetValue(object->m_profile_params.m_auto_roll_radius);
	m_lgthOffsetExtra->SetValue(object->m_profile_params.m_offset_extra);

	m_chkDoFinishingPass->SetValue(object->m_profile_params.m_do_finishing_pass);

	// set the tool combo to the correct tool
	for(unsigned int i = 0; i < tools_for_combo.size(); i++)if(tools_for_combo[i].first == object->m_tool_number){m_cmbTool->SetSelection(i); break;}

	m_lgthClearanceHeight->SetValue(object->m_depth_op_params.ClearanceHeight());
	m_lgthRapidDownToHeight->SetValue(object->m_depth_op_params.m_rapid_safety_space);
	m_lgthStartDepth->SetValue(object->m_depth_op_params.m_start_depth);
	m_lgthFinalDepth->SetValue(object->m_depth_op_params.m_final_depth);
	m_lgthStepDown->SetValue(object->m_depth_op_params.m_step_down);
	m_lgthZFinishDepth->SetValue(object->m_depth_op_params.m_z_finish_depth);
	m_lgthZThruDepth->SetValue(object->m_depth_op_params.m_z_thru_depth);
	m_lgthHFeed->SetValue(object->m_speed_op_params.m_horizontal_feed_rate);
	m_lgthVFeed->SetValue(object->m_speed_op_params.m_vertical_feed_rate);
	m_dblSpindleSpeed->SetValue(object->m_speed_op_params.m_spindle_speed);
	m_txtComment->SetValue(object->m_comment);
	m_chkActive->SetValue(object->m_active);
	m_txtTitle->SetValue(object->m_title);

	m_chkOnlyFinishingPass->SetValue(object->m_profile_params.m_only_finishing_pass);
	m_lgthFinishingFeedrate->SetValue(object->m_profile_params.m_finishing_h_feed_rate);
	m_cmbFinishingCutMode->SetValue((object->m_profile_params.m_finishing_cut_mode == CProfileParams::eClimb) ? _("climb") : _("conventional"));
	m_lgthFinishStepDown->SetValue(object->m_profile_params.m_finishing_step_down);

	EnableControls();

	m_ignore_event_functions = false;
}

void ProfileDlg::EnableControls()
{
	bool finish = m_chkDoFinishingPass->GetValue();

	m_chkOnlyFinishingPass->Enable(finish);
	m_lgthFinishingFeedrate->Enable(finish);
	m_cmbFinishingCutMode->Enable(finish);
	m_lgthFinishStepDown->Enable(finish);
	m_staticFinishingFeedrate->Enable(finish);
	m_staticFinishingCutMode->Enable(finish);
	m_staticFinishStepDown->Enable(finish);
}

void ProfileDlg::SetPicture(const wxString& name, bool pocket_picture)
{
	m_picture->SetPicture(theApp.GetResFolder() + _T("/bitmaps/") + (pocket_picture? _T("pocket/"):_T("profile/")) + name + _T(".png"), wxBITMAP_TYPE_PNG);
}

void ProfileDlg::SetPicture()
{
	wxWindow* w = FindFocus();

	if(w == m_cmbToolOnSide)
	{
		int sel = m_cmbToolOnSide->GetSelection();
		if(sel == 2)
		{
			SetPicture(_T("side on"));
		}
		else
		{
			switch(m_order)
			{
			case SketchOrderTypeOpen:
				SetPicture((sel == 1)? _T("side right"):_T("side left"));
				break;

			case SketchOrderTypeCloseCW:
			case SketchOrderTypeCloseCCW:
				SetPicture((sel == 1)? _T("side inside"):_T("side outside"));
				break;

			default:
				SetPicture((sel == 1)? _T("side inside or right"):_T("side outside or left"));
				break;
			}
		}
	}
	else if(w == m_cmbCutMode)
	{
		if(m_cmbCutMode->GetValue() == _("climb"))SetPicture(_T("climb milling"), true);
		else SetPicture(_T("conventional milling"), true);
	}
	else if(w == m_lgthRollRadius)SetPicture(_T("roll radius"));
	else if(w == m_lgthOffsetExtra)SetPicture(_T("offset extra"));
	else if(w == m_chkDoFinishingPass || w == m_chkOnlyFinishingPass)
	{
		if(m_chkDoFinishingPass->GetValue())
		{
			if(m_chkOnlyFinishingPass->GetValue())SetPicture(_T("only finishing"));
			else SetPicture(_T("do finishing pass"));
		}
		else SetPicture(_T("no finishing pass"));
	}
	else if(w == m_lgthClearanceHeight)SetPicture(_T("clearance height"),true);
	else if(w == m_lgthRapidDownToHeight)SetPicture(_T("rapid down height"),true);
	else if(w == m_lgthStartDepth)SetPicture(_T("start depth"),true);
	else if(w == m_lgthFinalDepth)SetPicture(_T("final depth"),true);
	else if(w == m_lgthStepDown)SetPicture(_T("step down"),true);
	else if(w == m_lgthZFinishDepth)SetPicture(_T("z finish depth"),true);
	else if(w == m_lgthZThruDepth)SetPicture(_T("z thru depth"),true);
	else if(w == m_cmbFinishingCutMode)
	{
		if(m_cmbFinishingCutMode->GetValue() == _("climb"))SetPicture(_T("climb milling"), true);
		else SetPicture(_T("conventional milling"), true);
	}
	else if(w == m_lgthFinishStepDown)SetPicture(_T("step down"),true);

	else SetPicture(_T("general"));
}

void ProfileDlg::OnChildFocus(wxChildFocusEvent& event)
{
	if(m_ignore_event_functions)return;
	if(event.GetWindow())
	{
		SetPicture();
	}
}

void ProfileDlg::OnComboOrCheck( wxCommandEvent& event )
{
	if(m_ignore_event_functions)return;
	SetPicture();
}

void ProfileDlg::OnCheckFinishingPass(wxCommandEvent& event)
{
	if(m_ignore_event_functions)return;
	EnableControls();
	SetPicture();
}

void ProfileDlg::SetSketchOrderAndCombo()
{
	m_order = SketchOrderTypeUnknown;

	if(m_object->GetNumSketches() == 1)
	{
		HeeksObj* sketch = heeksCAD->GetIDObject(SketchType, m_object->m_sketches.front());
		if((sketch) && (sketch->GetType() == SketchType))
		{
			m_order = heeksCAD->GetSketchOrder(sketch);
		}
	}

	switch(m_order)
	{
	case SketchOrderTypeOpen:
		m_cmbToolOnSide->SetString (0, _("left"));
		m_cmbToolOnSide->SetString (1, _("right"));
		break;

	case SketchOrderTypeCloseCW:
	case SketchOrderTypeCloseCCW:
		m_cmbToolOnSide->SetString (0, _("outside"));
		m_cmbToolOnSide->SetString (1, _("inside"));
		break;

	default:
		m_cmbToolOnSide->SetString (0, _("outside or left"));
		m_cmbToolOnSide->SetString (1, _("inside or right"));
		break;
	}
}