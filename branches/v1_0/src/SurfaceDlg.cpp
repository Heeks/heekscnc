// SurfaceDlg.cpp
// Copyright (c) 2014, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include "Surface.h"
#include "SurfaceDlg.h"
#include "interface/NiceTextCtrl.h"

BEGIN_EVENT_TABLE(SurfaceDlg, SolidsDlg)
    EVT_CHECKBOX(ID_SAME_FOR_EACH_POSITION, HeeksObjDlg::OnComboOrCheck)
    EVT_BUTTON(wxID_HELP, SurfaceDlg::OnHelp)
END_EVENT_TABLE()

SurfaceDlg::SurfaceDlg(wxWindow *parent, HeeksObj* object, const wxString& title, bool top_level)
             : SolidsDlg(parent, object, title, false)
{
	// add all the controls to the left side
	leftControls.push_back(MakeLabelAndControl(_("Tolerance"), m_lgthTolerance = new CLengthCtrl(this)));
	leftControls.push_back(MakeLabelAndControl(_("Material Allowance"), m_lgthMaterialAllowance = new CLengthCtrl(this)));
	leftControls.push_back( HControl( m_chkSameForEachPosition = new wxCheckBox( this, ID_SAME_FOR_EACH_POSITION, _("Same for Each Pattern Position") ), wxALL ));

	if(top_level)
	{
		HeeksObjDlg::AddControlsAndCreate();
		m_idsSolids->SetFocus();
	}
}

void SurfaceDlg::GetDataRaw(HeeksObj* object)
{
	((CSurface*)object)->m_tolerance = m_lgthTolerance->GetValue();
	((CSurface*)object)->m_material_allowance = m_lgthMaterialAllowance->GetValue();
	((CSurface*)object)->m_same_for_each_pattern_position = m_chkSameForEachPosition->GetValue();
	SolidsDlg::GetDataRaw(object);
}

void SurfaceDlg::SetFromDataRaw(HeeksObj* object)
{
	m_lgthTolerance->SetValue(((CSurface*)object)->m_tolerance);
	m_lgthMaterialAllowance->SetValue(((CSurface*)object)->m_material_allowance);
	m_chkSameForEachPosition->SetValue(((CSurface*)object)->m_same_for_each_pattern_position != 0);
	SolidsDlg::SetFromDataRaw(object);
}

void SurfaceDlg::SetPicture(const wxString& name)
{
	HeeksObjDlg::SetPicture(name, _T("surface"));
}

void SurfaceDlg::SetPictureByWindow(wxWindow* w)
{
	if(w == m_lgthTolerance)SetPicture(_T("tolerance"));
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
	::wxLaunchDefaultBrowser(_T("http://heeks.net/help/surface"));
}

bool SurfaceDlg::Do(CSurface* object)
{
	SurfaceDlg dlg(heeksCAD->GetMainFrame(), object);

	while(1)
	{
		int result = dlg.ShowModal();

		if(result == wxID_OK)
		{
			dlg.GetData(object);
			return true;
		}
		else if(result == ID_SOLIDS_PICK)
		{
			dlg.PickSolids();
		}
		else
		{
			return false;
		}
	}
}