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

BEGIN_EVENT_TABLE(SpeedOpDlg, OpDlg)
    EVT_COMBOBOX(ID_TOOL,HeeksObjDlg::OnComboOrCheck)
    EVT_COMBOBOX(ID_PATTERN,HeeksObjDlg::OnComboOrCheck)
    EVT_COMBOBOX(ID_SURFACE,HeeksObjDlg::OnComboOrCheck)
END_EVENT_TABLE()

SpeedOpDlg::SpeedOpDlg(wxWindow *parent, CSpeedOp* object, bool some_controls_on_left, const wxString& title, bool top_level)
             : OpDlg(parent, object, title, false)
{
	std::list<HControl> *feeds_and_speeds_control_list = &rightControls;
	if(some_controls_on_left)feeds_and_speeds_control_list = &leftControls;

	// add some of the controls to the right side
	feeds_and_speeds_control_list->push_back(MakeLabelAndControl(_("Horizontal Feedrate"), m_lgthHFeed = new CLengthCtrl(this)));
	feeds_and_speeds_control_list->push_back(MakeLabelAndControl(_("Vertical Feedrate"), m_lgthVFeed = new CLengthCtrl(this)));
	feeds_and_speeds_control_list->push_back(MakeLabelAndControl(_("Spindle Speed"), m_dblSpindleSpeed = new CDoubleCtrl(this)));

	if(top_level)
	{
		OpDlg::AddControlsAndCreate();
		m_cmbTool->SetFocus();
	}
}

void SpeedOpDlg::GetDataRaw(HeeksObj* object)
{
	((CSpeedOp*)object)->m_speed_op_params.m_horizontal_feed_rate = m_lgthHFeed->GetValue();
	((CSpeedOp*)object)->m_speed_op_params.m_vertical_feed_rate = m_lgthVFeed->GetValue();
	((CSpeedOp*)object)->m_speed_op_params.m_spindle_speed = m_dblSpindleSpeed->GetValue();

	OpDlg::GetDataRaw(object);
}

void SpeedOpDlg::SetFromDataRaw(HeeksObj* object)
{
	m_lgthHFeed->SetValue(((CSpeedOp*)object)->m_speed_op_params.m_horizontal_feed_rate);
	m_lgthVFeed->SetValue(((CSpeedOp*)object)->m_speed_op_params.m_vertical_feed_rate);
	m_dblSpindleSpeed->SetValue(((CSpeedOp*)object)->m_speed_op_params.m_spindle_speed);

	OpDlg::SetFromDataRaw(object);
}

void SpeedOpDlg::SetPicture(const wxString& name)
{
	HeeksObjDlg::SetPicture(name, _T("speedop"));
}

void SpeedOpDlg::SetPictureByWindow(wxWindow* w)
{
	OpDlg::SetPictureByWindow(w);
}
