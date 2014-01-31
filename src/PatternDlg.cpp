// PatternDlg.cpp
// Copyright (c) 2014, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include "PatternDlg.h"
#include "interface/NiceTextCtrl.h"

PatternDlg::PatternDlg(wxWindow *parent, HeeksObj* object, const wxString& title, bool top_level)
             : HeeksObjDlg(parent, object, title, false)
{
	// add all the controls to the left side
	leftControls.push_back(MakeLabelAndControl(_("Number of copies A"), m_txtCopies1 = new wxTextCtrl(this, wxID_ANY)));
	leftControls.push_back(MakeLabelAndControl(_("X shift A"), m_lgthXShift1 = new CLengthCtrl(this)));
	leftControls.push_back(MakeLabelAndControl(_("Y shift A"), m_lgthYShift1 = new CLengthCtrl(this)));
	leftControls.push_back(MakeLabelAndControl(_("Number of copies B"), m_txtCopies2 = new wxTextCtrl(this, wxID_ANY)));
	leftControls.push_back(MakeLabelAndControl(_("X shift B"), m_lgthXShift2 = new CLengthCtrl(this)));
	leftControls.push_back(MakeLabelAndControl(_("Y shift B"), m_lgthYShift2 = new CLengthCtrl(this)));

	if(top_level)
	{
		HeeksObjDlg::AddControlsAndCreate();
		m_txtCopies1->SetFocus();
	}
}

void PatternDlg::GetDataRaw(HeeksObj* object)
{
	long i = 0;
	m_txtCopies1->GetValue().ToLong(&i);
	((CPattern*)object)->m_copies1 = i;
	((CPattern*)object)->m_x_shift1 = m_lgthXShift1->GetValue();
	((CPattern*)object)->m_y_shift1 = m_lgthYShift1->GetValue();
	m_txtCopies2->GetValue().ToLong(&i);
	((CPattern*)object)->m_copies2 = i;
	((CPattern*)object)->m_x_shift2 = m_lgthXShift2->GetValue();
	((CPattern*)object)->m_y_shift2 = m_lgthYShift2->GetValue();
}

void PatternDlg::SetFromDataRaw(HeeksObj* object)
{
	m_txtCopies1->SetValue(wxString::Format(_T("%d"), ((CPattern*)object)->m_copies1));
	m_lgthXShift1->SetValue(((CPattern*)object)->m_x_shift1);
	m_lgthYShift1->SetValue(((CPattern*)object)->m_y_shift1);
	m_txtCopies2->SetValue(wxString::Format(_T("%d"), ((CPattern*)object)->m_copies2));
	m_lgthXShift2->SetValue(((CPattern*)object)->m_x_shift2);
	m_lgthYShift2->SetValue(((CPattern*)object)->m_y_shift2);
}

void PatternDlg::SetPicture(const wxString& name)
{
	HeeksObjDlg::SetPicture(name, _T("pattern"));
}

void PatternDlg::SetPictureByWindow(wxWindow* w)
{
	if(w == m_txtCopies1)SetPicture(_T("copies1"));
	else if(w == m_lgthXShift1)SetPicture(_T("xshift1"));
	else if(w == m_lgthYShift1)SetPicture(_T("yshift1"));
	else if(w == m_txtCopies2)SetPicture(_T("copies2"));
	else if(w == m_lgthXShift2)SetPicture(_T("xshift2"));
	else if(w == m_lgthYShift2)SetPicture(_T("yshift2"));
	else SetPicture(_T("general"));
}
