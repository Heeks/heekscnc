// PatternDlg.h
// Copyright (c) 2014, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "Pattern.h"
class CLengthCtrl;

#include "../../interface/HeeksObjDlg.h"

class PatternDlg : public HeeksObjDlg
{
protected:
	enum
	{
		ID_NUM_COPIES_A = 100,
		ID_X_SHIFT_A,
		ID_Y_SHIFT_A,
		ID_NUM_COPIES_B,
		ID_X_SHIFT_B,
		ID_Y_SHIFT_B,
	};

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

	void RedrawBitmap(wxBitmap &bitmap);
	static void RedrawBitmap(wxBitmap &bitmap, CPattern* pattern);
	static void RedrawBitmap(wxBitmap &bitmap, long numA, long numB, double xshiftA, double yshiftA, double xshiftB, double yshiftB);
	void OnHelp( wxCommandEvent& event );
	void OnTextChanged( wxCommandEvent& event );

    DECLARE_EVENT_TABLE()
};
