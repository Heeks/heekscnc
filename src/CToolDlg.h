// CToolDlg.h
// Copyright (c) 2010, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "CTool.h"
class PictureWindow;
class CLengthCtrl;
class CDoubleCtrl;
class CObjectIdsCtrl;

#include "../../interface/HeeksObjDlg.h"

class CToolDlg : public HeeksObjDlg
{
	wxTextCtrl *m_dlbToolNumber;
	wxComboBox *m_cmbMaterial;
	wxComboBox *m_cmbToolType;
	CLengthCtrl *m_dblDiameter;
	CLengthCtrl *m_dblToolLengthOffset;
	CLengthCtrl *m_dblFlatRadius;
	CLengthCtrl *m_dblCornerRadius;
	CDoubleCtrl *m_dblCuttingEdgeAngle;
	CLengthCtrl *m_dblCuttingEdgeHeight;
	wxComboBox *m_cmbTitleType;
	wxTextCtrl *m_txtTitle;

public:
    CToolDlg(wxWindow *parent, CTool* object, const wxString& title = wxString(_T("Tool Definition")), bool top_level = true);

	// HeeksObjDlg virtual functions
	void GetDataRaw(HeeksObj* object);
	void SetFromDataRaw(HeeksObj* object);
	void SetPictureByWindow(wxWindow* w);
	void SetPicture(const wxString& name);

	void EnableAndSetCornerFlatAndAngle(CToolParams::eToolType type);
	void OnComboTitleType( wxCommandEvent& event );
	void OnComboToolType(wxCommandEvent& event);
	void OnComboMaterial(wxCommandEvent& event);
	void OnTextCtrlEvent(wxCommandEvent& event);
	void OnHelp( wxCommandEvent& event );

    DECLARE_EVENT_TABLE()
};
