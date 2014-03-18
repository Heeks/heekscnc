// StockDlg.h
// Copyright (c) 2014, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

class CLengthCtrl;
class CObjectIdsCtrl;

#include "SolidsDlg.h"

class StockDlg : public SolidsDlg
{
public:
    StockDlg(wxWindow *parent, HeeksObj* object, const wxString& title = wxString(_T("Stock")), bool top_level = true);

	// HeeksObjDlg virtual functions
	void GetDataRaw(HeeksObj* object);
	void SetFromDataRaw(HeeksObj* object);
	void OnHelp( wxCommandEvent& event );

	static bool Do(CStock* object);

    DECLARE_EVENT_TABLE()
};
