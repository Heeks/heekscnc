// DepthOpDlg.cpp
// Copyright (c) 2010, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include "DepthOpDlg.h"
#include "../../interface/PictureFrame.h"
#include "../../interface/NiceTextCtrl.h"
#include "Profile.h"
#include "CTool.h"

BEGIN_EVENT_TABLE(DepthOpDlg, SpeedOpDlg)
END_EVENT_TABLE()

DepthOpDlg::DepthOpDlg(wxWindow *parent, CDepthOp* object, bool drill_pictures, const wxString& title, bool top_level)
:m_drill_pictures(drill_pictures), SpeedOpDlg(parent, object, false, title, false)
{
	leftControls.push_back(MakeLabelAndControl( _("Clearance Height"), m_lgthClearanceHeight = new CLengthCtrl(this)));
	leftControls.push_back(MakeLabelAndControl( _("Rapid Safety Space"), m_lgthRapidDownToHeight = new CLengthCtrl(this)));
	leftControls.push_back(MakeLabelAndControl( _("Start Depth"), m_lgthStartDepth = new CLengthCtrl(this)));
	leftControls.push_back(MakeLabelAndControl( _("Final Depth"), m_lgthFinalDepth = new CLengthCtrl(this)));
	leftControls.push_back(MakeLabelAndControl( _("Step Down"), m_lgthStepDown = new CLengthCtrl(this)));
	leftControls.push_back(MakeLabelAndControl( _("Z Finish Depth"), m_lgthZFinishDepth = new CLengthCtrl(this)));
	leftControls.push_back(MakeLabelAndControl( _("Z Through Depth"), m_lgthZThruDepth = new CLengthCtrl(this)));

	if(top_level)
	{
		HeeksObjDlg::AddControlsAndCreate();
		m_lgthClearanceHeight->SetFocus();
	}
}

void DepthOpDlg::GetDataRaw(HeeksObj* object)
{
	((CDepthOp*)object)->m_depth_op_params.m_clearance_height = m_lgthClearanceHeight->GetValue();
	((CDepthOp*)object)->m_depth_op_params.m_rapid_safety_space = m_lgthRapidDownToHeight->GetValue();
	((CDepthOp*)object)->m_depth_op_params.m_start_depth = m_lgthStartDepth->GetValue();
	((CDepthOp*)object)->m_depth_op_params.m_final_depth = m_lgthFinalDepth->GetValue();
	((CDepthOp*)object)->m_depth_op_params.m_step_down = m_lgthStepDown->GetValue();
	((CDepthOp*)object)->m_depth_op_params.m_z_finish_depth = m_lgthZFinishDepth->GetValue();
	((CDepthOp*)object)->m_depth_op_params.m_z_thru_depth = m_lgthZThruDepth->GetValue();

	SpeedOpDlg::GetDataRaw(object);
}

void DepthOpDlg::SetFromDataRaw(HeeksObj* object)
{
	m_lgthClearanceHeight->SetValue(((CDepthOp*)object)->m_depth_op_params.m_clearance_height);
	m_lgthRapidDownToHeight->SetValue(((CDepthOp*)object)->m_depth_op_params.m_rapid_safety_space);
	m_lgthStartDepth->SetValue(((CDepthOp*)object)->m_depth_op_params.m_start_depth);
	m_lgthFinalDepth->SetValue(((CDepthOp*)object)->m_depth_op_params.m_final_depth);
	m_lgthStepDown->SetValue(((CDepthOp*)object)->m_depth_op_params.m_step_down);
	m_lgthZFinishDepth->SetValue(((CDepthOp*)object)->m_depth_op_params.m_z_finish_depth);
	m_lgthZThruDepth->SetValue(((CDepthOp*)object)->m_depth_op_params.m_z_thru_depth);

	SpeedOpDlg::SetFromDataRaw(object);
}

void DepthOpDlg::SetPictureByWindow(wxWindow* w)
{
	if(w == m_lgthClearanceHeight)DepthOpDlg::SetPicture(m_drill_pictures ? _T("drill clearance height") : _T("clearance height"));
	else if(w == m_lgthRapidDownToHeight)DepthOpDlg::SetPicture(m_drill_pictures ? _T("drill rapid down height") : _T("rapid down height"));
	else if(w == m_lgthStartDepth)DepthOpDlg::SetPicture(m_drill_pictures ? _T("drill start depth") : _T("start depth"));
	else if(w == m_lgthFinalDepth)DepthOpDlg::SetPicture(m_drill_pictures ? _T("drill final depth") : _T("final depth"));
	else if(w == m_lgthStepDown)DepthOpDlg::SetPicture(m_drill_pictures ? _T("drill step down") : _T("step down"));
	else if(w == m_lgthZFinishDepth)DepthOpDlg::SetPicture(m_drill_pictures ? _T("drill z finish depth") : _T("z finish depth"));
	else if(w == m_lgthZThruDepth)DepthOpDlg::SetPicture(m_drill_pictures ? _T("drill z thru depth") : _T("z thru depth"));
	else SpeedOpDlg::SetPictureByWindow(w);
}

void DepthOpDlg::SetPicture(const wxString& name)
{
	HeeksObjDlg::SetPicture(name, _T("depthop"));
}
