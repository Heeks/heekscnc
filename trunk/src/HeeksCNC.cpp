// HeeksCNC.cpp
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "stdafx.h"

#include <wx/stdpaths.h>
#include <wx/dynlib.h>
#include <wx/aui/aui.h>
#include "../../interface/PropertyString.h"
#include "../../interface/Observer.h"
#include "../../interface/ToolImage.h"
#include "PythonStuff.h"
#include "Program.h"
#include "ProgramCanvas.h"
#include "OutputCanvas.h"
#include "CNCConfig.h"
#include "NCCode.h"
#include "Profile.h"
#include "Pocket.h"
#include "ZigZag.h"
#include "Adaptive.h"

CHeeksCADInterface* heeksCAD = NULL;

CHeeksCNCApp theApp;

CHeeksCNCApp::CHeeksCNCApp(){
	m_draw_cutter_radius = true;
	m_program = NULL;
	m_run_program_on_new_line = false;
	m_machiningBar = NULL;
}

CHeeksCNCApp::~CHeeksCNCApp(){
}

void CHeeksCNCApp::OnInitDLL()
{
}

void CHeeksCNCApp::OnDestroyDLL()
{
	// save any settings
	//config.Write("SolidSimWorkingDir", m_working_dir_for_solid_sim);

#if !defined WXUSINGDLL
	wxUninitialize();
#endif

	heeksCAD = NULL;
}

void OnMachiningBar( wxCommandEvent& event )
{
	wxAuiManager* aui_manager = heeksCAD->GetAuiManager();
	wxAuiPaneInfo& pane_info = aui_manager->GetPane(theApp.m_machiningBar);
	if(pane_info.IsOk()){
		pane_info.Show(event.IsChecked());
		aui_manager->Update();
	}
}

void OnUpdateMachiningBar( wxUpdateUIEvent& event )
{
	wxAuiManager* aui_manager = heeksCAD->GetAuiManager();
	event.Check(aui_manager->GetPane(theApp.m_machiningBar).IsShown());
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

static void OnOutputCanvas( wxCommandEvent& event )
{
	wxAuiManager* aui_manager = heeksCAD->GetAuiManager();
	wxAuiPaneInfo& pane_info = aui_manager->GetPane(theApp.m_output_canvas);
	if(pane_info.IsOk()){
		pane_info.Show(event.IsChecked());
		aui_manager->Update();
	}
}

static void OnUpdateOutputCanvas( wxUpdateUIEvent& event )
{
	wxAuiManager* aui_manager = heeksCAD->GetAuiManager();
	event.Check(aui_manager->GetPane(theApp.m_output_canvas).IsShown());
}

static bool GetSketches(std::list<int>& sketches)
{
	// check for at least one sketch selected

	const std::list<HeeksObj*>& list = heeksCAD->GetMarkedList();
	for(std::list<HeeksObj*>::const_iterator It = list.begin(); It != list.end(); It++)
	{
		HeeksObj* object = *It;
		if(object->GetType() == SketchType)sketches.push_back(object->m_id);
	}

	if(sketches.size() == 0)
	{
		wxMessageBox(_("You must select some sketches, first!"));
		return false;
	}

	return true;
}

static void NewProfileOpMenuCallback(wxCommandEvent &event)
{
	std::list<int> sketches;
	if(GetSketches(sketches))
	{
		CProfile *new_object = new CProfile(sketches);
		heeksCAD->AddUndoably(new_object, theApp.m_program->m_operations);
		heeksCAD->ClearMarkedList();
		heeksCAD->Mark(new_object);
	}
}

static void NewPocketOpMenuCallback(wxCommandEvent &event)
{
	std::list<int> sketches;
	if(GetSketches(sketches))
	{
		CPocket *new_object = new CPocket(sketches);
		heeksCAD->AddUndoably(new_object, theApp.m_program->m_operations);
		heeksCAD->ClearMarkedList();
		heeksCAD->Mark(new_object);
	}
}

static void NewZigZagOpMenuCallback(wxCommandEvent &event)
{
	// check for at least one solid selected
	std::list<int> solids;

	const std::list<HeeksObj*>& list = heeksCAD->GetMarkedList();
	for(std::list<HeeksObj*>::const_iterator It = list.begin(); It != list.end(); It++)
	{
		HeeksObj* object = *It;
		if(object->GetType() == SolidType || object->GetType() == StlSolidType)solids.push_back(object->m_id);
	}

	// if no selected solids, 
	if(solids.size() == 0)
	{
		// use all the solids in the drawing
		for(HeeksObj* object = heeksCAD->GetFirstObject();object; object = heeksCAD->GetNextObject())
		{
			if(object->GetType() == SolidType || object->GetType() == StlSolidType)solids.push_back(object->m_id);
		}
	}

	if(solids.size() == 0)
	{
		wxMessageBox(_("There are no solids!"));
		return;
	}

	CZigZag *new_object = new CZigZag(solids);
	heeksCAD->AddUndoably(new_object, theApp.m_program->m_operations);
	heeksCAD->ClearMarkedList();
	heeksCAD->Mark(new_object);
}

static void NewAdaptiveOpMenuCallback(wxCommandEvent &event)
{
	std::list<int> solids;
	std::list<int> sketches;

	const std::list<HeeksObj*>& list = heeksCAD->GetMarkedList();
	for(std::list<HeeksObj*>::const_iterator It = list.begin(); It != list.end(); It++)
	{
		HeeksObj* object = *It;
		if(object->GetType() == SolidType || object->GetType() == StlSolidType)solids.push_back(object->m_id);
		if(object->GetType() == SketchType) sketches.push_back(object->m_id);
	}

	// if no selected solids, 
	if(solids.size() == 0)
	{
		// use all the solids in the drawing
		for(HeeksObj* object = heeksCAD->GetFirstObject();object; object = heeksCAD->GetNextObject())
		{
			if(object->GetType() == SolidType || object->GetType() == StlSolidType)solids.push_back(object->m_id);
		}
	}

	if(solids.size() == 0)
	{
		wxMessageBox(_("There are no solids!"));
		return;
	}
#if 0
	if(sketches.size() == 0)
	{
		wxMessageBox(_("You must select some sketches first!"));
		return;
	}
#endif
	CAdaptive *new_object = new CAdaptive(solids, sketches);
	heeksCAD->AddUndoably(new_object, theApp.m_program->m_operations);
	heeksCAD->ClearMarkedList();
	heeksCAD->Mark(new_object);
}

static void MakeScriptMenuCallback(wxCommandEvent &event)
{
	// create the Python program
	theApp.m_program->RewritePythonProgram();
}

static void PostProcessMenuCallback(wxCommandEvent &event)
{
	{
		// clear the output file
		wxFile f(theApp.m_program->m_output_file.c_str(), wxFile::write);
		if(f.IsOpened())f.Write(_T("\n"));
	}
	bool pp_success = HeeksPyPostProcess();
	{
		// clear the backplot file
		wxString backplot_path = theApp.m_program->m_output_file + _T(".nc.xml");
		wxFile f(backplot_path.c_str(), wxFile::write);
		if(f.IsOpened())f.Write(_T("\n"));
	}
	if(pp_success)HeeksPyBackplot(theApp.m_program->m_output_file);
}

static void OpenNcFileMenuCallback(wxCommandEvent& event)
{
	wxString ext_str(_T("*.*")); // to do, use the machine's NC extension
	wxString wildcard_string = wxString(_("NC files")) + _T(" |") + ext_str;
    wxFileDialog dialog(theApp.m_output_canvas, _("Open NC file"), wxEmptyString, wxEmptyString, wildcard_string);
    dialog.CentreOnParent();

    if (dialog.ShowModal() == wxID_OK)
    {
		HeeksPyBackplot(dialog.GetPath().c_str());
	}
}

static void SaveNcFileMenuCallback(wxCommandEvent& event)
{
	wxString ext_str(_T("*.tap")); // to do, use the machine's NC extension
	wxString wildcard_string = wxString(_("NC files")) + _T(" |") + ext_str;
	wxFileDialog fd(theApp.m_output_canvas, _("Save NC file"), wxEmptyString, wxEmptyString, wildcard_string, wxSAVE|wxOVERWRITE_PROMPT);
	fd.SetFilterIndex(1);
	if (fd.ShowModal() == wxID_OK)
	{
		wxString nc_file_str = fd.GetPath().c_str();
		{
			wxFile ofs(nc_file_str.c_str(), wxFile::write);
			if(!ofs.IsOpened())
			{
				wxMessageBox(wxString(_("Couldn't open file")) + _T(" - ") + nc_file_str);
				return;
			}

			ofs.Write(theApp.m_output_canvas->m_textCtrl->GetValue());
		}
		HeeksPyBackplot(nc_file_str);
	}
}

static void AddToolBars()
{
	wxFrame* frame = heeksCAD->GetMainFrame();
	wxAuiManager* aui_manager = heeksCAD->GetAuiManager();
	if(theApp.m_machiningBar)delete theApp.m_machiningBar;
	theApp.m_machiningBar = new wxToolBar(frame, -1, wxDefaultPosition, wxDefaultSize, wxTB_NODIVIDER | wxTB_FLAT);
	theApp.m_machiningBar->SetToolBitmapSize(wxSize(ToolImage::GetBitmapSize(), ToolImage::GetBitmapSize()));
	heeksCAD->AddToolBarButton((wxToolBar*)(theApp.m_machiningBar), _("Profile"), ToolImage(_T("opprofile")), _T("New Profile Operation..."), NewProfileOpMenuCallback);
	heeksCAD->AddToolBarButton((wxToolBar*)(theApp.m_machiningBar), _("Pocket"), ToolImage(_T("pocket")), _T("New Pocket Operation..."), NewPocketOpMenuCallback);
	heeksCAD->AddToolBarButton((wxToolBar*)(theApp.m_machiningBar), _("ZigZag"), ToolImage(_T("zigzag")), _T("New ZigZag Operation..."), NewZigZagOpMenuCallback);
	heeksCAD->AddToolBarButton((wxToolBar*)(theApp.m_machiningBar), _("Adaptive"), ToolImage(_T("adapt")), _T("New Adaptive Roughing Operation..."), NewAdaptiveOpMenuCallback);
	heeksCAD->AddToolBarButton((wxToolBar*)(theApp.m_machiningBar), _("Python"), ToolImage(_T("python")), _T("Make Python Script"), MakeScriptMenuCallback);
	heeksCAD->AddToolBarButton((wxToolBar*)(theApp.m_machiningBar), _("PostProcess"), ToolImage(_T("postprocess")), _T("Post-Process"), PostProcessMenuCallback);
	heeksCAD->AddToolBarButton((wxToolBar*)(theApp.m_machiningBar), _("OpenNC"), ToolImage(_T("opennc")), _T("Open NC File"), OpenNcFileMenuCallback);
	heeksCAD->AddToolBarButton((wxToolBar*)(theApp.m_machiningBar), _("SaveNC"), ToolImage(_T("savenc")), _T("Save NC File"), SaveNcFileMenuCallback);
	theApp.m_machiningBar->Realize();
	aui_manager->AddPane(theApp.m_machiningBar, wxAuiPaneInfo().Name(_T("MachiningBar")).Caption(_T("MachineWorks tools")).ToolbarPane().Top());
	heeksCAD->RegisterToolBar(theApp.m_machiningBar);
}

void CHeeksCNCApp::OnStartUp(CHeeksCADInterface* h)
{
	heeksCAD = h;
#if !defined WXUSINGDLL
	wxInitialize();
#endif

	CNCConfig config;

	// About box, stuff
	heeksCAD->AddToAboutBox(wxString(_T("\n\n")) + _("HeeksCNC is the free machining add-on to HeeksCAD")
		+ _T("\n") + _("          http://code.google.com/p/heekscnc/")
		+ _T("\n") + _("Written by Dan Heeks, Hirutso Enni, Perttu Ahola")
		+ _T("\n") + _("With help from archivist, crotchet1, DanielFalck, fenn, Sliptonic")
		+ _T("\n\n") + _("geometry code, donated by Geoff Hawkesford, Camtek GmbH http://www.peps.de/")
		+ _T("\n") + _("pocketing code from http://code.google.com/p/libarea/ , derived from the kbool library written by Klaas Holwerda http://boolean.klaasholwerda.nl/bool.html")
		+ _T("\n") + _("Zig zag code from pycam http://sourceforge.net/svn/?group_id=237831")
		+ _T("\n") + _("Adaptive Roughing code from http://code.google.com/p/libactp/, using code written in 2004")
		+ _T("\n") + _("For a more modern version of adaptive roughing, see Julian Todd and Martin Dunschen of http://www.freesteel.co.uk/")
		+ _T("\n\n") + _("This HeeksCNC software installation is restricted by the GPL license http://www.gnu.org/licenses/gpl-3.0.txt")
		+ _T("\n") + _("  which means it is free and open source, and must stay that way")
		);

	// add menus and toolbars
	wxFrame* frame = heeksCAD->GetMainFrame();
	wxAuiManager* aui_manager = heeksCAD->GetAuiManager();

	// tool bars
	heeksCAD->RegisterAddToolBars(AddToolBars);
	AddToolBars();

	// Operations menu
	wxMenu *menuOperations = new wxMenu;
	heeksCAD->AddMenuItem(menuOperations, _("New Profile Operation..."), NewProfileOpMenuCallback);
	heeksCAD->AddMenuItem(menuOperations, _("New Pocket Operation..."), NewPocketOpMenuCallback);
	heeksCAD->AddMenuItem(menuOperations, _("New ZigZag Operation..."), NewZigZagOpMenuCallback);
	heeksCAD->AddMenuItem(menuOperations, _("New Adaptive Roughing Operation..."), NewAdaptiveOpMenuCallback);

	// Machining menu
	wxMenu *menuMachining = new wxMenu;
	menuMachining->AppendSubMenu(menuOperations, _("Operations"));
	heeksCAD->AddMenuItem(menuMachining, _("Make Python Script"), MakeScriptMenuCallback);
	heeksCAD->AddMenuItem(menuMachining, _("Post-Process"), PostProcessMenuCallback);
	frame->GetMenuBar()->Append(menuMachining,  _("Machining"));

	// add the program canvas
    m_program_canvas = new CProgramCanvas(frame);
	aui_manager->AddPane(m_program_canvas, wxAuiPaneInfo().Name(_T("Program")).Caption(_T("Program")).Bottom().BestSize(wxSize(600, 200)));

	// add the output canvas
    m_output_canvas = new COutputCanvas(frame);
	aui_manager->AddPane(m_output_canvas, wxAuiPaneInfo().Name(_T("Output")).Caption(_T("Output")).Bottom().BestSize(wxSize(600, 200)));

	bool program_visible;
	bool output_visible;

	config.Read(_T("ProgramVisible"), &program_visible);
	config.Read(_T("OutputVisible"), &output_visible);

	// Read NC Code colors
	CNCCode::ReadColorsFromConfig();

	aui_manager->GetPane(m_program_canvas).Show(program_visible);
	aui_manager->GetPane(m_output_canvas).Show(output_visible);

	// add tick boxes for them all on the view menu
	wxMenu* view_menu = heeksCAD->GetWindowMenu();
	heeksCAD->AddMenuCheckItem(view_menu, _T("Program"), OnProgramCanvas, OnUpdateProgramCanvas);
	heeksCAD->AddMenuCheckItem(view_menu, _T("Output"), OnOutputCanvas, OnUpdateOutputCanvas);
	heeksCAD->AddMenuCheckItem(view_menu, _T("Machining"), OnMachiningBar, OnUpdateMachiningBar);
	heeksCAD->RegisterHideableWindow(m_program_canvas);
	heeksCAD->RegisterHideableWindow(m_output_canvas);
	heeksCAD->RegisterHideableWindow(m_machiningBar);

	// add object reading functions
	heeksCAD->RegisterReadXMLfunction("Program", CProgram::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("nccode", CNCCode::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("Operations", COperations::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("Profile", CProfile::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("Pocket", CPocket::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("ZigZag", CZigZag::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("Adaptive", CAdaptive::ReadFromXMLElement);

	heeksCAD->SetDefaultLayout(wxString(_T("layout2|name=ToolBar;caption=General Tools;state=2108156;dir=1;layer=10;row=0;pos=0;prop=100000;bestw=279;besth=31;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=GeomBar;caption=Geometry Tools;state=2108156;dir=1;layer=10;row=0;pos=290;prop=100000;bestw=248;besth=31;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=SolidBar;caption=Solid Tools;state=2108156;dir=1;layer=10;row=1;pos=0;prop=100000;bestw=341;besth=31;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=970;floaty=297;floatw=296;floath=57|name=ViewingBar;caption=Viewing Tools;state=2108156;dir=1;layer=10;row=1;pos=352;prop=100000;bestw=248;besth=31;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=TransformBar;caption=Transformation Tools;state=2108156;dir=1;layer=10;row=1;pos=611;prop=100000;bestw=217;besth=31;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=Graphics;caption=Graphics;state=768;dir=5;layer=0;row=0;pos=0;prop=100000;bestw=800;besth=600;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=Objects;caption=Objects;state=2099196;dir=4;layer=1;row=0;pos=0;prop=100000;bestw=300;besth=400;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=Options;caption=Options;state=2099196;dir=4;layer=1;row=0;pos=1;prop=100000;bestw=300;besth=200;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=Input;caption=Input;state=2099196;dir=4;layer=1;row=0;pos=2;prop=100000;bestw=300;besth=200;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=Properties;caption=Properties;state=2099196;dir=4;layer=1;row=0;pos=3;prop=100000;bestw=300;besth=200;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=MachiningBar;caption=MachineWorks tools;state=2108156;dir=1;layer=10;row=0;pos=549;prop=100000;bestw=248;besth=31;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=Program;caption=Program;state=2099196;dir=3;layer=0;row=0;pos=0;prop=100000;bestw=600;besth=200;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=Output;caption=Output;state=2099196;dir=3;layer=0;row=0;pos=1;prop=100000;bestw=600;besth=200;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|dock_size(5,0,0)=504|dock_size(4,1,0)=234|dock_size(1,10,0)=33|dock_size(1,10,1)=33|dock_size(3,0,0)=219|")));
}

void CHeeksCNCApp::OnNewOrOpen(bool open)
{
	// check for existance of a program

	bool program_found = false;
	for(HeeksObj* object = heeksCAD->GetFirstObject(); object; object = heeksCAD->GetNextObject())
	{
		if(object->GetType() == ProgramType)
		{
			program_found = true;
			break;
		}
	}

	if(!program_found)
	{
		// add the program
		m_program = new CProgram;
		heeksCAD->GetMainObject()->Add(m_program, NULL);
		COperations *operations = new COperations;
		m_program->Add(operations, NULL);
		heeksCAD->WasAdded(m_program);
		theApp.m_program_canvas->Clear();
		theApp.m_output_canvas->Clear();
	}
}

void CHeeksCNCApp::GetOptions(std::list<Property *> *list){
	CNCCode::GetOptions(list);
}

void CHeeksCNCApp::OnFrameDelete()
{
	wxAuiManager* aui_manager = heeksCAD->GetAuiManager();
	CNCConfig config;
	config.Write(_T("ProgramVisible"), aui_manager->GetPane(m_program_canvas).IsShown());
	config.Write(_T("OutputVisible"), aui_manager->GetPane(m_output_canvas).IsShown());
	config.Write(_T("MachiningBarVisible"), aui_manager->GetPane(m_machiningBar).IsShown());

	CNCCode::WriteColorsToConfig();
}

wxString CHeeksCNCApp::GetDllFolder()
{
	return heeksCAD->GetExeFolder() + _T("/HeeksCNC");
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

 DECLARE_APP(MyApp)
 
 IMPLEMENT_APP(MyApp)
 
