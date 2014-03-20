// OpDlg.cpp
// Copyright (c) 2010, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include "OpDlg.h"
#include "../../interface/PictureFrame.h"
#include "../../interface/NiceTextCtrl.h"
#include "Profile.h"
#include "CTool.h"
#include "Program.h"
#include "Tools.h"
#include "Patterns.h"
#include "Surfaces.h"
#include "PatternDlg.h"

BEGIN_EVENT_TABLE(OpDlg, HeeksObjDlg)
    EVT_COMBOBOX(ID_TOOL,HeeksObjDlg::OnComboOrCheck)
    EVT_COMBOBOX(ID_PATTERN,HeeksObjDlg::OnComboOrCheck)
    EVT_COMBOBOX(ID_SURFACE,HeeksObjDlg::OnComboOrCheck)
END_EVENT_TABLE()

OpDlg::OpDlg(wxWindow *parent, COp* object, const wxString& title, bool top_level, bool want_tool_control, bool picture)
             : HeeksObjDlg(parent, object, title, false, picture)
{
	if(want_tool_control)leftControls.push_back(MakeLabelAndControl(_("Tool"), m_cmbTool = new HTypeObjectDropDown(this, ID_TOOL, ToolType, theApp.m_program->Tools())));
	else m_cmbTool = NULL;
	leftControls.push_back(MakeLabelAndControl(_("Pattern"), m_cmbPattern = new HTypeObjectDropDown(this, ID_PATTERN, PatternType, theApp.m_program->Patterns())));
	leftControls.push_back(MakeLabelAndControl(_("Surface"), m_cmbSurface = new HTypeObjectDropDown(this, ID_SURFACE, SurfaceType, theApp.m_program->Surfaces())));

	// add some of the controls to the right side
	rightControls.push_back(MakeLabelAndControl(_("Comment"), m_txtComment = new wxTextCtrl(this, wxID_ANY)));
	rightControls.push_back(HControl(m_chkActive = new wxCheckBox( this, wxID_ANY, _("Active") ), wxALL));
	rightControls.push_back(MakeLabelAndControl(_("Title"), m_txtTitle = new wxTextCtrl(this, wxID_ANY)));

	if(top_level)
	{
		HeeksObjDlg::AddControlsAndCreate();
		if(m_cmbTool)m_cmbTool->SetFocus();
		else m_cmbPattern->SetFocus();
	}
}

void OpDlg::GetDataRaw(HeeksObj* object)
{
	((COp*)object)->m_comment = m_txtComment->GetValue();
	((COp*)object)->m_active = m_chkActive->GetValue();
	((COp*)object)->m_title = m_txtTitle->GetValue();
	if(((COp*)object)->m_title != wxString(((COp*)object)->GetShortString()))((COp*)object)->m_title_made_from_id = false;
	if(m_cmbTool)((COp*)object)->m_tool_number = m_cmbTool->GetSelectedId();
	else ((COp*)object)->m_tool_number = 0;
	((COp*)object)->m_pattern = m_cmbPattern->GetSelectedId();
	((COp*)object)->m_surface = m_cmbSurface->GetSelectedId();

	HeeksObjDlg::GetDataRaw(object);
}

void OpDlg::SetFromDataRaw(HeeksObj* object)
{
	m_txtComment->SetValue(((COp*)object)->m_comment);
	m_chkActive->SetValue(((COp*)object)->m_active);
	m_txtTitle->SetValue(((COp*)object)->GetShortString());
	if(m_cmbTool)m_cmbTool->SelectById(((COp*)object)->m_tool_number);
	m_cmbPattern->SelectById(((COp*)object)->m_pattern);
	m_cmbSurface->SelectById(((COp*)object)->m_surface);

	HeeksObjDlg::SetFromDataRaw(object);
}

void OpDlg::SetPicture(const wxString& name)
{
	HeeksObjDlg::SetPicture(name, _T("op"));
}

void OpDlg::SetPictureByWindow(wxWindow* w)
{
	if(w == m_cmbPattern)
	{
		int pattern_id = m_cmbPattern->GetSelectedId();
		if(pattern_id != 0)
		{
			PatternDlg::RedrawBitmap(m_picture->m_bitmap, (CPattern*)(heeksCAD->GetIDObject(PatternType, pattern_id)));
			m_picture->m_bitmap_set = true;
			m_picture->Refresh();
		}
		else SetPicture(_T("general"));
	}
	else if(w == m_cmbSurface)HeeksObjDlg::SetPicture(_T("general"), _T("surface"));
	else SetPicture(_T("general"));
}
