// SurfaceDlg.cpp
// Copyright (c) 2014, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include "Surface.h"
#include "SurfaceDlg.h"
#include "interface/NiceTextCtrl.h"

enum
{
	ID_SOLIDS = 100,
	ID_TOLERANCE,
	ID_MIN_Z,
	ID_MATERIAL_ALLOWANCE,
	ID_SAME_FOR_EACH_POSITION,
};

BEGIN_EVENT_TABLE(SurfaceDlg, HeeksObjDlg)
    EVT_CHECKBOX(ID_SAME_FOR_EACH_POSITION, HeeksObjDlg::OnComboOrCheck)
    EVT_BUTTON(wxID_HELP, SurfaceDlg::OnHelp)
END_EVENT_TABLE()

SurfaceDlg::SurfaceDlg(wxWindow *parent, HeeksObj* object, const wxString& title, bool top_level)
             : HeeksObjDlg(parent, object, title, false)
{
	// add all the controls to the left side
	leftControls.push_back(MakeLabelAndControl(_("solids"), m_idsSolids = new CObjectIdsCtrl(this)));
	leftControls.push_back(MakeLabelAndControl(_("tolerance"), m_lgthTolerance = new CLengthCtrl(this)));
	leftControls.push_back(MakeLabelAndControl(_("minimum Z"), m_lgthMinZ = new CLengthCtrl(this)));
	leftControls.push_back(MakeLabelAndControl(_("material allowance"), m_lgthMaterialAllowance = new CLengthCtrl(this)));
	leftControls.push_back( HControl( m_chkSameForEachPosition = new wxCheckBox( this, ID_SAME_FOR_EACH_POSITION, _("same for each pattern position") ), wxALL ));

	if(top_level)
	{
		HeeksObjDlg::AddControlsAndCreate();
		m_idsSolids->SetFocus();
	}
}

void SurfaceDlg::GetDataRaw(HeeksObj* object)
{
	((CSurface*)object)->m_solids.clear();
	m_idsSolids->GetIDList(((CSurface*)object)->m_solids);
	((CSurface*)object)->m_tolerance = m_lgthTolerance->GetValue();
	((CSurface*)object)->m_min_z = m_lgthMinZ->GetValue();
	((CSurface*)object)->m_material_allowance = m_lgthMaterialAllowance->GetValue();
	((CSurface*)object)->m_same_for_each_pattern_position = m_chkSameForEachPosition->GetValue();
}

void SurfaceDlg::SetFromDataRaw(HeeksObj* object)
{
	m_idsSolids->SetFromIDList(((CSurface*)object)->m_solids);
	m_lgthTolerance->SetValue(((CSurface*)object)->m_tolerance);
	m_lgthMinZ->SetValue(((CSurface*)object)->m_min_z);
	m_lgthMaterialAllowance->SetValue(((CSurface*)object)->m_material_allowance);
	m_chkSameForEachPosition->SetValue(((CSurface*)object)->m_same_for_each_pattern_position != 0);
}

void SurfaceDlg::SetPicture(const wxString& name)
{
	HeeksObjDlg::SetPicture(name, _T("surface"));
}

void SurfaceDlg::SetPictureByWindow(wxWindow* w)
{
	if(w == m_lgthTolerance)SetPicture(_T("tolerance"));
	else if(w == m_lgthMinZ)SetPicture(_T("min z"));
	else if(w == m_lgthMaterialAllowance)SetPicture(_T("material allowance"));
	else if(w == m_chkSameForEachPosition)
	{
		if(m_chkSameForEachPosition->GetValue())SetPicture(_T("same for each position"));
		else SetPicture(_T("different for each position"));
	}
	else SetPicture(_T("general"));
}

void SurfaceDlg::OnHelp( wxCommandEvent& event )
{
	::wxLaunchDefaultBrowser(_T("http://heeks.net/surface"));
}
