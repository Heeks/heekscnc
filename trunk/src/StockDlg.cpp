// StockDlg.cpp
// Copyright (c) 2014, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include "Stock.h"
#include "StockDlg.h"
#include "../../interface/NiceTextCtrl.h"

BEGIN_EVENT_TABLE(StockDlg, SolidsDlg)
    EVT_BUTTON(wxID_HELP, StockDlg::OnHelp)
END_EVENT_TABLE()

StockDlg::StockDlg(wxWindow *parent, HeeksObj* object, const wxString& title, bool top_level)
             : SolidsDlg(parent, object, title, false, false)
{
	// add all the controls to the left side

	if(top_level)
	{
		HeeksObjDlg::AddControlsAndCreate();
		m_idsSolids->SetFocus();
	}
}

void StockDlg::GetDataRaw(HeeksObj* object)
{
	SolidsDlg::GetDataRaw(object);
}

void StockDlg::SetFromDataRaw(HeeksObj* object)
{
	SolidsDlg::SetFromDataRaw(object);
}

void StockDlg::OnHelp( wxCommandEvent& event )
{
	::wxLaunchDefaultBrowser(_T("http://heeks.net/help/stock"));
}

bool StockDlg::Do(CStock* object)
{
	StockDlg dlg(heeksCAD->GetMainFrame(), object);

	while(1)
	{
		int result = dlg.ShowModal();

		if(result == wxID_OK)
		{
			dlg.GetData(object);
			return true;
		}
		else if(result == ID_SOLIDS_PICK)
		{
			dlg.PickSolids();
		}
		else
		{
			return false;
		}
	}
}