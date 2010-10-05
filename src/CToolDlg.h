// CToolDlg.h
// Copyright (c) 2010, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

class CTool;
class PictureWindow;
class CLengthCtrl;
class CDoubleCtrl;
class CObjectIdsCtrl;

#include "interface/HDialogs.h"

class CToolDlg : public HDialog
{
	static wxBitmap* m_diameter_bitmap;
	static wxBitmap* m_tool_length_offset_bitmap;
	static wxBitmap* m_flat_radius_bitmap;
	static wxBitmap* m_corner_radius_bitmap;
	static wxBitmap* m_cutting_edge_angle_bitmap;
	static wxBitmap* m_cutting_edge_height_bitmap;
	static wxBitmap* m_general_bitmap;
	static wxBitmap* m_x_offset_bitmap;
	static wxBitmap* m_front_angle_bitmap;
	static wxBitmap* m_tool_angle_bitmap;
	static wxBitmap* m_back_angle_bitmap;
	static wxBitmap* m_orientation_bitmap;
	static wxBitmap* m_probe_offset_x_bitmap;
	static wxBitmap* m_probe_offset_y_bitmap;
	static wxBitmap* m_layer_height_bitmap;
	static wxBitmap* m_width_over_thickness_bitmap;
	static wxBitmap* m_temperature_bitmap;
	static wxBitmap* m_filament_diameter_bitmap;
	static wxBitmap* m_pitch_bitmap;
	static wxBitmap* m_direction_bitmap;
	

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

	// The following are for tap tools
	CDoubleCtrl *m_dblPitch;
	wxComboBox *m_cmbDirection;

public:
    CToolDlg(wxWindow *parent, CTool* object);
	void GetData(CTool* object);
	void SetFromData(CTool* object);
	void SetPicture();
	void SetPicture(wxBitmap** bitmap, const wxString& name);

	void OnChildFocus(wxChildFocusEvent& event);
	void OnComboTitleType( wxCommandEvent& event );
	void OnComboToolType(wxCommandEvent& event);
	void OnComboMaterial(wxCommandEvent& event);
	void OnComboExtrusionMaterial(wxCommandEvent& event);

	void OnComboDirection(wxCommandEvent& event);


    DECLARE_EVENT_TABLE()
};
