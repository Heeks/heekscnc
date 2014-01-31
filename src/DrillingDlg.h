// DrillingDlg.h
// Copyright (c) 2014, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

class CDrilling;
class PictureWindow;
class CLengthCtrl;
class CDoubleCtrl;
class CObjectIdsCtrl;

#include "SpeedOpDlg.h"

class DrillingDlg : public SpeedOpDlg
{
	CObjectIdsCtrl *m_idsPoints;
	CLengthCtrl *m_lgthStandOff;
	CDoubleCtrl *m_dblDwell;
	wxCheckBox *m_chkFeedRetract;
	wxCheckBox *m_chkStopSpindleAtBottom;
	CLengthCtrl *m_lgthClearanceHeight;

public:
    DrillingDlg(wxWindow *parent, CDrilling* object, const wxString& title = wxString(_T("Drilling Operation")), bool top_level = true);

	// SpeedOpDlg's virtual functions
	void GetDataRaw(HeeksObj* object);
	void SetFromDataRaw(HeeksObj* object);
	void SetPictureByWindow(wxWindow* w);

	void SetPicture(const wxString& name);

    DECLARE_EVENT_TABLE()
};
