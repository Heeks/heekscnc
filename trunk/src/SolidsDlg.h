// SolidsDlg.h
// Copyright (c) 2014, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

class CLengthCtrl;
class CObjectIdsCtrl;

#include "interface/HeeksObjDlg.h"

class SolidsDlg : public HeeksObjDlg
{
protected:
	enum
	{
		ID_SOLIDS = 100,
		ID_SOLIDS_PICK,
		ID_SOLIDS_ENUM_MAX,
	};

	CObjectIdsCtrl *m_idsSolids;
	wxButton *m_btnSolidsPick;

public:
    SolidsDlg(wxWindow *parent, HeeksObj* object, const wxString& title, bool top_level = true, bool picture = true);

	// HeeksObjDlg virtual functions
	void GetDataRaw(HeeksObj* object);
	void SetFromDataRaw(HeeksObj* object);

	void OnSolidsPick( wxCommandEvent& event );
	void PickSolids();

    DECLARE_EVENT_TABLE()
};
