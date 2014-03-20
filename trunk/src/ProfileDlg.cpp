// ProfileDlg.cpp
// Copyright (c) 2010, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include "ProfileDlg.h"
#include "../../interface/PictureFrame.h"
#include "../../interface/NiceTextCtrl.h"
#include "Profile.h"
#include "CTool.h"

BEGIN_EVENT_TABLE(ProfileDlg, SketchOpDlg)
    EVT_COMBOBOX(ID_TOOL_ON_SIDE,HeeksObjDlg::OnComboOrCheck)
    EVT_COMBOBOX(ID_CUT_MODE,HeeksObjDlg::OnComboOrCheck)
    EVT_CHECKBOX(ID_DO_FINISHING_PASS, ProfileDlg::OnCheckFinishingPass)
    EVT_BUTTON(wxID_HELP, ProfileDlg::OnHelp)
    EVT_CHECKBOX(ID_ONLY_FINISHING_PASS, HeeksObjDlg::OnComboOrCheck)
    EVT_COMBOBOX(ID_FINISH_CUT_MODE,HeeksObjDlg::OnComboOrCheck)
    EVT_COMBOBOX(ID_SKETCH,ProfileDlg::OnSketchCombo)
END_EVENT_TABLE()

ProfileDlg::ProfileDlg(wxWindow *parent, CProfile* object, const wxString& title, bool top_level)
: SketchOpDlg(parent, object, title, false)
{
	std::list<HControl> save_leftControls = leftControls;
	leftControls.clear();

	// add all the controls to the left side
	wxString tool_on_side_choices[] = {_("Left"), _("Right"), _("On")};
	leftControls.push_back(MakeLabelAndControl( _("Tool On Side"), m_cmbToolOnSide = new wxComboBox(this, ID_TOOL_ON_SIDE, _T(""), wxDefaultPosition, wxDefaultSize, 3, tool_on_side_choices)));

	SetSketchOrderAndCombo(((CProfile*)m_object)->m_sketch);

	wxString cut_mode_choices[] = {_("Conventional"), _("Climb")};
	leftControls.push_back(MakeLabelAndControl(_("Cut Mode"), m_cmbCutMode = new wxComboBox(this, ID_CUT_MODE, _T(""), wxDefaultPosition, wxDefaultSize, 2, cut_mode_choices)));
	leftControls.push_back(MakeLabelAndControl(_("Roll Radius"), m_lgthRollRadius = new CLengthCtrl(this)));
	leftControls.push_back(MakeLabelAndControl(_("Offset Extra"), m_lgthOffsetExtra = new CLengthCtrl(this)));
	leftControls.push_back( HControl( m_chkDoFinishingPass = new wxCheckBox( this, ID_DO_FINISHING_PASS, _("Do Finishing Pass") ), wxALL ));
	leftControls.push_back( HControl( m_chkOnlyFinishingPass = new wxCheckBox( this, ID_ONLY_FINISHING_PASS, _("Only Finishing Pass") ), wxALL ));
	leftControls.push_back(MakeLabelAndControl(_("Finishing Feed Rate"), m_lgthFinishingFeedrate = new CLengthCtrl(this), &m_staticFinishingFeedrate));
	leftControls.push_back(MakeLabelAndControl(_("Finishing Cut Mode"), m_cmbFinishingCutMode = new wxComboBox(this, ID_FINISH_CUT_MODE, _T(""), wxDefaultPosition, wxDefaultSize, 2, cut_mode_choices), &m_staticFinishingCutMode));
	leftControls.push_back(MakeLabelAndControl(_("Finish Step Down"), m_lgthFinishStepDown = new CLengthCtrl(this), &m_staticFinishStepDown));

	for(std::list<HControl>::iterator It = save_leftControls.begin(); It != save_leftControls.end(); It++)
	{
		leftControls.push_back(*It);
	}

	if(top_level)
	{
		HeeksObjDlg::AddControlsAndCreate();
		m_cmbSketch->SetFocus();
	}
}

void ProfileDlg::GetDataRaw(HeeksObj* object)
{
	switch (m_cmbToolOnSide->GetSelection())
	{
	case 1:
		((CProfile*)object)->m_profile_params.m_tool_on_side = CProfileParams::eRightOrInside;
		break;

	case 2:
		((CProfile*)object)->m_profile_params.m_tool_on_side = CProfileParams::eOn;
		break;

	case 0:
		((CProfile*)object)->m_profile_params.m_tool_on_side = CProfileParams::eLeftOrOutside;
		break;
	} // End switch

	((CProfile*)object)->m_profile_params.m_cut_mode = (m_cmbCutMode->GetValue().CmpNoCase(_("climb")) == 0) ? CProfileParams::eClimb : CProfileParams::eConventional;

	((CProfile*)object)->m_profile_params.m_auto_roll_radius = m_lgthRollRadius->GetValue();
	((CProfile*)object)->m_profile_params.m_offset_extra = m_lgthOffsetExtra->GetValue();
	((CProfile*)object)->m_profile_params.m_do_finishing_pass = m_chkDoFinishingPass->GetValue();

	((CProfile*)object)->m_profile_params.m_only_finishing_pass = m_chkOnlyFinishingPass->GetValue();
	((CProfile*)object)->m_profile_params.m_finishing_h_feed_rate = m_lgthFinishingFeedrate->GetValue();
	((CProfile*)object)->m_profile_params.m_finishing_cut_mode = (m_cmbFinishingCutMode->GetValue().CmpNoCase(_("climb")) == 0) ? CProfileParams::eClimb : CProfileParams::eConventional;
	((CProfile*)object)->m_profile_params.m_finishing_step_down = m_lgthFinishStepDown->GetValue();

	SketchOpDlg::GetDataRaw(object);
}

void ProfileDlg::SetFromDataRaw(HeeksObj* object)
{
	{
		int choice = int(CProfileParams::eOn);
		switch (((CProfile*)object)->m_profile_params.m_tool_on_side)
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

	m_cmbCutMode->SetValue((((CProfile*)object)->m_profile_params.m_cut_mode == CProfileParams::eClimb) ? _("Climb") : _("Conventional"));

	m_lgthRollRadius->SetValue(((CProfile*)object)->m_profile_params.m_auto_roll_radius);
	m_lgthOffsetExtra->SetValue(((CProfile*)object)->m_profile_params.m_offset_extra);

	m_chkDoFinishingPass->SetValue(((CProfile*)object)->m_profile_params.m_do_finishing_pass);

	m_chkOnlyFinishingPass->SetValue(((CProfile*)object)->m_profile_params.m_only_finishing_pass);
	m_lgthFinishingFeedrate->SetValue(((CProfile*)object)->m_profile_params.m_finishing_h_feed_rate);
	m_cmbFinishingCutMode->SetValue((((CProfile*)object)->m_profile_params.m_finishing_cut_mode == CProfileParams::eClimb) ? _("Climb") : _("Conventional"));
	m_lgthFinishStepDown->SetValue(((CProfile*)object)->m_profile_params.m_finishing_step_down);

	EnableControls();

	SketchOpDlg::SetFromDataRaw(object);
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

void ProfileDlg::SetPictureByWindow(wxWindow* w)
{
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
		if(m_cmbCutMode->GetValue() == _("Climb"))HeeksObjDlg::SetPicture(_T("climb milling"),_T("pocket"));
		else HeeksObjDlg::SetPicture(_T("conventional milling"),_T("pocket"));
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
	else if(w == m_cmbFinishingCutMode)
	{
		if(m_cmbFinishingCutMode->GetValue() == _("Climb"))HeeksObjDlg::SetPicture(_T("climb milling"),_T("pocket"));
		else HeeksObjDlg::SetPicture(_T("conventional milling"),_T("pocket"));
	}
	else if(w == m_lgthFinishStepDown)HeeksObjDlg::SetPicture(_T("step down"),_T("depthop"));
	else SketchOpDlg::SetPictureByWindow(w);

}

void ProfileDlg::SetPicture(const wxString& name)
{
	HeeksObjDlg::SetPicture(name, _T("profile"));
}

void ProfileDlg::OnCheckFinishingPass(wxCommandEvent& event)
{
	if(m_ignore_event_functions)return;
	EnableControls();
	HeeksObjDlg::SetPicture();
}

void ProfileDlg::OnHelp( wxCommandEvent& event )
{
	::wxLaunchDefaultBrowser(_T("http://heeks.net/help/profile"));
}

void ProfileDlg::SetSketchOrderAndCombo(int s)
{
	m_order = SketchOrderTypeUnknown;

	{
		HeeksObj* sketch = heeksCAD->GetIDObject(SketchType, s);
		if((sketch) && (sketch->GetType() == SketchType))
		{
			m_order = heeksCAD->GetSketchOrder(sketch);
		}
	}

	switch(m_order)
	{
	case SketchOrderTypeOpen:
		m_cmbToolOnSide->SetString (0, _("Left"));
		m_cmbToolOnSide->SetString (1, _("Right"));
		break;

	case SketchOrderTypeCloseCW:
	case SketchOrderTypeCloseCCW:
		m_cmbToolOnSide->SetString (0, _("Outside"));
		m_cmbToolOnSide->SetString (1, _("Inside"));
		break;

	default:
		m_cmbToolOnSide->SetString (0, _("Outside or Left"));
		m_cmbToolOnSide->SetString (1, _("Inside or Right"));
		break;
	}
}

void ProfileDlg::OnSketchCombo( wxCommandEvent& event )
{
	int choice = m_cmbToolOnSide->GetSelection();
	SetSketchOrderAndCombo(	m_cmbSketch->GetSelectedId() );
	m_cmbToolOnSide->SetSelection(choice);
}

bool ProfileDlg::Do(CProfile* object)
{
	ProfileDlg dlg(heeksCAD->GetMainFrame(), object);

	while(1)
	{
		int result = dlg.ShowModal();

		if(result == wxID_OK)
		{
			dlg.GetData(object);
			return true;
		}
		else if(result == ID_SKETCH_PICK)
		{
			dlg.PickSketch();
		}
		else
		{
			return false;
		}
	}
}