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
	CLengthCtrl *m_lgthClearanceHeight;
	CLengthCtrl *m_lgthStartDepth;
	CLengthCtrl *m_lgthStepDown;
	CLengthCtrl *m_lgthZFinishDepth;
	CLengthCtrl *m_lgthZThruDepth;
	CLengthCtrl *m_lgthFinalDepth;
	CLengthCtrl *m_lgthRapidDownToHeight;

public:
    DepthOpDlg(wxWindow *parent, CDepthOp* object, const wxString& title = wxString(_T("")), bool top_level = true);

	// HeeksObjDlg virtual functions
	void GetDataRaw(HeeksObj* object);
	void SetFromDataRaw(HeeksObj* object);
	void SetPictureByWindow(wxWindow* w);
	void SetPicture(const wxString& name);

	DECLARE_EVENT_TABLE()
};
