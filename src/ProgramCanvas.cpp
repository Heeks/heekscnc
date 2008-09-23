// ProgramCanvas.cpp

#include "stdafx.h"
#include "ProgramCanvas.h"
#include "Program.h"
#include "OutputCanvas.h"
#include "../../HeeksCAD/interface/Tool.h"
#include "../../HeeksCAD/interface/InputMode.h"
#include "../../HeeksCAD/interface/LeftAndRight.h"
#include "../../HeeksCAD/interface/PropertyInt.h"
#include "../../HeeksCAD/interface/PropertyDouble.h"
#include "../../HeeksCAD/interface/PropertyChoice.h"
#include "PythonStuff.h"

BEGIN_EVENT_TABLE(CProgramCanvas, wxScrolledWindow)
    EVT_SIZE(CProgramCanvas::OnSize)
END_EVENT_TABLE()

static void RunProgram()
{
	theApp.m_program->DestroyGLLists();
	heeksCAD->Repaint();// clear the screen
	theApp.m_output_canvas->m_textCtrl->Clear(); // clear the output window
	theApp.m_program->m_create_display_list_next_render = true;
	std::list<HeeksObj*> list;
	list.push_back(theApp.m_program);
	heeksCAD->DrawObjectsOnFront(list);
}

static void OnRun(wxCommandEvent& event)
{
	RunProgram();
}


class CAdderApply:public Tool{
private:
	static wxBitmap* m_bitmap;

public:
	void Run();
	const char* GetTitle(){return "Apply";}
	wxBitmap* Bitmap()
	{
		if(m_bitmap == NULL)
		{
			wxString exe_folder = heeksCAD->GetExeFolder();
			m_bitmap = new wxBitmap(exe_folder + "/bitmaps/apply.png", wxBITMAP_TYPE_PNG);
		}
		return m_bitmap;
	}
	const char* GetToolTip(){return "Add move, and finish";}
};
wxBitmap* CAdderApply::m_bitmap = NULL;

static CAdderApply adder_apply;

class CAdderCancel:public Tool{
private:
	static wxBitmap* m_bitmap;

public:
	void Run(){
		// return to Select mode
		heeksCAD->SetInputMode(heeksCAD->GetSelectMode());
		heeksCAD->Repaint();
	}
	const char* GetTitle(){return "Cancel";}
	wxBitmap* Bitmap()
	{
		if(m_bitmap == NULL)
		{
			wxString exe_folder = heeksCAD->GetExeFolder();
			m_bitmap = new wxBitmap(exe_folder + "/bitmaps/cancel.png", wxBITMAP_TYPE_PNG);
		}
		return m_bitmap;
	}
	const char* GetToolTip(){return "Finish without adding anything";}
};
wxBitmap* CAdderCancel::m_bitmap = NULL;

static CAdderCancel adder_cancel;

class CInitialApply: public CAdderApply
{
	void Run();
	const char* GetToolTip(){return "Add spinde speed and feed rates, and finish";}
};

static CInitialApply initial_apply;

class CInitialAdder: public CInputMode
{
public:
	double m_spindle_speed;
	double m_hfeed;
	double m_vfeed;

	CInitialAdder(){}
	virtual ~CInitialAdder(void){}

	void ReadConfigValues()
	{
		theApp.m_config->Read("SpindleSpeed", &m_spindle_speed, 1000);
		theApp.m_config->Read("HFeed", &m_hfeed, 100);
		theApp.m_config->Read("VFeed", &m_vfeed, 100);
	}
	void WriteConfigValues()
	{
			theApp.m_config->Write("SpindleSpeed", m_spindle_speed);
			theApp.m_config->Write("HFeed", m_hfeed);
			theApp.m_config->Write("VFeed", m_vfeed);
	}

	const char* GetTitle(){return "Adding speeds and feeds";}
	bool OnModeChange()
	{
		ReadConfigValues();
		return true;
	}

	void GetProperties(std::list<Property *> *list);
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p)
	{
		t_list->push_back(&initial_apply);
		t_list->push_back(&adder_cancel);
	}

	void AddTheInitialText()
	{
		char str[1024];
		sprintf(str, "spindle(%lf)\nrate(%lf, %lf)\n", m_spindle_speed, m_hfeed, m_vfeed);
		theApp.m_program_canvas->m_textCtrl->WriteText(str);
		WriteConfigValues();
		heeksCAD->SetInputMode(heeksCAD->GetSelectMode());
		heeksCAD->Repaint();
	}

	bool Done()
	{
		return theApp.m_program_canvas->m_textCtrl->GetValue().Find("spindle") != -1;
	}
};

static CInitialAdder initial_adder;

class CToolApply: public CAdderApply
{
	void Run();
	const char* GetToolTip(){return "Add tool command, and finish";}
};

static CToolApply tool_apply;

class CToolAdder: public CInputMode
{
public:
	int m_station_number;
	double m_diameter;
	double m_corner_radius;

	CToolAdder(){}
	virtual ~CToolAdder(void){}

	void ReadConfigValues()
	{
		theApp.m_config->Read("StationNumber", &m_station_number, 1);
		theApp.m_config->Read("ToolDiameter", &m_diameter, 5);
		theApp.m_config->Read("ToolCornerRad", &m_corner_radius, 0);
	}
	void WriteConfigValues()
	{
		theApp.m_config->Write("StationNumber", m_station_number);
		theApp.m_config->Write("ToolDiameter", m_diameter);
		theApp.m_config->Write("ToolCornerRad", m_corner_radius);
	}

	const char* GetTitle(){return "Adding tool command";}
	bool OnModeChange()
	{
		ReadConfigValues();
		return true;
	}

	void GetProperties(std::list<Property *> *list);
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p)
	{
		t_list->push_back(&tool_apply);
		t_list->push_back(&adder_cancel);
	}

	void AddTheToolText()
	{
		if(!initial_adder.Done())
		{
			initial_adder.ReadConfigValues();
			initial_adder.AddTheInitialText();
		}

		char str[1024];
		sprintf(str, "tool(%d, %lf, %lf)\n", m_station_number, m_diameter, m_corner_radius);
		theApp.m_program_canvas->m_textCtrl->WriteText(str);
		WriteConfigValues();
		heeksCAD->SetInputMode(heeksCAD->GetSelectMode());
		heeksCAD->Repaint();
	}

	bool Done()
	{
		return theApp.m_program_canvas->m_textCtrl->GetValue().Find("tool") != -1;
	}
};

static CToolAdder tool_adder;

void CToolApply::Run(){
	heeksCAD->RefreshInput();
	tool_adder.AddTheToolText();
}

static void set_station_number(int value){tool_adder.m_station_number = value; heeksCAD->RefreshInput(); heeksCAD->Repaint();}
static void set_diameter(double value){tool_adder.m_diameter = value; heeksCAD->RefreshInput(); heeksCAD->Repaint();}
static void set_corner_radius(double value){tool_adder.m_corner_radius = value; heeksCAD->RefreshInput(); heeksCAD->Repaint();}

void CToolAdder::GetProperties(std::list<Property *> *list)
{
	list->push_back(new PropertyInt("station number", m_station_number, set_station_number));
	list->push_back(new PropertyDouble("diameter", m_diameter, set_diameter));
	list->push_back(new PropertyDouble("corner radius", m_corner_radius, set_corner_radius));
}

void CInitialApply::Run(){
	heeksCAD->RefreshInput();
	initial_adder.AddTheInitialText();
}

static void set_spindle_speed(double value){initial_adder.m_spindle_speed = value; heeksCAD->RefreshInput(); heeksCAD->Repaint();}
static void set_hfeed(double value){initial_adder.m_hfeed = value; heeksCAD->RefreshInput(); heeksCAD->Repaint();}
static void set_vfeed(double value){initial_adder.m_vfeed = value; heeksCAD->RefreshInput(); heeksCAD->Repaint();}

void CInitialAdder::GetProperties(std::list<Property *> *list)
{
	list->push_back(new PropertyDouble("spindle speed", m_spindle_speed, set_spindle_speed));
	list->push_back(new PropertyDouble("horizontal feed rate", m_hfeed, set_hfeed));
	list->push_back(new PropertyDouble("vertical feed rate", m_vfeed, set_vfeed));
}

class CMoveAdder: public CInputMode
{
public:
	int m_mode; // 0 - rapid, 1 - feed, 2 - rapidxy, 3 - rapidz, 4 - feedxy, 5 - feedz
	double m_pos[3];

	CMoveAdder():m_mode(0){
		m_pos[0] = m_pos[1] = m_pos[2] = 0.0;
	}
	virtual ~CMoveAdder(void){}

	// virtual functions for InputMode
	const char* GetTitle(){return "Adding a move";}
	void OnMouse( wxMouseEvent& event )
	{
		if(event.MiddleIsDown() || event.GetWheelRotation() != 0)
		{
			heeksCAD->GetSelectMode()->OnMouse(event);
		}
		else{
			if(event.Dragging() || event.Moving())
			{
				SetPosition(event.GetPosition());
			}

			if(event.LeftDown()){
				SetPosition(event.GetPosition());
				AddTheMove();
			}
		}
	}

	void OnRender()
	{
		// draw a move
	}

	void GetProperties(std::list<Property *> *list);

	void GetTools(std::list<Tool*>* t_list, const wxPoint* p)
	{
		t_list->push_back(&adder_apply);
		t_list->push_back(&adder_cancel);
	}

	void SetPosition(const wxPoint& point)
	{
		if(heeksCAD->Digitize(point, m_pos)){
			heeksCAD->RefreshInput();
			heeksCAD->Repaint(true);
		}
	}

	void AddTheMove()
	{
		if(!tool_adder.Done())
		{
			tool_adder.ReadConfigValues();
			tool_adder.AddTheToolText();
		}

		char str[1024];
		switch(m_mode){
			case 0:
				sprintf(str, "rapid(%lf, %lf, %lf)\n", m_pos[0], m_pos[1], m_pos[2]);
				break;
			case 1:
				sprintf(str, "feed(%lf, %lf, %lf)\n", m_pos[0], m_pos[1], m_pos[2]);
				break;
			case 2:
				sprintf(str, "rapidxy(%lf, %lf)\n", m_pos[0], m_pos[1]);
				break;
			case 3:
				sprintf(str, "rapidz(%lf)\n", m_pos[2]);
				break;
			case 4:
				sprintf(str, "feedxy(%lf, %lf)\n", m_pos[0], m_pos[1]);
				break;
			case 5:
				sprintf(str, "feedz(%lf)\n", m_pos[2]);
				break;
		}
		theApp.m_program_canvas->m_textCtrl->WriteText(str);
		heeksCAD->SetInputMode(heeksCAD->GetSelectMode());
		heeksCAD->Repaint();
	}
};

static CMoveAdder move_adder;

void CAdderApply::Run(){
	heeksCAD->RefreshInput();
	move_adder.AddTheMove();
}

static void set_x(double value){move_adder.m_pos[0] = value; heeksCAD->RefreshInput(); heeksCAD->Repaint();}
static void set_y(double value){move_adder.m_pos[1] = value; heeksCAD->RefreshInput(); heeksCAD->Repaint();}
static void set_z(double value){move_adder.m_pos[2] = value; heeksCAD->RefreshInput(); heeksCAD->Repaint();}

class CProfileApply: public CAdderApply
{
	void Run();
	const char* GetToolTip(){return "Add profile command, and finish";}
};

static CProfileApply profile_apply;

class CProfileAdder: public CInputMode, CLeftAndRight
{
public:
	int m_line_arcs_number;
	int m_offset_type; // 0 = "left", 1 = "right", 2 = "on"
	double m_finish_x, m_finish_y;

	CProfileAdder(){}
	virtual ~CProfileAdder(void){}

	void ReadConfigValues()
	{
		theApp.m_config->Read("ProfileOpLANum", &m_line_arcs_number, 1);
		theApp.m_config->Read("ProfileOpOffsetType", &m_offset_type, 0);
		theApp.m_config->Read("ProfileOpFinishX", &m_finish_x, 0);
		theApp.m_config->Read("ProfileOpFinishY", &m_finish_y, 0);
	}
	void WriteConfigValues()
	{
		theApp.m_config->Write("ProfileOpLANum", m_line_arcs_number);
		theApp.m_config->Write("ProfileOpOffsetType", m_offset_type);
		theApp.m_config->Write("ProfileOpFinishX", m_finish_x);
		theApp.m_config->Write("ProfileOpFinishY", m_finish_y);
	}

	const char* GetTitle(){return "Adding profile command";}
	bool OnModeChange()
	{
		ReadConfigValues();
		return true;
	}

	void GetProperties(std::list<Property *> *list);
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);
	void OnMouse( wxMouseEvent& event )
	{
		bool event_used = false;
		if(LeftAndRightPressed(event, event_used))
		{
			AddTheProfileOpText();
		}
	}

	void AddTheProfileOpText()
	{
		if(!tool_adder.Done())
		{
			tool_adder.ReadConfigValues();
			tool_adder.AddTheToolText();
		}

		const char* dirstr = "left";
		switch(m_offset_type){
			case 1:
				dirstr = "right";
				break;
			case 2:
				dirstr = "on";
				break;
		}

		char str[1024];
		char roll_off_str[1024] = "\"NOT_SET\", \"NOT_SET\"";
		if(m_offset_type != 2)
		{
			sprintf(roll_off_str, "%lf, %lf", m_finish_x, m_finish_y);
		}
		sprintf(str, "profile(%d, \"%s\", %s)\n", m_line_arcs_number, dirstr, roll_off_str);
		theApp.m_program_canvas->m_textCtrl->WriteText(str);
		WriteConfigValues();
		heeksCAD->SetInputMode(heeksCAD->GetSelectMode());
		heeksCAD->Repaint();
	}

	bool Done()
	{
		return theApp.m_program_canvas->m_textCtrl->GetValue().Find("profile") != -1;
	}
};

static CProfileAdder profile_adder;

void CProfileApply::Run(){
	heeksCAD->RefreshInput();
	profile_adder.AddTheProfileOpText();
}

class OpPickObjectsTool: public Tool
{
	static wxBitmap* m_bitmap;
public:
	const char* GetTitle(){return "Pick Shape";}
	const char* GetToolTip(){return "Pick shape objects ( clears previous shape objects first )";}
	void Run()
	{
		heeksCAD->PickObjects("Pick shape objects");
		const std::list<HeeksObj*>& marked_list = heeksCAD->GetMarkedList();
		if(marked_list.size() > 0)
		{
			profile_adder.m_line_arcs_number = marked_list.front()->GetID();
		}
		heeksCAD->ClearMarkedList();
	}

	wxBitmap* Bitmap()
	{
		if(m_bitmap == NULL)
		{
			wxString exe_folder = heeksCAD->GetExeFolder();
			m_bitmap = new wxBitmap(exe_folder + "/../HeeksCNC/bitmaps/pickopobjects.png", wxBITMAP_TYPE_PNG);
		}
		return m_bitmap;
	}
};

wxBitmap* OpPickObjectsTool::m_bitmap = NULL;

static OpPickObjectsTool pick_objects_tool;

class OpPickPosTool: public Tool
{
	static wxBitmap* m_bitmap;
public:
	const char* GetTitle(){return "Pick Finish Pos";}
	const char* GetToolTip(){return "Pick Finish XY coordinates";}
	void Run()
	{
		double pos[3];
		if(heeksCAD->PickPosition("Pick finish position", pos)){
			profile_adder.m_finish_x = pos[0];
			profile_adder.m_finish_y = pos[1];
			heeksCAD->RefreshInput();
		}
	}

	wxBitmap* Bitmap()
	{
		if(m_bitmap == NULL)
		{
			wxString exe_folder = heeksCAD->GetExeFolder();
			m_bitmap = new wxBitmap(exe_folder + "/../HeeksCNC/bitmaps/pickoppos.png", wxBITMAP_TYPE_PNG);
		}
		return m_bitmap;
	}
};

wxBitmap* OpPickPosTool::m_bitmap = NULL;

static OpPickPosTool pick_pos_tool;

static void set_line_arcs_number(int value){profile_adder.m_line_arcs_number = value; heeksCAD->RefreshInput(); heeksCAD->Repaint();}
static void set_offset_type(int value){profile_adder.m_offset_type = value; heeksCAD->RefreshInput(); heeksCAD->Repaint();}
static void set_finish_x(double value){profile_adder.m_finish_x = value; heeksCAD->RefreshInput(); heeksCAD->Repaint();}
static void set_finish_y(double value){profile_adder.m_finish_y = value; heeksCAD->RefreshInput(); heeksCAD->Repaint();}

void CProfileAdder::GetProperties(std::list<Property *> *list)
{
	list->push_back(new PropertyInt("line arc collection ID", m_line_arcs_number, set_line_arcs_number));
	std::list<std::string> choices;
	choices.push_back(std::string("left"));
	choices.push_back(std::string("right"));
	choices.push_back(std::string("on"));
	list->push_back(new PropertyChoice("offset type", choices, m_offset_type, set_offset_type));
	if(m_offset_type != 2)
	{
		list->push_back(new PropertyDouble("X roll off position", m_finish_x, set_finish_x));
		list->push_back(new PropertyDouble("Y roll off position", m_finish_y, set_finish_y));
	}
}

void CProfileAdder::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
	t_list->push_back(&profile_apply);
	t_list->push_back(&adder_cancel);
	t_list->push_back(&pick_objects_tool);
	t_list->push_back(&pick_pos_tool);
}

void CMoveAdder::GetProperties(std::list<Property *> *list)
{
	if(m_mode != 3 && m_mode != 5){
		list->push_back(new PropertyDouble("X", m_pos[0], set_x));
		list->push_back(new PropertyDouble("Y", m_pos[1], set_y));
	}
	if(m_mode != 2 && m_mode != 4){
		list->push_back(new PropertyDouble("Z", m_pos[2], set_z));
	}
}

static void OnAddRapid(wxCommandEvent& event)
{
	move_adder.m_mode = 0;
	heeksCAD->SetInputMode(&move_adder);
}

static void OnAddRapidXY(wxCommandEvent& event)
{
	move_adder.m_mode = 2;
	heeksCAD->SetInputMode(&move_adder);
}

static void OnAddRapidZ(wxCommandEvent& event)
{
	move_adder.m_mode = 3;
	heeksCAD->SetInputMode(&move_adder);
}

static void OnAddFeed(wxCommandEvent& event)
{
	move_adder.m_mode = 1;
	heeksCAD->SetInputMode(&move_adder);
}

static void OnAddFeedXY(wxCommandEvent& event)
{
	move_adder.m_mode = 4;
	heeksCAD->SetInputMode(&move_adder);
}

static void OnAddFeedZ(wxCommandEvent& event)
{
	move_adder.m_mode = 5;
	heeksCAD->SetInputMode(&move_adder);
}

static void OnAddInitial(wxCommandEvent& event)
{
	heeksCAD->SetInputMode(&initial_adder);
}

static void OnAddTool(wxCommandEvent& event)
{
	heeksCAD->SetInputMode(&tool_adder);
}

static void OnAddProfileOp(wxCommandEvent& event)
{
	heeksCAD->SetInputMode(&profile_adder);
}

static void OnPostProcess(wxCommandEvent& event)
{
	HeeksPyPostProcess();
}

CProgramCanvas::CProgramCanvas(wxWindow* parent)
        : wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                           wxHSCROLL | wxVSCROLL | wxNO_FULL_REPAINT_ON_RESIZE)
{
	m_textCtrl = new CProgramTextCtrl( this, 100, _T(""),
		wxPoint(180,170), wxSize(200,70), wxTE_MULTILINE | wxTE_DONTWRAP);

	// make a tool bar
	m_toolBar = new wxToolBar(this, -1, wxDefaultPosition, wxDefaultSize, wxTB_NODIVIDER | wxTB_FLAT);
	m_toolBar->SetToolBitmapSize(wxSize(32, 32));

	// add toolbar buttons
	wxString exe_folder = heeksCAD->GetExeFolder();
	heeksCAD->AddToolBarButton(m_toolBar, _T("Run"), wxBitmap(exe_folder + "/../HeeksCNC/bitmaps/run.png", wxBITMAP_TYPE_PNG), _T("Run the program"), OnRun);
	heeksCAD->AddToolBarButton(m_toolBar, _T("Add Initial"), wxBitmap(exe_folder + "/../HeeksCNC/bitmaps/initial.png", wxBITMAP_TYPE_PNG), _T("Add spindle speed and feed rates"), OnAddInitial);
	heeksCAD->AddToolBarButton(m_toolBar, _T("Add Tool"), wxBitmap(exe_folder + "/../HeeksCNC/bitmaps/tool.png", wxBITMAP_TYPE_PNG), _T("Add tool command"), OnAddTool);
	heeksCAD->AddToolBarButton(m_toolBar, _T("Add Rapid XYZ"), wxBitmap(exe_folder + "/../HeeksCNC/bitmaps/rapid.png", wxBITMAP_TYPE_PNG), _T("Add a rapid move"), OnAddRapid);
	heeksCAD->AddToolBarButton(m_toolBar, _T("Add Rapid XY"), wxBitmap(exe_folder + "/../HeeksCNC/bitmaps/rapidxy.png", wxBITMAP_TYPE_PNG), _T("Add a rapid move"), OnAddRapidXY);
	heeksCAD->AddToolBarButton(m_toolBar, _T("Add Rapid Z"), wxBitmap(exe_folder + "/../HeeksCNC/bitmaps/rapidz.png", wxBITMAP_TYPE_PNG), _T("Add a rapid move"), OnAddRapidZ);
	heeksCAD->AddToolBarButton(m_toolBar, _T("Add Feed XYZ"), wxBitmap(exe_folder + "/../HeeksCNC/bitmaps/feed.png", wxBITMAP_TYPE_PNG), _T("Add a feed move"), OnAddFeed);
	heeksCAD->AddToolBarButton(m_toolBar, _T("Add Feed XY"), wxBitmap(exe_folder + "/../HeeksCNC/bitmaps/feedxy.png", wxBITMAP_TYPE_PNG), _T("Add a feed move"), OnAddFeedXY);
	heeksCAD->AddToolBarButton(m_toolBar, _T("Add Feed Z"), wxBitmap(exe_folder + "/../HeeksCNC/bitmaps/feedz.png", wxBITMAP_TYPE_PNG), _T("Add a feed move"), OnAddFeedZ);
	heeksCAD->AddToolBarButton(m_toolBar, _T("Add Profile Operation"), wxBitmap(exe_folder + "/../HeeksCNC/bitmaps/opprofile.png", wxBITMAP_TYPE_PNG), _T("Add a profile operation"), OnAddProfileOp);

	// these buttons always go at then end
	heeksCAD->AddToolBarButton(m_toolBar, _T("Post Process"), wxBitmap(exe_folder + "/../HeeksCNC/bitmaps/postprocess.png", wxBITMAP_TYPE_PNG), _T("Post process"), OnPostProcess);

	m_toolBar->Realize();

	Resize();
}


void CProgramCanvas::OnSize(wxSizeEvent& event)
{
    Resize();

    event.Skip();
}

void CProgramCanvas::Resize()
{
	wxSize size = GetClientSize();
	wxSize toolbar_size = m_toolBar->GetClientSize();
	m_textCtrl->SetSize(0, 0, size.x, size.y - 39 );
	m_toolBar->SetSize(0, size.y - 39 , size.x, 39 );
}

void CProgramCanvas::Clear()
{
	m_textCtrl->Clear();
}

BEGIN_EVENT_TABLE(CProgramTextCtrl, wxTextCtrl)
	EVT_CHAR(CProgramTextCtrl::OnChar)
    EVT_DROP_FILES(CProgramTextCtrl::OnDropFiles)
    EVT_MENU(wxID_CUT, CProgramTextCtrl::OnCut)
    EVT_MENU(wxID_COPY, CProgramTextCtrl::OnCopy)
    EVT_MENU(wxID_PASTE, CProgramTextCtrl::OnPaste)
    EVT_MENU(wxID_UNDO, CProgramTextCtrl::OnUndo)
    EVT_MENU(wxID_REDO, CProgramTextCtrl::OnRedo)
    EVT_MENU(wxID_CLEAR, CProgramTextCtrl::OnDelete)
    EVT_MENU(wxID_SELECTALL, CProgramTextCtrl::OnSelectAll)
END_EVENT_TABLE()

void CProgramTextCtrl::OnDropFiles(wxDropFilesEvent& event)
{
	__super::OnDropFiles(event);
}

void CProgramTextCtrl::OnChar(wxKeyEvent& event)
{
	__super::OnChar(event);

	// check for Enter key
    switch ( event.GetKeyCode() )
    {
        case WXK_RETURN:
			{
				if(theApp.m_run_program_on_new_line)
				{
					RunProgram();
				}
			}
			break;
	}
}

void CProgramTextCtrl::OnCut(wxCommandEvent& event)
{
	__super::OnCut(event);
}

void CProgramTextCtrl::OnCopy(wxCommandEvent& event)
{
	__super::OnCopy(event);
}

void CProgramTextCtrl::OnPaste(wxCommandEvent& event)
{
	__super::OnPaste(event);
}

void CProgramTextCtrl::OnUndo(wxCommandEvent& event)
{
	__super::OnUndo(event);
}

void CProgramTextCtrl::OnRedo(wxCommandEvent& event)
{
	__super::OnRedo(event);
}

void CProgramTextCtrl::OnDelete(wxCommandEvent& event)
{
	__super::OnDelete(event);
}

void CProgramTextCtrl::OnSelectAll(wxCommandEvent& event)
{
	__super::OnSelectAll(event);
}

void CProgramTextCtrl::WriteText(const wxString& text)
{
	__super::WriteText(text);

	if(theApp.m_run_program_on_new_line)
	{
		RunProgram();
	}
}