// HeeksCNC.cpp

#include "stdafx.h"

#include <wx/stdpaths.h>
#include <wx/dynlib.h>
#include "../../HeeksCAD/interface/PropertyString.h"
#include "../../HeeksCAD/interface/Observer.h"
#include "OpProfile.h"
#include "PythonStuff.h"
#include "Program.h"
#include "ProgramCanvas.h"
#include "OutputCanvas.h"

CHeeksCADInterface* heeksCAD = NULL;

CHeeksCNCApp theApp;

CHeeksCNCApp::CHeeksCNCApp(){
	m_working_dir_for_solid_sim = "C:\\Documents and Settings\\dan\\Desktop\\cavedemo\\";
	m_triangles_file_for_solid_sim = "data\\tri.tri";
	m_command_for_solid_sim = "justfly.exe -f -d3000 -c0x808080 -sdata\\tri.tri";
	m_draw_cutter_radius = true;
	m_program = NULL;
	m_run_program_on_new_line = true;
}

CHeeksCNCApp::~CHeeksCNCApp(){
}

void CHeeksCNCApp::OnInitDLL()
{
#if !defined WXUSINGDLL
    wxInitialize();
#endif

	wxStandardPaths sp;

	wxDynamicLibrary *executable = new wxDynamicLibrary(sp.GetExecutablePath());
	CHeeksCADInterface* (*HeeksCADGetInterface)(void) = (CHeeksCADInterface* (*)(void))(executable->GetSymbol("HeeksCADGetInterface"));
	if(HeeksCADGetInterface){
		heeksCAD = (*HeeksCADGetInterface)();
	}

	m_config = new wxConfig("HeeksCNC");
	m_config->Read("SolidSimWorkingDir", &m_working_dir_for_solid_sim);
	m_config->Read("SolidSimTrianglesFile", &m_triangles_file_for_solid_sim);
	m_config->Read("SolidSimCommand", &m_command_for_solid_sim);
}

void CHeeksCNCApp::OnDestroyDLL()
{
	{
		m_config = new wxConfig("HeeksCNC");
		m_config->Write("SolidSimWorkingDir", m_working_dir_for_solid_sim);
		m_config->Write("SolidSimTrianglesFile", m_triangles_file_for_solid_sim);
		m_config->Write("SolidSimCommand", m_command_for_solid_sim);
	}

#if !defined WXUSINGDLL
	wxUninitialize();
#endif

	heeksCAD = NULL;
}

void OnOpProfileButton(wxCommandEvent& event){
	const std::list<HeeksObj*> &list = heeksCAD->GetMarkedList();
	COpProfile* new_object = new COpProfile();
	new_object->m_objects = list;
	new_object->CalculateOrMarkOutOfDate();

	heeksCAD->AddUndoably(new_object, theApp.m_program);
	heeksCAD->ClearMarkedList();
	heeksCAD->Mark(new_object);
}

void OnSolidSimButton(wxCommandEvent& event){
	wxMessageBox("In OnSolidSimButton");
}

void OnProgramCanvas( wxCommandEvent& event )
{
	wxAuiManager* aui_manager = heeksCAD->GetAuiManager();
	wxAuiPaneInfo& pane_info = aui_manager->GetPane(theApp.m_program_canvas);
	if(pane_info.IsOk()){
		pane_info.Show(event.IsChecked());
		aui_manager->Update();
	}
}

void OnUpdateProgramCanvas( wxUpdateUIEvent& event )
{
	wxAuiManager* aui_manager = heeksCAD->GetAuiManager();
	event.Check(aui_manager->GetPane(theApp.m_program_canvas).IsShown());
}

void OnOutputCanvas( wxCommandEvent& event )
{
	wxAuiManager* aui_manager = heeksCAD->GetAuiManager();
	wxAuiPaneInfo& pane_info = aui_manager->GetPane(theApp.m_output_canvas);
	if(pane_info.IsOk()){
		pane_info.Show(event.IsChecked());
		aui_manager->Update();
	}
}

void OnUpdateOutputCanvas( wxUpdateUIEvent& event )
{
	wxAuiManager* aui_manager = heeksCAD->GetAuiManager();
	event.Check(aui_manager->GetPane(theApp.m_output_canvas).IsShown());
}

void CHeeksCNCApp::OnStartUp()
{
	// add menus and toolbars

	// add the program canvas
	wxFrame* frame = heeksCAD->GetMainFrame();
	wxAuiManager* aui_manager = heeksCAD->GetAuiManager();
    m_program_canvas = new CProgramCanvas(frame);
	aui_manager->AddPane(m_program_canvas, wxAuiPaneInfo().Name("Program").Caption("Program").Bottom().BestSize(wxSize(600, 200)));

	// add the output canvas
    m_output_canvas = new COutputCanvas(frame);
	aui_manager->AddPane(m_output_canvas, wxAuiPaneInfo().Name("Output").Caption("Output").Bottom().BestSize(wxSize(600, 200)));

	bool program_visible;
	bool output_visible;

	theApp.m_config->Read("ProgramVisible", &program_visible);
	theApp.m_config->Read("OutputVisible", &output_visible);

	aui_manager->GetPane(m_program_canvas).Show(program_visible);
	aui_manager->GetPane(m_output_canvas).Show(output_visible);

	// add tick boxes for them all on the view menu
	wxMenu* view_menu = heeksCAD->GetViewMenu();
	heeksCAD->AddMenuCheckItem(view_menu, _T("Program"), OnProgramCanvas, OnUpdateProgramCanvas);
	heeksCAD->AddMenuCheckItem(view_menu, _T("Output"), OnOutputCanvas, OnUpdateOutputCanvas);
	heeksCAD->RegisterHideableWindow(m_program_canvas);
	heeksCAD->RegisterHideableWindow(m_output_canvas);
}

void CHeeksCNCApp::OnNewOrOpen()
{
	// add the program
	m_program = new CProgram;
	heeksCAD->GetMainObject()->Add(m_program, NULL);
	heeksCAD->WasAdded(m_program);
	theApp.m_program_canvas->Clear();
}

void on_solid_sim_wd_edit(const char* wd){theApp.m_working_dir_for_solid_sim = wd;}
void on_solid_sim_tf_edit(const char* tf){theApp.m_triangles_file_for_solid_sim = tf;}
void on_solid_sim_cm_edit(const char* cm){theApp.m_command_for_solid_sim = cm;}


void CHeeksCNCApp::GetOptions(std::list<Property *> *list){
	// solid sim
	list->push_back(new PropertyString("SolidSimWorkingDir", m_working_dir_for_solid_sim, on_solid_sim_wd_edit));
	list->push_back(new PropertyString("SolidSimTrianglesFile", m_triangles_file_for_solid_sim, on_solid_sim_tf_edit));
	list->push_back(new PropertyString("SolidSimCommand", m_command_for_solid_sim, on_solid_sim_cm_edit));
}

void CHeeksCNCApp::OnFrameDelete()
{
	wxAuiManager* aui_manager = heeksCAD->GetAuiManager();
	theApp.m_config->Write("ProgramVisible", aui_manager->GetPane(m_program_canvas).IsShown());
	theApp.m_config->Write("OutputVisible", aui_manager->GetPane(m_output_canvas).IsShown());
}

class MyApp : public wxApp
{
 
 public:
 
   virtual bool OnInit(void);
 
 };
 
 bool MyApp::OnInit(void)
 
 {
 
   return true;
 
 }

 namespace geoff_geometry{

      int   UNITS = MM;

      double TOLERANCE = 1.0e-06;

      double TOLERANCE_SQ = TOLERANCE * TOLERANCE;

      double TIGHT_TOLERANCE = 1.0e-09;

      double UNIT_VECTOR_TOLERANCE = 1.0e-10;

      double RESOLUTION = 1.0e-06;

}
 
 DECLARE_APP(MyApp)
 
 IMPLEMENT_APP(MyApp)
 