// CToolDlg.cpp
// Copyright (c) 2010, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include "CToolDlg.h"
#include "interface/PictureFrame.h"
#include "interface/NiceTextCtrl.h"
#include "CTool.h"

enum
{
    ID_TITLE_TYPE = 100,
    ID_MATERIAL,
    ID_TOOL_TYPE,
    ID_MAX_ADVANCE_PER_REVOLUTION,
    ID_DIAMETER,
    ID_TOOL_LENGTH_OFFSET,
    ID_FLAT_RADIUS,
    ID_CORNER_RADIUS,
    ID_CUTTING_EDGE_ANGLE,
    ID_CUTTING_EDGE_HEIGHT,
    ID_GRADIENT,
    ID_TITLE,
    ID_VISIBLE,
    ID_XOFFSET,
    ID_FRONTANGLE,
    ID_TOOLANGLE,
    ID_BACKANGLE,
    ID_ORIENTATION,
    ID_PROBEOFFSETX,
    ID_PROBEOFFSETY,
    ID_EXTRUSIONMATERIAL,
    ID_FEEDRATE,
    ID_LAYERHEIGHT,
    ID_WIDTHOVERTHICKNESS,
    ID_TEMPERATURE,
    ID_FLOWRATE,
    ID_FILAMENTDIAMETER,
    ID_DIRECTION,
    ID_PITCH,
};

BEGIN_EVENT_TABLE(CToolDlg, HDialog)
    EVT_CHILD_FOCUS(CToolDlg::OnChildFocus)
    EVT_COMBOBOX(ID_TITLE_TYPE,CToolDlg::OnComboTitleType)
    EVT_COMBOBOX(ID_TOOL_TYPE, CToolDlg::OnComboToolType)
    EVT_COMBOBOX(ID_MATERIAL, CToolDlg::OnComboMaterial)
    EVT_COMBOBOX(ID_EXTRUSIONMATERIAL, CToolDlg::OnComboExtrusionMaterial)
    EVT_COMBOBOX(ID_DIRECTION, CToolDlg::OnComboDirection)
END_EVENT_TABLE()

wxBitmap* CToolDlg::m_diameter_bitmap = NULL;
wxBitmap* CToolDlg::m_tool_length_offset_bitmap = NULL;
wxBitmap* CToolDlg::m_flat_radius_bitmap = NULL;
wxBitmap* CToolDlg::m_corner_radius_bitmap = NULL;
wxBitmap* CToolDlg::m_cutting_edge_angle_bitmap = NULL;
wxBitmap* CToolDlg::m_cutting_edge_height_bitmap = NULL;
wxBitmap* CToolDlg::m_general_bitmap = NULL;
wxBitmap* CToolDlg::m_x_offset_bitmap = NULL;
wxBitmap* CToolDlg::m_front_angle_bitmap = NULL;
wxBitmap* CToolDlg::m_tool_angle_bitmap = NULL;
wxBitmap* CToolDlg::m_back_angle_bitmap = NULL;
wxBitmap* CToolDlg::m_orientation_bitmap = NULL;
wxBitmap* CToolDlg::m_probe_offset_x_bitmap = NULL;
wxBitmap* CToolDlg::m_probe_offset_y_bitmap = NULL;
wxBitmap* CToolDlg::m_layer_height_bitmap = NULL;
wxBitmap* CToolDlg::m_width_over_thickness_bitmap = NULL;
wxBitmap* CToolDlg::m_temperature_bitmap = NULL;
wxBitmap* CToolDlg::m_filament_diameter_bitmap = NULL;
wxBitmap* CToolDlg::m_pitch_bitmap = NULL;
wxBitmap* CToolDlg::m_direction_bitmap = NULL;

CToolDlg::CToolDlg(wxWindow *parent, CTool* object)
             : HDialog(parent, wxID_ANY, wxString(_T("Tool Definition")))
{
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

	// The following are for extrusion and should be hidden for all others
	wxString extrusionmaterials[] = {_("ABS Plastic"),_("PLA Plastic"),_("HDPE Plastic"),_("Other") };
	AddLabelAndControl(sizerRight, _("Extrusion Material"),m_cmbExtrusionMaterial = new wxComboBox(this, ID_EXTRUSIONMATERIAL, _T(""), wxDefaultPosition, wxDefaultSize, 4, extrusionmaterials));
	AddLabelAndControl(sizerRight, _("Feed Rate"),m_dblFeedRate = new CDoubleCtrl(this, ID_MAX_ADVANCE_PER_REVOLUTION));
	AddLabelAndControl(sizerRight, _("Layer Height"),m_dblLayerHeight = new CDoubleCtrl(this, ID_MAX_ADVANCE_PER_REVOLUTION));
	AddLabelAndControl(sizerRight, _("Width Over Thickness ratio"),m_dblWidthOverThickness = new CDoubleCtrl(this, ID_MAX_ADVANCE_PER_REVOLUTION));
	AddLabelAndControl(sizerRight, _("Temperature"),m_dblTemperature = new CDoubleCtrl(this, ID_MAX_ADVANCE_PER_REVOLUTION));
	AddLabelAndControl(sizerRight, _("Flow Rate"),m_dblFlowrate = new CDoubleCtrl(this, ID_MAX_ADVANCE_PER_REVOLUTION));
	AddLabelAndControl(sizerRight, _("Filament Diameter"),m_dblFilamentDiameter = new CDoubleCtrl(this, ID_MAX_ADVANCE_PER_REVOLUTION));

	AddLabelAndControl(sizerRight, _("Title"), m_txtTitle = new wxTextCtrl(this, ID_TITLE));
	sizerRight->Add( m_chkVisible = new wxCheckBox( this, ID_VISIBLE, _("Visible") ), 0, wxALL, control_border );	

	// The following are for taps and should be hidden for all others
	AddLabelAndControl(sizerRight, _("Pitch"),m_dblPitch = new CDoubleCtrl(this, ID_PITCH));
	wxString tapdirections[] = {_("right hand"),_("left hand") };
	AddLabelAndControl(sizerRight, _("Tap direction"),m_cmbDirection = new wxComboBox(this, ID_DIRECTION, _T(""), 
											  wxDefaultPosition, wxDefaultSize, 2, tapdirections));

	// add OK and Cancel to right side
    wxBoxSizer *sizerOKCancel = MakeOkAndCancel(wxHORIZONTAL);
	sizerRight->Add( sizerOKCancel, 0, wxALL | wxALIGN_RIGHT | wxALIGN_BOTTOM, control_border );

	// add all the controls to the left side
	wxString title_choices[] = {_("Leave manually assigned title"), _("Automatically Generate Title")};
	AddLabelAndControl(sizerLeft, _("Title Type"), m_cmbTitleType = new wxComboBox(this, ID_TITLE_TYPE, _T(""), wxDefaultPosition, wxDefaultSize, 2, title_choices));

	wxString materials[] = {_("High Speed Steel"),_("Carbide") };
	AddLabelAndControl(sizerLeft, _("Tool Material"), m_cmbMaterial = new wxComboBox(this, ID_MATERIAL, _T(""), wxDefaultPosition, wxDefaultSize, 2, materials));

	wxString tool_types[] = {_("Drill Bit"), _("Centre Drill Bit"), _("End Mill"), _("Slot Cutter"), _("Ball End Mill"), _("Chamfer"), _("Turning Tool"), _("Touch Probe"), _("Tool Length Switch"), _("Extrusion"),_("Tap Tool")};
	AddLabelAndControl(sizerLeft, _("Tool Type"), m_cmbToolType = new wxComboBox(this, ID_TOOL_TYPE, _T(""), wxDefaultPosition, wxDefaultSize, 11, tool_types));

	AddLabelAndControl(sizerLeft, _("Max advance per revolution"), m_dblMaxAdvancePerRevolution = new CDoubleCtrl(this, ID_MAX_ADVANCE_PER_REVOLUTION));
	AddLabelAndControl(sizerLeft, _("Diameter"), m_dblDiameter = new CDoubleCtrl(this, ID_DIAMETER));
	AddLabelAndControl(sizerLeft, _("Tool length offset"), m_dblToolLengthOffset = new CDoubleCtrl(this, ID_TOOL_LENGTH_OFFSET));
	AddLabelAndControl(sizerLeft, _("Flat radius"), m_dblFlatRadius = new CDoubleCtrl(this, ID_FLAT_RADIUS));
	AddLabelAndControl(sizerLeft, _("Corner radius"), m_dblCornerRadius = new CDoubleCtrl(this, ID_CORNER_RADIUS));
	AddLabelAndControl(sizerLeft, _("Cutting edge angle"), m_dblCuttingEdgeAngle = new CDoubleCtrl(this, ID_CUTTING_EDGE_ANGLE));
	AddLabelAndControl(sizerLeft, _("Cutting edge height"), m_dblCuttingEdgeHeight = new CDoubleCtrl(this, ID_CUTTING_EDGE_HEIGHT));
	AddLabelAndControl(sizerLeft, _("gradient"), m_dblGradient = new CDoubleCtrl(this, ID_GRADIENT));


	// The following are all for lathe tools and should be hidden for all others
	 AddLabelAndControl(sizerLeft, _("X Offset"),m_dblXOffset = new CDoubleCtrl(this, ID_MAX_ADVANCE_PER_REVOLUTION));
	 AddLabelAndControl(sizerLeft, _("Front Angle"),m_dblFrontAngle = new CDoubleCtrl(this, ID_MAX_ADVANCE_PER_REVOLUTION));
	 AddLabelAndControl(sizerLeft, _("Tool Angle"),m_dblToolAngle = new CDoubleCtrl(this, ID_MAX_ADVANCE_PER_REVOLUTION));
	 AddLabelAndControl(sizerLeft, _("Back Angle"),m_dblBackAngle = new CDoubleCtrl(this, ID_MAX_ADVANCE_PER_REVOLUTION));
	 AddLabelAndControl(sizerLeft, _("Orientation"),m_lgthorientation = new CLengthCtrl(this, ID_MAX_ADVANCE_PER_REVOLUTION));

	// The following are for probe tools and should be hidden for all others
	
	 AddLabelAndControl(sizerLeft, _("Probe Offset X"),m_dblProbeOffsetX = new CDoubleCtrl(this, ID_MAX_ADVANCE_PER_REVOLUTION));
	 AddLabelAndControl(sizerLeft, _("Probe Offset Y"),m_dblProbeOffsetY = new CDoubleCtrl(this, ID_MAX_ADVANCE_PER_REVOLUTION));
	
	SetFromData(object);

    SetSizer( sizerMain );
    sizerMain->SetSizeHints(this);
	sizerMain->Fit(this);

    m_cmbTitleType->SetFocus();

	m_ignore_event_functions = false;

	SetPicture();
}

void CToolDlg::GetData(CTool* object)
{
	if(m_ignore_event_functions)return;
	m_ignore_event_functions = true;

	//need to deal with the combo box for title type
	//need to deal with the combo box for material
	//need to deal with the combo box for Tool Type.
	
	object->m_params.m_max_advance_per_revolution = m_dblMaxAdvancePerRevolution->GetValue();
	object->m_params.m_diameter = m_dblDiameter->GetValue();
	object->m_params.m_tool_length_offset = m_dblToolLengthOffset->GetValue();	
	object->m_params.m_flat_radius = m_dblFlatRadius->GetValue();
	object->m_params.m_corner_radius = m_dblCornerRadius->GetValue();
	object->m_params.m_cutting_edge_angle = m_dblCuttingEdgeAngle->GetValue();
	object->m_params.m_cutting_edge_height = m_dblCuttingEdgeHeight->GetValue();

	// The following are all for lathe tools.  They become relevant when the m_type = eTurningTool

	// The following are for extrusion


	 object->m_params.m_x_offset = m_dblXOffset->GetValue();
	 object->m_params.m_front_angle = m_dblFrontAngle->GetValue();
	 object->m_params.m_tool_angle = m_dblToolAngle->GetValue();
	 object->m_params.m_back_angle = m_dblBackAngle->GetValue();
	 object->m_params.m_orientation = m_lgthorientation->GetValue();

	// The following are for probe tools
	
	 object->m_params.m_probe_offset_x = m_dblProbeOffsetX->GetValue();
	 object->m_params.m_probe_offset_y = m_dblProbeOffsetY->GetValue();
	
	
	// The following are for extrusion
	
	//need to deal with the combo box for title type
	 //object->m_params.m_extrusion_material = m_cmbExtrusionMaterial->GetValue();
	 object->m_params.m_feedrate = m_dblFeedRate->GetValue();
	 object->m_params.m_layer_height = m_dblLayerHeight->GetValue();
	 object->m_params.m_width_over_thickness = m_dblWidthOverThickness->GetValue();
	 object->m_params.m_temperature = m_dblTemperature->GetValue();
	 object->m_params.m_flowrate = m_dblFlowrate->GetValue();
	 object->m_params.m_filament_diameter = m_dblFilamentDiameter->GetValue();

	 // The following are for tap tools

	 // need to deal with direction combo box

	 // object->m_params.m_direction = m_cmbDirection->GetValue();
	 object->m_params.m_pitch = m_dblPitch->GetValue();	 

	//need to deal with the check box for visible
	
	object->m_title = m_txtTitle->GetValue();

	m_ignore_event_functions = false;
}

void CToolDlg::SetFromData(CTool* object)
{
	m_ignore_event_functions = true;

	//need to deal with the combo box for title type
	//need to deal with the combo box for material
	//need to deal with the combo box for Tool Type.
	
	m_dblMaxAdvancePerRevolution->SetValue(object->m_params.m_max_advance_per_revolution);
	m_dblDiameter->SetValue(object->m_params.m_diameter);
	m_dblToolLengthOffset->SetValue(object->m_params.m_tool_length_offset);
	m_dblFlatRadius->SetValue(object->m_params.m_flat_radius);
	m_dblCornerRadius->SetValue(object->m_params.m_corner_radius);
	m_dblCuttingEdgeAngle->SetValue(object->m_params.m_cutting_edge_angle);
	m_dblCuttingEdgeHeight->SetValue(object->m_params.m_cutting_edge_height);

	 m_dblXOffset ->SetValue(object->m_params.m_x_offset);
	 m_dblFrontAngle->SetValue(object->m_params.m_front_angle);
	 m_dblToolAngle->SetValue(object->m_params.m_tool_angle);
	 m_dblBackAngle->SetValue(object->m_params.m_back_angle);
	 m_lgthorientation->SetValue(object->m_params.m_orientation);

	// The following are for probe tools
	
	 m_dblProbeOffsetX->SetValue(object->m_params.m_probe_offset_x);
	 m_dblProbeOffsetY->SetValue(object->m_params.m_probe_offset_y);
	
	
	// The following are for extrusion
	 //m_cmbExtrusionMaterial->SetValue(object->m_params.m_max_advance_per_revolution);
	 m_dblFeedRate->SetValue(object->m_params.m_feedrate);
	 m_dblLayerHeight->SetValue(object->m_params.m_layer_height);
	 m_dblWidthOverThickness->SetValue(object->m_params.m_width_over_thickness);
	 m_dblTemperature->SetValue(object->m_params.m_temperature);
	 m_dblFlowrate->SetValue(object->m_params.m_flowrate);
	 m_dblFilamentDiameter->SetValue(object->m_params.m_filament_diameter);
	
	// The following are for tap tools
	
	 m_dblPitch->SetValue(object->m_params.m_pitch);
	//need to deal with the text box for title

	// m_cmbDirection->SetValue(object->m_params.m_direction);
	
	//need to deal with the text box for title
	//need to deal with the check box for visible

	m_ignore_event_functions = false;
}

void CToolDlg::SetPicture(wxBitmap** bitmap, const wxString& name)
{
	m_picture->SetPicture(bitmap, theApp.GetResFolder() + _T("/bitmaps/ctool/") + name + _T(".png"), wxBITMAP_TYPE_PNG);
}

void CToolDlg::SetPicture()
{
	wxWindow* w = FindFocus();

	if(w == m_dblDiameter)SetPicture(&m_diameter_bitmap, _T("diameter"));
	else if(w == m_dblToolLengthOffset)SetPicture(&m_tool_length_offset_bitmap, _T("toollengthoffset"));
	else if(w == m_dblFlatRadius)SetPicture(&m_flat_radius_bitmap, _T("flatradius"));
	else if(w == m_dblCornerRadius)SetPicture(&m_corner_radius_bitmap, _T("cornerradius"));
	else if(w == m_dblCuttingEdgeAngle)SetPicture(&m_cutting_edge_angle_bitmap, _T("cuttingedgeangle"));
	else if(w == m_dblCuttingEdgeHeight)SetPicture(&m_cutting_edge_height_bitmap, _T("cuttingedgeheight"));
	

	// The following are all for lathe tools.  They become relevant when the m_type = eTurningTool
	 else if(w == m_dblXOffset)SetPicture(&m_x_offset_bitmap, _T("xoffset"));
	 else if(w == m_dblFrontAngle)SetPicture(&m_front_angle_bitmap, _T("frontangle"));
	 else if(w == m_dblToolAngle)SetPicture(&m_tool_angle_bitmap, _T("toolangle"));
	 else if(w == m_dblBackAngle)SetPicture(&m_back_angle_bitmap, _T("backangle"));
	 else if(w == m_lgthorientation)SetPicture(&m_orientation_bitmap, _T("orientation"));

	// The following are for probe tools
	
	 else if(w == m_dblProbeOffsetX)SetPicture(&m_probe_offset_x_bitmap, _T("probeoffsetx"));
	 else if(w == m_dblProbeOffsetY)SetPicture(&m_probe_offset_y_bitmap, _T("probeoffsety"));
	
	
	// The following are for extrusion
	 else if(w == m_dblLayerHeight)SetPicture(&m_layer_height_bitmap, _T("layerheight"));
	 else if(w == m_dblWidthOverThickness)SetPicture(&m_width_over_thickness_bitmap, _T("wovert"));
	 else if(w == m_dblTemperature)SetPicture(&m_temperature_bitmap, _T("temperature"));
	 else if(w == m_dblFilamentDiameter)SetPicture(&m_filament_diameter_bitmap, _T("filament"));
	 
	// The following are for tap tools
	
	//	 else if(w == m_dblPitch)SetPicture(&m_probe_offset_x_bitmap, _T("probeoffsetx"));
	// else if(w == m_dblProbeOffsetY)SetPicture(&m_probe_offset_y_bitmap, _T("probeoffsety"));
	else SetPicture(&m_general_bitmap, _T("general"));	
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

void CToolDlg::OnComboToolType(wxCommandEvent& event)
{
	if(m_ignore_event_functions)return;
//	SetPicture();
}

void CToolDlg::OnComboExtrusionMaterial(wxCommandEvent& event)
{
	if(m_ignore_event_functions)return;
//	SetPicture();
}

void CToolDlg::OnComboDirection(wxCommandEvent& event)
{
	if(m_ignore_event_functions)return;
//	SetPicture();
}
