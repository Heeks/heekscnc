// ScriptOpDlg.h
// Copyright (c) 2014, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

class CScriptOp;
class PictureWindow;
class CLengthCtrl;
class CDoubleCtrl;
class CObjectIdsCtrl;

#include "OpDlg.h"

class ScriptOpDlg : public OpDlg
{
protected:
	enum
	{
		ID_SCRIPT_TXT = ID_OP_ENUM_MAX,
		ID_SCRIPTOP_ENUM_MAX,
	};

	wxTextCtrl *m_txtScript;

public:
	ScriptOpDlg(wxWindow *parent, CScriptOp* object, const wxString& title = wxString(_T("Script Operation")), bool top_level = true);

	// HeeksObjDlg virtual functions
	void GetDataRaw(HeeksObj* object);
	void SetFromDataRaw(HeeksObj* object);
	void SetPictureByWindow(wxWindow* w);
	void SetPicture(const wxString& name);

	void OnScriptText( wxCommandEvent& event );
	void OnHelp( wxCommandEvent& event );

    DECLARE_EVENT_TABLE()
};
