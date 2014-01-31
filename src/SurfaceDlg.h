// SurfaceDlg.h
// Copyright (c) 2014, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

class CLengthCtrl;
class CObjectIdsCtrl;

#include "interface/HeeksObjDlg.h"

class SurfaceDlg : public HeeksObjDlg
{
	CObjectIdsCtrl *m_idsSolids;
	CLengthCtrl *m_lgthTolerance;
	CLengthCtrl *m_lgthMinZ;
	CLengthCtrl *m_lgthMaterialAllowance;
	wxCheckBox *m_chkSameForEachPosition;

public:
    SurfaceDlg(wxWindow *parent, HeeksObj* object, const wxString& title = wxString(_T("Surface")), bool top_level = true);

	// HeeksObjDlg virtual functions
	void GetDataRaw(HeeksObj* object);
	void SetFromDataRaw(HeeksObj* object);
	void SetPictureByWindow(wxWindow* w);
	void SetPicture(const wxString& name);

    DECLARE_EVENT_TABLE()
};
