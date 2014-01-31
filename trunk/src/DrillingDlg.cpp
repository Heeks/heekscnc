// DrillingDlg.cpp
// Copyright (c) 2010, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include "DrillingDlg.h"
#include "interface/PictureFrame.h"
#include "interface/NiceTextCtrl.h"
#include "Drilling.h"
#include "CTool.h"

enum
{
	ID_FEED_RETRACT = 100,
	ID_STOP_SPINDLE,
};

BEGIN_EVENT_TABLE(DrillingDlg, SpeedOpDlg)
    EVT_CHECKBOX(ID_FEED_RETRACT, HeeksObjDlg::OnComboOrCheck)
    EVT_CHECKBOX(ID_STOP_SPINDLE, HeeksObjDlg::OnComboOrCheck)
END_EVENT_TABLE()

DrillingDlg::DrillingDlg(wxWindow *parent, CDrilling* object, const wxString& title, bool top_level)
: SpeedOpDlg(parent, object, true, title, false)
{
	std::list<HControl> save_leftControls = leftControls;
	leftControls.clear();

	// add all the controls to the left side
	leftControls.push_back(MakeLabelAndControl(_("points"), m_idsPoints = new CObjectIdsCtrl(this)));
	leftControls.push_back(MakeLabelAndControl(_("stand off"), m_lgthStandOff = new CLengthCtrl(this)));
	leftControls.push_back(MakeLabelAndControl(_("dwell"), m_dblDwell = new CDoubleCtrl(this)));
	leftControls.push_back( HControl( m_chkFeedRetract = new wxCheckBox( this, ID_FEED_RETRACT, _("feed retract") ), wxALL ));
	leftControls.push_back( HControl( m_chkStopSpindleAtBottom = new wxCheckBox( this, ID_STOP_SPINDLE, _("stop spindle at bottom") ), wxALL ));
	leftControls.push_back(MakeLabelAndControl(_("clearance height"), m_lgthClearanceHeight = new CLengthCtrl(this)));

	for(std::list<HControl>::iterator It = save_leftControls.begin(); It != save_leftControls.end(); It++)
	{
		leftControls.push_back(*It);
	}

	if(top_level)
	{
		HeeksObjDlg::AddControlsAndCreate();
		m_idsPoints->SetFocus();
	}
}

void DrillingDlg::GetDataRaw(HeeksObj* object)
{
	((CDrilling*)object)->m_points.clear();
	m_idsPoints->GetIDList(((CDrilling*)object)->m_points);
	((CDrilling*)object)->m_params.m_standoff = m_lgthStandOff->GetValue();
	((CDrilling*)object)->m_params.m_dwell = m_dblDwell->GetValue();
	((CDrilling*)object)->m_params.m_retract_mode = m_chkFeedRetract->GetValue();
	((CDrilling*)object)->m_params.m_spindle_mode = m_chkStopSpindleAtBottom->GetValue();
	((CDrilling*)object)->m_params.m_clearance_height = m_lgthClearanceHeight->GetValue();

	SpeedOpDlg::GetDataRaw(object);
}

void DrillingDlg::SetFromDataRaw(HeeksObj* object)
{
	m_idsPoints->SetFromIDList(((CDrilling*)object)->m_points);
	m_lgthStandOff->SetValue(((CDrilling*)object)->m_params.m_standoff);
	m_dblDwell->SetValue(((CDrilling*)object)->m_params.m_dwell);
	m_chkFeedRetract->SetValue(((CDrilling*)object)->m_params.m_retract_mode != 0);
	m_chkStopSpindleAtBottom->SetValue(((CDrilling*)object)->m_params.m_spindle_mode != 0);
	m_lgthClearanceHeight->SetValue(((CDrilling*)object)->m_params.m_clearance_height);

	SpeedOpDlg::SetFromDataRaw(object);
}

void DrillingDlg::SetPictureByWindow(wxWindow* w)
{
	if(w == m_lgthStandOff)SetPicture(_T("stand off"));
	else if(w == m_dblDwell)SetPicture(_T("dwell"));
	else if(w == m_chkFeedRetract)
	{
		if(m_chkFeedRetract->GetValue())SetPicture(_T("feed retract"));
		else SetPicture(_T("rapid retract"));
	}
	else if(w == m_chkStopSpindleAtBottom)
	{
		if(m_chkStopSpindleAtBottom->GetValue())SetPicture(_T("stop spindle at bottom"));
		else SetPicture(_T("dont stop spindle"));
	}
	else if(w == m_lgthClearanceHeight)SetPicture(_T("clearance height"));
	else SpeedOpDlg::SetPictureByWindow(w);

}

void DrillingDlg::SetPicture(const wxString& name)
{
	HeeksObjDlg::SetPicture(name, _T("drilling"));
}
