// PocketDlg.cpp
// Copyright (c) 2014, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include "PocketDlg.h"
#include "interface/NiceTextCtrl.h"
#include "Pocket.h"

BEGIN_EVENT_TABLE(PocketDlg, SketchOpDlg)
    EVT_COMBOBOX(ID_STARTING_PLACE,HeeksObjDlg::OnComboOrCheck)
    EVT_COMBOBOX(ID_CUT_MODE,HeeksObjDlg::OnComboOrCheck)
    EVT_CHECKBOX(ID_KEEP_TOOL_DOWN, HeeksObjDlg::OnComboOrCheck)
    EVT_CHECKBOX(ID_USE_ZIG_ZAG, PocketDlg::OnCheckUseZigZag)
    EVT_CHECKBOX(ID_ZIG_UNIDIRECTIONAL, HeeksObjDlg::OnComboOrCheck)
    EVT_BUTTON(wxID_HELP, PocketDlg::OnHelp)
END_EVENT_TABLE()

PocketDlg::PocketDlg(wxWindow *parent, CPocket* object, const wxString& title, bool top_level)
             : SketchOpDlg(parent, object, title, false)
{
	std::list<HControl> save_leftControls = leftControls;
	leftControls.clear();

	// add all the controls to the left side
	leftControls.push_back(MakeLabelAndControl(_("Step Over"), m_lgthStepOver = new CLengthCtrl(this)));
	leftControls.push_back(MakeLabelAndControl(_("Material Allowance"), m_lgthMaterialAllowance = new CLengthCtrl(this)));

	wxString starting_place_choices[] = {_("Boundary"), _("Center")};
	leftControls.push_back(MakeLabelAndControl(_("Starting Place"), m_cmbStartingPlace = new wxComboBox(this, ID_STARTING_PLACE, _T(""), wxDefaultPosition, wxDefaultSize, 2, starting_place_choices)));

	wxString cut_mode_choices[] = {_("Conventional"), _("Climb")};
	leftControls.push_back(MakeLabelAndControl(_("Cut Mode"), m_cmbCutMode = new wxComboBox(this, ID_CUT_MODE, _T(""), wxDefaultPosition, wxDefaultSize, 2, cut_mode_choices)));

	leftControls.push_back( HControl( m_chkKeepToolDown = new wxCheckBox( this, ID_KEEP_TOOL_DOWN, _("Keep Tool Down") ), wxALL ));
	leftControls.push_back( HControl( m_chkUseZigZag = new wxCheckBox( this, ID_USE_ZIG_ZAG, _("Use Zig Zag") ), wxALL ));
	leftControls.push_back(MakeLabelAndControl(_("Zig Zag Angle"), m_dblZigAngle = new CDoubleCtrl(this)));
	leftControls.push_back( HControl( m_chkZigUnidirectional = new wxCheckBox( this, ID_ZIG_UNIDIRECTIONAL, _("Zig Unidirectional") ), wxALL ));

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

void PocketDlg::GetDataRaw(HeeksObj* object)
{
	((CPocket*)object)->m_pocket_params.m_step_over = m_lgthStepOver->GetValue();
	((CPocket*)object)->m_pocket_params.m_material_allowance = m_lgthMaterialAllowance->GetValue();
	((CPocket*)object)->m_pocket_params.m_starting_place = (m_cmbStartingPlace->GetValue().CmpNoCase(_("Center")) == 0) ? 1 : 0;
	((CPocket*)object)->m_pocket_params.m_cut_mode = (m_cmbCutMode->GetValue().CmpNoCase(_("climb")) == 0) ? CPocketParams::eClimb : CPocketParams::eConventional;
	((CPocket*)object)->m_pocket_params.m_keep_tool_down_if_poss = m_chkKeepToolDown->GetValue();
	((CPocket*)object)->m_pocket_params.m_use_zig_zag = m_chkUseZigZag->GetValue();
	if(((CPocket*)object)->m_pocket_params.m_use_zig_zag)((CPocket*)object)->m_pocket_params.m_zig_angle = m_dblZigAngle->GetValue();
	if(((CPocket*)object)->m_pocket_params.m_use_zig_zag)((CPocket*)object)->m_pocket_params.m_zig_unidirectional = m_chkZigUnidirectional->GetValue();

	SketchOpDlg::GetDataRaw(object);
}

void PocketDlg::SetFromDataRaw(HeeksObj* object)
{
	m_lgthStepOver->SetValue(((CPocket*)object)->m_pocket_params.m_step_over);
	m_lgthMaterialAllowance->SetValue(((CPocket*)object)->m_pocket_params.m_material_allowance);
	m_cmbStartingPlace->SetValue((((CPocket*)object)->m_pocket_params.m_starting_place == 1) ? _("Center") : _("Boundary"));
	m_cmbCutMode->SetValue((((CPocket*)object)->m_pocket_params.m_cut_mode == CPocketParams::eClimb) ? _("Climb") : _("Conventional"));

	m_chkKeepToolDown->SetValue(((CPocket*)object)->m_pocket_params.m_keep_tool_down_if_poss);
	m_chkUseZigZag->SetValue(((CPocket*)object)->m_pocket_params.m_use_zig_zag);
	if(((CPocket*)object)->m_pocket_params.m_use_zig_zag) m_dblZigAngle->SetValue(((CPocket*)object)->m_pocket_params.m_zig_angle);
	if(((CPocket*)object)->m_pocket_params.m_use_zig_zag) m_chkZigUnidirectional->SetValue(((CPocket*)object)->m_pocket_params.m_zig_unidirectional);

	EnableZigZagControls();

	SketchOpDlg::SetFromDataRaw(object);
}

void PocketDlg::SetPicture(const wxString& name)
{
	HeeksObjDlg::SetPicture(name, _T("pocket"));
}

void PocketDlg::SetPictureByWindow(wxWindow* w)
{
	if(w == m_lgthStepOver)SetPicture(_T("step over"));
	else if(w == m_lgthMaterialAllowance)SetPicture(_T("material allowance"));
	else if(w == m_cmbStartingPlace)
	{
		if(m_cmbStartingPlace->GetValue() == _("Boundary"))SetPicture(_T("starting boundary"));
		else SetPicture(_T("starting center"));
	}
	else if(w == m_cmbCutMode)
	{
		if(m_cmbCutMode->GetValue() == _("Climb"))SetPicture(_T("climb milling"));
		else SetPicture(_T("conventional milling"));
	}
	else if(w == m_chkKeepToolDown)
	{
		if(m_chkKeepToolDown->GetValue())SetPicture(_T("tool down"));
		else SetPicture(_T("not tool down"));
	}
	else if(w == m_chkUseZigZag || w == m_chkZigUnidirectional)
	{
		if(m_chkUseZigZag->GetValue())
		{
			if(m_chkZigUnidirectional->GetValue())SetPicture(_T("zig unidirectional"));
			else SetPicture(_T("use zig zag"));
		}
		else SetPicture(_T("general"));
	}
	else if(w == m_dblZigAngle)SetPicture(_T("zig angle"));
	else SketchOpDlg::SetPictureByWindow(w);
}

void PocketDlg::OnCheckUseZigZag(wxCommandEvent& event)
{
	if(m_ignore_event_functions)return;
	EnableZigZagControls();
	HeeksObjDlg::SetPicture();
}

void PocketDlg::EnableZigZagControls()
{
	bool enable = m_chkUseZigZag->GetValue();

	m_dblZigAngle->Enable(enable);
	m_chkZigUnidirectional->Enable(enable);
}

void PocketDlg::OnHelp( wxCommandEvent& event )
{
	::wxLaunchDefaultBrowser(_T("http://heeks.net/help/pocket"));
}

bool PocketDlg::Do(CPocket* object)
{
	PocketDlg dlg(heeksCAD->GetMainFrame(), object);

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
