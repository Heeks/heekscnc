// OpDlg.h
// Copyright (c) 2014, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

class COp;
class PictureWindow;
class CLengthCtrl;
class CDoubleCtrl;
class CObjectIdsCtrl;

#include "interface/HeeksObjDlg.h"

class OpDlg : public HeeksObjDlg
{
protected:
	enum
	{
		ID_TOOL = 100,
		ID_PATTERN,
		ID_SURFACE,
		ID_OP_ENUM_MAX,
	};

	wxTextCtrl *m_txtComment;
	wxCheckBox *m_chkActive;
	wxTextCtrl *m_txtTitle;
	HTypeObjectDropDown *m_cmbTool;
	HTypeObjectDropDown *m_cmbPattern;
	HTypeObjectDropDown *m_cmbSurface;

public:
	OpDlg(wxWindow *parent, COp* object, const wxString& title = wxString(_T("")), bool top_level = true, bool want_tool_control = true, bool picture = true);

	// HeeksObjDlg virtual functions
	void GetDataRaw(HeeksObj* object);
	void SetFromDataRaw(HeeksObj* object);
	void SetPictureByWindow(wxWindow* w);
	void SetPicture(const wxString& name);

    DECLARE_EVENT_TABLE()
};
