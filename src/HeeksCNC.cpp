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
#include "Tapping.h"
#include "Positioning.h"
#include "CTool.h"
#include "SpeedReference.h"
#include "CuttingRate.h"
#include "Operations.h"
#include "Tools.h"
#include "interface/strconv.h"
#include "CNCPoint.h"
#include "Probing.h"
#include "Excellon.h"
#include "Tags.h"
#include "Tag.h"
#include "ScriptOp.h"
#include "AttachOp.h"
#include "Simulate.h"

#include <sstream>

CHeeksCADInterface* heeksCAD = NULL;

CHeeksCNCApp theApp;

extern void ImportToolsFile( const wxChar *file_path );
extern void ImportSpeedReferencesFile( const wxChar *file_path );

extern CTool::tap_sizes_t metric_tap_sizes[];
extern CTool::tap_sizes_t unified_thread_standard_tap_sizes[];
extern CTool::tap_sizes_t british_standard_whitworth_tap_sizes[];

wxString HeeksCNCType(const int type);

CHeeksCNCApp::CHeeksCNCApp(){
	m_draw_cutter_radius = true;
	m_program = NULL;
	m_run_program_on_new_line = false;
	m_machiningBar = NULL;
	m_icon_texture_number = 0;
	m_machining_hidden = false;
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

static bool GetSketches(std::list<int>& sketches, std::list<int> &tools )
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

	if(sketches.size() == 0)
	{
		wxMessageBox(_("You must select some sketches, first!"));
		return false;
	}

	return true;
}

static void NewProfileOpMenuCallback(wxCommandEvent &event)
{
	std::list<int> drill_bits;
	std::list<int> tools;
	std::list<int> sketches;
	int milling_tool_number = -1;
	if(GetSketches(sketches, tools))
	{
		// Look through the tools that have been selected.  If any of them
		// are drill or centre bits then create Drilling cycle objects as well.
		// If we find a milling bit then pass that through to the CProfile object.
		std::list<int>::const_iterator l_itTool;
		for (l_itTool = tools.begin(); l_itTool != tools.end(); l_itTool++)
		{
			HeeksObj *ob = heeksCAD->GetIDObject( ToolType, *l_itTool );
			if (ob != NULL)
			{
				switch (((CTool *)ob)->m_params.m_type)
				{
					case CToolParams::eDrill:
					case CToolParams::eCentreDrill:
						// Keep a list for later.  We need to create the Profile object
						// before we create Drilling objects that refer to it.
						drill_bits.push_back( *l_itTool );
						break;

					case CToolParams::eChamfer:
					case CToolParams::eEndmill:
					case CToolParams::eSlotCutter:
					case CToolParams::eBallEndMill:
						// We only want one.  Just keep overwriting this variable.
						milling_tool_number = ((CTool *)ob)->m_tool_number;
						break;

					default:
						break;
				} // End switch
			} // End if - then
		} // End for

		heeksCAD->CreateUndoPoint();
		CProfile *new_object = new CProfile(sketches, milling_tool_number);
		new_object->AddMissingChildren();
		theApp.m_program->Operations()->Add(new_object,NULL);
		heeksCAD->ClearMarkedList();
		heeksCAD->Mark(new_object);

		CDrilling::Symbols_t profiles;
		profiles.push_back( CDrilling::Symbol_t( new_object->GetType(), new_object->m_id ) );

		for (l_itTool = drill_bits.begin(); l_itTool != drill_bits.end(); l_itTool++)
		{
			HeeksObj *ob = heeksCAD->GetIDObject( ToolType, *l_itTool );
			if (ob != NULL)
			{
				CDrilling *new_object = new CDrilling( profiles, ((CTool *)ob)->m_tool_number, -1 );
				theApp.m_program->Operations()->Add(new_object, NULL);
				heeksCAD->ClearMarkedList();
				heeksCAD->Mark(new_object);
			} // End if - then
		} // End for
		heeksCAD->Changed();
	}
}

static void NewPocketOpMenuCallback(wxCommandEvent &event)
{
	std::list<int> tools;
	std::list<int> sketches;
	if(GetSketches(sketches, tools))
	{
		CPocket *new_object = new CPocket(sketches, (tools.size()>0)?(*tools.begin()):-1 );
		if(new_object->Edit())
		{
			heeksCAD->CreateUndoPoint();
			theApp.m_program->Operations()->Add(new_object, NULL);
			heeksCAD->Changed();
		}
		else
			delete new_object;
	}
}

static void NewDrillingOpMenuCallback(wxCommandEvent &event)
{
	std::vector<CNCPoint> intersections;
	CDrilling::Symbols_t symbols;
	CDrilling::Symbols_t Tools;
	int tool_number = 0;

	const std::list<HeeksObj*>& list = heeksCAD->GetMarkedList();
	for(std::list<HeeksObj*>::const_iterator It = list.begin(); It != list.end(); It++)
	{
		HeeksObj* object = *It;
		if (object->GetType() == ToolType)
		{
			Tools.push_back( CDrilling::Symbol_t( object->GetType(), object->m_id ) );
			tool_number = ((CTool *)object)->m_tool_number;
		} // End if - then
		else
		{
		    if (CDrilling::ValidType( object->GetType() ))
		    {
                symbols.push_back( CDrilling::Symbol_t( object->GetType(), object->m_id ) );
		    }
		} // End if - else
	} // End for

	double depth = -1;
	CDrilling::Symbols_t ToolsThatMatchCircles;
	CDrilling drill( symbols, -1, -1 );

	intersections = CDrilling::FindAllLocations(&drill);

	if ((Tools.size() == 0) && (ToolsThatMatchCircles.size() > 0))
	{
		// The operator didn't point to a tool object and one of the circles that they
		// did point to happenned to match the diameter of an existing tool.  Use that
		// one as our default.  The operator can always overwrite it later on.

		std::copy( ToolsThatMatchCircles.begin(), ToolsThatMatchCircles.end(),
				std::inserter( Tools, Tools.begin() ));
	} // End if - then

	if (intersections.size() == 0)
	{
		wxMessageBox(_("You must select some points, circles or other intersecting elements first!"));
		return;
	}

	if(Tools.size() > 1)
	{
		wxMessageBox(_("You may only select a single tool for each drilling operation.!"));
		return;
	}

	heeksCAD->CreateUndoPoint();
	CDrilling *new_object = new CDrilling( symbols, tool_number, depth );
	theApp.m_program->Operations()->Add(new_object, NULL);
	heeksCAD->ClearMarkedList();
	heeksCAD->Mark(new_object);
	heeksCAD->Changed();
}


static void NewTappingOpMenuCallback(wxCommandEvent &event)
{
	std::vector<CNCPoint> intersections;
	CTapping::Symbols_t symbols;
	CTapping::Symbols_t Tools;
	int tool_number = 0;

	const std::list<HeeksObj*>& list = heeksCAD->GetMarkedList();
	for(std::list<HeeksObj*>::const_iterator It = list.begin(); It != list.end(); It++)
	{
		HeeksObj* object = *It;
		if (object->GetType() == ToolType)
		{
			Tools.push_back( CTapping::Symbol_t( object->GetType(), object->m_id ) );
			tool_number = ((CTool *)object)->m_tool_number;
		} // End if - then
		else
		{
		    if (CTapping::ValidType( object->GetType() ))
		    {
                symbols.push_back( CTapping::Symbol_t( object->GetType(), object->m_id ) );
		    }
		} // End if - else
	} // End for

	double depth = -1;
	CTapping::Symbols_t ToolsThatMatchCircles;
	CTapping tap( symbols, -1, -1 );

	intersections = CDrilling::FindAllLocations(&tap);

	if ((Tools.size() == 0) && (ToolsThatMatchCircles.size() > 0))
	{
		// The operator didn't point to a tool object and one of the circles that they
		// did point to happenned to match the diameter of an existing tool.  Use that
		// one as our default.  The operator can always overwrite it later on.

		std::copy( ToolsThatMatchCircles.begin(), ToolsThatMatchCircles.end(),
				std::inserter( Tools, Tools.begin() ));
	} // End if - then

	if (intersections.size() == 0)
	{
		wxMessageBox(_("You must select some points, circles or other intersecting elements first!"));
		return;
	}

	if(Tools.size() > 1)
	{
		wxMessageBox(_("You may only select a single tool for each tapping operation.!"));
		return;
	}

	heeksCAD->CreateUndoPoint();
	//CDrilling *new_object = new CDrilling( symbols, tool_number, depth );
	CTapping *new_object = new CTapping( symbols, tool_number, depth );
	theApp.m_program->Operations()->Add(new_object, NULL);
	heeksCAD->ClearMarkedList();
	heeksCAD->Mark(new_object);
	heeksCAD->Changed();
}

static void NewAttachOpMenuCallback(wxCommandEvent &event)
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

	if(solids.size() == 0)
	{
		wxMessageBox(_("There are no solids!"));
		return;
	}

	heeksCAD->CreateUndoPoint();
	CAttachOp *new_object = new CAttachOp(solids, 0.01, 0.0);
	theApp.m_program->Operations()->Add(new_object, NULL);
	heeksCAD->ClearMarkedList();
	heeksCAD->Mark(new_object);
	heeksCAD->Changed();
}

static void NewUnattachOpMenuCallback(wxCommandEvent &event)
{
	heeksCAD->CreateUndoPoint();
	CUnattachOp *new_object = new CUnattachOp();
	theApp.m_program->Operations()->Add(new_object, NULL);
	heeksCAD->ClearMarkedList();
	heeksCAD->Mark(new_object);
	heeksCAD->Changed();
}

static void NewPositioningOpMenuCallback(wxCommandEvent &event)
{
	std::vector<CNCPoint> intersections;
	CDrilling::Symbols_t symbols;

	const std::list<HeeksObj*>& list = heeksCAD->GetMarkedList();
	for(std::list<HeeksObj*>::const_iterator It = list.begin(); It != list.end(); It++)
	{
		HeeksObj* object = *It;
		if (object != NULL)
		{
		    if (CPositioning::ValidType( object->GetType() ))
		    {
                symbols.push_back( CDrilling::Symbol_t( object->GetType(), object->m_id ) );
		    }
		} // End if - then
	} // End for

    if (symbols.size() == 0)
    {
        wxMessageBox(_("You must select some points, circles or other intersecting elements first!"));
        return;
    }

	heeksCAD->CreateUndoPoint();
	CPositioning *new_object = new CPositioning( symbols );
	theApp.m_program->Operations()->Add(new_object, NULL);
	heeksCAD->ClearMarkedList();
	heeksCAD->Mark(new_object);
	heeksCAD->Changed();
}

static void NewProbe_Centre_MenuCallback(wxCommandEvent &event)
{
	CTool::ToolNumber_t tool_number = 0;

	const std::list<HeeksObj*>& list = heeksCAD->GetMarkedList();
	for(std::list<HeeksObj*>::const_iterator It = list.begin(); It != list.end(); It++)
	{
		HeeksObj* object = *It;
		if ((object != NULL) && (object->GetType() == ToolType))
		{
			tool_number = ((CTool *)object)->m_tool_number;
		} // End if - then
	} // End for

	if (tool_number == 0)
	{
		tool_number = CTool::FindFirstByType( CToolParams::eTouchProbe );
	} // End if - then

	heeksCAD->CreateUndoPoint();
	CProbe_Centre *new_object = new CProbe_Centre( tool_number );
	theApp.m_program->Operations()->Add(new_object, NULL);
	heeksCAD->ClearMarkedList();
	heeksCAD->Mark(new_object);
	heeksCAD->Changed();
}

static void NewProbe_Grid_MenuCallback(wxCommandEvent &event)
{
	CTool::ToolNumber_t tool_number = 0;

	const std::list<HeeksObj*>& list = heeksCAD->GetMarkedList();
	for(std::list<HeeksObj*>::const_iterator It = list.begin(); It != list.end(); It++)
	{
		HeeksObj* object = *It;
		if ((object != NULL) && (object->GetType() == ToolType))
		{
			tool_number = ((CTool *)object)->m_tool_number;
		} // End if - then
	} // End for

	if (tool_number == 0)
	{
		tool_number = CTool::FindFirstByType( CToolParams::eTouchProbe );
	} // End if - then

	heeksCAD->CreateUndoPoint();
	CProbe_Grid *new_object = new CProbe_Grid( tool_number );
	theApp.m_program->Operations()->Add(new_object, NULL);
	heeksCAD->ClearMarkedList();
	heeksCAD->Mark(new_object);
	heeksCAD->Changed();
}


static void NewProbe_Edge_MenuCallback(wxCommandEvent &event)
{
	CTool::ToolNumber_t tool_number = 0;

	const std::list<HeeksObj*>& list = heeksCAD->GetMarkedList();
	for(std::list<HeeksObj*>::const_iterator It = list.begin(); It != list.end(); It++)
	{
		HeeksObj* object = *It;
		if ((object != NULL) && (object->GetType() == ToolType))
		{
			tool_number = ((CTool *)object)->m_tool_number;
		} // End if - then
	} // End for

	if (tool_number == 0)
	{
		tool_number = CTool::FindFirstByType( CToolParams::eTouchProbe );
	} // End if - then

	heeksCAD->CreateUndoPoint();
	CProbe_Edge *new_object = new CProbe_Edge( tool_number );
	theApp.m_program->Operations()->Add(new_object, NULL);
	heeksCAD->ClearMarkedList();
	heeksCAD->Mark(new_object);
	heeksCAD->Changed();
}

static void DesignRulesAdjustment(const bool apply_changes)
{
	std::list<wxString> changes;

	HeeksObj* operations = theApp.m_program->Operations();

	for(HeeksObj* obj = operations->GetFirstChild(); obj; obj = operations->GetNextChild())
	{
		if (COperations::IsAnOperation( obj->GetType() ))
		{
			if (obj != NULL)
			{
				std::list<wxString> change = ((COp *)obj)->DesignRulesAdjustment(apply_changes);
				std::copy( change.begin(), change.end(), std::inserter( changes, changes.end() ));
			} // End if - then
		} // End if - then
	} // End for

	std::wostringstream l_ossChanges;
	for (std::list<wxString>::const_iterator l_itChange = changes.begin(); l_itChange != changes.end(); l_itChange++)
	{
		l_ossChanges << l_itChange->c_str();
	} // End for

	if (l_ossChanges.str().size() > 0)
	{
		wxMessageBox( l_ossChanges.str().c_str() );
	} // End if - then

} // End DesignRulesAdjustmentMenuCallback() routine


static void DesignRulesAdjustmentMenuCallback(wxCommandEvent &event)
{
	DesignRulesAdjustment(true);
} // End DesignRulesAdjustmentMenuCallback() routine

static void DesignRulesCheckMenuCallback(wxCommandEvent &event)
{
	DesignRulesAdjustment(false);
} // End DesignRulesCheckMenuCallback() routine

static void NewSpeedReferenceMenuCallback(wxCommandEvent &event)
{
	heeksCAD->CreateUndoPoint();
	CSpeedReference *new_object = new CSpeedReference(_T("Fill in material name"), int(CToolParams::eCarbide), 0.0, 0.0);
	theApp.m_program->SpeedReferences()->Add(new_object, NULL);
	heeksCAD->ClearMarkedList();
	heeksCAD->Mark(new_object);
	heeksCAD->Changed();
}

static void NewCuttingRateMenuCallback(wxCommandEvent &event)
{
	heeksCAD->CreateUndoPoint();
	CCuttingRate *new_object = new CCuttingRate(0.0, 0.0);
	theApp.m_program->SpeedReferences()->Add(new_object, NULL);
	heeksCAD->ClearMarkedList();
	heeksCAD->Mark(new_object);
	heeksCAD->Changed();
}

static void NewScriptOpMenuCallback(wxCommandEvent &event)
{
	heeksCAD->CreateUndoPoint();
	CScriptOp *new_object = new CScriptOp();
	theApp.m_program->Operations()->Add(new_object, NULL);
	heeksCAD->ClearMarkedList();
	heeksCAD->Mark(new_object);
	heeksCAD->Changed();
}

static void AddNewTool(CToolParams::eToolType type)
{
	// Add a new tool.
	heeksCAD->CreateUndoPoint();
	CTool *new_object = new CTool(NULL, type, heeksCAD->GetNextID(ToolType));
	theApp.m_program->Tools()->Add(new_object, NULL);
	heeksCAD->ClearMarkedList();
	heeksCAD->Mark(new_object);
	heeksCAD->Changed();
}

static void NewDrillMenuCallback(wxCommandEvent &event)
{
	AddNewTool(CToolParams::eDrill);
}

static void NewMetricTappingToolMenuCallback(wxCommandEvent &event)
{
    wxString message(_("Select tap size"));
    wxString caption(_("Standard Tap Sizes"));

    wxArrayString choices;

    for (::size_t i=0; (metric_tap_sizes[i].diameter > 0.0); i++)
    {
        choices.Add(metric_tap_sizes[i].description);
    }

    wxString choice = ::wxGetSingleChoice( message, caption, choices );

    for (::size_t i=0; (metric_tap_sizes[i].diameter > 0.0); i++)
    {
        if ((choices.size() > 0) && (choice == choices[i]))
        {
            // Add a new tool.
            heeksCAD->CreateUndoPoint();
            CTool *new_object = new CTool(NULL, CToolParams::eTapTool, heeksCAD->GetNextID(ToolType));
            new_object->m_params.m_diameter = metric_tap_sizes[i].diameter;
            new_object->m_params.m_pitch = metric_tap_sizes[i].pitch;
            new_object->m_params.m_direction = 0;    // Right hand thread.
            new_object->ResetTitle();
            theApp.m_program->Tools()->Add(new_object, NULL);
            heeksCAD->ClearMarkedList();
            heeksCAD->Mark(new_object);
            heeksCAD->Changed();
            return;
        }
    }
}

static void NewUnifiedThreadingStandardTappingToolMenuCallback(wxCommandEvent &event)
{
	wxString message(_("Select tap size"));
    wxString caption(_("Standard Tap Sizes"));

    wxArrayString choices;

    for (::size_t i=0; (unified_thread_standard_tap_sizes[i].diameter > 0.0); i++)
    {
        choices.Add(unified_thread_standard_tap_sizes[i].description);
    }

    wxString choice = ::wxGetSingleChoice( message, caption, choices );

    for (::size_t i=0; (unified_thread_standard_tap_sizes[i].diameter > 0.0); i++)
    {
        if ((choices.size() > 0) && (choice == choices[i]))
        {
            // Add a new tool.
            heeksCAD->CreateUndoPoint();
            CTool *new_object = new CTool(NULL, CToolParams::eTapTool, heeksCAD->GetNextID(ToolType));
            new_object->m_params.m_diameter = unified_thread_standard_tap_sizes[i].diameter;
            new_object->m_params.m_pitch = unified_thread_standard_tap_sizes[i].pitch;
            new_object->m_params.m_direction = 0;    // Right hand thread.
            new_object->ResetTitle();
            theApp.m_program->Tools()->Add(new_object, NULL);
            heeksCAD->ClearMarkedList();
            heeksCAD->Mark(new_object);
            heeksCAD->Changed();
            return;
        }
    }
}

static void NewBritishStandardWhitworthTappingToolMenuCallback(wxCommandEvent &event)
{
	wxString message(_("Select tap size"));
    wxString caption(_("Standard Tap Sizes"));

    wxArrayString choices;

    for (::size_t i=0; (british_standard_whitworth_tap_sizes[i].diameter > 0.0); i++)
    {
        choices.Add(british_standard_whitworth_tap_sizes[i].description);
    }

    wxString choice = ::wxGetSingleChoice( message, caption, choices );

    for (::size_t i=0; (british_standard_whitworth_tap_sizes[i].diameter > 0.0); i++)
    {
        if ((choices.size() > 0) && (choice == choices[i]))
        {
            // Add a new tool.
            heeksCAD->CreateUndoPoint();
            CTool *new_object = new CTool(NULL, CToolParams::eTapTool, heeksCAD->GetNextID(ToolType));
            new_object->m_params.m_diameter = british_standard_whitworth_tap_sizes[i].diameter;
            new_object->m_params.m_pitch = british_standard_whitworth_tap_sizes[i].pitch;
            new_object->m_params.m_direction = 0;    // Right hand thread.
            new_object->ResetTitle();
            theApp.m_program->Tools()->Add(new_object, NULL);
            heeksCAD->ClearMarkedList();
            heeksCAD->Mark(new_object);
            heeksCAD->Changed();
            return;
        }
    }
}

static void NewTapToolMenuCallback(wxCommandEvent &event)
{
	AddNewTool(CToolParams::eTapTool);
}

static void NewEngraverToolMenuCallback(wxCommandEvent &event)
{
	AddNewTool(CToolParams::eEngravingTool);
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

static void NewTouchProbeMenuCallback(wxCommandEvent &event)
{
	AddNewTool(CToolParams::eTouchProbe);
}

static void NewToolLengthSwitchMenuCallback(wxCommandEvent &event)
{
	AddNewTool(CToolParams::eToolLengthSwitch);
}

static void NewDragKnifeMenuCallback(wxCommandEvent &event)
{
	AddNewTool(CToolParams::eDragKnife);
}

static void MakeScriptMenuCallback(wxCommandEvent &event)
{
	// create the Python program
	theApp.m_program->RewritePythonProgram();
}

void CHeeksCNCApp::RunPythonScript()
{
	{
		// clear the output file
		wxFile f(m_program->GetOutputFileName().c_str(), wxFile::write);
		if(f.IsOpened())f.Write(_T("\n"));
	}
	{
		// clear the backplot file
		wxString backplot_path = m_program->GetOutputFileName() + _T(".nc.xml");
		wxFile f(backplot_path.c_str(), wxFile::write);
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

static void SimulateCallback(wxCommandEvent &event)
{
	RunVoxelcutSimulation();
}

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
{   wxStandardPaths sp;
    wxString user_docs =sp.GetDocumentsDir();
    wxString ncdir;
    //ncdir =  user_docs + _T("/nc");
    ncdir =  user_docs; //I was getting tired of having to start out at the root directory in linux
	wxString ext_str(_T("*.*")); // to do, use the machine's NC extension
	wxString wildcard_string = wxString(_("NC files")) + _T(" |") + ext_str;
    wxString defaultDir = ncdir;
	wxFileDialog fd(theApp.m_output_canvas, _("Save NC file"), defaultDir, wxEmptyString, wildcard_string, wxSAVE|wxOVERWRITE_PROMPT);
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
static CCallbackTool new_touch_probe(_("New Touch Probe..."), _T("probe"), NewTouchProbeMenuCallback);
static CCallbackTool new_tool_length_switch(_("New Tool Length Switch..."), _T("probe"), NewToolLengthSwitchMenuCallback);
static CCallbackTool new_tap_tool(_("New Tap Tool..."), _T("tap"), NewTapToolMenuCallback);
static CCallbackTool new_engraver_tool(_("New Engraver Tool..."), _T("engraver"), NewEngraverToolMenuCallback);
static CCallbackTool new_drag_knife(_("New Drag Knife..."), _T("knife"), NewDragKnifeMenuCallback);

void CHeeksCNCApp::GetNewToolTools(std::list<Tool*>* t_list)
{
	t_list->push_back(&new_drill_tool);
	t_list->push_back(&new_centre_drill_tool);
	t_list->push_back(&new_endmill_tool);
	t_list->push_back(&new_slotdrill_tool);
	t_list->push_back(&new_ball_end_mill_tool);
	t_list->push_back(&new_chamfer_mill_tool);
	t_list->push_back(&new_touch_probe);
	t_list->push_back(&new_tool_length_switch);
	t_list->push_back(&new_tap_tool);
	t_list->push_back(&new_engraver_tool);
	t_list->push_back(&new_drag_knife);
}

static void AddToolBars()
{
	if(!theApp.m_machining_hidden)
	{
		wxFrame* frame = heeksCAD->GetMainFrame();
		wxAuiManager* aui_manager = heeksCAD->GetAuiManager();
		if(theApp.m_machiningBar)delete theApp.m_machiningBar;
		theApp.m_machiningBar = new wxToolBar(frame, -1, wxDefaultPosition, wxDefaultSize, wxTB_NODIVIDER | wxTB_FLAT);
		theApp.m_machiningBar->SetToolBitmapSize(wxSize(ToolImage::GetBitmapSize(), ToolImage::GetBitmapSize()));

		heeksCAD->StartToolBarFlyout(_("Milling operations"));
		heeksCAD->AddFlyoutButton(_("Profile"), ToolImage(_T("opprofile")), _("New Profile Operation..."), NewProfileOpMenuCallback);
		heeksCAD->AddFlyoutButton(_("Pocket"), ToolImage(_T("pocket")), _("New Pocket Operation..."), NewPocketOpMenuCallback);
		heeksCAD->AddFlyoutButton(_("Drill"), ToolImage(_T("drilling")), _("New Drill Cycle Operation..."), NewDrillingOpMenuCallback);
		heeksCAD->AddFlyoutButton(_("Tap"), ToolImage(_T("optap")), _("New Tapping Operation..."), NewTappingOpMenuCallback);
		heeksCAD->EndToolBarFlyout((wxToolBar*)(theApp.m_machiningBar));

		heeksCAD->StartToolBarFlyout(_("3D Milling operations"));
		heeksCAD->AddFlyoutButton(_("Attach"), ToolImage(_T("attach")), _("New Attach Operation..."), NewAttachOpMenuCallback);
		heeksCAD->AddFlyoutButton(_("Unattach"), ToolImage(_T("unattach")), _("New Unattach Operation..."), NewUnattachOpMenuCallback);
		heeksCAD->EndToolBarFlyout((wxToolBar*)(theApp.m_machiningBar));

		heeksCAD->StartToolBarFlyout(_("Other operations"));
		heeksCAD->AddFlyoutButton(_("Positioning"), ToolImage(_T("locating")), _("New Positioning Operation..."), NewPositioningOpMenuCallback);
		heeksCAD->AddFlyoutButton(_("Probing"), ToolImage(_T("probe")), _("New Probe Centre Operation..."), NewProbe_Centre_MenuCallback);
		heeksCAD->AddFlyoutButton(_("Probing"), ToolImage(_T("probe")), _("New Probe Edge Operation..."), NewProbe_Edge_MenuCallback);
		heeksCAD->AddFlyoutButton(_("Probing"), ToolImage(_T("probe")), _("New Probe Grid Operation..."), NewProbe_Grid_MenuCallback);
		heeksCAD->AddFlyoutButton(_("ScriptOp"), ToolImage(_T("scriptop")), _("New Script Operation..."), NewScriptOpMenuCallback);
		heeksCAD->EndToolBarFlyout((wxToolBar*)(theApp.m_machiningBar));

		heeksCAD->AddToolBarButton((wxToolBar*)(theApp.m_machiningBar), _("Tool"), ToolImage(_T("drill")), _("New Tool Definition..."), NewDrillMenuCallback);
		heeksCAD->StartToolBarFlyout(_("Design Rules"));
		heeksCAD->AddFlyoutButton(_("Design Rules Check"), ToolImage(_("design_rules_check")), _("Design Rules Check..."), DesignRulesCheckMenuCallback);
		heeksCAD->AddFlyoutButton(_("Design Rules Adjustment"), ToolImage(_("design_rules_adjustment")), _("Design Rules Adjustment..."), DesignRulesAdjustmentMenuCallback);
		heeksCAD->EndToolBarFlyout((wxToolBar*)(theApp.m_machiningBar));

		heeksCAD->StartToolBarFlyout(_("Speeds"));
		heeksCAD->AddFlyoutButton(_("Speed Reference"), ToolImage(_T("speed_reference")), _("Add Speed Reference..."), NewSpeedReferenceMenuCallback);
		heeksCAD->AddFlyoutButton(_("Cutting Rate"), ToolImage(_T("cutting_rate")), _("Add Cutting Rate Reference..."), NewCuttingRateMenuCallback);
		heeksCAD->EndToolBarFlyout((wxToolBar*)(theApp.m_machiningBar));

		heeksCAD->StartToolBarFlyout(_("Post Processing"));
		heeksCAD->AddFlyoutButton(_("PostProcess"), ToolImage(_T("postprocess")), _("Post-Process"), PostProcessMenuCallback);
		heeksCAD->AddFlyoutButton(_("Make Python Script"), ToolImage(_T("python")), _("Make Python Script"), MakeScriptMenuCallback);
		heeksCAD->AddFlyoutButton(_("Run Python Script"), ToolImage(_T("runpython")), _("Run Python Script"), RunScriptMenuCallback);
		heeksCAD->AddFlyoutButton(_("OpenNC"), ToolImage(_T("opennc")), _("Open NC File"), OpenNcFileMenuCallback);
		heeksCAD->AddFlyoutButton(_("SaveNC"), ToolImage(_T("savenc")), _("Save NC File"), SaveNcFileMenuCallback);
		heeksCAD->AddFlyoutButton(_("Send to Machine"), ToolImage(_T("tomachine")), _("Send to Machine"), SendToMachineMenuCallback);
		heeksCAD->AddFlyoutButton(_("Cancel"), ToolImage(_T("cancel")), _("Cancel Python Script"), CancelMenuCallback);
		heeksCAD->AddFlyoutButton(_("Simulate"), ToolImage(_T("simulate")), _("Simulate"), SimulateCallback);
		heeksCAD->EndToolBarFlyout((wxToolBar*)(theApp.m_machiningBar));

		theApp.m_machiningBar->Realize();
		aui_manager->AddPane(theApp.m_machiningBar, wxAuiPaneInfo().Name(_T("MachiningBar")).Caption(_T("Machining tools")).ToolbarPane().Top());
		heeksCAD->RegisterToolBar(theApp.m_machiningBar);
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


void CHeeksCNCApp::OnStartUp(CHeeksCADInterface* h, const wxString& dll_path)
{
	m_dll_path = dll_path;
	heeksCAD = h;
#if !defined WXUSINGDLL
	wxInitialize();
#endif

	CNCConfig config(ConfigScope());

	// About box, stuff
	heeksCAD->AddToAboutBox(wxString(_T("\n\n")) + _("HeeksCNC is the free machining add-on to HeeksCAD")
		+ _T("\n") + _T("          http://code.google.com/p/heekscnc/")
		+ _T("\n") + _("Written by Dan Heeks, Hirutso Enni, Perttu Ahola, David Nicholls")
		+ _T("\n") + _("With help from archivist, crotchet1, DanielFalck, fenn, Sliptonic")
		+ _T("\n\n") + _("geometry code, donated by Geoff Hawkesford, Camtek GmbH http://www.peps.de/")
		+ _T("\n") + _("pocketing code from http://code.google.com/p/libarea/ , derived from the kbool library written by Klaas Holwerda http://boolean.klaasholwerda.nl/bool.html")
		+ _T("\n") + _("Zig zag code from opencamlib http://code.google.com/p/opencamlib/")
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

	// Milling Operations menu
	wxMenu *menuMillingOperations = new wxMenu;
	heeksCAD->AddMenuItem(menuMillingOperations, _("Profile Operation..."), ToolImage(_T("opprofile")), NewProfileOpMenuCallback);
	heeksCAD->AddMenuItem(menuMillingOperations, _("Pocket Operation..."), ToolImage(_T("pocket")), NewPocketOpMenuCallback);
	heeksCAD->AddMenuItem(menuMillingOperations, _("Drilling Operation..."), ToolImage(_T("drilling")), NewDrillingOpMenuCallback);
	heeksCAD->AddMenuItem(menuMillingOperations, _("Tapping Operation..."), ToolImage(_T("optap")), NewTappingOpMenuCallback);

	wxMenu *menu3dMillingOperations = new wxMenu;
	heeksCAD->AddMenuItem(menu3dMillingOperations, _("Attach Operation..."), ToolImage(_T("attach")), NewAttachOpMenuCallback);
	heeksCAD->AddMenuItem(menu3dMillingOperations, _("Unattach Operation..."), ToolImage(_T("unattach")), NewUnattachOpMenuCallback);

	// Additive Operations menu
	wxMenu *menuOperations = new wxMenu;
	heeksCAD->AddMenuItem(menuOperations, _("Script Operation..."), ToolImage(_T("scriptop")), NewScriptOpMenuCallback);
	heeksCAD->AddMenuItem(menuOperations, _("Design Rules Check..."), ToolImage(_T("design_rules_check")), DesignRulesCheckMenuCallback);
	heeksCAD->AddMenuItem(menuOperations, _("Design Rules Adjustment..."), ToolImage(_T("design_rules_adjustment")), DesignRulesAdjustmentMenuCallback);
	heeksCAD->AddMenuItem(menuOperations, _("Speed Reference..."), ToolImage(_T("speed_reference")), NewSpeedReferenceMenuCallback);
	heeksCAD->AddMenuItem(menuOperations, _("Cutting Rate Reference..."), ToolImage(_T("cutting_rate")), NewCuttingRateMenuCallback);
	heeksCAD->AddMenuItem(menuOperations, _("Positioning Operation..."), ToolImage(_T("locating")), NewPositioningOpMenuCallback);
	heeksCAD->AddMenuItem(menuOperations, _("Probe Centre Operation..."), ToolImage(_T("probe")), NewProbe_Centre_MenuCallback);
	heeksCAD->AddMenuItem(menuOperations, _("Probe Edge Operation..."), ToolImage(_T("probe")), NewProbe_Edge_MenuCallback);
	heeksCAD->AddMenuItem(menuOperations, _("Probe Grid Operation..."), ToolImage(_T("probe")), NewProbe_Grid_MenuCallback);

    // Tapping tools menu
	wxMenu *menuTappingTools = new wxMenu;
	heeksCAD->AddMenuItem(menuTappingTools, _("Any sized tap..."), ToolImage(_T("tap")), NewTapToolMenuCallback);
	heeksCAD->AddMenuItem(menuTappingTools, _("Pick from Metric standard sizes"), ToolImage(_T("tap")), NewMetricTappingToolMenuCallback);
	heeksCAD->AddMenuItem(menuTappingTools, _("Pick from Unified Threading Standard (UNC, UNF or UNEF)"), ToolImage(_T("tap")), NewUnifiedThreadingStandardTappingToolMenuCallback);
	heeksCAD->AddMenuItem(menuTappingTools, _("Pick from British Standard Whitworth standard sizes"), ToolImage(_T("tap")), NewBritishStandardWhitworthTappingToolMenuCallback);

	// Tools menu
	wxMenu *menuTools = new wxMenu;
	heeksCAD->AddMenuItem(menuTools, _("Drill..."), ToolImage(_T("drill")), NewDrillMenuCallback);
	heeksCAD->AddMenuItem(menuTools, _("Centre Drill..."), ToolImage(_T("centredrill")), NewCentreDrillMenuCallback);
	heeksCAD->AddMenuItem(menuTools, _("End Mill..."), ToolImage(_T("endmill")), NewEndmillMenuCallback);
	heeksCAD->AddMenuItem(menuTools, _("Slot Drill..."), ToolImage(_T("slotdrill")), NewSlotCutterMenuCallback);
	heeksCAD->AddMenuItem(menuTools, _("Ball End Mill..."), ToolImage(_T("ballmill")), NewBallEndMillMenuCallback);
	heeksCAD->AddMenuItem(menuTools, _("Chamfer Mill..."), ToolImage(_T("chamfmill")), NewChamferMenuCallback);
	heeksCAD->AddMenuItem(menuTools, _("Touch Probe..."), ToolImage(_T("probe")), NewTouchProbeMenuCallback);
	heeksCAD->AddMenuItem(menuTools, _("Tool Length Switch..."), ToolImage(_T("probe")), NewToolLengthSwitchMenuCallback);
	heeksCAD->AddMenuItem(menuTools, _("Tap Tool..."), ToolImage(_T("tap")), NULL, NULL, menuTappingTools);

	// Machining menu
	wxMenu *menuMachining = new wxMenu;
	heeksCAD->AddMenuItem(menuMachining, _("Add New Milling Operation"), ToolImage(_T("ops")), NULL, NULL, menuMillingOperations);
	heeksCAD->AddMenuItem(menuMachining, _("Add New 3D Operation"), ToolImage(_T("ops")), NULL, NULL, menu3dMillingOperations);
	heeksCAD->AddMenuItem(menuMachining, _("Add Other Operation"), ToolImage(_T("ops")), NULL, NULL, menuOperations);
	heeksCAD->AddMenuItem(menuMachining, _("Add New Tool"), ToolImage(_T("tools")), NULL, NULL, menuTools);
	heeksCAD->AddMenuItem(menuMachining, _("Make Python Script"), ToolImage(_T("python")), MakeScriptMenuCallback);
	heeksCAD->AddMenuItem(menuMachining, _("Run Python Script"), ToolImage(_T("runpython")), RunScriptMenuCallback);
	heeksCAD->AddMenuItem(menuMachining, _("Post-Process"), ToolImage(_T("postprocess")), PostProcessMenuCallback);
	heeksCAD->AddMenuItem(menuMachining, _("Simulate"), ToolImage(_T("simulate")), SimulateCallback);
	heeksCAD->AddMenuItem(menuMachining, _("Open NC File"), ToolImage(_T("opennc")), OpenNcFileMenuCallback);
	heeksCAD->AddMenuItem(menuMachining, _("Save NC File"), ToolImage(_T("savenc")), SaveNcFileMenuCallback);
	heeksCAD->AddMenuItem(menuMachining, _("Send to Machine"), ToolImage(_T("tomachine")), SendToMachineMenuCallback);
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

	// add tick boxes for them all on the view menu
	wxMenu* window_menu = heeksCAD->GetWindowMenu();
	heeksCAD->AddMenuItem(window_menu, _T("Program"), wxBitmap(), OnProgramCanvas, OnUpdateProgramCanvas, NULL, true);
	heeksCAD->AddMenuItem(window_menu, _T("Output"), wxBitmap(), OnOutputCanvas, OnUpdateOutputCanvas, NULL, true);
	heeksCAD->AddMenuItem(window_menu, _T("Machining"), wxBitmap(), OnMachiningBar, OnUpdateMachiningBar, NULL, true);
	heeksCAD->RegisterHideableWindow(m_program_canvas);
	heeksCAD->RegisterHideableWindow(m_output_canvas);
	heeksCAD->RegisterHideableWindow(m_machiningBar);

	// add object reading functions
	heeksCAD->RegisterReadXMLfunction("Program", CProgram::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("nccode", CNCCode::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("Operations", COperations::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("Tools", CTools::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("Profile", CProfile::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("Pocket", CPocket::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("Drilling", CDrilling::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("Locating", CPositioning::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("Positioning", CPositioning::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("ProbeCentre", CProbe_Centre::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("ProbeEdge", CProbe_Edge::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("ProbeGrid", CProbe_Grid::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("Tool", CTool::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("CuttingTool", CTool::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("Tapping", CTapping::ReadFromXMLElement);

	heeksCAD->RegisterReadXMLfunction("SpeedReferences", CSpeedReferences::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("SpeedReference", CSpeedReference::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("CuttingRate", CCuttingRate::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("Tags", CTags::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("Tag", CTag::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("ScriptOp", CScriptOp::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("AttachOp", CAttachOp::ReadFromXMLElement);
	heeksCAD->RegisterReadXMLfunction("UnattachOp", CUnattachOp::ReadFromXMLElement);

#ifdef WIN32
	heeksCAD->SetDefaultLayout(wxString(_T("layout2|name=ToolBar;caption=General Tools;state=2108156;dir=1;layer=10;row=0;pos=0;prop=100000;bestw=279;besth=31;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=GeomBar;caption=Geometry Tools;state=2108156;dir=1;layer=10;row=0;pos=695;prop=100000;bestw=145;besth=31;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=384;floaty=355;floatw=172;floath=71|name=SolidBar;caption=Solid Tools;state=2108156;dir=1;layer=10;row=0;pos=851;prop=100000;bestw=116;besth=31;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=444;floaty=368;floatw=143;floath=71|name=ViewingBar;caption=Viewing Tools;state=2108156;dir=1;layer=10;row=0;pos=566;prop=100000;bestw=118;besth=31;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=480;floaty=399;floatw=145;floath=71|name=Graphics;caption=Graphics;state=768;dir=5;layer=0;row=0;pos=0;prop=100000;bestw=800;besth=600;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=Objects;caption=Objects;state=2099196;dir=4;layer=1;row=0;pos=0;prop=100000;bestw=300;besth=400;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=204;floaty=327;floatw=318;floath=440|name=Options;caption=Options;state=2099196;dir=4;layer=1;row=0;pos=1;prop=100000;bestw=300;besth=200;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=Input;caption=Input;state=2099196;dir=4;layer=1;row=0;pos=2;prop=100000;bestw=300;besth=200;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=Properties;caption=Properties;state=2099196;dir=4;layer=1;row=0;pos=3;prop=100000;bestw=300;besth=200;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=MachiningBar;caption=Machining tools;state=2108156;dir=1;layer=10;row=0;pos=290;prop=100000;bestw=265;besth=31;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=Program;caption=Program;state=2099198;dir=3;layer=0;row=0;pos=0;prop=100000;bestw=600;besth=200;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=Output;caption=Output;state=2099196;dir=3;layer=0;row=0;pos=0;prop=100000;bestw=600;besth=200;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|dock_size(5,0,0)=504|dock_size(4,1,0)=205|dock_size(1,10,0)=33|dock_size(3,0,0)=219|")));
#else
	heeksCAD->SetDefaultLayout(wxString(_T("layout2|name=ToolBar;caption=General Tools;state=2108156;dir=1;layer=10;row=0;pos=0;prop=100000;bestw=328;besth=40;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=469;floaty=243;floatw=345;floath=64|name=GeomBar;caption=Geometry Tools;state=2108156;dir=1;layer=10;row=0;pos=339;prop=100000;bestw=174;besth=38;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=319;floaty=311;floatw=191;floath=62|name=SolidBar;caption=Solid Tools;state=2108156;dir=1;layer=10;row=0;pos=638;prop=100000;bestw=140;besth=38;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=373;floaty=261;floatw=157;floath=62|name=ViewingBar;caption=Viewing Tools;state=2108156;dir=1;layer=10;row=0;pos=524;prop=100000;bestw=140;besth=40;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=334;floaty=257;floatw=119;floath=64|name=Graphics;caption=Graphics;state=768;dir=5;layer=0;row=0;pos=0;prop=100000;bestw=800;besth=600;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=Objects;caption=Objects;state=2099196;dir=4;layer=1;row=0;pos=0;prop=100000;bestw=300;besth=400;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=204;floaty=327;floatw=318;floath=440|name=Options;caption=Options;state=2099196;dir=4;layer=1;row=0;pos=1;prop=100000;bestw=300;besth=200;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=Input;caption=Input;state=2099196;dir=4;layer=1;row=0;pos=2;prop=100000;bestw=300;besth=200;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=Properties;caption=Properties;state=2099196;dir=4;layer=1;row=0;pos=3;prop=100000;bestw=300;besth=200;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=MachiningBar;caption=Machining tools;state=2108156;dir=1;layer=10;row=0;pos=791;prop=100000;bestw=178;besth=40;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=357;floaty=413;floatw=195;floath=64|name=Program;caption=Program;state=2099196;dir=3;layer=0;row=0;pos=0;prop=100000;bestw=600;besth=200;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=Output;caption=Output;state=2099196;dir=3;layer=0;row=0;pos=1;prop=100000;bestw=600;besth=200;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|dock_size(5,0,0)=504|dock_size(4,1,0)=334|dock_size(3,0,0)=110|dock_size(1,10,0)=42|")));
#endif


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
	{
        std::list<wxString> file_extensions;
        file_extensions.push_back(_T("speed"));
        file_extensions.push_back(_T("speeds"));
        file_extensions.push_back(_T("feed"));
        file_extensions.push_back(_T("feeds"));
        file_extensions.push_back(_T("feedsnspeeds"));
        if (! heeksCAD->RegisterFileOpenHandler( file_extensions, ImportSpeedReferencesFile ))
        {
            printf("Failed to register handler for Speed References files\n");
        }
	}

	heeksCAD->RegisterUnitsChangeHandler( UnitsChangedHandler );
	heeksCAD->RegisterHeeksTypesConverter( HeeksCNCType );
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
		heeksCAD->Changed();

		std::list<wxString> directories;
		wxString directory_separator;

		#ifdef WIN32
			directory_separator = _T("\\");
		#else
			directory_separator = _T("/");
		#endif

		wxStandardPaths standard_paths;
		directories.push_back( standard_paths.GetUserConfigDir() );	// Look for a user-specific file first
		directories.push_back( GetDllFolder() );	// And then look in the application-delivered directory
#ifdef CMAKE_UNIX
	#ifdef RUNINPLACE
		directories.push_back( GetResFolder() );
	#else
		directories.push_back( _T("/usr/lib/heekscnc") ); //Location if installed by CMAKE
	#endif
#endif //CMAKE_UNIX
		bool tool_table_found = false;
		bool speed_references_found = false;
		bool fixtures_found = false;

		for (std::list<wxString>::iterator l_itDirectory = directories.begin();
			l_itDirectory != directories.end(); l_itDirectory++)
		{
 			printf("Looking for default data in '%s'\n", Ttc(l_itDirectory->c_str()));

			// Must be a new file.
			// Read in any default speed reference or tool table data.
			std::list<wxString> all_file_names = GetFileNames( l_itDirectory->utf8_str() );
			std::list<wxString> seed_file_names;

			for (std::list<wxString>::iterator l_itFileName = all_file_names.begin();
					l_itFileName != all_file_names.end(); l_itFileName++)
			{
				if (l_itFileName->Find( _("default") ) != -1)
				{
					wxString path;
					path << *l_itDirectory << directory_separator << *l_itFileName;

					seed_file_names.push_back(path);
				} // End if - then
			} // End for

			seed_file_names.sort();	// Sort them so that the user can assign an order alphabetically if they wish.
			for (std::list<wxString>::const_iterator l_itFile = seed_file_names.begin(); l_itFile != seed_file_names.end(); l_itFile++)
			{
				wxString lowercase_file_name( *l_itFile );
				lowercase_file_name.MakeLower();

				if ((speed_references_found == false) && (lowercase_file_name.Find(_T("speed")) != -1))
				{
					printf("Importing data from %s\n",  Ttc(l_itFile->c_str()));
					heeksCAD->OpenXMLFile( l_itFile->c_str(), theApp.m_program->SpeedReferences() );
					heeksCAD->Changed();
					speed_references_found = true;
				} // End if - then
				else if ((speed_references_found == false) && (lowercase_file_name.Find(_T("feed")) != -1))
				{
					printf("Importing data from %s\n",  Ttc(l_itFile->c_str()));
					heeksCAD->OpenXMLFile( l_itFile->c_str(), theApp.m_program->SpeedReferences() );
					heeksCAD->Changed();
					speed_references_found = true;
				} // End if - then
				else if ((tool_table_found == false) && (lowercase_file_name.Find(_T("tool")) != -1))
				{
					printf("Importing data from %s\n",  Ttc(l_itFile->c_str()));
					heeksCAD->OpenXMLFile( l_itFile->c_str(), theApp.m_program->Tools() );
					heeksCAD->Changed();
					tool_table_found = true;
				}
			} // End for
		} // End for
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
	CNCConfig config(ConfigScope());
	config.Write(_T("ProgramVisible"), aui_manager->GetPane(m_program_canvas).IsShown());
	config.Write(_T("OutputVisible"), aui_manager->GetPane(m_output_canvas).IsShown());
	config.Write(_T("MachiningBarVisible"), aui_manager->GetPane(m_machiningBar).IsShown());

	CNCCode::WriteColorsToConfig();
	CProfile::WriteToConfig();
	CPocket::WriteToConfig();
	CSpeedOp::WriteToConfig();
	CSendToMachine::WriteToConfig();
	config.Write(_T("UseClipperNotBoolean"), m_use_Clipper_not_Boolean);
    config.Write(_T("UseDOSNotUnix"), m_use_DOS_not_Unix);
}

wxString CHeeksCNCApp::GetDllFolder()
{
	return m_dll_path;
}

wxString CHeeksCNCApp::GetResFolder()
{wxStandardPaths sp;
#if defined(WIN32) || defined(RUNINPLACE) //compile with 'RUNINPLACE=yes make' then skip 'sudo make install'
	#ifdef CMAKE_UNIX
		return (m_dll_path + _T("/.."));
	#else
		return m_dll_path;
	#endif
#else
#ifdef CMAKE_UNIX
    return (_T("/usr/share/heekscnc"));
    //return sp.GetResourcesDir();
#else
	return (m_dll_path + _T("/../../share/heekscnc"));
	//return sp.GetResourcesDir();
#endif
#endif
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
	case ZigZagType:       return(_("ZigZag"));
	case AdaptiveType:       return(_("Adaptive"));
	case DrillingType:       return(_("Drilling"));
	case ToolType:       return(_("Tool"));
	case ToolsType:       return(_("Tools"));
	case CounterBoreType:       return(_("CounterBore"));
	case TurnRoughType:       return(_("TurnRough"));
	case FixtureType:       return(_("Fixture"));
	case FixturesType:       return(_("Fixtures"));
	case SpeedReferenceType:       return(_("SpeedReference"));
	case SpeedReferencesType:       return(_("SpeedReferences"));
	case CuttingRateType:       return(_("CuttingRate"));
	case PositioningType:       return(_("Positioning"));
	case BOMType:       return(_("BOM"));
	case TrsfNCCodeType:      return(_("TrsfNCCode"));
	case ProbeCentreType:       return(_("ProbeCentre"));
	case ProbeEdgeType:       return(_("ProbeEdge"));
	case ContourType:       return(_("Contour"));
	case ChamferType:       return(_("Chamfer"));
	case InlayType:       return(_("Inlay"));
	case ProbeGridType:       return(_("ProbeGrid"));
	case TagsType:       return(_("Tags"));
	case TagType:       return(_("Tag"));
	case ScriptOpType:       return(_("ScriptOp"));
	case AttachOpType:       return(_("AttachOp"));
	case UnattachOpType:       return(_("UnattachOp"));
	case WaterlineType:       return(_("Waterline"));
	case RaftType:       return(_("Raft"));
	case TappingType:       return(_("Tapping"));

	default:
        return(_T("")); // Indicates that this function could not make the conversion.
    } // End switch
} // End HeeksCNCType() routine
