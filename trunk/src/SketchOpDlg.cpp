// SketchOpDlg.cpp
// Copyright (c) 2014, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include "SketchOpDlg.h"
#include "interface/PictureFrame.h"
#include "interface/NiceTextCtrl.h"
#include "SketchOp.h"

BEGIN_EVENT_TABLE(SketchOpDlg, DepthOpDlg)
    EVT_COMBOBOX(ID_SKETCH,HeeksObjDlg::OnComboOrCheck)
    EVT_BUTTON(ID_SKETCH_PICK,SketchOpDlg::OnSketchPick)
END_EVENT_TABLE()

SketchOpDlg::SketchOpDlg(wxWindow *parent, CDepthOp* object, const wxString& title, bool top_level)
:DepthOpDlg(parent, object, false, title, false)
{
	leftControls.push_back(MakeLabelAndControl(_("Sketches"), m_cmbSketch = new HTypeObjectDropDown(this, ID_SKETCH, SketchType, heeksCAD->GetMainObject()), m_btnSketchPick = new wxButton(this, ID_SKETCH_PICK, _("Pick"))));

	if(top_level)
	{
		HeeksObjDlg::AddControlsAndCreate();
		m_cmbSketch->SetFocus();
	}
}

void SketchOpDlg::GetDataRaw(HeeksObj* object)
{
	((CSketchOp*)object)->m_sketch = m_cmbSketch->GetSelectedId();

	DepthOpDlg::GetDataRaw(object);
}

void SketchOpDlg::SetFromDataRaw(HeeksObj* object)
{
	m_cmbSketch->SelectById(((CSketchOp*)object)->m_sketch);

	DepthOpDlg::SetFromDataRaw(object);
}

void SketchOpDlg::OnSketchPick( wxCommandEvent& event )
{
	EndModal(ID_SKETCH_PICK);
}

void SketchOpDlg::PickSketch()
{
	heeksCAD->ClearMarkedList();
	heeksCAD->PickObjects(_("Pick a sketch"), MARKING_FILTER_SKETCH_GROUP, true);

	m_cmbSketch->Recreate();
	Fit();

	const std::list<HeeksObj*> &list = heeksCAD->GetMarkedList();
	m_cmbSketch->SelectById((list.size() > 0) ? (list.front()->GetID()):0);
}
