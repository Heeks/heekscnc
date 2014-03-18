// SketchOpDlg.h
// Copyright (c) 2014, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

class CDepthOp;
class PictureWindow;
class CLengthCtrl;
class CDoubleCtrl;
class CObjectIdsCtrl;

#include "DepthOpDlg.h"

class SketchOpDlg : public DepthOpDlg
{
protected:
	enum
	{
		ID_SKETCH = ID_DEPTHOP_ENUM_MAX,
		ID_SKETCH_PICK,
		ID_SKETCH_ENUM_MAX,
	};

	HTypeObjectDropDown *m_cmbSketch;
	wxButton *m_btnSketchPick;

public:
    SketchOpDlg(wxWindow *parent, CDepthOp* object, const wxString& title = wxString(_T("Sketch Operation")), bool top_level = true);

	// HeeksObjDlg virtual functions
	void GetDataRaw(HeeksObj* object);
	void SetFromDataRaw(HeeksObj* object);

	void OnSketchPick( wxCommandEvent& event );
	void PickSketch();

	DECLARE_EVENT_TABLE()
};
