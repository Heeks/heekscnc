// CToolDlg.h
// Copyright (c) 2010, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

class CTool;
class PictureWindow;
class CLengthCtrl;
class CDoubleCtrl;
class CObjectIdsCtrl;

class CToolDlg : public wxDialog
{
	wxBitmap m_diameter_bitmap;
	wxBitmap m_tool_length_offset_bitmap;
	wxBitmap m_flat_radius_bitmap;
	wxBitmap m_corner_radius_bitmap;
	wxBitmap m_cutting_edge_angle_bitmap;
	wxBitmap m_cutting_edge_height_bitmap;
	wxBitmap m_general_bitmap;
	wxBitmap m_x_offset_bitmap;
	wxBitmap m_front_angle_bitmap;
	wxBitmap m_tool_angle_bitmap;
	wxBitmap m_back_angle_bitmap;
	wxBitmap m_orientation_bitmap;
	wxBitmap m_probe_offset_x_bitmap;
	wxBitmap m_probe_offset_y_bitmap;
	wxBitmap m_layer_height_bitmap;
	wxBitmap m_width_over_thickness_bitmap;
	wxBitmap m_temperature_bitmap;
	wxBitmap m_filament_diameter_bitmap;
	

	wxComboBox *m_cmbTitleType;
	wxComboBox *m_cmbMaterial;
	wxComboBox *m_cmbToolType;

	CDoubleCtrl *m_dblMaxAdvancePerRevolution;
	CDoubleCtrl *m_dblDiameter;
	CDoubleCtrl *m_dblToolLengthOffset;
	CDoubleCtrl *m_dblFlatRadius;
	CDoubleCtrl *m_dblCornerRadius;
	CDoubleCtrl *m_dblCuttingEdgeAngle;
	CDoubleCtrl *m_dblCuttingEdgeHeight;
	CDoubleCtrl *m_dblGradient;
	wxCheckBox *m_chkVisible;
	PictureWindow *m_picture;
	
	wxTextCtrl *m_txtTitle;

	// The following are all for lathe tools.  They become relevant when the m_type = eTurningTool
	CDoubleCtrl *m_dblXOffset;
	CDoubleCtrl *m_dblFrontAngle;
	CDoubleCtrl *m_dblToolAngle;
	CDoubleCtrl *m_dblBackAngle;
	CLengthCtrl *m_lgthorientation;

	// The following are for probe tools
	
	CDoubleCtrl *m_dblProbeOffsetX;
	CDoubleCtrl *m_dblProbeOffsetY;
	
	
	// The following are for extrusion
	wxComboBox *m_cmbExtrusionMaterial;
	CDoubleCtrl *m_dblFeedRate;
	CDoubleCtrl *m_dblLayerHeight;
	CDoubleCtrl *m_dblWidthOverThickness;
	CDoubleCtrl *m_dblTemperature;
	CDoubleCtrl *m_dblFlowrate;
	CDoubleCtrl *m_dblFilamentDiameter;



	bool m_ignore_event_functions;

	void AddLabelAndControl(wxBoxSizer* sizer, const wxString& label, wxWindow* control);

public:
    CToolDlg(wxWindow *parent, CTool* object);
	void GetData(CTool* object);
	void SetFromData(CTool* object);
	void SetPicture();

	void OnChildFocus(wxChildFocusEvent& event);
	void OnComboTitleType( wxCommandEvent& event );
	void OnComboToolType(wxCommandEvent& event);
	void OnComboMaterial(wxCommandEvent& event);
	void OnComboExtrusionMaterial(wxCommandEvent& event);


    DECLARE_EVENT_TABLE()
};
