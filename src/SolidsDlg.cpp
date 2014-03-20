// SolidsDlg.cpp
// Copyright (c) 2014, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include "SolidsDlg.h"
#include "../../interface/NiceTextCtrl.h"
#include "Stock.h"
#include "Reselect.h"

BEGIN_EVENT_TABLE(SolidsDlg, HeeksObjDlg)
    EVT_COMBOBOX(ID_SOLIDS,HeeksObjDlg::OnComboOrCheck)
    EVT_BUTTON(ID_SOLIDS_PICK,SolidsDlg::OnSolidsPick)
END_EVENT_TABLE()

SolidsDlg::SolidsDlg(wxWindow *parent, HeeksObj* object, const wxString& title, bool top_level, bool picture)
             : HeeksObjDlg(parent, object, title, false, picture)
{
	// add all the controls to the left side
	leftControls.push_back(MakeLabelAndControl(_("Solids"), m_idsSolids = new CObjectIdsCtrl(this), m_btnSolidsPick = new wxButton(this, ID_SOLIDS_PICK, _("Pick"))));

	if(top_level)
	{
		HeeksObjDlg::AddControlsAndCreate();
		m_idsSolids->SetFocus();
	}
}

void SolidsDlg::GetDataRaw(HeeksObj* object)
{
	((CStock*)object)->m_solids.clear();
	m_idsSolids->GetIDList(((CStock*)object)->m_solids);

	HeeksObjDlg::GetDataRaw(object);
}

void SolidsDlg::SetFromDataRaw(HeeksObj* object)
{
	m_idsSolids->SetFromIDList(((CStock*)object)->m_solids);
	HeeksObjDlg::SetFromDataRaw(object);
}

void SolidsDlg::OnSolidsPick( wxCommandEvent& event )
{
	EndModal(ID_SOLIDS_PICK);
}

void SolidsDlg::PickSolids()
{
	heeksCAD->ClearMarkedList();
	heeksCAD->PickObjects(_("Pick solids"), MARKING_FILTER_SOLIDS_GROUP);

	std::list<int> solids;
	ReselectSolids::GetSolids( solids );
	m_idsSolids->SetFromIDList(solids);
	Fit();

	heeksCAD->ClearMarkedList();
}
