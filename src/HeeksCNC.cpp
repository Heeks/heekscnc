// HeeksCNC.cpp
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "stdafx.h"
#include <errno.h>

#ifndef WIN32
	#include <dirent.h>
#endif

#include <wx/stdpaths.h>
#include <wx/dynlib.h>
#include <wx/aui/aui.h>
#include <wx/filename.h>
#include "interface/PropertyString.h"
#include "interface/PropertyCheck.h"
#include "interface/PropertyList.h"
#include "interface/Observer.h"
#include "interface/ToolImage.h"
#include "PythonStuff.h"
#include "Program.h"
#include "ProgramCanvas.h"
#include "OutputCanvas.h"
#include "CNCConfig.h"
#include "NCCode.h"
#include "Profile.h"
#include "Pocket.h"
#include "Drilling.h"
#include "CTool.h"
#include "Operations.h"
#include "Tools.h"
#include "interface/strconv.h"
#include "interface/Tool.h"
#include "CNCPoint.h"
#include "Excellon.h"
#include "Tags.h"
#include "Tag.h"
#include "ScriptOp.h"
#include "Simulate.h"
#include "Pattern.h"
#include "Patterns.h"
#include "Surface.h"
#include "Surfaces.h"
#include "Stock.h"
#include "Stocks.h"

#include <sstream>

CHeeksCADInterface* heeksCAD = NULL;

CHeeksCNCApp theApp;

extern void ImportToolsFile( const wxChar *file_path );

wxString HeeksCNCType(const int type);

CHeeksCNCApp::CHeeksCNCApp(){
	m_draw_cutter_radius = true;
	m_program = NULL;
	m_run_program_on_new_line = false;
	m_machiningBar = NULL;
	m_icon_texture_number = 0;
	m_machining_hidden = false;
	m_settings_restored = false;
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

static void OnPrintCanvas( wxCommandEvent& event )
{
	wxAuiManager* aui_manager = heeksCAD->GetAuiManager();
	wxAuiPaneInfo& pane_info = aui_manager->GetPane(theApp.m_print_canvas);
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

static void OnUpdatePrintCanvas( wxUpdateUIEvent& event )
{
	wxAuiManager* aui_manager = heeksCAD->GetAuiManager();
	event.Check(aui_manager->GetPane(theApp.m_print_canvas).IsShown());
}

static void GetSketches(std::list<int>& sketches, std::list<int> &tools )
{
	// check for at least one sketch selected

	const std::list<HeeksObj*>& list = heeksCAD->GetMarkedList();
	for(std::list<HeeksObj*>::const_iterator It = list.begin(); It != list.end(); It++)
	{
		HeeksObj* object = *It;
		if(object->GetIDGroupType() == SketchType)
		{
			sketches.push_back(object->m_id);
		} // End if - then

		if ((object != NULL) && (object->GetType() == ToolType))
		{
			tools.push_back( object->m_id );
		} // End if - then
	}
}

static void NewProfileOp()
{
	std::list<int> tools;
	std::list<int> sketches;
	GetSketches(sketches, tools);

	int sketch = 0;
	if(sketches.size() > 0)sketch = sketches.front();

	CProfile *new_object = new CProfile(sketch, (tools.size()>0)?(*tools.begin()):-1);
	new_object->SetID(heeksCAD->GetNextID(ProfileType));
	new_object->AddMissingChildren(); // add the tags container

	if(new_object->Edit())
	{
		heeksCAD->StartHistory();
		heeksCAD->AddUndoably(new_object, theApp.m_program->Operations());

		if(sketches.size() > 1)
		{
			for(std::list<int>::iterator It = sketches.begin(); It != sketches.end(); It++)
			{
				if(It == sketches.begin())continue;
				CProfile* copy = (CProfile*)(new_object->MakeACopy());
				copy->m_sketch = *It;
				heeksCAD->AddUndoably(copy, theApp.m_program->Operations());
			}
		}
		heeksCAD->EndHistory();
	}
	else
		delete new_object;
}

static void NewProfileOpMenuCallback(wxCommandEvent &event)
{
	NewProfileOp();
}

static void NewPocketOp()
{
	std::list<int> tools;
	std::list<int> sketches;
	GetSketches(sketches, tools);

	int sketch = 0;
	if(sketches.size() > 0)sketch = sketches.front();

	CPocket *new_object = new CPocket(sketch, (tools.size()>0)?(*tools.begin()):-1 );
	new_object->SetID(heeksCAD->GetNextID(PocketType));

	if(new_object->Edit())
	{
		heeksCAD->StartHistory();
		heeksCAD->AddUndoably(new_object, theApp.m_program->Operations());

		if(sketches.size() > 1)
		{
			for(std::list<int>::iterator It = sketches.begin(); It != sketches.end(); It++)
			{
				if(It == sketches.begin())continue;
				CPocket* copy = (CPocket*)(new_object->MakeACopy());
				copy->m_sketch = *It;
				heeksCAD->AddUndoably(copy, theApp.m_program->Operations());
			}
		}
		heeksCAD->EndHistory();
	}
	else
		delete new_object;
}

static void NewPocketOpMenuCallback(wxCommandEvent &event)
{
	NewPocketOp();
}

static void AddNewObjectUndoablyAndMarkIt(HeeksObj* new_object, HeeksObj* parent)
{
	heeksCAD->StartHistory();
	heeksCAD->AddUndoably(new_object, parent);
	heeksCAD->ClearMarkedList();
	heeksCAD->Mark(new_object);
	heeksCAD->EndHistory();
}

static void NewDrillingOp()
{
	std::list<int> points;

	const std::list<HeeksObj*>& list = heeksCAD->GetMarkedList();
	for(std::list<HeeksObj*>::const_iterator It = list.begin(); It != list.end(); It++)
	{
		HeeksObj* object = *It;
		if (object->GetType() == PointType)
		{
            points.push_back( object->m_id );
		} // End if - else
	} // End for

	{
		CDrilling *new_object = new CDrilling( points, 0, -1 );
		new_object->SetID(heeksCAD->GetNextID(DrillingType));
		if(new_object->Edit())
		{
			heeksCAD->StartHistory();
			heeksCAD->AddUndoably(new_object, theApp.m_program->Operations());
			heeksCAD->EndHistory();
		}
		else
			delete new_object;
	}
}

static void NewDrillingOpMenuCallback(wxCommandEvent &event)
{
	NewDrillingOp();
}

static void NewScriptOpMenuCallback(wxCommandEvent &event)
{
	CScriptOp *new_object = new CScriptOp();
	new_object->SetID(heeksCAD->GetNextID(ScriptOpType));
	if(new_object->Edit())
	{
		heeksCAD->StartHistory();
		AddNewObjectUndoablyAndMarkIt(new_object, theApp.m_program->Operations());
		heeksCAD->EndHistory();
	}
	else
		delete new_object;
}

static void NewPatternMenuCallback(wxCommandEvent &event)
{
	CPattern *new_object = new CPattern();

	if(new_object->Edit())
	{
		heeksCAD->StartHistory();
		AddNewObjectUndoablyAndMarkIt(new_object, theApp.m_program->Patterns());
		heeksCAD->EndHistory();
	}
	else
		delete new_object;
}

static void NewSurfaceMenuCallback(wxCommandEvent &event)
{
	// check for at least one solid selected
	std::list<int> solids;

	const std::list<HeeksObj*>& list = heeksCAD->GetMarkedList();
	for(std::list<HeeksObj*>::const_iterator It = list.begin(); It != list.end(); It++)
	{
		HeeksObj* object = *It;
		if(object->GetIDGroupType() == SolidType)solids.push_back(object->m_id);
	}

	// if no selected solids,
	if(solids.size() == 0)
	{
		// use all the solids in the drawing
		for(HeeksObj* object = heeksCAD->GetFirstObject();object; object = heeksCAD->GetNextObject())
		{
			if(object->GetIDGroupType() == SolidType)solids.push_back(object->m_id);
		}
	}

	{
		CSurface *new_object = new CSurface();
		new_object->m_solids = solids;
		if(new_object->Edit())
		{
			heeksCAD->StartHistory();
			AddNewObjectUndoablyAndMarkIt(new_object, theApp.m_program->Surfaces());
			heeksCAD->EndHistory();
		}
		else
			delete new_object;
	}
}

static void NewStockMenuCallback(wxCommandEvent &event)
{
	// check for at least one solid selected
	std::list<int> solids;

	const std::list<HeeksObj*>& list = heeksCAD->GetMarkedList();
	for(std::list<HeeksObj*>::const_iterator It = list.begin(); It != list.end(); It++)
	{
		HeeksObj* object = *It;
		if(object->GetIDGroupType() == SolidType)solids.push_back(object->m_id);
	}

	// if no selected solids,
	if(solids.size() == 0)
	{
		// use all the solids in the drawing
		for(HeeksObj* object = heeksCAD->GetFirstObject();object; object = heeksCAD->GetNextObject())
		{
			if(object->GetIDGroupType() == SolidType)solids.push_back(object->m_id);
		}
	}

	{
		CStock *new_object = new CStock();
		new_object->m_solids = solids;
		if(new_object->Edit())
		{
			heeksCAD->StartHistory();
			AddNewObjectUndoablyAndMarkIt(new_object, theApp.m_program->Stocks());
			heeksCAD->EndHistory();
		}
		else
			delete new_object;
	}
}

static void AddNewTool(CToolParams::eToolType type)
{
	// find next available tool number
	int max_tool_number = 0;
	for(HeeksObj* object = theApp.m_program->Tools()->GetFirstChild(); object; object = theApp.m_program->Tools()->GetNextChild())
	{
		if(object->GetType() == ToolType)
		{
			int tool_number = ((CTool*)object)->m_tool_number;
			if(tool_number > max_tool_number)max_tool_number = tool_number;
		}
	}

	// Add a new tool.
	CTool *new_object = new CTool(NULL, type, max_tool_number + 1);
	if(new_object->Edit())
		AddNewObjectUndoablyAndMarkIt(new_object, theApp.m_program->Tools());
	else
		delete new_object;
}

static void NewDrillMenuCallback(wxCommandEvent &event)
{
	AddNewTool(CToolParams::eDrill);
}

static void NewCentreDrillMenuCallback(wxCommandEvent &event)
{
	AddNewTool(CToolParams::eCentreDrill);
}

static void NewEndmillMenuCallback(wxCommandEvent &event)
{
	AddNewTool(CToolParams::eEndmill);
}

static void NewSlotCutterMenuCallback(wxCommandEvent &event)
{
	AddNewTool(CToolParams::eSlotCutter);
}

static void NewBallEndMillMenuCallback(wxCommandEvent &event)
{
	AddNewTool(CToolParams::eBallEndMill);
}

static void NewChamferMenuCallback(wxCommandEvent &event)
{
	AddNewTool(CToolParams::eChamfer);
}

void CHeeksCNCApp::RunPythonScript()
{
	{
		// clear the output file
		wxFile f(m_program->GetOutputFileName().c_str(), wxFile::write);
		if(f.IsOpened())f.Write(_T("\n"));
	}

	// Check to see if someone has modified the contents of the
	// program canvas manually.  If so, replace the m_python_program
	// with the edited program.  We don't want to do this without
	// this check since the maximum size of m_textCtrl is sometimes
	// a limitation to the size of the python program.  If the first 'n' characters
	// of m_python_program matches the full contents of the m_textCtrl then
	// it's likely that the text control holds as much of the python program
	// as it can hold but more may still exist in m_python_program.
	unsigned int text_control_length = m_program_canvas->m_textCtrl->GetLastPosition();
	if (m_program->m_python_program.substr(0,text_control_length) != m_program_canvas->m_textCtrl->GetValue())
	{
        // copy the contents of the program canvas to the string
        m_program->m_python_program.clear();
        m_program->m_python_program << theApp.m_program_canvas->m_textCtrl->GetValue();
	}

#ifdef FREE_VERSION
	::wxLaunchDefaultBrowser(_T("http://heeks.net/help/buy-heekscnc-1-0"));
#endif

	HeeksPyPostProcess(m_program, m_program->GetOutputFileName(), true );
}

static void RunScriptMenuCallback(wxCommandEvent &event)
{
	theApp.RunPythonScript();
}

static void PostProcessMenuCallback(wxCommandEvent &event)
{
	// write the python program
	theApp.m_program->RewritePythonProgram();

	// run it
	theApp.RunPythonScript();
}

static void CancelMenuCallback(wxCommandEvent &event)
{
	HeeksPyCancel();
}

#ifdef WIN32
static void SimulateCallback(wxCommandEvent &event)
{
	RunVoxelcutSimulation();
}
#endif

static void OpenNcFileMenuCallback(wxCommandEvent& event)
{
	wxString ext_str(_T("*.*")); // to do, use the machine's NC extension
	wxString wildcard_string = wxString(_("NC files")) + _T(" |") + ext_str;
    wxFileDialog dialog(theApp.m_output_canvas, _("Open NC file"), wxEmptyString, wxEmptyString, wildcard_string);
    dialog.CentreOnParent();

    if (dialog.ShowModal() == wxID_OK)
    {
		HeeksPyBackplot(theApp.m_program, theApp.m_program, dialog.GetPath().c_str());
	}
}

// create a temporary file for ngc output
// run appropriate command to make 'Machine' read ngc file
// linux/emc/axis: this would typically entail calling axis-remote <filename>

static void SendToMachineMenuCallback(wxCommandEvent& event)
{
	HeeksSendToMachine(theApp.m_output_canvas->m_textCtrl->GetValue());
}

static void SaveNcFileMenuCallback(wxCommandEvent& event)
{
#if wxCHECK_VERSION(3, 0, 0)
	wxStandardPaths& sp = wxStandardPaths::Get();
#else
	wxStandardPaths sp;
#endif
    wxString user_docs =sp.GetDocumentsDir();
    wxString ncdir;
    //ncdir =  user_docs + _T("/nc");
    ncdir =  user_docs; //I was getting tired of having to start out at the root directory in linux
	wxString ext_str(_T("*.*")); // to do, use the machine's NC extension
	wxString wildcard_string = wxString(_("NC files")) + _T(" |") + ext_str;
    wxString defaultDir = ncdir;
	wxFileDialog fd(theApp.m_output_canvas, _("Save NC file"), defaultDir, wxEmptyString, wildcard_string, wxFD_SAVE|wxFD_OVERWRITE_PROMPT);
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
               

          if(theApp.m_use_DOS_not_Unix == true)   //DF -added to get DOS line endings HeeksCNC running on Unix 
            {
                long line_num= 0;
                bool ok = true;
                int nLines = theApp.m_output_canvas->m_textCtrl->GetNumberOfLines();
            for ( int nLine = 0; ok && nLine < nLines; nLine ++)
                {   
                    ok = ofs.Write(theApp.m_output_canvas->m_textCtrl->GetLineText(line_num) + _T("\r\n") );
                    line_num = line_num+1;
                }
            }

            else
			    ofs.Write(theApp.m_output_canvas->m_textCtrl->GetValue());
		}
		HeeksPyBackplot(theApp.m_program, theApp.m_program, nc_file_str);
	}
}

static void HelpMenuCallback(wxCommandEvent& event)
{
	::wxLaunchDefaultBrowser(_T("http://heeks.net/help"));
}

// a class to re-use existing "OnButton" functions in a Tool class
class CCallbackTool: public Tool{
public:
	wxString m_title;
	wxString m_bitmap_path;
	void(*m_callback)(wxCommandEvent&);

	CCallbackTool(const wxString& title, const wxString& bitmap_path, void(*callback)(wxCommandEvent&)): m_title(title), m_bitmap_path(bitmap_path), m_callback(callback){}

	// Tool's virtual functions
	const wxChar* GetTitle(){return m_title;}
	void Run(){
		wxCommandEvent dummy_evt;
		(*m_callback)(dummy_evt);
	}
	wxString BitmapPath(){ return m_bitmap_path;}
};

static CCallbackTool new_drill_tool(_("New Drill..."), _T("drill"), NewDrillMenuCallback);
static CCallbackTool new_centre_drill_tool(_("New Centre Drill..."), _T("centredrill"), NewCentreDrillMenuCallback);
static CCallbackTool new_endmill_tool(_("New End Mill..."), _T("endmill"), NewEndmillMenuCallback);
static CCallbackTool new_slotdrill_tool(_("New Slot Drill..."), _T("slotdrill"), NewSlotCutterMenuCallback);
static CCallbackTool new_ball_end_mill_tool(_("New Ball End Mill..."), _T("ballmill"), NewBallEndMillMenuCallback);
static CCallbackTool new_chamfer_mill_tool(_("New Chamfer Mill..."), _T("chamfmill"), NewChamferMenuCallback);

void CHeeksCNCApp::GetNewToolTools(std::list<Tool*>* t_list)
{
	t_list->push_back(&new_drill_tool);
	t_list->push_back(&new_centre_drill_tool);
	t_list->push_back(&new_endmill_tool);
	t_list->push_back(&new_slotdrill_tool);
	t_list->push_back(&new_ball_end_mill_tool);
	t_list->push_back(&new_chamfer_mill_tool);
}

static CCallbackTool new_pattern_tool(_("New Pattern..."), _T("pattern"), NewPatternMenuCallback);

void CHeeksCNCApp::GetNewPatternTools(std::list<Tool*>* t_list)
{
	t_list->push_back(&new_pattern_tool);
}

static CCallbackTool new_surface_tool(_("New Surface..."), _T("surface"), NewSurfaceMenuCallback);

void CHeeksCNCApp::GetNewSurfaceTools(std::list<Tool*>* t_list)
{
	t_list->push_back(&new_surface_tool);
}

static CCallbackTool new_stock_tool(_("New Stock..."), _T("stock"), NewStockMenuCallback);

void CHeeksCNCApp::GetNewStockTools(std::list<Tool*>* t_list)
{
	t_list->push_back(&new_stock_tool);
}

#define MAX_XML_SCRIPT_OPS 10

std::vector< CXmlScriptOp > script_ops;
int script_op_flyout_index = 0;

static void NewXmlScriptOp(int i)
{
	CScriptOp *new_object = new CScriptOp();
	new_object->m_title_made_from_id = false;
	new_object->m_title = script_ops[i].m_name;
	new_object->m_str = script_ops[i].m_script;
	new_object->m_user_icon = true;
	new_object->m_user_icon_name = script_ops[i].m_icon;

	if(new_object->Edit())
	{
		heeksCAD->StartHistory();
		AddNewObjectUndoablyAndMarkIt(new_object, theApp.m_program->Operations());
		heeksCAD->EndHistory();
	}
	else
		delete new_object;
}

static void NewXmlScriptOpCallback0(wxCommandEvent &event)
{
	NewXmlScriptOp(0);
}

static void NewXmlScriptOpCallback1(wxCommandEvent &event)
{
	NewXmlScriptOp(1);
}

static void NewXmlScriptOpCallback2(wxCommandEvent &event)
{
	NewXmlScriptOp(2);
}

static void NewXmlScriptOpCallback3(wxCommandEvent &event)
{
	NewXmlScriptOp(3);
}

static void NewXmlScriptOpCallback4(wxCommandEvent &event)
{
	NewXmlScriptOp(4);
}

static void NewXmlScriptOpCallback5(wxCommandEvent &event)
{
	NewXmlScriptOp(5);
}

static void NewXmlScriptOpCallback6(wxCommandEvent &event)
{
	NewXmlScriptOp(6);
}

static void NewXmlScriptOpCallback7(wxCommandEvent &event)
{
	NewXmlScriptOp(7);
}

static void NewXmlScriptOpCallback8(wxCommandEvent &event)
{
	NewXmlScriptOp(8);
}

static void NewXmlScriptOpCallback9(wxCommandEvent &event)
{
	NewXmlScriptOp(9);
}

static void AddXmlScriptOpMenuItems(wxMenu *menu = NULL)
{
	script_ops.clear();
	CProgram::GetScriptOps(script_ops);

	int i = 0;
	for(std::vector< CXmlScriptOp >::iterator It = script_ops.begin(); It != script_ops.end(); It++, i++)
	{
		CXmlScriptOp &s = *It;
		if(i >= MAX_XML_SCRIPT_OPS)break;
		void(*onButtonFunction)(wxCommandEvent&) = NULL;
		switch(i)
		{
		case 0:
			onButtonFunction = NewXmlScriptOpCallback0;
			break;
		case 1:
			onButtonFunction = NewXmlScriptOpCallback1;
			break;
		case 2:
			onButtonFunction = NewXmlScriptOpCallback2;
			break;
		case 3:
			onButtonFunction = NewXmlScriptOpCallback3;
			break;
		case 4:
			onButtonFunction = NewXmlScriptOpCallback4;
			break;
		case 5:
			onButtonFunction = NewXmlScriptOpCallback5;
			break;
		case 6:
			onButtonFunction = NewXmlScriptOpCallback6;
			break;
		case 7:
			onButtonFunction = NewXmlScriptOpCallback7;
			break;
		case 8:
			onButtonFunction = NewXmlScriptOpCallback8;
			break;
		case 9:
			onButtonFunction = NewXmlScriptOpCallback9;
			break;
		}

		if(menu)
			heeksCAD->AddMenuItem(menu, s.m_name, ToolImage(s.m_bitmap), onButtonFunction);
		else
			heeksCAD->AddFlyoutButton(s.m_name, ToolImage(s.m_bitmap), s.m_name, onButtonFunction);
	}
}	

static void AddToolBars()
{
	if(!theApp.m_machining_hidden)
	{
		wxFrame* frame = heeksCAD->GetMainFrame();
		if(theApp.m_machiningBar)delete theApp.m_machiningBar;
		theApp.m_machiningBar = new wxToolBar(frame, -1, wxDefaultPosition, wxDefaultSize, wxTB_NODIVIDER | wxTB_FLAT);
		theApp.m_machiningBar->SetToolBitmapSize(wxSize(ToolImage::GetBitmapSize(), ToolImage::GetBitmapSize()));

		heeksCAD->StartToolBarFlyout(_("Milling operations"));
		heeksCAD->AddFlyoutButton(_T("Profile"), ToolImage(_T("opprofile")), _("New Profile Operation..."), NewProfileOpMenuCallback);
		heeksCAD->AddFlyoutButton(_T("Pocket"), ToolImage(_T("pocket")), _("New Pocket Operation..."), NewPocketOpMenuCallback);
		heeksCAD->AddFlyoutButton(_T("Drill"), ToolImage(_T("drilling")), _("New Drill Cycle Operation..."), NewDrillingOpMenuCallback);
		heeksCAD->EndToolBarFlyout((wxToolBar*)(theApp.m_machiningBar));

		heeksCAD->StartToolBarFlyout(_("Other operations"));
		heeksCAD->AddFlyoutButton(_T("ScriptOp"), ToolImage(_T("scriptop")), _("New Script Operation..."), NewScriptOpMenuCallback);
		heeksCAD->AddFlyoutButton(_T("Pattern"), ToolImage(_T("pattern")), _("New Pattern..."), NewPatternMenuCallback);
		heeksCAD->AddFlyoutButton(_T("Surface"), ToolImage(_T("surface")), _("New Surface..."), NewSurfaceMenuCallback);
		heeksCAD->AddFlyoutButton(_T("Stock"), ToolImage(_T("stock")), _("New Stock..."), NewStockMenuCallback);
		AddXmlScriptOpMenuItems();

		heeksCAD->EndToolBarFlyout((wxToolBar*)(theApp.m_machiningBar));

		heeksCAD->StartToolBarFlyout(_("Tools"));
		heeksCAD->AddFlyoutButton(_T("drill"), ToolImage(_T("drill")), _("Drill..."), NewDrillMenuCallback);
		heeksCAD->AddFlyoutButton(_T("centredrill"), ToolImage(_T("centredrill")), _("Centre Drill..."), NewCentreDrillMenuCallback);
		heeksCAD->AddFlyoutButton(_T("endmill"), ToolImage(_T("endmill")), _("End Mill..."), NewEndmillMenuCallback);
		heeksCAD->AddFlyoutButton(_T("slotdrill"), ToolImage(_T("slotdrill")), _("Slot Drill..."), NewSlotCutterMenuCallback);
		heeksCAD->AddFlyoutButton(_T("ballmill"), ToolImage(_T("ballmill")), _("Ball End Mill..."), NewBallEndMillMenuCallback);
		heeksCAD->AddFlyoutButton(_T("chamfmill"), ToolImage(_T("chamfmill")), _("Chamfer Mill..."), NewChamferMenuCallback);
		heeksCAD->EndToolBarFlyout((wxToolBar*)(theApp.m_machiningBar));

		heeksCAD->StartToolBarFlyout(_("Post Processing"));
		heeksCAD->AddFlyoutButton(_T("PostProcess"), ToolImage(_T("postprocess")), _("Post-Process"), PostProcessMenuCallback);
		heeksCAD->AddFlyoutButton(_T("Run Python Script"), ToolImage(_T("runpython")), _("Run Python Script"), RunScriptMenuCallback);
		heeksCAD->AddFlyoutButton(_T("OpenNC"), ToolImage(_T("opennc")), _("Open NC File"), OpenNcFileMenuCallback);
		heeksCAD->AddFlyoutButton(_T("SaveNC"), ToolImage(_T("savenc")), _("Save NC File"), SaveNcFileMenuCallback);
#ifndef WIN32
		heeksCAD->AddFlyoutButton(_T("Send to Machine"), ToolImage(_T("tomachine")), _("Send to Machine"), SendToMachineMenuCallback);
#endif
		heeksCAD->AddFlyoutButton(_T("Cancel"), ToolImage(_T("cancel")), _("Cancel Python Script"), CancelMenuCallback);
#ifdef WIN32
		heeksCAD->AddFlyoutButton(_T("Simulate"), ToolImage(_T("simulate")), _("Simulate"), SimulateCallback);
#endif
		heeksCAD->EndToolBarFlyout((wxToolBar*)(theApp.m_machiningBar));

		theApp.m_machiningBar->Realize();
		heeksCAD->RegisterToolBar(theApp.m_machiningBar, _T("MachiningBar"), _("Machining tools"));
	}
}

void OnBuildTexture()
{
	wxString filepath = theApp.GetResFolder() + _T("/icons/iconimage.png");
	theApp.m_icon_texture_number = heeksCAD->LoadIconsTexture(filepath.c_str());
}

static void ImportExcellonDrillFile( const wxChar *file_path )
{
    Excellon drill;

	wxString message(_("Select how the file is to be interpreted"));
    wxString caption(_("Excellon drill file interpretation"));

    wxArrayString choices;

    choices.Add(_("Produce drill pattern as described"));
    choices.Add(_("Produce mirrored drill pattern"));

    wxString choice = ::wxGetSingleChoice( message, caption, choices );

    if (choice == choices[0])
    {
        drill.Read( Ttc(file_path), false );
    }

    if (choice == choices[1])
    {
        drill.Read( Ttc(file_path), true );
    }
}

static void UnitsChangedHandler( const double units )
{
    // The view units have changed.  See if the user wants the NC output units to change
    // as well.

    if (units != theApp.m_program->m_units)
    {
        int response;
        response = wxMessageBox( _("Would you like to change the NC code generation units too?"), _("Change Units"), wxYES_NO );
        if (response == wxYES)
        {
            theApp.m_program->m_units = units;
        }
    }
}

class SketchBox{
public:
	CBox m_box;
	gp_Vec m_latest_shift;

	SketchBox(const CBox &box);

	SketchBox(const SketchBox &s)
	{
		m_box = s.m_box;
		m_latest_shift = s.m_latest_shift;
	}

	void UpdateBoxAndSetShift(const CBox &new_box)
	{
		// use Centre
		double old_centre[3], new_centre[3];
		m_box.Centre(old_centre);
		new_box.Centre(new_centre);
		m_latest_shift = gp_Vec(new_centre[0] - old_centre[0], new_centre[1] - old_centre[1], 0.0);
		m_box = new_box;
	}
};
SketchBox::SketchBox(const CBox &box)
	{
		m_box = box;
		m_latest_shift = gp_Vec(0, 0, 0);
	}

class HeeksCADObserver: public Observer
{
public:
	std::map<int, SketchBox> m_box_map;

	void OnChanged(const std::list<HeeksObj*>* added, const std::list<HeeksObj*>* removed, const std::list<HeeksObj*>* modified)
	{
		if(added)
		{
			for(std::list<HeeksObj*>::const_iterator It = added->begin(); It != added->end(); It++)
			{
				HeeksObj* object = *It;
				if(object->GetType() == SketchType)
				{
					CBox box;
					object->GetBox(box);
					m_box_map.insert(std::make_pair(object->GetID(), SketchBox(box)));
				}
			}
		}

		if(modified)
		{
			for(std::list<HeeksObj*>::const_iterator It = modified->begin(); It != modified->end(); It++)
			{
				HeeksObj* object = *It;
				if(object->GetType() == SketchType)
				{
					CBox new_box;
					object->GetBox(new_box);
					std::map<int, SketchBox>::iterator FindIt = m_box_map.find(object->GetID());
					if(FindIt != m_box_map.end())
					{
						SketchBox &sketch_box = FindIt->second;
						sketch_box.UpdateBoxAndSetShift(new_box);
					}
				}
			}

			// check all the profile operations, so we can move the tags
			for(HeeksObj* object = theApp.m_program->Operations()->GetFirstChild(); object; object = theApp.m_program->Operations()->GetNextChild())
			{
				if(object->GetType() == ProfileType)
				{
					CProfile* profile = (CProfile*)object;
					std::map<int, SketchBox>::iterator FindIt = m_box_map.find(object->GetID());
					if (FindIt != m_box_map.end())
					{
						SketchBox &sketch_box = FindIt->second;
						for (HeeksObj* tag = profile->Tags()->GetFirstChild(); tag; tag = profile->Tags()->GetNextChild())
						{
							((CTag*)tag)->m_pos[0] += sketch_box.m_latest_shift.X();
							((CTag*)tag)->m_pos[1] += sketch_box.m_latest_shift.Y();
						}

						profile->m_profile_params.m_start[0] += sketch_box.m_latest_shift.X();
						profile->m_profile_params.m_start[1] += sketch_box.m_latest_shift.Y();
						profile->m_profile_params.m_start[2] += sketch_box.m_latest_shift.Z();

						profile->m_profile_params.m_end[0] += sketch_box.m_latest_shift.X();
						profile->m_profile_params.m_end[1] += sketch_box.m_latest_shift.Y();
						profile->m_profile_params.m_end[2] += sketch_box.m_latest_shift.Z();

						profile->m_profile_params.m_roll_on_point[0] += sketch_box.m_latest_shift.X();
						profile->m_profile_params.m_roll_on_point[1] += sketch_box.m_latest_shift.Y();
						profile->m_profile_params.m_roll_on_point[2] += sketch_box.m_latest_shift.Z();

						profile->m_profile_params.m_roll_off_point[0] += sketch_box.m_latest_shift.X();
						profile->m_profile_params.m_roll_off_point[1] += sketch_box.m_latest_shift.Y();
						profile->m_profile_params.m_roll_off_point[2] += sketch_box.m_latest_shift.Z();
					}
				}
			}

			for(std::map<int, SketchBox>::iterator It = m_box_map.begin(); It != m_box_map.end(); It++)
			{
				SketchBox &sketch_box = It->second;
				sketch_box.m_latest_shift = gp_Vec(0, 0, 0);
			}
		}
	}

	void Clear()
	{
		m_box_map.clear();
	}
}heekscad_observer;

class NewProfileOpTool:public Tool
{
	// Tool's virtual functions
	const wxChar* GetTitle(){return _("New Profile Operation");}
	void Run(){
		NewProfileOp();
	}
	wxString BitmapPath(){ return _T("opprofile");}
};

class NewPocketOpTool:public Tool
{
	// Tool's virtual functions
	const wxChar* GetTitle(){return _("New Pocket Operation");}
	void Run(){
		NewPocketOp();
	}
	wxString BitmapPath(){ return _T("pocket");}
};

class NewDrillingOpTool:public Tool
{
	// Tool's virtual functions
	const wxChar* GetTitle(){return _("New Drilling Operation");}
	void Run(){
		NewDrillingOp();
	}
	wxString BitmapPath(){ return _T("drilling");}
};

static void GetMarkedListTools(std::list<Tool*>& t_list)
{
	std::set<int> group_types;

	const std::list<HeeksObj*>& list = heeksCAD->GetMarkedList();
	for(std::list<HeeksObj*>::const_iterator It = list.begin(); It != list.end(); It++)
	{
		HeeksObj* object = *It;
		group_types.insert(object->GetIDGroupType());
	}

	for(std::set<int>::iterator It = group_types.begin(); It != group_types.end(); It++)
	{
		switch(*It)
		{
		case SketchType:
			t_list.push_back(new NewProfileOpTool);
			t_list.push_back(new NewPocketOpTool);
			break;
		case PointType:
			t_list.push_back(new NewDrillingOpTool);
			break;
		}
	}
}

static void OnRestoreDefaults()
{
	CNCConfig config;
	config.DeleteAll();
	theApp.m_settings_restored = true;
}

void CHeeksCNCApp::OnStartUp(CHeeksCADInterface* h, const wxString& dll_path)
{
	m_dll_path = dll_path;
	heeksCAD = h;
#if !defined WXUSINGDLL
	wxInitialize();
#endif

	// to do, use os_id
	wxOperatingSystemId os_id = wxGetOsVersion();

	CNCConfig config;

	// About box, stuff
	heeksCAD->AddToAboutBox(wxString(_T("\n\n")) + _("HeeksCNC is the free machining add-on to HeeksCAD")
		+ _T("\n") + _T("          http://code.google.com/p/heekscnc/")
		+ _T("\n") + _("Written by Dan Heeks, Hirutso Enni, Perttu Ahola, David Nicholls")
		+ _T("\n") + _("With help from archivist, crotchet1, DanielFalck, fenn, Sliptonic")
		+ _T("\n\n") + _("geometry code, donated by Geoff Hawkesford, Camtek GmbH http://www.peps.de/")
		+ _T("\n") + _("pocketing code from http://code.google.com/p/libarea/ , derived from the kbool library written by Klaas Holwerda http://boolean.klaasholwerda.nl/bool.html")
		+ _T("\n") + _("Zig zag code from opencamlib http://code.google.com/p/opencamlib/")
		+ _T("\n\n") + _("This HeeksCNC software installation is restricted by the GPL license http://www.gnu.org/licenses/gpl-3.0.txt")
		+ _T("\n") + _("  which means it is free and open source, and must stay that way")
		);

	// add menus and toolbars
	wxFrame* frame = heeksCAD->GetMainFrame();
	wxAuiManager* aui_manager = heeksCAD->GetAuiManager();

	// tool bars
	heeksCAD->RegisterAddToolBars(AddToolBars);
	AddToolBars();

	// Help menu
	wxMenu *menuHelp = heeksCAD->GetHelpMenu();
	menuHelp->AppendSeparator();
	heeksCAD->AddMenuItem(menuHelp, _("Online HeeksCNC Manual"), ToolImage(_T("help")), HelpMenuCallback);

	// Milling Operations menu
	wxMenu *menuMillingOperations = new wxMenu;
	heeksCAD->AddMenuItem(menuMillingOperations, _("Profile Operation..."), ToolImage(_T("opprofile")), NewProfileOpMenuCallback);
	heeksCAD->AddMenuItem(menuMillingOperations, _("Pocket Operation..."), ToolImage(_T("pocket")), NewPocketOpMenuCallback);
	heeksCAD->AddMenuItem(menuMillingOperations, _("Drilling Operation..."), ToolImage(_T("drilling")), NewDrillingOpMenuCallback);

	// Additive Operations menu
	wxMenu *menuOperations = new wxMenu;
	heeksCAD->AddMenuItem(menuOperations, _("Script Operation..."), ToolImage(_T("scriptop")), NewScriptOpMenuCallback);
	heeksCAD->AddMenuItem(menuOperations, _("Pattern..."), ToolImage(_T("pattern")), NewPatternMenuCallback);
	heeksCAD->AddMenuItem(menuOperations, _("Surface..."), ToolImage(_T("surface")), NewSurfaceMenuCallback);
	heeksCAD->AddMenuItem(menuOperations, _("Stock..."), ToolImage(_T("stock")), NewStockMenuCallback);
	AddXmlScriptOpMenuItems(menuOperations);

	// Tools menu
	wxMenu *menuTools = new wxMenu;
	heeksCAD->AddMenuItem(menuTools, _("Drill..."), ToolImage(_T("drill")), NewDrillMenuCallback);
	heeksCAD->AddMenuItem(menuTools, _("Centre Drill..."), ToolImage(_T("centredrill")), NewCentreDrillMenuCallback);
	heeksCAD->AddMenuItem(menuTools, _("End Mill..."), ToolImage(_T("endmill")), NewEndmillMenuCallback);
	heeksCAD->AddMenuItem(menuTools, _("Slot Drill..."), ToolImage(_T("slotdrill")), NewSlotCutterMenuCallback);
	heeksCAD->AddMenuItem(menuTools, _("Ball End Mill..."), ToolImage(_T("ballmill")), NewBallEndMillMenuCallback);
	heeksCAD->AddMenuItem(menuTools, _("Chamfer Mill..."), ToolImage(_T("chamfmill")), NewChamferMenuCallback);

	// Machining menu
	wxMenu *menuMachining = new wxMenu;
	heeksCAD->AddMenuItem(menuMachining, _("Add New Milling Operation"), ToolImage(_T("ops")), NULL, NULL, menuMillingOperations);
	heeksCAD->AddMenuItem(menuMachining, _("Add Other Operation"), ToolImage(_T("ops")), NULL, NULL, menuOperations);
	heeksCAD->AddMenuItem(menuMachining, _("Add New Tool"), ToolImage(_T("tools")), NULL, NULL, menuTools);
	heeksCAD->AddMenuItem(menuMachining, _("Run Python Script"), ToolImage(_T("runpython")), RunScriptMenuCallback);
	heeksCAD->AddMenuItem(menuMachining, _("Post-Process"), ToolImage(_T("postprocess")), PostProcessMenuCallback);
#ifdef WIN32
	heeksCAD->AddMenuItem(menuMachining, _("Simulate"), ToolImage(_T("simulate")), SimulateCallback);
#endif
	heeksCAD->AddMenuItem(menuMachining, _("Open NC File..."), ToolImage(_T("opennc")), OpenNcFileMenuCallback);
	heeksCAD->AddMenuItem(menuMachining, _("Save NC File as..."), ToolImage(_T("savenc")), SaveNcFileMenuCallback);
#ifndef WIN32
	heeksCAD->AddMenuItem(menuMachining, _("Send to Machine"), ToolImage(_T("tomachine")), SendToMachineMenuCallback);
#endif
	frame->GetMenuBar()->Insert( frame->GetMenuBar()->GetMenuCount()-1, menuMachining,  _("&Machining"));

	// add the program canvas
	m_program_canvas = new CProgramCanvas(frame);
	aui_manager->AddPane(m_program_canvas, wxAuiPaneInfo().Name(_("Program")).Caption(_("Program")).Bottom().BestSize(wxSize(600, 200)));

	// add the output canvas
	m_output_canvas = new COutputCanvas(frame);
	aui_manager->AddPane(m_output_canvas, wxAuiPaneInfo().Name(_("Output")).Caption(_("Output")).Bottom().BestSize(wxSize(600, 200)));

	// add the print canvas
	m_print_canvas = new CPrintCanvas(frame);
	aui_manager->AddPane(m_print_canvas, wxAuiPaneInfo().Name(_("Print")).Caption(_("Print")).Bottom().BestSize(wxSize(600, 200)));

	bool program_visible;
	bool output_visible;
	bool print_visible;

	config.Read(_T("ProgramVisible"), &program_visible);
	config.Read(_T("OutputVisible"), &output_visible);
	config.Read(_T("PrintVisible"), &print_visible);

	// read other settings
	CNCCode::ReadColorsFromConfig();
	CProfile::ReadFromConfig();
	CPocket::ReadFromConfig();
	CSpeedOp::ReadFromConfig();
	CSendToMachine::ReadFromConfig();
	config.Read(_T("UseClipperNotBoolean"), &m_use_Clipper_not_Boolean, false);
	config.Read(_T("UseDOSNotUnix"), &m_use_DOS_not_Unix, false);
	aui_manager->GetPane(m_program_canvas).Show(program_visible);
	aui_manager->GetPane(m_output_canvas).Show(output_visible);
	aui_manager->GetPane(m_print_canvas).Show(print_visible);

	// add tick boxes for them all on the view menu
	wxMenu* window_menu = heeksCAD->GetWindowMenu();
	window_menu->AppendSeparator();
	heeksCAD->AddMenuItem(window_menu, _("Program"), wxBitmap(), OnProgramCanvas, OnUpdateProgramCanvas, NULL, true);
	heeksCAD->AddMenuItem(window_menu, _("Output"), wxBitmap(), OnOutputCanvas, OnUpdateOutputCanvas, NULL, true);
	heeksCAD->AddMenuItem(window_menu, _("Print"), wxBitmap(), OnPrintCanvas, OnUpdatePrintCanvas, NULL, true);
	window_menu->AppendSeparator();
	heeksCAD->AddMenuItem(window_menu, _("Machining Tool Bar"), wxBitmap(), OnMachiningBar, OnUpdateMachiningBar, NULL, true);
	heeksCAD->RegisterHideableWindow(m_program_canvas);
	heeksCAD->RegisterHideableWindow(m_output_canvas);
	heeksCAD->RegisterHideableWindow(m_print_canvas);
	heeksCAD->RegisterHideableWindow(m_machiningBar);

	// add object reading functions
	heeksCAD->RegisterReadXMLfunction("Program", CProgram::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("nccode", CNCCode::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("Operations", COperations::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("Tools", CTools::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("Profile", CProfile::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("Pocket", CPocket::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("Drilling", CDrilling::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("Tool", CTool::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("CuttingTool", CTool::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("Tags", CTags::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("Tag", CTag::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("ScriptOp", CScriptOp::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("Pattern", CPattern::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("Patterns", CPatterns::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("Surface", CSurface::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("Surfaces", CSurfaces::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("Stock", CStock::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("Stocks", CStocks::ReadFromXMLElement);

	// icons
	heeksCAD->RegisterOnBuildTexture(OnBuildTexture);

	// Import functions.
	{
        std::list<wxString> file_extensions;
        file_extensions.push_back(_T("cnc"));
        file_extensions.push_back(_T("drl"));
        file_extensions.push_back(_T("drill"));
        if (! heeksCAD->RegisterFileOpenHandler( file_extensions, ImportExcellonDrillFile ))
        {
            printf("Failed to register handler for Excellon dril files\n");
        }
	}
	{
        std::list<wxString> file_extensions;
        file_extensions.push_back(_T("tool"));
        file_extensions.push_back(_T("tools"));
        file_extensions.push_back(_T("tooltable"));
        if (! heeksCAD->RegisterFileOpenHandler( file_extensions, ImportToolsFile ))
        {
            printf("Failed to register handler for Tool Table files\n");
        }
	}

	heeksCAD->RegisterObserver(&heekscad_observer);

	heeksCAD->RegisterUnitsChangeHandler( UnitsChangedHandler );
	heeksCAD->RegisterHeeksTypesConverter( HeeksCNCType );

	heeksCAD->RegisterMarkeListTools(&GetMarkedListTools);
	heeksCAD->RegisterOnRestoreDefaults(&OnRestoreDefaults);
}

std::list<wxString> CHeeksCNCApp::GetFileNames( const char *p_szRoot ) const
#ifdef WIN32
{
	std::list<wxString>	results;

	WIN32_FIND_DATA file_data;
	HANDLE hFind;

	std::string pattern = std::string(p_szRoot) + "\\*";
	hFind = FindFirstFile(Ctt(pattern.c_str()), &file_data);

	// Now recurse down until we find document files within 'current' directories.
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if ((file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) continue;

			results.push_back( file_data.cFileName );
		} while (FindNextFile( hFind, &file_data));

		FindClose(hFind);
	} // End if - then

	return(results);
} // End of GetFileNames() method.
#else
{
	// We're in UNIX land now.

	std::list<wxString>	results;

	DIR *pdir = opendir(p_szRoot);	// Look in the current directory for files
				// whose names begin with "default."
	if (pdir != NULL)
	{
		struct dirent *pent = NULL;
		while ((pent=readdir(pdir)))
		{
			results.push_back(Ctt(pent->d_name));
		} // End while
		closedir(pdir);
	} // End if - then

	return(results);
} // End of GetFileNames() method
#endif



void CHeeksCNCApp::OnNewOrOpen(bool open, int res)
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

		m_program->AddMissingChildren();
		heeksCAD->GetMainObject()->Add(m_program, NULL);
		theApp.m_program_canvas->Clear();
		theApp.m_output_canvas->Clear();
		theApp.m_print_canvas->Clear();

		heeksCAD->OpenXMLFile(GetResourceFilename(wxT("default.tooltable")), theApp.m_program->Tools());
	} // End if - then
}

void on_set_use_clipper(bool value, HeeksObj* object)
{
	theApp.m_use_Clipper_not_Boolean = value;
}

void on_set_use_DOS(bool value, HeeksObj* object)
{
    theApp.m_use_DOS_not_Unix = value;

}

void CHeeksCNCApp::GetOptions(std::list<Property *> *list){
	PropertyList* machining_options = new PropertyList(_("machining options"));
	CNCCode::GetOptions(&(machining_options->m_list));
	CSpeedOp::GetOptions(&(machining_options->m_list));
	CProfile::GetOptions(&(machining_options->m_list));
	CPocket::GetOptions(&(machining_options->m_list));
	CSendToMachine::GetOptions(&(machining_options->m_list));
	machining_options->m_list.push_back ( new PropertyCheck ( _("Use Clipper not Boolean"), m_use_Clipper_not_Boolean, NULL, on_set_use_clipper ) );
	machining_options->m_list.push_back ( new PropertyCheck ( _("Use DOS Line Endings"), m_use_DOS_not_Unix, NULL, on_set_use_DOS ) );

	list->push_back(machining_options);

	PropertyList* excellon_options = new PropertyList(_("Excellon options"));
	Excellon::GetOptions(&(excellon_options->m_list));
	list->push_back(excellon_options);
}

void CHeeksCNCApp::OnFrameDelete()
{
	wxAuiManager* aui_manager = heeksCAD->GetAuiManager();
	CNCConfig config;
	config.Write(_T("ProgramVisible"), aui_manager->GetPane(m_program_canvas).IsShown());
	config.Write(_T("OutputVisible"), aui_manager->GetPane(m_output_canvas).IsShown());
	config.Write(_T("PrintVisible"), aui_manager->GetPane(m_print_canvas).IsShown());
	config.Write(_T("MachiningBarVisible"), aui_manager->GetPane(m_machiningBar).IsShown());

	CNCCode::WriteColorsToConfig();
	CProfile::WriteToConfig();
	CPocket::WriteToConfig();
	CSpeedOp::WriteToConfig();
	CSendToMachine::WriteToConfig();
	config.Write(_T("UseClipperNotBoolean"), m_use_Clipper_not_Boolean);
    config.Write(_T("UseDOSNotUnix"), m_use_DOS_not_Unix);
}

Python CHeeksCNCApp::SetTool( const int new_tool )
{
	Python python;

	// Select the right tool.
	CTool *pTool = (CTool *) CTool::Find(new_tool);
	if (pTool != NULL)
	{
		if (m_tool_number != new_tool)
		{

			python << _T("tool_change( id=") << new_tool << _T(")\n");
		}

		if(m_attached_to_surface)
		{
			python << _T("nc.creator.set_ocl_cutter(") << pTool->OCLDefinition(m_attached_to_surface) << _T(")\n");
		}
	} // End if - then

	m_tool_number = new_tool;

    return(python);
}

wxString CHeeksCNCApp::GetDllFolder() const
{
	return m_dll_path;
}

wxString CHeeksCNCApp::GetResFolder() const
{
#if defined(WIN32) || defined(RUNINPLACE) //compile with 'RUNINPLACE=yes make' then skip 'sudo make install'
  #ifdef CMAKE_UNIX
	return (m_dll_path + _T("/.."));
  #else
	return m_dll_path;
  #endif
#else
  #ifdef CMAKE_UNIX
	// Unix
    #if wxCHECK_VERSION(3, 0, 0)
	wxStandardPaths& sp = wxStandardPaths::Get();
    #else
	wxStandardPaths sp;
    #endif
	return (sp.GetInstallPrefix() + wxT("/share/heekscnc"));
  #else // CMAKE_UNIX
	// Windows
	return (m_dll_path + _T("/../../share/heekscnc"));
  #endif // CMAKE_UNIX
#endif // defined(WIN32) || defined(RUNINPLACE)
}

wxString CHeeksCNCApp::GetResourceFilename(const wxString resource, const bool writableOnly) const
{
	wxString filename;

#ifdef WIN32
	// Windows
	filename = GetDllFolder() + wxT("/") + resource;
#else
	// Unix
	// Under Unix, it looks for a user-defined resource first.
	// According to FreeDesktop XDG standards, HeeksCNC user-defineable resources should be placed in XDG_CONFIG_HOME (usually: ~/.config/heekscnc/)
	filename = (wxGetenv(wxT("XDG_CONFIG_HOME")) ? wxString(wxGetenv(wxT("XDG_CONFIG_HOME"))) : wxFileName::GetHomeDir() + wxT("/.config"));
  filename += wxT("/heekscnc/") + resource;

	// Under Unix user can't save its resources in system (permissions denied), so we always return a user-writable file
	if(!writableOnly)
	{
		// If user-defined file exists, the resource is located
		if(!wxFileName::FileExists(filename))
		{
			// Else it fallbacks to system-wide resource file (installed with HeeksCNC)
			filename = GetResFolder() + wxT("/") + resource;
			// Note: it should be a good idea to use wxStandardPaths::GetResourcesDir() but it returns HeeksCAD's resource dir (eg. /usr/share/heekscad)
		}
	}
	else
	{
		// Writable file is wanted, so ressource directories should exists (ie. mkdir -p ~/.config/heekscnc)
		wxFileName fn(filename);
		wxFileName::Mkdir(fn.GetPath(), 0700, wxPATH_MKDIR_FULL);
	}
	
#endif
	wprintf(wxT("Resource: ") + resource + wxT(" found at: ") + filename + wxT("\n"));
	return filename;
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


wxString HeeksCNCType( const int type )
{
    switch (type)
    {
    case ProgramType:       return(_("Program"));
	case NCCodeBlockType:       return(_("NCCodeBlock"));
	case NCCodeType:       return(_("NCCode"));
	case OperationsType:       return(_("Operations"));
	case ProfileType:       return(_("Profile"));
	case PocketType:       return(_("Pocket"));
	case DrillingType:       return(_("Drilling"));
	case ToolType:       return(_("Tool"));
	case ToolsType:       return(_("Tools"));
	case TagsType:       return(_("Tags"));
	case TagType:       return(_("Tag"));
	case ScriptOpType:       return(_("ScriptOp"));

	default:
        return(_T("")); // Indicates that this function could not make the conversion.
    } // End switch
} // End HeeksCNCType() routine
