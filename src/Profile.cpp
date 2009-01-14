// Profile.cpp
// copyright Dan Heeks from Jan 11th 2009

#include "stdafx.h"
#include "Profile.h"
#include "CNCConfig.h"
#include "ProgramCanvas.h"
#include "../../interface/HeeksObj.h"

void CProfileParams::set_initial_values()
{
	CNCConfig config;
	config.Read(_T("ProfileToolDiameter"), &m_tool_diameter, 3.0);
	config.Read(_T("ProfileClearanceHeight"), &m_clearance_height, 5.0);
	config.Read(_T("ProfileFinalDepth"), &m_final_depth, -0.1);
	config.Read(_T("ProfileRapidDown"), &m_rapid_down_to_height, 2.0);
	config.Read(_T("ProfileHorizFeed"), &m_horizontal_feed_rate, 100.0);
	config.Read(_T("ProfileVertFeed"), &m_vertical_feed_rate, 100.0);
	config.Read(_T("ProfileSpindleSpeed"), &m_spindle_speed, 7000);
}

void CProfileParams::write_values_to_config()
{
	CNCConfig config;
	config.Write(_T("ProfileToolDiameter"), m_tool_diameter);
	config.Write(_T("ProfileClearanceHeight"), m_clearance_height);
	config.Write(_T("ProfileFinalDepth"), m_final_depth);
	config.Write(_T("ProfileRapidDown"), m_rapid_down_to_height);
	config.Write(_T("ProfileHorizFeed"), m_horizontal_feed_rate);
	config.Write(_T("ProfileVertFeed"), m_vertical_feed_rate);
	config.Write(_T("ProfileSpindleSpeed"), m_spindle_speed);
}

int CProfile::DoDialog()
{
	CProfileDialog dlg(heeksCAD->GetMainFrame(), _("Profile Cut"), *this);
	return dlg.ShowModal();
}

static void WriteSketchDefn(HeeksObj* sketch)
{
	theApp.m_program_canvas->m_textCtrl->AppendText(wxString::Format(_T("k%d = kurve.new()\n"), sketch->m_id));

	bool started = false;

	for(HeeksObj* span_object = sketch->GetFirstChild(); span_object; span_object = sketch->GetNextChild())
	{
		double s[3] = {0, 0, 0};
		double e[3] = {0, 0, 0};
		double c[3] = {0, 0, 0};

		if(span_object){
			int type = span_object->GetType();
			if(type == LineType || type == ArcType)
			{
				if(!started)
				{
					span_object->GetStartPoint(s);
					theApp.m_program_canvas->m_textCtrl->AppendText(wxString::Format(_T("kurve.add_point(k%d, %d, %g, %g, %g, %g)\n"), sketch->m_id, 0, s[0], s[1], 0.0, 0.0));
					started = true;
				}
				span_object->GetEndPoint(e);
				if(type == LineType)
				{
					theApp.m_program_canvas->m_textCtrl->AppendText(wxString::Format(_T("kurve.add_point(k%d, %d, %g, %g, %g, %g)\n"), sketch->m_id, 0, e[0], e[1], 0.0, 0.0));
				}
				else if(type == ArcType)
				{
					span_object->GetCentrePoint(c);
					double pos[3];
					heeksCAD->GetArcAxis(span_object, pos);
					int span_type = (pos[2] >=0) ? 1:-1;
					theApp.m_program_canvas->m_textCtrl->AppendText(wxString::Format(_T("kurve.add_point(k%d, %d, %g, %g, %g, %g)\n"), sketch->m_id, span_type, e[0], e[1], c[0], c[1]));
				}
			}
		}
	}
}

void CProfile::AppendTextToProgram()
{
	theApp.m_program_canvas->m_textCtrl->AppendText(wxString::Format(_T("clearance = %g\n"), m_params.m_clearance_height));
	theApp.m_program_canvas->m_textCtrl->AppendText(wxString::Format(_T("rapid_down_to_height = %g\n"), m_params.m_rapid_down_to_height));
	theApp.m_program_canvas->m_textCtrl->AppendText(wxString::Format(_T("final_depth = %g\n"), m_params.m_final_depth));
	theApp.m_program_canvas->m_textCtrl->AppendText(wxString::Format(_T("spindle(%g)\n"), m_params.m_spindle_speed));
	theApp.m_program_canvas->m_textCtrl->AppendText(wxString::Format(_T("feedrate(%g)\n"), m_params.m_horizontal_feed_rate));
	theApp.m_program_canvas->m_textCtrl->AppendText(wxString::Format(_T("tool(1, %g, 0)\n"), m_params.m_tool_diameter));
	for(std::list<HeeksObj*>::iterator It = m_sketches.begin(); It != m_sketches.end(); It++)
	{
		HeeksObj* sketch = *It;

		// write a kurve definition
		WriteSketchDefn(sketch);
		wxString side_string;
		switch(m_params.m_tool_on_side)
		{
		case 1:
			side_string = _T("left");
			break;
		case -1:
			side_string = _T("right");
			break;
		default:
			side_string = _T("on");
			break;
		}
		theApp.m_program_canvas->m_textCtrl->AppendText(wxString::Format(_T("profile(k%d, '%s', clearance, rapid_down_to_height, final_depth)\n"), sketch->m_id, side_string.c_str()));
	}
}

BEGIN_EVENT_TABLE( CProfileDialog, wxDialog )
EVT_BUTTON( wxID_OK, CProfileDialog::OnButtonOK )
EVT_BUTTON( wxID_CANCEL, CProfileDialog::OnButtonCancel )
END_EVENT_TABLE()

CProfileDialog::CProfileDialog(wxWindow *parent, const wxString& title, CProfile& profile):wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	m_profile = &profile;

    m_panel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);

    // create buttons
    wxButton *button1 = new wxButton(m_panel, wxID_OK);
    wxButton *button2 = new wxButton(m_panel, wxID_CANCEL);

    wxBoxSizer *mainsizer = new wxBoxSizer( wxVERTICAL );

    wxFlexGridSizer *gridsizer = new wxFlexGridSizer(2, 5, 5);

	gridsizer->Add(new wxStaticText(m_panel, wxID_ANY, _("Tool diameter")), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL));
	m_diameter_text_ctrl = new wxTextCtrl(m_panel, wxID_ANY, wxString::Format(_T("%g"), m_profile->m_params.m_tool_diameter));
	gridsizer->Add(m_diameter_text_ctrl, wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL).Expand());

    wxString choices[] =
    {
        _("Left"),
        _("Right"),
        _("On"),
    };

	int choice = 0;
	if(m_profile->m_params.m_tool_on_side == -1)choice = 1;
	else if(m_profile->m_params.m_tool_on_side == 0)choice = 2;

	gridsizer->Add(new wxStaticText(m_panel, wxID_ANY, _("Tool on side")), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL));
	m_tool_on_side_combo = new wxComboBox( m_panel, wxID_ANY, _T("This"), wxDefaultPosition, wxDefaultSize, 3, choices, wxTE_PROCESS_ENTER);
	m_tool_on_side_combo->SetSelection(0);
	gridsizer->Add(m_tool_on_side_combo, wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL).Expand());

	gridsizer->Add(new wxStaticText(m_panel, wxID_ANY, _("Clearance height")), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL));
	m_clearance_text_ctrl = new wxTextCtrl(m_panel, wxID_ANY, wxString::Format(_T("%g"), m_profile->m_params.m_clearance_height));
	gridsizer->Add(m_clearance_text_ctrl, wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL).Expand());

	gridsizer->Add(new wxStaticText(m_panel, wxID_ANY, _("Final depth")), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL));
	m_final_depth_text_ctrl = new wxTextCtrl(m_panel, wxID_ANY, wxString::Format(_T("%g"), m_profile->m_params.m_final_depth));
	gridsizer->Add(m_final_depth_text_ctrl, wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL).Expand());

	gridsizer->Add(new wxStaticText(m_panel, wxID_ANY, _("Rapid down to depth")), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL));
	m_rapid_down_text_ctrl = new wxTextCtrl(m_panel, wxID_ANY, wxString::Format(_T("%g"), m_profile->m_params.m_rapid_down_to_height));
	gridsizer->Add(m_rapid_down_text_ctrl, wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL).Expand());

	gridsizer->Add(new wxStaticText(m_panel, wxID_ANY, _("Horizontal feedrate")), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL));
	m_horizontal_feed_text_ctrl = new wxTextCtrl(m_panel, wxID_ANY, wxString::Format(_T("%g"), m_profile->m_params.m_horizontal_feed_rate));
	gridsizer->Add(m_horizontal_feed_text_ctrl, wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL).Expand());

	gridsizer->Add(new wxStaticText(m_panel, wxID_ANY, _("Vertical feedrate")), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL));
	m_vertical_feed_text_ctrl = new wxTextCtrl(m_panel, wxID_ANY, wxString::Format(_T("%g"), m_profile->m_params.m_vertical_feed_rate));
	gridsizer->Add(m_vertical_feed_text_ctrl, wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL).Expand());

	gridsizer->Add(new wxStaticText(m_panel, wxID_ANY, _("Spindle speed")), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL));
	m_spindle_speed_text_ctrl = new wxTextCtrl(m_panel, wxID_ANY, wxString::Format(_T("%g"), m_profile->m_params.m_spindle_speed));
	gridsizer->Add(m_spindle_speed_text_ctrl, wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL).Expand());

	gridsizer->AddGrowableCol(1, 1);

    wxBoxSizer *bottomsizer = new wxBoxSizer( wxHORIZONTAL );

    bottomsizer->Add( button1, 0, wxALL, 10 );
    bottomsizer->Add( button2, 0, wxALL, 10 );

    mainsizer->Add( gridsizer, wxSizerFlags().Align(wxALIGN_CENTER).Border(wxALL, 10).Expand() );
    mainsizer->Add( bottomsizer, wxSizerFlags().Align(wxALIGN_CENTER) );

    // tell frame to make use of sizer (or constraints, if any)
    m_panel->SetAutoLayout( true );
    m_panel->SetSizer( mainsizer );

#ifndef __WXWINCE__
    // don't allow frame to get smaller than what the sizers tell ye
    mainsizer->SetSizeHints( this );
#endif

    Show(true);
}

static bool GetNumber(wxTextCtrl* text_ctrl, double *value)
{
	if(text_ctrl->GetValue().ToDouble(value))return true;
	wxMessageBox(_("this must be a number!")); 
	text_ctrl->SetFocus();
	return false;
}

static bool CheckGreaterThanZero(wxTextCtrl* text_ctrl, double value)
{
	if(value > 0.000000001)return true;
	wxMessageBox(_("this must be greater than zero!")); 
	text_ctrl->SetFocus();
	return false;
}

void CProfileDialog::OnButtonOK(wxCommandEvent& event)
{
	CProfileParams temp_params;

	if(!GetNumber(m_diameter_text_ctrl, &temp_params.m_tool_diameter))return;
	if(!CheckGreaterThanZero(m_diameter_text_ctrl, temp_params.m_tool_diameter))return;
	if(!GetNumber(m_clearance_text_ctrl, &temp_params.m_clearance_height))return;
	if(!GetNumber(m_final_depth_text_ctrl, &temp_params.m_final_depth))return;
	if(!GetNumber(m_rapid_down_text_ctrl, &temp_params.m_rapid_down_to_height))return;
	if(!CheckGreaterThanZero(m_rapid_down_text_ctrl, temp_params.m_rapid_down_to_height))return;
	if(!GetNumber(m_horizontal_feed_text_ctrl, &temp_params.m_horizontal_feed_rate))return;
	if(!CheckGreaterThanZero(m_horizontal_feed_text_ctrl, temp_params.m_horizontal_feed_rate))return;
	if(!GetNumber(m_vertical_feed_text_ctrl, &temp_params.m_vertical_feed_rate))return;
	if(!CheckGreaterThanZero(m_vertical_feed_text_ctrl, temp_params.m_vertical_feed_rate))return;
	if(!GetNumber(m_spindle_speed_text_ctrl, &temp_params.m_spindle_speed))return;
	if(!CheckGreaterThanZero(m_spindle_speed_text_ctrl, temp_params.m_spindle_speed))return;
	switch(m_tool_on_side_combo->GetSelection())
	{
	case 0:
		temp_params.m_tool_on_side = 1;
		break;
	case 1:
		temp_params.m_tool_on_side = -1;
		break;
	default:
		temp_params.m_tool_on_side = 0;
		break;
	}

	m_profile->m_params = temp_params;

	EndModal(wxID_OK);
}

void CProfileDialog::OnButtonCancel(wxCommandEvent& event)
{
	EndModal(wxID_CANCEL);
}
