// ProgramCanvas.cpp

#include "stdafx.h"
#include "ProgramCanvas.h"
#include "Program.h"
#include "OutputCanvas.h"
#include "../../HeeksCAD/interface/Tool.h"
#include "../../HeeksCAD/interface/InputMode.h"
#include "../../HeeksCAD/interface/PropertyDouble.h"

BEGIN_EVENT_TABLE(CProgramCanvas, wxScrolledWindow)
    EVT_SIZE(CProgramCanvas::OnSize)
END_EVENT_TABLE()

static void OnRun(wxCommandEvent& event)
{
	theApp.m_program->DestroyGLLists();
	heeksCAD->Repaint();// clear the screen
	theApp.m_output_canvas->m_textCtrl->Clear(); // clear the output window
	theApp.m_program->m_create_display_list_next_render = true;
	std::list<HeeksObj*> list;
	list.push_back(theApp.m_program);
	heeksCAD->DrawObjectsOnFront(list);
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

class CMoveAdder: public CInputMode
{
public:
	int m_mode; // 0 - rapid, 1 - feed
	double m_pos[3];

	CMoveAdder():m_mode(0){
		m_pos[0] = m_pos[1] = m_pos[2] = 0.0;
	}
	virtual ~CMoveAdder(void){}

	// virtual functions for InputMode
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

	bool OnStart()
	{
		m_pos[0] = 0.0;
		m_pos[1] = 0.0;
		m_pos[2] = 0.0;
	}

	void OnRender()
	{
		// draw a move
	}

	void GetProperties(std::list<Property *> *list);

	void GetTools(std::list<Tool*>* t_list, const wxPoint* p)
	{
		t_list->push_back(new CAdderApply);
		t_list->push_back(new CAdderCancel);
	}

	void SetPosition(const wxPoint& point)
	{
		if(heeksCAD->Digitize(point, m_pos)){
			heeksCAD->RefreshOptions();
			heeksCAD->Repaint(true);
		}
	}

	void AddTheMove()
	{
		char str[1024];
		sprintf(str, "%s(%lf, %lf, %lf)\n", m_mode ? "feed" : "rapid", m_pos[0], m_pos[1], m_pos[2]);
		theApp.m_program_canvas->m_textCtrl->AppendText(str);
		heeksCAD->SetInputMode(heeksCAD->GetSelectMode());
		heeksCAD->Repaint();
	}
};

static CMoveAdder move_adder;

class CInitialApply: public CAdderApply
{
	void Run();
	const char* GetToolTip(){return "Add move, and finish";}
};

class CInitialAdder: public CInputMode
{
public:
	double m_spindle_speed;
	double m_hfeed;
	double m_vfeed;

	CInitialAdder():m_spindle_speed(1000), m_hfeed(100), m_vfeed(100){}
	virtual ~CInitialAdder(void){}

	bool OnStart()
	{
		m_spindle_speed = 1000;
		m_hfeed = 100;
		m_vfeed = 100;
	}

	void GetProperties(std::list<Property *> *list);
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p)
	{
		t_list->push_back(new CInitialApply);
		t_list->push_back(new CAdderCancel);
	}

	void AddTheInitialText()
	{
		char str[1024];
		sprintf(str, "spindle(%lf)\nrate(%lf, %lf)\n", m_spindle_speed, m_hfeed, m_vfeed);
		theApp.m_program_canvas->m_textCtrl->AppendText(str);
		heeksCAD->SetInputMode(heeksCAD->GetSelectMode());
		heeksCAD->Repaint();
	}
};

static CInitialAdder initial_adder;

void CInitialApply::Run(){
	initial_adder.AddTheInitialText();
}

static void set_spindle_speed(double value){initial_adder.m_spindle_speed = value; heeksCAD->Repaint();}
static void set_hfeed(double value){initial_adder.m_hfeed = value; heeksCAD->Repaint();}
static void set_vfeed(double value){initial_adder.m_vfeed = value; heeksCAD->Repaint();}

void CInitialAdder::GetProperties(std::list<Property *> *list)
{
	list->push_back(new PropertyDouble("spindle speed", m_spindle_speed, set_spindle_speed));
	list->push_back(new PropertyDouble("horizontal feed rate", m_hfeed, set_hfeed));
	list->push_back(new PropertyDouble("vertical feed rate", m_vfeed, set_vfeed));
}

void CAdderApply::Run(){
	move_adder.AddTheMove();
}

static void set_x(double value){move_adder.m_pos[0] = value; heeksCAD->Repaint();}
static void set_y(double value){move_adder.m_pos[1] = value; heeksCAD->Repaint();}
static void set_z(double value){move_adder.m_pos[2] = value; heeksCAD->Repaint();}

void CMoveAdder::GetProperties(std::list<Property *> *list)
{
	list->push_back(new PropertyDouble("X", m_pos[0], set_x));
	list->push_back(new PropertyDouble("Y", m_pos[1], set_y));
	list->push_back(new PropertyDouble("Z", m_pos[2], set_z));
}

static void OnAddRapid(wxCommandEvent& event)
{
	move_adder.m_mode = 0;
	heeksCAD->SetInputMode(&move_adder);
}

static void OnAddFeed(wxCommandEvent& event)
{
	move_adder.m_mode = 1;
	heeksCAD->SetInputMode(&move_adder);
}

static void OnAddInitial(wxCommandEvent& event)
{
	heeksCAD->SetInputMode(&initial_adder);
}

CProgramCanvas::CProgramCanvas(wxWindow* parent)
        : wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                           wxHSCROLL | wxVSCROLL | wxNO_FULL_REPAINT_ON_RESIZE)
{
	m_textCtrl = new CProgramTextCtrl( this, 100, _T(""),
		wxPoint(180,170), wxSize(200,70), wxTE_MULTILINE);

	// make a tool bar
	m_toolBar = new wxToolBar(this, -1, wxDefaultPosition, wxDefaultSize, wxTB_NODIVIDER | wxTB_FLAT);
	m_toolBar->SetToolBitmapSize(wxSize(32, 32));

	// add toolbar buttons
	wxString exe_folder = heeksCAD->GetExeFolder();
	heeksCAD->AddToolBarButton(m_toolBar, _T("Run"), wxBitmap(exe_folder + "/../HeeksCNC/bitmaps/run.png", wxBITMAP_TYPE_PNG), _T("Run the program"), OnRun);
	heeksCAD->AddToolBarButton(m_toolBar, _T("Add Initial"), wxBitmap(exe_folder + "/../HeeksCNC/bitmaps/initial.png", wxBITMAP_TYPE_PNG), _T("Add spindle speed and feed rates"), OnAddInitial);
	heeksCAD->AddToolBarButton(m_toolBar, _T("Add Rapid"), wxBitmap(exe_folder + "/../HeeksCNC/bitmaps/rapid.png", wxBITMAP_TYPE_PNG), _T("Add a rapid move"), OnAddRapid);
	heeksCAD->AddToolBarButton(m_toolBar, _T("Add Feed"), wxBitmap(exe_folder + "/../HeeksCNC/bitmaps/feed.png", wxBITMAP_TYPE_PNG), _T("Add a feed move"), OnAddFeed);
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