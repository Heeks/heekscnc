// CToolDlg.cpp
// Copyright (c) 2014, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include "CToolDlg.h"
#include "../../interface/NiceTextCtrl.h"

enum
{
    ID_TITLE_TYPE = 100,
    ID_TOOL_TYPE,
    ID_MATERIAL,
    ID_DIRECTION,
};

BEGIN_EVENT_TABLE(CToolDlg, HeeksObjDlg)
    EVT_COMBOBOX(ID_TITLE_TYPE,CToolDlg::OnComboTitleType)
    EVT_COMBOBOX(ID_TOOL_TYPE, CToolDlg::OnComboToolType)
    EVT_COMBOBOX(ID_MATERIAL, CToolDlg::OnComboMaterial)
    EVT_TEXT(wxID_ANY,CToolDlg::OnTextCtrlEvent)
    EVT_BUTTON(wxID_HELP, CToolDlg::OnHelp)
END_EVENT_TABLE()

CToolDlg::CToolDlg(wxWindow *parent, CTool* object, const wxString& title, bool top_level)
             : HeeksObjDlg(parent, object, title, false)
{
	// add some of the controls to the right side
	rightControls.push_back(MakeLabelAndControl(_("Title"), m_txtTitle = new wxTextCtrl(this, wxID_ANY)));
	wxString title_choices[] = {_("Leave manually assigned title"), _("Automatically Generate Title")};
	rightControls.push_back(MakeLabelAndControl(_("Title Type"), m_cmbTitleType = new wxComboBox(this, ID_TITLE_TYPE, _T(""), wxDefaultPosition, wxDefaultSize, 2, title_choices)));

	// add all the controls to the left side
	leftControls.push_back(MakeLabelAndControl(_("Tool Number"), m_dlbToolNumber = new wxTextCtrl(this, wxID_ANY)));
	wxString materials[] = {_("High Speed Steel"),_("Carbide") };
	leftControls.push_back(MakeLabelAndControl(_("Tool Material"), m_cmbMaterial = new wxComboBox(this, ID_MATERIAL, _T(""), wxDefaultPosition, wxDefaultSize, 2, materials)));
	wxString tool_types[] = {_("Drill Bit"), _("Centre Drill Bit"), _("End Mill"), _("Slot Cutter"), _("Ball End Mill"), _("Chamfer"), _("Engraving Bit")};
	leftControls.push_back(MakeLabelAndControl(_("Tool Type"), m_cmbToolType = new wxComboBox(this, ID_TOOL_TYPE, _T(""), wxDefaultPosition, wxDefaultSize, sizeof(tool_types)/sizeof(CToolParams::eToolType), tool_types)));
	leftControls.push_back(MakeLabelAndControl(_("Diameter"), m_dblDiameter = new CLengthCtrl(this)));
	leftControls.push_back(MakeLabelAndControl(_("Tool Length Offset"), m_dblToolLengthOffset = new CLengthCtrl(this)));
	leftControls.push_back(MakeLabelAndControl(_("Flat Radius"), m_dblFlatRadius = new CLengthCtrl(this)));
	leftControls.push_back(MakeLabelAndControl(_("Corner Radius"), m_dblCornerRadius = new CLengthCtrl(this)));
	leftControls.push_back(MakeLabelAndControl(_("Cutting Edge Angle"), m_dblCuttingEdgeAngle = new CDoubleCtrl(this)));
	leftControls.push_back(MakeLabelAndControl(_("Cutting Edge Height"), m_dblCuttingEdgeHeight = new CLengthCtrl(this)));

	if(top_level)
	{
		HeeksObjDlg::AddControlsAndCreate();
		m_dblDiameter->SetFocus();
	}
}

void CToolDlg::GetDataRaw(HeeksObj* object)
{
	long i = 0;
	m_dlbToolNumber->GetValue().ToLong(&i);
	((CTool*)object)->m_tool_number = i;
	((CTool*)object)->m_params.m_material = m_cmbMaterial->GetSelection();
	((CTool*)object)->m_params.m_type = (CToolParams::eToolType)(m_cmbToolType->GetSelection());
	((CTool*)object)->m_params.m_diameter = m_dblDiameter->GetValue();
	((CTool*)object)->m_params.m_tool_length_offset = m_dblToolLengthOffset->GetValue();	
	((CTool*)object)->m_params.m_flat_radius = m_dblFlatRadius->GetValue();
	((CTool*)object)->m_params.m_corner_radius = m_dblCornerRadius->GetValue();
	((CTool*)object)->m_params.m_cutting_edge_angle = m_dblCuttingEdgeAngle->GetValue();
	((CTool*)object)->m_params.m_cutting_edge_height = m_dblCuttingEdgeHeight->GetValue();
	((CTool*)object)->m_title = m_txtTitle->GetValue();
	((CTool*)object)->m_params.m_automatically_generate_title = (m_cmbTitleType->GetSelection() != 0);
}

void CToolDlg::SetFromDataRaw(HeeksObj* object)
{
	m_dlbToolNumber->SetValue(wxString::Format(_T("%d"), ((CTool*)object)->m_tool_number));
	m_txtTitle->SetValue(((CTool*)object)->m_title);
	m_cmbMaterial->SetSelection(((CTool*)object)->m_params.m_material);
	m_cmbToolType->SetSelection(((CTool*)object)->m_params.m_type);
	m_dblDiameter->SetValue(((CTool*)object)->m_params.m_diameter);
	m_dblToolLengthOffset->SetValue(((CTool*)object)->m_params.m_tool_length_offset);
	m_dblCuttingEdgeHeight->SetValue(((CTool*)object)->m_params.m_cutting_edge_height);
	m_txtTitle->SetValue(((CTool*)object)->m_title);
	m_cmbTitleType->SetSelection(((CTool*)object)->m_params.m_automatically_generate_title ? 1:0);

	EnableAndSetCornerFlatAndAngle(((CTool*)object)->m_params.m_type);
}

void CToolDlg::SetPicture(const wxString& name)
{
	HeeksObjDlg::SetPicture(name, _T("ctool"));
}

void CToolDlg::SetPictureByWindow(wxWindow* w)
{
	CToolParams::eToolType type = (CToolParams::eToolType)(m_cmbToolType->GetSelection());

	switch(type)
	{
		case CToolParams::eDrill:
			if(w == m_dblDiameter)SetPicture(_T("drill_diameter"));
			else if(w == m_dblToolLengthOffset)SetPicture(_T("drill_offset"));
			else if(w == m_dblFlatRadius)SetPicture(_T("drill_flat"));
			else if(w == m_dblCornerRadius)SetPicture(_T("drill_corner"));
			else if(w == m_dblCuttingEdgeAngle)SetPicture(_T("drill_angle"));
			else if(w == m_dblCuttingEdgeHeight)SetPicture(_T("drill_height"));
			else SetPicture(_T("drill"));
			break;
		case CToolParams::eCentreDrill:
			if(w == m_dblDiameter)SetPicture(_T("centre_drill_diameter"));
			else if(w == m_dblToolLengthOffset)SetPicture(_T("centre_drill_offset"));
			else if(w == m_dblFlatRadius)SetPicture(_T("centre_drill_flat"));
			else if(w == m_dblCornerRadius)SetPicture(_T("centre_drill_corner"));
			else if(w == m_dblCuttingEdgeAngle)SetPicture(_T("centre_drill_angle"));
			else if(w == m_dblCuttingEdgeHeight)SetPicture(_T("centre_drill_height"));
			else SetPicture(_T("centre_drill"));
			break;
		case CToolParams::eEndmill:
		case CToolParams::eSlotCutter:
			if(w == m_dblDiameter)SetPicture(_T("end_mill_diameter"));
			else if(w == m_dblToolLengthOffset)SetPicture(_T("end_mill_offset"));
			else if(w == m_dblFlatRadius)SetPicture(_T("end_mill_flat"));
			else if(w == m_dblCornerRadius)SetPicture(_T("end_mill_corner"));
			else if(w == m_dblCuttingEdgeAngle)SetPicture(_T("end_mill_angle"));
			else if(w == m_dblCuttingEdgeHeight)SetPicture(_T("end_mill_height"));
			else SetPicture(_T("end_mill"));
			break;
		case CToolParams::eBallEndMill:
			if(w == m_dblDiameter)SetPicture(_T("ball_mill_diameter"));
			else if(w == m_dblToolLengthOffset)SetPicture(_T("ball_mill_offset"));
			else if(w == m_dblFlatRadius)SetPicture(_T("ball_mill_flat"));
			else if(w == m_dblCornerRadius)SetPicture(_T("ball_mill_corner"));
			else if(w == m_dblCuttingEdgeAngle)SetPicture(_T("ball_mill_angle"));
			else if(w == m_dblCuttingEdgeHeight)SetPicture(_T("ball_mill_height"));
			else SetPicture(_T("ball_mill"));
			break;
		case CToolParams::eChamfer:
			if(w == m_dblDiameter)SetPicture(_T("chamfer_diameter"));
			else if(w == m_dblToolLengthOffset)SetPicture(_T("chamfer_offset"));
			else if(w == m_dblFlatRadius)SetPicture(_T("chamfer_flat"));
			else if(w == m_dblCornerRadius)SetPicture(_T("chamfer_corner"));
			else if(w == m_dblCuttingEdgeAngle)SetPicture(_T("chamfer_angle"));
			else if(w == m_dblCuttingEdgeHeight)SetPicture(_T("chamfer_height"));
			else SetPicture(_T("chamfer"));
			break;
		case CToolParams::eEngravingTool:
			if(w == m_dblDiameter)SetPicture(_T("engraver_diameter"));
			else if(w == m_dblToolLengthOffset)SetPicture(_T("engraver_offset"));
			else if(w == m_dblFlatRadius)SetPicture(_T("engraver_flat"));
			else if(w == m_dblCornerRadius)SetPicture(_T("engraver_corner"));
			else if(w == m_dblCuttingEdgeAngle)SetPicture(_T("engraver_angle"));
			else if(w == m_dblCuttingEdgeHeight)SetPicture(_T("engraver_height"));
			else SetPicture(_T("engraver"));
			break;
		default:
			SetPicture(_T("undefined"));
			break;
	}
}

void CToolDlg::OnComboTitleType( wxCommandEvent& event )
{
	if(m_ignore_event_functions)return;
//	SetPicture();
}

void CToolDlg::OnComboMaterial( wxCommandEvent& event )
{
	if(m_ignore_event_functions)return;
//	//SetPicture();
}

void CToolDlg::OnTextCtrlEvent(wxCommandEvent& event)
{
	if(m_ignore_event_functions)return;

	// something's changed, recalculate the title
	CTool* object = (CTool*)(m_object->MakeACopy());
	GetData(object);
	m_ignore_event_functions = true;
	object->ResetTitle();
	m_txtTitle->SetValue(object->m_title);
	delete object;

	m_ignore_event_functions = false;
}

void CToolDlg::OnComboToolType(wxCommandEvent& event)
{
	if(m_ignore_event_functions)return;
	CToolParams::eToolType type = (CToolParams::eToolType)(m_cmbToolType->GetSelection());
	EnableAndSetCornerFlatAndAngle(type);
	HeeksObjDlg::SetPicture();
}

void CToolDlg::EnableAndSetCornerFlatAndAngle(CToolParams::eToolType type)
{
	switch(type)
	{
		case CToolParams::eDrill:
		case CToolParams::eCentreDrill:
			m_dblCornerRadius->Enable(false);
			m_dblCornerRadius->SetLabel(_T(""));
			m_dblFlatRadius->Enable(false);
			m_dblFlatRadius->SetLabel(_T(""));
			m_dblCuttingEdgeAngle->Enable();
			m_dblCuttingEdgeAngle->SetValue(((CTool*)m_object)->m_params.m_cutting_edge_angle);
			break;
		case CToolParams::eEndmill:
		case CToolParams::eSlotCutter:
			m_dblCornerRadius->Enable();
			m_dblCornerRadius->SetValue(((CTool*)m_object)->m_params.m_corner_radius);
			m_dblFlatRadius->Enable(false);
			m_dblFlatRadius->SetLabel(_T(""));
			m_dblCuttingEdgeAngle->Enable(false);
			m_dblCuttingEdgeAngle->SetLabel(_T(""));
			break;
		case CToolParams::eBallEndMill:
			m_dblCornerRadius->Enable(false);
			m_dblCornerRadius->SetLabel(_T(""));
			m_dblFlatRadius->Enable(false);
			m_dblFlatRadius->SetLabel(_T(""));
			m_dblCuttingEdgeAngle->Enable(false);
			m_dblCuttingEdgeAngle->SetLabel(_T(""));
			break;
		case CToolParams::eChamfer:
		case CToolParams::eEngravingTool:
			m_dblCornerRadius->Enable(false);
			m_dblCornerRadius->SetLabel(_T(""));
			m_dblFlatRadius->Enable();
			m_dblFlatRadius->SetValue(((CTool*)m_object)->m_params.m_flat_radius);
			m_dblCuttingEdgeAngle->Enable();
			m_dblCuttingEdgeAngle->SetValue(((CTool*)m_object)->m_params.m_cutting_edge_angle);
			break;
		default:
			break;
	}
}

void CToolDlg::OnHelp( wxCommandEvent& event )
{
	::wxLaunchDefaultBrowser(_T("http://heeks.net/help/tool"));
}
