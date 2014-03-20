// PatternDlg.cpp
// Copyright (c) 2014, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include "PatternDlg.h"
#include "../../interface/NiceTextCtrl.h"
#include "../../interface/PictureFrame.h"

BEGIN_EVENT_TABLE(PatternDlg, HeeksObjDlg)
	EVT_TEXT(ID_NUM_COPIES_A, PatternDlg::OnTextChanged)
	EVT_TEXT(ID_X_SHIFT_A, PatternDlg::OnTextChanged)
	EVT_TEXT(ID_Y_SHIFT_A, PatternDlg::OnTextChanged)
	EVT_TEXT(ID_NUM_COPIES_B, PatternDlg::OnTextChanged)
	EVT_TEXT(ID_X_SHIFT_B, PatternDlg::OnTextChanged)
	EVT_TEXT(ID_Y_SHIFT_B, PatternDlg::OnTextChanged)
    EVT_BUTTON(wxID_HELP, PatternDlg::OnHelp)
END_EVENT_TABLE()

PatternDlg::PatternDlg(wxWindow *parent, HeeksObj* object, const wxString& title, bool top_level)
             : HeeksObjDlg(parent, object, title, false)
{
	// add all the controls to the left side
	leftControls.push_back(MakeLabelAndControl(_("Number of Copies A"), m_txtCopies1 = new wxTextCtrl(this, ID_NUM_COPIES_A)));
	leftControls.push_back(MakeLabelAndControl(_("X Shift A"), m_lgthXShift1 = new CLengthCtrl(this, ID_X_SHIFT_A)));
	leftControls.push_back(MakeLabelAndControl(_("Y Shift A"), m_lgthYShift1 = new CLengthCtrl(this, ID_Y_SHIFT_A)));
	leftControls.push_back(MakeLabelAndControl(_("Number of Copies B"), m_txtCopies2 = new wxTextCtrl(this, ID_NUM_COPIES_B)));
	leftControls.push_back(MakeLabelAndControl(_("X Shift B"), m_lgthXShift2 = new CLengthCtrl(this, ID_X_SHIFT_B)));
	leftControls.push_back(MakeLabelAndControl(_("Y Shift B"), m_lgthYShift2 = new CLengthCtrl(this, ID_Y_SHIFT_B)));

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

static wxBitmap shape_bitmap;
static wxBitmap shape2_bitmap;

static void DrawPatternShape(wxMemoryDC &dc, int x, int y, bool original_shape)
{
	dc.DrawBitmap(original_shape ? shape_bitmap : shape2_bitmap, x - 13, y - 16);
}

static const int PATTERN_MARGIN_X = 24;
static const int PATTERN_MARGIN_Y = 26;

void PatternDlg::RedrawBitmap(wxBitmap &bitmap)
{
	long numA, numB;
	m_txtCopies1->GetValue().ToLong(&numA);
	m_txtCopies2->GetValue().ToLong(&numB);

	double xshiftA = m_lgthXShift1->GetValue();
	double yshiftA = m_lgthYShift1->GetValue();
	double xshiftB = m_lgthXShift2->GetValue();
	double yshiftB = m_lgthYShift2->GetValue();

	RedrawBitmap(bitmap, numA, numB, xshiftA, yshiftA, xshiftB, yshiftB);
}

void PatternDlg::RedrawBitmap(wxBitmap &bitmap, CPattern* pattern)
{
	RedrawBitmap(bitmap, pattern->m_copies1, pattern->m_copies2, pattern->m_x_shift1, pattern->m_y_shift1, pattern->m_x_shift2, pattern->m_y_shift2);
}

void PatternDlg::RedrawBitmap(wxBitmap &bitmap, long numA, long numB, double xshiftA, double yshiftA, double xshiftB, double yshiftB)
{
	// paint a picture with Draw commands
	bitmap = wxBitmap(300, 200);
	wxMemoryDC dc(bitmap);

	// paint background
	dc.SetBrush(*wxWHITE_BRUSH);
	dc.SetPen(*wxWHITE_PEN);
	dc.DrawRectangle(0, 0, 300, 200);


	// limit numbers for the drawing
	if(numA > 20)numA = 20;
	if(numB > 20)numB = 20;

	double total_x = fabs(xshiftA) * numA + fabs(xshiftB) * numB;
	double total_y = fabs(yshiftA) * numA + fabs(yshiftB) * numB;

	CBox box;
	box.Insert(0, 0, 0);
	if(numA > 0 && numB > 0)
	{
		box.Insert(xshiftA * (numA-1), yshiftA * (numA-1), 0);
		box.Insert(xshiftB * (numB-1), yshiftB * (numB-1), 0);
		box.Insert(xshiftA * (numA-1) + xshiftB * (numB-1), yshiftA * (numA-1) + yshiftB * (numB-1), 0);
	}

	double with_margins_x = box.Width() + 2*PATTERN_MARGIN_X;
	double with_margins_y = box.Height() + 2*PATTERN_MARGIN_Y;

	double scale_x = 1000000000.0;
	if(box.Width() > 0.000000001)scale_x = (300.0 - 2*PATTERN_MARGIN_X)/box.Width();
	double scale_y = 1000000000.0;
	if(box.Height() > 0.000000001)scale_y = (200.0 - 2*PATTERN_MARGIN_Y)/box.Height();

	// use the smallest scale
	double scale = scale_x;
	if(scale_y < scale)scale = scale_y;

	double ox = -box.MinX();
	double oy = -box.MinY();

	wxImage img(
#ifdef HEEKSCAD
		wxGetApp().GetResFolder()
#else
		theApp.GetResFolder()
#endif
		+ _T("/bitmaps/pattern/shape.png"), wxBITMAP_TYPE_PNG);

	wxImage img2(
#ifdef HEEKSCAD
		wxGetApp().GetResFolder()
#else
		theApp.GetResFolder()
#endif
		+ _T("/bitmaps/pattern/shape2.png"), wxBITMAP_TYPE_PNG);

	int num = numA;
	if(numB>num)num = numB;
	if(num > 5)
	{
		double s = (24.0 - num)/19.0;
		int neww = 26.0 * s;
		int newh = 31.0 * s;
		img.Rescale(neww, newh);
		img2.Rescale(neww, newh);
	}

	shape_bitmap = wxBitmap(img);
	shape2_bitmap = wxBitmap(img2);

	// draw the pattern
	for(int j = 0; j<numB; j++)
	{
		for(int i = 0; i<numA; i++)
		{
			double x = PATTERN_MARGIN_X + (ox + i * xshiftA + j * xshiftB)*scale;
			int ix = x;
			double y = PATTERN_MARGIN_Y + (oy + i * yshiftA + j * yshiftB)*scale;
			int iy = 200 - y;
			DrawPatternShape(dc, ix, iy, (i==0) && (j==0));
		}
	}

	dc.SelectObject(wxNullBitmap);
}

void PatternDlg::SetPictureByWindow(wxWindow* w)
{
	RedrawBitmap(m_picture->m_bitmap);
	m_picture->m_bitmap_set = true;

	m_picture->Refresh();
}

void PatternDlg::OnHelp( wxCommandEvent& event )
{
	::wxLaunchDefaultBrowser(_T("http://heeks.net/help/pattern"));
}

void PatternDlg::OnTextChanged( wxCommandEvent& event )
{
	RedrawBitmap(m_picture->m_bitmap);

	m_picture->Refresh();
}
