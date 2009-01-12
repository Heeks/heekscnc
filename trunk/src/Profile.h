// Profile.h

class CProfileParams{
public:
	double m_tool_diameter;
	double m_clearance_height;
	double m_final_depth;
	double m_rapid_down_to_height;
	double m_horizontal_feed_rate;
	double m_vertical_feed_rate;
	double m_spindle_speed;
	int m_tool_on_side; // -1=right, 0=on, 1=left

	void set_initial_values();
	void write_values_to_config();
};

class CProfile{
public:
	std::list<HeeksObj*> m_sketches;
	CProfileParams m_params;

	CProfile(const std::list<HeeksObj*> &sketches):m_sketches(sketches){m_params.set_initial_values();}

	int DoDialog();
	void AppendTextToProgram();
};

class CProfileDialog: public wxDialog{
public:
	CProfile* m_profile;
    wxPanel *m_panel;
	wxTextCtrl* m_diameter_text_ctrl;
	wxComboBox* m_tool_on_side_combo;
	wxTextCtrl* m_clearance_text_ctrl;
	wxTextCtrl* m_final_depth_text_ctrl;
	wxTextCtrl* m_rapid_down_text_ctrl;
	wxTextCtrl* m_horizontal_feed_text_ctrl;
	wxTextCtrl* m_vertical_feed_text_ctrl;
	wxTextCtrl* m_spindle_speed_text_ctrl;

	CProfileDialog(wxWindow *parent, const wxString& title, CProfile& profile);

    void OnButtonOK(wxCommandEvent& event);
    void OnButtonCancel(wxCommandEvent& event);

private:
	DECLARE_EVENT_TABLE()
};
