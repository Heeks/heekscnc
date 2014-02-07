// SpeedOpDlg.cpp
// Copyright (c) 2010, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include "SpeedOpDlg.h"
#include "interface/PictureFrame.h"
#include "interface/NiceTextCtrl.h"
#include "Profile.h"
#include "CTool.h"
#include "Program.h"
#include "Tools.h"
#include "Patterns.h"
#include "Surfaces.h"

enum
{
	ID_TOOL = 200,
	ID_PATTERN,
	ID_SURFACE,
};

BEGIN_EVENT_TABLE(SpeedOpDlg, HeeksObjDlg)
    EVT_COMBOBOX(ID_TOOL,HeeksObjDlg::OnComboOrCheck)
    EVT_COMBOBOX(ID_PATTERN,HeeksObjDlg::OnComboOrCheck)
    EVT_COMBOBOX(ID_SURFACE,HeeksObjDlg::OnComboOrCheck)
END_EVENT_TABLE()

SpeedOpDlg::SpeedOpDlg(wxWindow *parent, CSpeedOp* object, bool some_controls_on_left, const wxString& title, bool top_level)
             : HeeksObjDlg(parent, object, title, false)
{
	leftControls.push_back(MakeLabelAndControl(_("Tool"), m_cmbTool = new HTypeObjectDropDown(this, ID_TOOL, ToolType, theApp.m_program->Tools())));
	leftControls.push_back(MakeLabelAndControl(_("Pattern"), m_cmbPattern = new HTypeObjectDropDown(this, ID_PATTERN, PatternType, theApp.m_program->Patterns())));
	leftControls.push_back(MakeLabelAndControl(_("Surface"), m_cmbSurface = new HTypeObjectDropDown(this, ID_SURFACE, SurfaceType, theApp.m_program->Surfaces())));

	std::list<HControl> *feeds_and_speeds_control_list = &rightControls;
	if(some_controls_on_left)feeds_and_speeds_control_list = &leftControls;

	// add some of the controls to the right side
	feeds_and_speeds_control_list->push_back(MakeLabelAndControl(_("horizontal feedrate"), m_lgthHFeed = new CLengthCtrl(this)));
	feeds_and_speeds_control_list->push_back(MakeLabelAndControl(_("vertical feedrate"), m_lgthVFeed = new CLengthCtrl(this)));
	feeds_and_speeds_control_list->push_back(MakeLabelAndControl(_("spindle speed"), m_dblSpindleSpeed = new CDoubleCtrl(this)));
	rightControls.push_back(MakeLabelAndControl(_("comment"), m_txtComment = new wxTextCtrl(this, wxID_ANY)));
	rightControls.push_back(HControl(m_chkActive = new wxCheckBox( this, wxID_ANY, _("active") ), wxALL));
	rightControls.push_back(MakeLabelAndControl(_("title"), m_txtTitle = new wxTextCtrl(this, wxID_ANY)));

	if(top_level)
	{
		HeeksObjDlg::AddControlsAndCreate();
		m_cmbTool->SetFocus();
	}
}

void SpeedOpDlg::GetDataRaw(HeeksObj* object)
{
	((CSpeedOp*)object)->m_speed_op_params.m_horizontal_feed_rate = m_lgthHFeed->GetValue();
	((CSpeedOp*)object)->m_speed_op_params.m_vertical_feed_rate = m_lgthVFeed->GetValue();
	((CSpeedOp*)object)->m_speed_op_params.m_spindle_speed = m_dblSpindleSpeed->GetValue();
	((CSpeedOp*)object)->m_comment = m_txtComment->GetValue();
	((CSpeedOp*)object)->m_active = m_chkActive->GetValue();
	((CSpeedOp*)object)->m_title = m_txtTitle->GetValue();
	((CSpeedOp*)object)->m_tool_number = m_cmbTool->GetSelectedId();
	((CSpeedOp*)object)->m_pattern = m_cmbPattern->GetSelectedId();
	((CSpeedOp*)object)->m_surface = m_cmbSurface->GetSelectedId();

	HeeksObjDlg::GetDataRaw(object);
}

void SpeedOpDlg::SetFromDataRaw(HeeksObj* object)
{
	m_lgthHFeed->SetValue(((CSpeedOp*)object)->m_speed_op_params.m_horizontal_feed_rate);
	m_lgthVFeed->SetValue(((CSpeedOp*)object)->m_speed_op_params.m_vertical_feed_rate);
	m_dblSpindleSpeed->SetValue(((CSpeedOp*)object)->m_speed_op_params.m_spindle_speed);
	m_txtComment->SetValue(((CSpeedOp*)object)->m_comment);
	m_chkActive->SetValue(((CSpeedOp*)object)->m_active);
	m_txtTitle->SetValue(((CSpeedOp*)object)->m_title);
	m_cmbTool->SelectById(((CSpeedOp*)object)->m_tool_number);
	m_cmbPattern->SelectById(((CSpeedOp*)object)->m_pattern);
	m_cmbSurface->SelectById(((CSpeedOp*)object)->m_surface);

	HeeksObjDlg::SetFromDataRaw(object);
}

void SpeedOpDlg::SetPicture(const wxString& name)
{
	HeeksObjDlg::SetPicture(name, _T("speedop"));
}

void SpeedOpDlg::SetPictureByWindow(wxWindow* w)
{
	if(w == m_cmbPattern)HeeksObjDlg::SetPicture(_T("general"), _T("pattern"));
	else if(w == m_cmbSurface)HeeksObjDlg::SetPicture(_T("general"), _T("surface"));
	else SetPicture(_T("general"));
}
