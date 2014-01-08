// CToolDlg.h
// Copyright (c) 2010, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "CTool.h"
class PictureWindow;
class CLengthCtrl;
class CDoubleCtrl;
class CObjectIdsCtrl;

#include "interface/HDialogs.h"

class CToolDlg : public HDialog
{
	wxTextCtrl *m_dlbToolNumber;
	wxComboBox *m_cmbMaterial;
	wxComboBox *m_cmbToolType;
	CDoubleCtrl *m_dblDiameter;
	CDoubleCtrl *m_dblToolLengthOffset;
	CDoubleCtrl *m_dblFlatRadius;
	CDoubleCtrl *m_dblCornerRadius;
	CDoubleCtrl *m_dblCuttingEdgeAngle;
	CDoubleCtrl *m_dblCuttingEdgeHeight;
	PictureWindow *m_picture;
	wxComboBox *m_cmbTitleType;
	wxTextCtrl *m_txtTitle;

	CTool* m_ptool;

public:
    CToolDlg(wxWindow *parent, CTool* object);
	void GetData(CTool* object);
	void SetFromData(CTool* object);
	void SetPicture();
	void SetPicture(const wxString& name);
	void EnableAndSetCornerFlatAndAngle(CToolParams::eToolType type);

	void OnChildFocus(wxChildFocusEvent& event);
	void OnComboTitleType( wxCommandEvent& event );
	void OnComboToolType(wxCommandEvent& event);
	void OnComboMaterial(wxCommandEvent& event);
	void OnTextCtrlEvent(wxCommandEvent& event);

    DECLARE_EVENT_TABLE()
};
