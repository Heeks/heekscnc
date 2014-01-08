// CToolDlg.cpp
// Copyright (c) 2010, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include "CToolDlg.h"
#include "interface/PictureFrame.h"
#include "interface/NiceTextCtrl.h"

enum
{
    ID_TITLE_TYPE = 100,
    ID_TOOL_TYPE,
    ID_MATERIAL,
    ID_DIRECTION,
};

BEGIN_EVENT_TABLE(CToolDlg, HDialog)
    EVT_CHILD_FOCUS(CToolDlg::OnChildFocus)
    EVT_COMBOBOX(ID_TITLE_TYPE,CToolDlg::OnComboTitleType)
    EVT_COMBOBOX(ID_TOOL_TYPE, CToolDlg::OnComboToolType)
    EVT_COMBOBOX(ID_MATERIAL, CToolDlg::OnComboMaterial)
    EVT_TEXT(wxID_ANY,CToolDlg::OnTextCtrlEvent)
END_EVENT_TABLE()

CToolDlg::CToolDlg(wxWindow *parent, CTool* object)
             : HDialog(parent, wxID_ANY, wxString(_T("Tool Definition")))
{
	m_ptool = object;
	m_ignore_event_functions = true;
    wxBoxSizer *sizerMain = new wxBoxSizer(wxHORIZONTAL);

	// add left sizer
    wxBoxSizer *sizerLeft = new wxBoxSizer(wxVERTICAL);
    sizerMain->Add( sizerLeft, 0, wxALL, control_border );

	// add right sizer
    wxBoxSizer *sizerRight = new wxBoxSizer(wxVERTICAL);
    sizerMain->Add( sizerRight, 0, wxALL, control_border );

	// add picture to right side
	m_picture = new PictureWindow(this, wxSize(300, 200));
	wxBoxSizer *pictureSizer = new wxBoxSizer(wxVERTICAL);
	pictureSizer->Add(m_picture, 1, wxGROW);
    sizerRight->Add( pictureSizer, 0, wxALL, control_border );

	// add some of the controls to the right side
	AddLabelAndControl(sizerRight, _("Title"), m_txtTitle = new wxTextCtrl(this, wxID_ANY));
	wxString title_choices[] = {_("Leave manually assigned title"), _("Automatically Generate Title")};
	AddLabelAndControl(sizerRight, _("Title Type"), m_cmbTitleType = new wxComboBox(this, ID_TITLE_TYPE, _T(""), wxDefaultPosition, wxDefaultSize, 2, title_choices));

	// add OK and Cancel to right side
    wxBoxSizer *sizerOKCancel = MakeOkAndCancel(wxHORIZONTAL);
	sizerRight->Add( sizerOKCancel, 0, wxALL | wxALIGN_RIGHT | wxALIGN_BOTTOM, control_border );

	// add all the controls to the left side
	AddLabelAndControl(sizerLeft, _("Tool Number"), m_dlbToolNumber = new wxTextCtrl(this, wxID_ANY));
	wxString materials[] = {_("High Speed Steel"),_("Carbide") };
	AddLabelAndControl(sizerLeft, _("Tool Material"), m_cmbMaterial = new wxComboBox(this, ID_MATERIAL, _T(""), wxDefaultPosition, wxDefaultSize, 2, materials));
	wxString tool_types[] = {_("Drill Bit"), _("Centre Drill Bit"), _("End Mill"), _("Slot Cutter"), _("Ball End Mill"), _("Chamfer"), _("Engraving Bit")};
	AddLabelAndControl(sizerLeft, _("Tool Type"), m_cmbToolType = new wxComboBox(this, ID_TOOL_TYPE, _T(""), wxDefaultPosition, wxDefaultSize, sizeof(tool_types)/sizeof(CToolParams::eToolType), tool_types));
	AddLabelAndControl(sizerLeft, _("Diameter"), m_dblDiameter = new CDoubleCtrl(this));
	AddLabelAndControl(sizerLeft, _("Tool length offset"), m_dblToolLengthOffset = new CDoubleCtrl(this));
	AddLabelAndControl(sizerLeft, _("Flat radius"), m_dblFlatRadius = new CDoubleCtrl(this));
	AddLabelAndControl(sizerLeft, _("Corner radius"), m_dblCornerRadius = new CDoubleCtrl(this));
	AddLabelAndControl(sizerLeft, _("Cutting edge angle"), m_dblCuttingEdgeAngle = new CDoubleCtrl(this));
	AddLabelAndControl(sizerLeft, _("Cutting edge height"), m_dblCuttingEdgeHeight = new CDoubleCtrl(this));
	
	SetFromData(object);

    SetSizer( sizerMain );
    sizerMain->SetSizeHints(this);
	sizerMain->Fit(this);

    m_dlbToolNumber->SetFocus();

	m_ignore_event_functions = false;

	SetPicture();
}

void CToolDlg::GetData(CTool* object)
{
	if(m_ignore_event_functions)return;
	m_ignore_event_functions = true;

	long i = 0;
	m_dlbToolNumber->GetValue().ToLong(&i);
	object->m_tool_number = i;
	object->m_params.m_material = m_cmbMaterial->GetSelection();
	object->m_params.m_type = (CToolParams::eToolType)(m_cmbToolType->GetSelection());
	object->m_params.m_diameter = m_dblDiameter->GetValue();
	object->m_params.m_tool_length_offset = m_dblToolLengthOffset->GetValue();	
	object->m_params.m_flat_radius = m_dblFlatRadius->GetValue();
	object->m_params.m_corner_radius = m_dblCornerRadius->GetValue();
	object->m_params.m_cutting_edge_angle = m_dblCuttingEdgeAngle->GetValue();
	object->m_params.m_cutting_edge_height = m_dblCuttingEdgeHeight->GetValue();
	object->m_title = m_txtTitle->GetValue();
	object->m_params.m_automatically_generate_title = (m_cmbTitleType->GetSelection() != 0);

	m_ignore_event_functions = false;
}

void CToolDlg::SetFromData(CTool* object)
{
	m_ignore_event_functions = true;

	m_dlbToolNumber->SetValue(wxString::Format(_T("%d"), object->m_tool_number));
	m_txtTitle->SetValue(object->m_title);
	m_cmbMaterial->SetSelection(object->m_params.m_material);
	m_cmbToolType->SetSelection(object->m_params.m_type);
	m_dblDiameter->SetValue(object->m_params.m_diameter);
	m_dblToolLengthOffset->SetValue(object->m_params.m_tool_length_offset);
	m_dblCuttingEdgeHeight->SetValue(object->m_params.m_cutting_edge_height);
	m_txtTitle->SetValue(object->m_title);
	m_cmbTitleType->SetSelection(object->m_params.m_automatically_generate_title ? 1:0);

	EnableAndSetCornerFlatAndAngle(object->m_params.m_type);

	m_ignore_event_functions = false;
}

void CToolDlg::SetPicture(const wxString& name)
{
	m_picture->SetPicture(theApp.GetResFolder() + _T("/bitmaps/ctool/") + name + _T(".png"), wxBITMAP_TYPE_PNG);
}

void CToolDlg::SetPicture()
{
	wxWindow* w = FindFocus();

	wxString bitmap_title;

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

void CToolDlg::OnChildFocus(wxChildFocusEvent& event)
{
	if(m_ignore_event_functions)return;
	if(event.GetWindow())
	{
		SetPicture();
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
	CTool* object = (CTool*)(m_ptool->MakeACopy());
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
	SetPicture();
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
			m_dblCuttingEdgeAngle->Enable(false);
			m_dblCuttingEdgeAngle->SetLabel(_T(""));
			break;
		case CToolParams::eEndmill:
		case CToolParams::eSlotCutter:
		case CToolParams::eBallEndMill:
			m_dblCornerRadius->Enable();
			m_dblCornerRadius->SetValue(m_ptool->m_params.m_corner_radius);
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
			m_dblFlatRadius->SetValue(m_ptool->m_params.m_flat_radius);
			m_dblCuttingEdgeAngle->Enable();
			m_dblCuttingEdgeAngle->SetValue(m_ptool->m_params.m_cutting_edge_angle);
			break;
		default:
			break;
	}
}

