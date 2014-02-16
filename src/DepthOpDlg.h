// DepthOpDlg.h
// Copyright (c) 2014, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

class CDepthOp;
class PictureWindow;
class CLengthCtrl;
class CDoubleCtrl;
class CObjectIdsCtrl;

#include "SpeedOpDlg.h"

class DepthOpDlg : public SpeedOpDlg
{
protected:
	enum
	{
		ID_DEPTHOP_ENUM_MAX = ID_SPEEDOP_ENUM_MAX,
	};

	CLengthCtrl *m_lgthClearanceHeight;
	CLengthCtrl *m_lgthStartDepth;
	CLengthCtrl *m_lgthStepDown;
	CLengthCtrl *m_lgthZFinishDepth;
	CLengthCtrl *m_lgthZThruDepth;
	CLengthCtrl *m_lgthFinalDepth;
	CLengthCtrl *m_lgthRapidDownToHeight;

	bool m_drill_pictures;

public:
    DepthOpDlg(wxWindow *parent, CDepthOp* object, bool drill_pictures = false, const wxString& title = wxString(_T("Depth Operation")), bool top_level = true);

	// HeeksObjDlg virtual functions
	void GetDataRaw(HeeksObj* object);
	void SetFromDataRaw(HeeksObj* object);
	void SetPictureByWindow(wxWindow* w);
	void SetPicture(const wxString& name);

	DECLARE_EVENT_TABLE()
};
