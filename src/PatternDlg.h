// PatternDlg.h
// Copyright (c) 2014, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "Pattern.h"
class CLengthCtrl;

#include "interface/HeeksObjDlg.h"

class PatternDlg : public HeeksObjDlg
{
	wxTextCtrl *m_txtCopies1;
	CLengthCtrl *m_lgthXShift1;
	CLengthCtrl *m_lgthYShift1;
	wxTextCtrl *m_txtCopies2;
	CLengthCtrl *m_lgthXShift2;
	CLengthCtrl *m_lgthYShift2;

public:
    PatternDlg(wxWindow *parent, HeeksObj* object, const wxString& title = wxString(_T("Pattern")), bool top_level = true);

	// HeeksObjDlg virtual functions
	void GetDataRaw(HeeksObj* object);
	void SetFromDataRaw(HeeksObj* object);
	void SetPictureByWindow(wxWindow* w);
	void SetPicture(const wxString& name);
};
