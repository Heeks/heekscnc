// ScriptOpDlg.cpp
// Copyright (c) 2010, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include "ScriptOpDlg.h"
#include "interface/PictureFrame.h"
#include "interface/NiceTextCtrl.h"
#include "ScriptOp.h"
#include "Program.h"
#include "Tools.h"
#include "Patterns.h"
#include "Surfaces.h"

BEGIN_EVENT_TABLE(ScriptOpDlg, OpDlg)
    EVT_TEXT(ID_SCRIPT_TXT, ScriptOpDlg::OnScriptText)
    EVT_BUTTON(wxID_HELP, ScriptOpDlg::OnHelp)
END_EVENT_TABLE()

ScriptOpDlg::ScriptOpDlg(wxWindow *parent, CScriptOp* object, const wxString& title, bool top_level)
             : OpDlg(parent, object, title, false, false, false)
{
	{
		wxBoxSizer *sizer_vertical = new wxBoxSizer(wxVERTICAL);
		wxStaticText *static_label = new wxStaticText(this, wxID_ANY, _("Script"));
		sizer_vertical->Add( static_label, 0, wxRIGHT | wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, control_border );
		sizer_vertical->Add( m_txtScript = new wxTextCtrl(this, ID_SCRIPT_TXT, wxEmptyString, wxDefaultPosition, wxSize(300, 200), wxTE_MULTILINE | wxTE_DONTWRAP | wxTE_RICH | wxTE_RICH2), 1, wxLEFT | wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL, control_border );
		leftControls.push_back(HControl(sizer_vertical, wxEXPAND | wxALL));
	}

	if(top_level)
	{
		HeeksObjDlg::AddControlsAndCreate();
		m_txtScript->SetFocus();
	}
}

void ScriptOpDlg::GetDataRaw(HeeksObj* object)
{
	((CScriptOp*)object)->m_str = m_txtScript->GetValue();

	OpDlg::GetDataRaw(object);
}

void ScriptOpDlg::SetFromDataRaw(HeeksObj* object)
{
	m_txtScript->SetValue(((CScriptOp*)object)->m_str);

	OpDlg::SetFromDataRaw(object);
}

void ScriptOpDlg::SetPicture(const wxString& name)
{
	HeeksObjDlg::SetPicture(name, _T("scriptop"));
}

void ScriptOpDlg::SetPictureByWindow(wxWindow* w)
{
	OpDlg::SetPictureByWindow(w);
}

void ScriptOpDlg::OnScriptText( wxCommandEvent& event )
{
}

void ScriptOpDlg::OnHelp( wxCommandEvent& event )
{
	::wxLaunchDefaultBrowser(_T("http://heeks.net/help/scriptop"));
}
