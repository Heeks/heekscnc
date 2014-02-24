// SpeedOpDlg.h
// Copyright (c) 2014, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

class CSpeedOp;
class PictureWindow;
class CLengthCtrl;
class CDoubleCtrl;
class CObjectIdsCtrl;

#include "OpDlg.h"

class SpeedOpDlg : public OpDlg
{
protected:
	enum
	{
		ID_SPEEDOP_ENUM_MAX = ID_OP_ENUM_MAX,
	};

	CLengthCtrl *m_lgthHFeed;
	CLengthCtrl *m_lgthVFeed;
	CDoubleCtrl *m_dblSpindleSpeed;

public:
	SpeedOpDlg(wxWindow *parent, CSpeedOp* object, bool some_controls_on_left, const wxString& title = wxString(_T("")), bool top_level = true);

	// HeeksObjDlg virtual functions
	void GetDataRaw(HeeksObj* object);
	void SetFromDataRaw(HeeksObj* object);
	void SetPictureByWindow(wxWindow* w);
	void SetPicture(const wxString& name);

    DECLARE_EVENT_TABLE()
};
