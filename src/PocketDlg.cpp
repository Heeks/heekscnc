// PocketDlg.cpp
// Copyright (c) 2014, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include "PocketDlg.h"
#include "interface/NiceTextCtrl.h"
#include "Pocket.h"

enum
{
	ID_SKETCH = 100,
	ID_STARTING_PLACE,
	ID_CUT_MODE,
	ID_KEEP_TOOL_DOWN,
	ID_USE_ZIG_ZAG,
	ID_ZIG_UNIDIRECTIONAL,
};

BEGIN_EVENT_TABLE(PocketDlg, DepthOpDlg)
    EVT_COMBOBOX(ID_SKETCH,HeeksObjDlg::OnComboOrCheck)
    EVT_COMBOBOX(ID_STARTING_PLACE,HeeksObjDlg::OnComboOrCheck)
    EVT_COMBOBOX(ID_CUT_MODE,HeeksObjDlg::OnComboOrCheck)
    EVT_CHECKBOX(ID_KEEP_TOOL_DOWN, HeeksObjDlg::OnComboOrCheck)
    EVT_CHECKBOX(ID_USE_ZIG_ZAG, PocketDlg::OnCheckUseZigZag)
    EVT_CHECKBOX(ID_ZIG_UNIDIRECTIONAL, HeeksObjDlg::OnComboOrCheck)
END_EVENT_TABLE()

PocketDlg::PocketDlg(wxWindow *parent, CPocket* object, const wxString& title, bool top_level)
             : DepthOpDlg(parent, object, title, false)
{
	std::list<HControl> save_leftControls = leftControls;
	leftControls.clear();

	// add all the controls to the left side
	leftControls.push_back(MakeLabelAndControl(_("sketches"), m_cmbSketch = new HTypeObjectDropDown(this, ID_SKETCH, SketchType, heeksCAD->GetMainObject())));
	leftControls.push_back(MakeLabelAndControl(_("step over"), m_lgthStepOver = new CLengthCtrl(this)));
	leftControls.push_back(MakeLabelAndControl(_("material allowance"), m_lgthMaterialAllowance = new CLengthCtrl(this)));

	wxString starting_place_choices[] = {_("boundary"), _("center")};
	leftControls.push_back(MakeLabelAndControl(_("starting place"), m_cmbStartingPlace = new wxComboBox(this, ID_STARTING_PLACE, _T(""), wxDefaultPosition, wxDefaultSize, 2, starting_place_choices)));

	wxString cut_mode_choices[] = {_("conventional"), _("climb")};
	leftControls.push_back(MakeLabelAndControl(_("cut mode"), m_cmbCutMode = new wxComboBox(this, ID_CUT_MODE, _T(""), wxDefaultPosition, wxDefaultSize, 2, cut_mode_choices)));

	leftControls.push_back( HControl( m_chkUseZigZag = new wxCheckBox( this, ID_USE_ZIG_ZAG, _("use zig zag") ), wxALL ));
	leftControls.push_back( HControl( m_chkKeepToolDown = new wxCheckBox( this, ID_KEEP_TOOL_DOWN, _("keep tool down") ), wxALL ));
	leftControls.push_back(MakeLabelAndControl(_("zig zag angle"), m_dblZigAngle = new CDoubleCtrl(this)));
	leftControls.push_back( HControl( m_chkZigUnidirectional = new wxCheckBox( this, ID_ZIG_UNIDIRECTIONAL, _("zig unidirectional") ), wxALL ));

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
	((CPocket*)object)->m_sketches.clear();
	int sketch = m_cmbSketch->GetSelectedId();
	if(sketch != 0)((CPocket*)object)->m_sketches.push_back(sketch);
	((CPocket*)object)->m_pocket_params.m_material_allowance = m_lgthMaterialAllowance->GetValue();
	((CPocket*)object)->m_pocket_params.m_starting_place = m_cmbStartingPlace->GetValue() ? 1:0;
	((CPocket*)object)->m_pocket_params.m_cut_mode = (m_cmbCutMode->GetValue().CmpNoCase(_("climb")) == 0) ? CPocketParams::eClimb : CPocketParams::eConventional;
	((CPocket*)object)->m_pocket_params.m_keep_tool_down_if_poss = m_chkKeepToolDown->GetValue();
	((CPocket*)object)->m_pocket_params.m_use_zig_zag = m_chkUseZigZag->GetValue();
	if(((CPocket*)object)->m_pocket_params.m_use_zig_zag)((CPocket*)object)->m_pocket_params.m_zig_angle = m_dblZigAngle->GetValue();

	DepthOpDlg::GetDataRaw(object);
}

void PocketDlg::SetFromDataRaw(HeeksObj* object)
{
	if(((CPocket*)object)->m_sketches.size() > 0)
		m_cmbSketch->SelectById(((CPocket*)object)->m_sketches.front());
	else
		m_cmbSketch->SelectById(0);
	m_lgthStepOver->SetValue(((CPocket*)object)->m_pocket_params.m_step_over);
	m_lgthMaterialAllowance->SetValue(((CPocket*)object)->m_pocket_params.m_material_allowance);
	m_cmbStartingPlace->SetValue((((CPocket*)object)->m_pocket_params.m_starting_place == 0) ? _("boundary") : _("center"));
	m_cmbCutMode->SetValue((((CPocket*)object)->m_pocket_params.m_cut_mode == CPocketParams::eClimb) ? _("climb") : _("conventional"));

	m_chkKeepToolDown->SetValue(((CPocket*)object)->m_pocket_params.m_keep_tool_down_if_poss);
	m_chkUseZigZag->SetValue(((CPocket*)object)->m_pocket_params.m_use_zig_zag);
	if(((CPocket*)object)->m_pocket_params.m_use_zig_zag)m_dblZigAngle->SetValue(((CPocket*)object)->m_pocket_params.m_zig_angle);

	EnableZigZagControls();

	DepthOpDlg::SetFromDataRaw(object);
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
		if(m_cmbStartingPlace->GetValue() == _("boundary"))SetPicture(_T("starting boundary"));
		else SetPicture(_T("starting center"));
	}
	else if(w == m_cmbCutMode)
	{
		if(m_cmbCutMode->GetValue() == _("climb"))SetPicture(_T("climb milling"));
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
	else DepthOpDlg::SetPictureByWindow(w);
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
