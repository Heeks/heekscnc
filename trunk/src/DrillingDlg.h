// DrillingDlg.h
// Copyright (c) 2014, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

class CDrilling;
class PictureWindow;
class CLengthCtrl;
class CDoubleCtrl;
class CObjectIdsCtrl;

#include "DepthOpDlg.h"

class DrillingDlg : public DepthOpDlg
{
	CObjectIdsCtrl *m_idsPoints;
	CDoubleCtrl *m_dblDwell;
	wxCheckBox *m_chkFeedRetract;
	wxCheckBox *m_chkStopSpindleAtBottom;
	wxButton *m_btnPointsPick;
	wxCheckBox *m_chkInternalCoolantOn;
	wxCheckBox *m_chkRapidToClearance;

public:
	DrillingDlg(wxWindow *parent, CDrilling* object, const wxString& title = wxString(_("Drilling Operation")), bool top_level = true);

	static bool Do(CDrilling* object);

	// HeeksObjDlg's virtual functions
	void GetDataRaw(HeeksObj* object);
	void SetFromDataRaw(HeeksObj* object);
	void SetPictureByWindow(wxWindow* w);

	void SetPicture(const wxString& name);
	void OnPointsPick( wxCommandEvent& event );
	void OnHelp( wxCommandEvent& event );

	DECLARE_EVENT_TABLE()
};

