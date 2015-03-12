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
	ID_POINTS_PICK,
	ID_STOP_SPINDLE,
	ID_INTERNAL_COOLANT_ON,
	ID_RAPID_TO_CLEARANCE,
};

BEGIN_EVENT_TABLE(DrillingDlg, DepthOpDlg)
    EVT_CHECKBOX(ID_FEED_RETRACT, HeeksObjDlg::OnComboOrCheck)
    EVT_BUTTON(ID_POINTS_PICK, DrillingDlg::OnPointsPick)
    EVT_CHECKBOX(ID_STOP_SPINDLE, HeeksObjDlg::OnComboOrCheck)
    EVT_CHECKBOX(ID_INTERNAL_COOLANT_ON, HeeksObjDlg::OnComboOrCheck)
    EVT_CHECKBOX(ID_RAPID_TO_CLEARANCE, HeeksObjDlg::OnComboOrCheck)
    EVT_BUTTON(wxID_HELP, DrillingDlg::OnHelp)
END_EVENT_TABLE()

DrillingDlg::DrillingDlg(wxWindow *parent, CDrilling* object, const wxString& title, bool top_level)
: DepthOpDlg(parent, object, true, title, false)
{
	std::list<HControl> save_leftControls = leftControls;
	leftControls.clear();

	// add all the controls to the left side
	leftControls.push_back(MakeLabelAndControl(_("Points"), m_idsPoints = new CObjectIdsCtrl(this), m_btnPointsPick = new wxButton(this, ID_POINTS_PICK, _("Pick"))));
	leftControls.push_back(MakeLabelAndControl(_("Dwell"), m_dblDwell = new CDoubleCtrl(this)));
	leftControls.push_back( HControl( m_chkFeedRetract = new wxCheckBox( this, ID_FEED_RETRACT, _("Feed Retract") ), wxALL ));
	leftControls.push_back( HControl( m_chkRapidToClearance = new wxCheckBox( this, ID_RAPID_TO_CLEARANCE, _("Rapid to Clearance") ), wxALL ));
	leftControls.push_back( HControl( m_chkStopSpindleAtBottom = new wxCheckBox( this, ID_STOP_SPINDLE, _("Stop Spindle at Bottom") ), wxALL ));
	leftControls.push_back( HControl( m_chkInternalCoolantOn = new wxCheckBox( this, ID_INTERNAL_COOLANT_ON, _("Interal Coolant On") ), wxALL ));

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
	
	((CDrilling*)object)->m_params.m_dwell = m_dblDwell->GetValue();
	((CDrilling*)object)->m_params.m_retract_mode = m_chkFeedRetract->GetValue();
	((CDrilling*)object)->m_params.m_spindle_mode = m_chkStopSpindleAtBottom->GetValue();
	((CDrilling*)object)->m_params.m_internal_coolant_on = m_chkInternalCoolantOn->GetValue();
	((CDrilling*)object)->m_params.m_rapid_to_clearance = m_chkRapidToClearance->GetValue();

	DepthOpDlg::GetDataRaw(object);
}

void DrillingDlg::SetFromDataRaw(HeeksObj* object)
{
	m_idsPoints->SetFromIDList(((CDrilling*)object)->m_points);
	m_dblDwell->SetValue(((CDrilling*)object)->m_params.m_dwell);
	m_chkFeedRetract->SetValue(((CDrilling*)object)->m_params.m_retract_mode != 0);
	m_chkStopSpindleAtBottom->SetValue(((CDrilling*)object)->m_params.m_spindle_mode != 0);
	m_chkInternalCoolantOn->SetValue(((CDrilling*)object)->m_params.m_internal_coolant_on != 0);
	m_chkRapidToClearance->SetValue(((CDrilling*)object)->m_params.m_rapid_to_clearance != 0);

	DepthOpDlg::SetFromDataRaw(object);
}

void DrillingDlg::SetPictureByWindow(wxWindow* w)
{
	if(w == m_dblDwell)SetPicture(_T("dwell"));
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
	else if(w == m_chkInternalCoolantOn)
	{
		if(m_chkInternalCoolantOn->GetValue())SetPicture(_T("internal coolant on"));
		else SetPicture(_T("internal coolant off"));
	}
	else if(w == m_chkRapidToClearance)
	{
		if(m_chkRapidToClearance->GetValue())SetPicture(_T("rapid to clearance"));
		else SetPicture(_T("rapid to standoff"));
	}
	else DepthOpDlg::SetPictureByWindow(w);

}

void DrillingDlg::SetPicture(const wxString& name)
{
	HeeksObjDlg::SetPicture(name, _T("drilling"));
}

void DrillingDlg::OnPointsPick( wxCommandEvent& event )
{
	EndModal(ID_POINTS_PICK);
}

void DrillingDlg::OnHelp( wxCommandEvent& event )
{
	::wxLaunchDefaultBrowser(_T("http://heeks.net/help/drilling"));
}

bool DrillingDlg::Do(CDrilling* object)
{
	DrillingDlg dlg(heeksCAD->GetMainFrame(), object);

	while(1)
	{
		int result = dlg.ShowModal();

		if(result == wxID_OK)
		{
			dlg.GetData(object);
			return true;
		}
		else if(result == ID_POINTS_PICK)
		{
			heeksCAD->ClearMarkedList();
			heeksCAD->PickObjects(_("Pick points to drill"), MARKING_FILTER_POINT, false);

			std::list<int> ids;
			const std::list<HeeksObj*> &list = heeksCAD->GetMarkedList();
			if(list.size() > 0)
			{
				for(std::list<HeeksObj*>::const_iterator It = list.begin(); It != list.end(); It++)
				{
					HeeksObj* object = *It;
					ids.push_back(object->GetID());
				}
			}

			dlg.m_idsPoints->SetFromIDList(ids);

			dlg.Fit();
		}
		else
		{
			return false;
		}
	}
}
