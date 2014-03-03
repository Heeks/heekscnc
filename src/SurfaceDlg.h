// SurfaceDlg.h
// Copyright (c) 2014, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

class CLengthCtrl;
class CObjectIdsCtrl;

#include "SolidsDlg.h"

class SurfaceDlg : public SolidsDlg
{
protected:
	enum
	{
		ID_TOLERANCE = ID_SOLIDS_ENUM_MAX,
		ID_MIN_Z,
		ID_MATERIAL_ALLOWANCE,
		ID_SAME_FOR_EACH_POSITION,
		ID_SURFACE_ENUM_MAX,
	};

	CLengthCtrl *m_lgthTolerance;
	CLengthCtrl *m_lgthMaterialAllowance;
	wxCheckBox *m_chkSameForEachPosition;

public:
    SurfaceDlg(wxWindow *parent, HeeksObj* object, const wxString& title = wxString(_T("Surface")), bool top_level = true);

	// HeeksObjDlg virtual functions
	void GetDataRaw(HeeksObj* object);
	void SetFromDataRaw(HeeksObj* object);
	void SetPictureByWindow(wxWindow* w);
	void SetPicture(const wxString& name);
	void OnHelp( wxCommandEvent& event );

	static bool Do(CSurface* object);

    DECLARE_EVENT_TABLE()
};
