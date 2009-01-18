// ProgramCanvas.cpp

#include "stdafx.h"
#include <wx/config.h>
#include <wx/confbase.h>
#include <wx/fileconf.h>
#include "ProgramCanvas.h"
#include "Program.h"
#include "Profile.h"
#include "ZigZag.h"
#include "OutputCanvas.h"
#include "../../interface/Tool.h"
#include "../../interface/InputMode.h"
#include "../../interface/LeftAndRight.h"
#include "../../interface/PropertyInt.h"
#include "../../interface/PropertyDouble.h"
#include "../../interface/PropertyChoice.h"
#include "PythonStuff.h"

BEGIN_EVENT_TABLE(CProgramCanvas, wxScrolledWindow)
    EVT_SIZE(CProgramCanvas::OnSize)
END_EVENT_TABLE()

//	if(heeksCAD->PickPosition(_T("Pick finish position"), pos)){

static void OnProfile(wxCommandEvent& event)
{
	// check for at least one sketch selected
	std::list<int> sketches;

	const std::list<HeeksObj*>& list = heeksCAD->GetMarkedList();
	for(std::list<HeeksObj*>::const_iterator It = list.begin(); It != list.end(); It++)
	{
		HeeksObj* object = *It;
		if(object->GetType() == SketchType)sketches.push_back(object->m_id);
	}

	// if no selected sketches, 
	if(sketches.size() == 0)
	{
		// use all the sketches in the drawing
		for(HeeksObj* object = heeksCAD->GetFirstObject();object; object = heeksCAD->GetNextObject())
		{
			if(object->GetType() == SketchType)sketches.push_back(object->m_id);
		}
	}

	if(sketches.size() == 0)
	{
		wxMessageBox(_("There are no sketches!"));
		return;
	}

	CProfile *new_object = new CProfile(sketches);
	heeksCAD->AddUndoably(new_object, theApp.m_program->m_operations);
	heeksCAD->ClearMarkedList();
	heeksCAD->Mark(new_object);
}

static void OnZigZag(wxCommandEvent& event)
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
		// use all the sketches in the drawing
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

static void OnPython(wxCommandEvent& event)
{
	// create the Python program
	theApp.m_program->RewritePythonProgram();
}

static void OnPostProcess(wxCommandEvent& event)
{
	bool pp_success = HeeksPyPostProcess();
	if(pp_success)HeeksPyBackplot(theApp.m_program->m_output_file);
}

CProgramCanvas::CProgramCanvas(wxWindow* parent)
        : wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                           wxHSCROLL | wxVSCROLL | wxNO_FULL_REPAINT_ON_RESIZE)
{
	m_textCtrl = new wxTextCtrl( this, 100, _T(""),	wxPoint(180,170), wxSize(200,70), wxTE_MULTILINE | wxTE_DONTWRAP);

	// make a tool bar
	m_toolBar = new wxToolBar(this, -1, wxDefaultPosition, wxDefaultSize, wxTB_NODIVIDER | wxTB_FLAT);
	m_toolBar->SetToolBitmapSize(wxSize(32, 32));

	// add toolbar buttons
	heeksCAD->AddToolBarButton(m_toolBar, _T("Profile"), wxBitmap(theApp.GetDllFolder() + _T("/bitmaps/opprofile.png"), wxBITMAP_TYPE_PNG), _T("Cut around selected sketches"), OnProfile);
	heeksCAD->AddToolBarButton(m_toolBar, _T("Python"), wxBitmap(theApp.GetDllFolder() + _T("/bitmaps/python.png"), wxBITMAP_TYPE_PNG), _T("Create Python program from the objects"), OnPython);
	heeksCAD->AddToolBarButton(m_toolBar, _T("Post Process"), wxBitmap(theApp.GetDllFolder() + _T("/bitmaps/postprocess.png"), wxBITMAP_TYPE_PNG), _T("Post process"), OnPostProcess);

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
