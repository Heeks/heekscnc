// 	HeeksCNCInterface.cpp

#include "stdafx.h"
#include "HeeksCNCInterface.h"
#include "Program.h"
#include "Operations.h"
#include "CTool.h"
#include "Tools.h"
#include "../../interface/HDialogs.h"
#include <wx/aui/aui.h>

CProgram* CHeeksCNCInterface::GetProgram()
{
	return theApp.m_program;
}

CTools* CHeeksCNCInterface::GetTools()
{
	if(theApp.m_program == NULL)return NULL;
	return theApp.m_program->Tools();
}

std::vector< std::pair< int, wxString > > CHeeksCNCInterface::FindAllTools()
{
	std::vector< std::pair< int, wxString > > tools;
	HTypeObjectDropDown::GetObjectArrayString(ToolType, theApp.m_program->Tools(), tools);
	return tools;
}

int CHeeksCNCInterface::FindFirstToolByType( unsigned int type )
{
	return CTool::FindFirstByType((CToolParams::eToolType)type);
}

COperations* CHeeksCNCInterface::GetOperations()
{
	if(theApp.m_program == NULL)return NULL;
	return theApp.m_program->Operations();
}

void CHeeksCNCInterface::RegisterOnRewritePython( void(*callbackfunc)() )
{
	theApp.m_OnRewritePython_list.push_back(callbackfunc);
}

void CHeeksCNCInterface::RegisterOperationType( int type )
{
	theApp.m_external_op_types.insert(type);
}

bool CHeeksCNCInterface::IsAnOperation( int type )
{
	return COperations::IsAnOperation(type);
}

wxString CHeeksCNCInterface::GetDllFolder()
{
	return theApp.GetDllFolder();
}

void CHeeksCNCInterface::SetMachinesFile( const wxString& filepath )
{
	CProgram::alternative_machines_file = filepath;
}

void CHeeksCNCInterface::HideMachiningMenu()
{
	wxAuiManager* aui_manager = heeksCAD->GetAuiManager();
	aui_manager->GetPane(theApp.m_machiningBar).Show(false);
	aui_manager->Update();
	aui_manager->DetachPane(theApp.m_machiningBar);
	heeksCAD->RemoveHideableWindow(theApp.m_machiningBar);
	wxMenu* window_menu = heeksCAD->GetWindowMenu();
	window_menu->Remove(window_menu->FindItem(_T("Machining")));
	theApp.m_machining_hidden = true;

	heeksCAD->RemoveToolBar(theApp.m_machiningBar);
	wxFrame* frame = heeksCAD->GetMainFrame();
	wxMenuBar* menu_bar = frame->GetMenuBar();
	int pos = menu_bar->FindMenu(_("Machining"));
	if(pos != wxNOT_FOUND)
		menu_bar->Remove(pos);
}

void CHeeksCNCInterface::SetProcessRedirect(bool redirect)
{
	CPyProcess::redirect = redirect;
}

void CHeeksCNCInterface::PostProcess()
{
	// write the python program
	theApp.m_program->RewritePythonProgram();

	// run it
	theApp.RunPythonScript();
}
