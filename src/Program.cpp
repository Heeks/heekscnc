// Program.cpp
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include "Program.h"
#include "PythonStuff.h"
#include "../../tinyxml/tinyxml.h"
#include "ProgramCanvas.h"
#include "NCCode.h"
#include "../../interface/MarkedObject.h"
#include "../../interface/PropertyString.h"
#include "../../interface/PropertyFile.h"
#include "../../interface/PropertyChoice.h"
#include "../../interface/PropertyDouble.h"
#include "../../interface/PropertyLength.h"
#include "../../interface/PropertyCheck.h"
#include "../../interface/Tool.h"
#include "Profile.h"
#include "Pocket.h"
#include "Drilling.h"
#include "CTool.h"
#include "Op.h"
#include "CNCConfig.h"
#include "SpeedOp.h"
#include "Operations.h"
#include "Tools.h"
#include "Patterns.h"
#include "Surfaces.h"
#include "Stocks.h"
#include "../../interface/strconv.h"
#include "Pattern.h"
#include "Surface.h"
#include "Stock.h"
#include "../../src/Geom.h"
#include "ProgramDlg.h"

#include <wx/stdpaths.h>
#include <wx/filename.h>

#include <vector>
#include <algorithm>
#include <fstream>
#include <memory>
using namespace std;

wxString CProgram::alternative_machines_file = _T("");

CProgram::CProgram():m_nc_code(NULL), m_operations(NULL), m_tools(NULL), m_patterns(NULL), m_surfaces(NULL), m_stocks(NULL)
, m_script_edited(false)
{
	ReadDefaultValues();
}

CProgram::CProgram( const CProgram & rhs ) : IdNamedObjList(rhs)
{
    m_nc_code = NULL;
    m_operations = NULL;
    m_tools = NULL;
	m_patterns = NULL;
	m_surfaces = NULL;
	m_stocks = NULL;
    m_script_edited = false;
    m_machine = rhs.m_machine;
    m_output_file = rhs.m_output_file;
    m_output_file_name_follows_data_file_name = rhs.m_output_file_name_follows_data_file_name;

    m_script_edited = rhs.m_script_edited;
    m_units = rhs.m_units;

	m_path_control_mode = rhs.m_path_control_mode;
	m_motion_blending_tolerance = rhs.m_motion_blending_tolerance;
	m_naive_cam_tolerance = rhs.m_naive_cam_tolerance;

    ReloadPointers();
    AddMissingChildren();

    if ((m_nc_code != NULL) && (rhs.m_nc_code != NULL)) *m_nc_code = *(rhs.m_nc_code);
    if ((m_operations != NULL) && (rhs.m_operations != NULL)) *m_operations = *(rhs.m_operations);
    if ((m_tools != NULL) && (rhs.m_tools != NULL)) *m_tools = *(rhs.m_tools);
    if ((m_patterns != NULL) && (rhs.m_patterns != NULL)) *m_patterns = *(rhs.m_patterns);
    if ((m_surfaces != NULL) && (rhs.m_surfaces != NULL)) *m_surfaces = *(rhs.m_surfaces);
    if ((m_stocks != NULL) && (rhs.m_stocks != NULL)) *m_stocks = *(rhs.m_stocks);
}

CProgram::~CProgram()
{
	// Only remove the global pointer if 'we are the one'.  When a file is imported, extra
	// CProgram objects exist temporarily.  They're not all used as the master data pointer.
	if (theApp.m_program == this)
	{
		theApp.m_program = NULL;
	}
}

const wxBitmap &CProgram::GetIcon()
{
	static wxBitmap* icon = NULL;
	if(icon == NULL)icon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/program.png")));
	return *icon;
}

void CProgram::glCommands(bool select, bool marked, bool no_color)
{
    if(m_nc_code != NULL)
	{
		if(select)glPushName(m_nc_code->GetIndex());
		m_nc_code->glCommands(select, marked, no_color);
		if(select)glPopName();
	}
	if(!select)
	{
		if(m_tools != NULL)m_tools->glCommands(select, marked, no_color);
		if(m_operations != NULL)m_operations->glCommands(select, false, no_color);
	}
}

HeeksObj *CProgram::MakeACopy(void)const
{
	return new CProgram(*this);
}

/**
	This is ALMOST the same as the assignment operator.  The difference is that
	this, and its subordinate methods, augment themselves with the contents
	of the object passed in rather than replacing their own copies.
 */
void CProgram::CopyFrom(const HeeksObj* object)
{
	if (object->GetType() == GetType())
	{
		CProgram *rhs = (CProgram *) object;
		// IdNamedObjList::operator=(*rhs);	// I don't think this will do anything in this case. But it might one day.

		if ((m_nc_code != NULL) && (rhs->m_nc_code != NULL)) m_nc_code->CopyFrom( rhs->m_nc_code );
		if ((m_operations != NULL) && (rhs->m_operations != NULL)) m_operations->CopyFrom( rhs->m_operations );
		if ((m_tools != NULL) && (rhs->m_tools != NULL)) m_tools->CopyFrom( rhs->m_tools );
		if ((m_patterns != NULL) && (rhs->m_patterns != NULL)) m_patterns->CopyFrom( rhs->m_patterns );
		if ((m_surfaces != NULL) && (rhs->m_surfaces != NULL)) m_surfaces->CopyFrom( rhs->m_surfaces );
		if ((m_stocks != NULL) && (rhs->m_stocks != NULL)) m_stocks->CopyFrom( rhs->m_stocks );

		m_machine = rhs->m_machine;
		m_output_file = rhs->m_output_file;
		m_output_file_name_follows_data_file_name = rhs->m_output_file_name_follows_data_file_name;

		m_script_edited = rhs->m_script_edited;
		m_units = rhs->m_units;

		m_path_control_mode = rhs->m_path_control_mode;
		m_motion_blending_tolerance = rhs->m_motion_blending_tolerance;
		m_naive_cam_tolerance = rhs->m_naive_cam_tolerance;
	}
}

void CProgram::Clear()
{
	if (m_nc_code != NULL)m_nc_code->Clear();
	if (m_operations != NULL)m_operations->Clear();
	if (m_tools != NULL)m_tools->Clear();
	if (m_patterns != NULL)m_patterns->Clear();
	if (m_surfaces != NULL)m_surfaces->Clear();
	if (m_stocks != NULL)m_stocks->Clear();
}

CProgram & CProgram::operator= ( const CProgram & rhs )
{
	if (this != &rhs)
	{
		IdNamedObjList::operator=(rhs);
		ReloadPointers();

		if ((m_nc_code != NULL) && (rhs.m_nc_code != NULL)) *m_nc_code = *(rhs.m_nc_code);
		if ((m_operations != NULL) && (rhs.m_operations != NULL)) *m_operations = *(rhs.m_operations);
		if ((m_tools != NULL) && (rhs.m_tools != NULL)) *m_tools = *(rhs.m_tools);
		if ((m_patterns != NULL) && (rhs.m_patterns != NULL)) *m_patterns = *(rhs.m_patterns);
		if ((m_surfaces != NULL) && (rhs.m_surfaces != NULL)) *m_surfaces = *(rhs.m_surfaces);
		if ((m_stocks != NULL) && (rhs.m_stocks != NULL)) *m_stocks = *(rhs.m_stocks);

		m_machine = rhs.m_machine;
		m_output_file = rhs.m_output_file;
		m_output_file_name_follows_data_file_name = rhs.m_output_file_name_follows_data_file_name;

		m_script_edited = rhs.m_script_edited;
		m_units = rhs.m_units;

		m_path_control_mode = rhs.m_path_control_mode;
		m_motion_blending_tolerance = rhs.m_motion_blending_tolerance;
		m_naive_cam_tolerance = rhs.m_naive_cam_tolerance;
	}

	return(*this);
}


CMachine::CMachine()
{
}

CMachine::CMachine( const CMachine & rhs )
{
	*this = rhs;	// call the assignment operator
}


CMachine & CMachine::operator= ( const CMachine & rhs )
{
	if (this != &rhs)
	{
		post = rhs.post;
		reader = rhs.reader;
		suffix = rhs.suffix;
		description = rhs.description;
		py_params = rhs.py_params;
	} // End if - then

	return(*this);
} // End assignment operator.


static void on_set_machine(int value, HeeksObj* object, bool from_undo_redo)
{
	std::vector<CMachine> machines;
	CProgram::GetMachines(machines);
	((CProgram*)object)->m_machine = machines[value];
	((CProgram*)object)->WriteDefaultValues();
	heeksCAD->RefreshProperties();
}

static void on_set_output_file(const wxChar* value, HeeksObj* object)
{
	((CProgram*)object)->m_output_file = value;
	((CProgram*)object)->WriteDefaultValues();
}

static void on_set_units(int value, HeeksObj* object, bool from_undo_redo)
{
	((CProgram*)object)->m_units = ((value == 0) ? 1.0:25.4);

	object->WriteDefaultValues();

    if (heeksCAD->GetViewUnits() != ((CProgram*)object)->m_units)
    {
        int response;
        response = wxMessageBox( _("Would you like to change the HeeksCAD view units too?"), _("Change Units"), wxYES_NO );
        if (response == wxYES)
        {
            heeksCAD->SetViewUnits(((CProgram*)object)->m_units, true);
            heeksCAD->RefreshOptions();
        }
    }
    heeksCAD->RefreshProperties();
}

static void on_set_output_file_name_follows_data_file_name(int zero_based_choice, HeeksObj *object, bool from_undo_redo)
{
	CProgram *pProgram = (CProgram *) object;
	pProgram->m_output_file_name_follows_data_file_name = (zero_based_choice != 0);
	object->WriteDefaultValues();
	heeksCAD->RefreshProperties();
}

static void on_set_path_control_mode(int zero_based_choice, HeeksObj *object, bool from_undo_redo)
{
	CProgram *pProgram = (CProgram *) object;
	pProgram->m_path_control_mode = CProgram::ePathControlMode_t(zero_based_choice);
	object->WriteDefaultValues();
}

static void on_set_motion_blending_tolerance(double value, HeeksObj *object)
{
	CProgram *pProgram = (CProgram *) object;
	pProgram->m_motion_blending_tolerance = value;
	object->WriteDefaultValues();

}

static void on_set_naive_cam_tolerance(double value, HeeksObj *object)
{
	CProgram *pProgram = (CProgram *) object;
	pProgram->m_naive_cam_tolerance = value;
	object->WriteDefaultValues();
}


void CProgram::GetProperties(std::list<Property *> *list)
{
	{
		std::vector<CMachine> machines;
		GetMachines(machines);

		std::list< wxString > choices;
		int choice = 0;
		for(unsigned int i = 0; i < machines.size(); i++)
		{
			CMachine& machine = machines[i];
			choices.push_back(machine.description);
			if(machine.description == m_machine.description)choice = i;
		}
		list->push_back ( new PropertyChoice ( _("machine"),  choices, choice, this, on_set_machine ) );
	}

	{
		std::list<wxString> choices;
		int choice = int(m_output_file_name_follows_data_file_name?1:0);
		choices.push_back(_T("False"));
		choices.push_back(_T("True"));

		list->push_back(new PropertyChoice(_("output file name follows data file name"), choices, choice, this, on_set_output_file_name_follows_data_file_name));
	}

	if (m_output_file_name_follows_data_file_name == false)
	{
		list->push_back(new PropertyFile(_("output file"), m_output_file, this, on_set_output_file));
	} // End if - then

	{
		std::list< wxString > choices;
		choices.push_back ( wxString ( _("mm") ) );
		choices.push_back ( wxString ( _("inch") ) );
		int choice = 0;
		if(m_units > 25.0)choice = 1;
		list->push_back ( new PropertyChoice ( _("units for nc output"),  choices, choice, this, on_set_units ) );
	}

	m_machine.GetProperties(this, list);

	{
		std::list< wxString > choices;
		choices.push_back(_("Exact Path Mode"));
		choices.push_back(_("Exact Stop Mode"));
		choices.push_back(_("Best Possible Speed"));
		choices.push_back(_("Undefined"));

		list->push_back ( new PropertyChoice ( _("Path Control Mode"),  choices, (int) m_path_control_mode, this, on_set_path_control_mode ) );

		if (m_path_control_mode == eBestPossibleSpeed)
		{
			list->push_back( new PropertyLength( _("Motion Blending Tolerance"), m_motion_blending_tolerance, this, on_set_motion_blending_tolerance ) );
			list->push_back( new PropertyLength( _("Naive CAM Tolerance"), m_naive_cam_tolerance, this, on_set_naive_cam_tolerance ) );
		} // End if - then
	}

	IdNamedObjList::GetProperties(list);
}

void CMachine::GetProperties(CProgram *parent, std::list<Property *> *list)
{
} // End GetProperties() method



bool CProgram::CanAdd(HeeksObj* object)
{
    if (object == NULL) return(false);

	return object->GetType() == NCCodeType ||
		object->GetType() == OperationsType ||
		object->GetType() == ToolsType ||
		object->GetType() == PatternsType ||
		object->GetType() == SurfacesType ||
		object->GetType() == StocksType
		;
}

bool CProgram::CanAddTo(HeeksObj* owner)
{
	return ((owner != NULL) && (owner->GetType() == DocumentType));
}

void CProgram::SetClickMarkPoint(MarkedObject* marked_object, const double* ray_start, const double* ray_direction)
{
	if(marked_object->m_map.size() > 0)
	{
		HeeksObj* object = marked_object->m_map.begin()->first;
		if(object)
		{
			switch(object->GetType())
			{
			case NCCodeType:
				((CNCCode*)object)->SetClickMarkPoint(marked_object, ray_start, ray_direction);
				break;
			case NCCodeBlockType:
				((CNCCodeBlock*)object)->SetClickMarkPoint(marked_object, ray_start, ray_direction);
				break;
			}
		}
	}
}

void CProgram::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element;
	element = heeksCAD->NewXMLElement( "Program" );
	heeksCAD->LinkXMLEndChild( root,  element );
	element->SetAttribute( "machine", m_machine.description.utf8_str());
	element->SetAttribute( "output_file", m_output_file.utf8_str());
	element->SetAttribute( "output_file_name_follows_data_file_name", (int) (m_output_file_name_follows_data_file_name?1:0));

	element->SetAttribute( "program", theApp.m_program_canvas->m_textCtrl->GetValue().utf8_str());
	element->SetDoubleAttribute( "units", m_units);

	element->SetAttribute( "ProgramPathControlMode", int(m_path_control_mode));
	element->SetDoubleAttribute( "ProgramMotionBlendingTolerance", m_motion_blending_tolerance);
	element->SetDoubleAttribute( "ProgramNaiveCamTolerance", m_naive_cam_tolerance);

	m_machine.WriteBaseXML(element);
	WriteBaseXML(element);
}

bool CProgram::Add(HeeksObj* object, HeeksObj* prev_object)
{
	switch(object->GetType())
	{
	case NCCodeType:
		m_nc_code = (CNCCode*)object;
		break;

	case OperationsType:
		m_operations = (COperations*)object;
		break;

	case ToolsType:
		m_tools = (CTools*)object;
		break;

	case PatternsType:
		m_patterns = (CPatterns*)object;
		break;

	case SurfacesType:
		m_surfaces = (CSurfaces*)object;
		break;

	case StocksType:
		m_stocks = (CStocks*)object;
		break;
	}

	return IdNamedObjList::Add(object, prev_object);
}

void CProgram::Remove(HeeksObj* object)
{
	// This occurs when the HeeksCAD application performs a 'Reset()'.  This, in turn, deletes
	// the whole of the master data tree.  With this tree's destruction, we must ensure that
	// we delete ourselves cleanly so that this plugin doesn't end up with pointers to
	// deallocated memory.
	//
	// Since these pointes are also stored as children, the ObjList::~ObjList() destructor will
	// delete the children but our pointers to them won't get cleaned up.  That's what this
	// method is all about.

	if(object == m_nc_code)m_nc_code = NULL;
	else if(object == m_operations)m_operations = NULL;
	else if(object == m_tools)m_tools = NULL;
	else if(object == m_patterns)m_patterns = NULL;
	else if(object == m_surfaces)m_surfaces = NULL;
	else if(object == m_stocks)m_stocks = NULL;

	IdNamedObjList::Remove(object);
}

// static member function
HeeksObj* CProgram::ReadFromXMLElement(TiXmlElement* pElem)
{
	CProgram* new_object = new CProgram;
	if (theApp.m_program == NULL)
	{
	    theApp.m_program = new_object;
	}

	// get the attributes
	for(TiXmlAttribute* a = pElem->FirstAttribute(); a; a = a->Next())
	{
		std::string name(a->Name());
		if(name == "machine")new_object->m_machine = GetMachine(Ctt(a->Value()));
		else if(name == "output_file"){new_object->m_output_file.assign(Ctt(a->Value()));}
		else if(name == "output_file_name_follows_data_file_name"){new_object->m_output_file_name_follows_data_file_name = (atoi(a->Value()) != 0); }
		else if(name == "program"){theApp.m_program_canvas->m_textCtrl->SetValue(Ctt(a->Value()));}
		else if(name == "units"){new_object->m_units = a->DoubleValue();}
		else if(name == "ProgramPathControlMode"){new_object->m_path_control_mode = ePathControlMode_t(atoi(a->Value()));}
		else if(name == "ProgramMotionBlendingTolerance"){new_object->m_motion_blending_tolerance = a->DoubleValue();}
		else if(name == "ProgramNaiveCamTolerance"){new_object->m_naive_cam_tolerance = a->DoubleValue();}
	}

	new_object->ReadBaseXML(pElem);
	new_object->m_machine.ReadBaseXML(pElem);

	new_object->AddMissingChildren();

	return new_object;
}


void CMachine::WriteBaseXML(TiXmlElement *element)
{
} // End WriteBaseXML() method

void CMachine::ReadBaseXML(TiXmlElement* element)
{
} // End ReadBaseXML() method

void ApplyPatternToText(Python &python, int p, std::set<int> &patterns_written)
{
	CPattern* pattern = (CPattern*)heeksCAD->GetIDObject(PatternType, p);
	if(pattern)
	{
		// if pattern not already written
		if(patterns_written.find(p) == patterns_written.end())
		{
			// write a pattern definition
			python << _T("pattern") << p << _T(" = [");
			std::list<gp_Trsf> matrices;
			pattern->GetMatrices(matrices);
			for(std::list<gp_Trsf>::iterator It = matrices.begin(); It != matrices.end(); It++)
			{
				if(It != matrices.begin())python << _T(", ");
				gp_Trsf &mat = *It;
				python << _T("area.Matrix([");
				double m[16];
				extract(mat, m);
				for(int i = 0; i<16; i++)
				{
					if(i>0)python<<_T(", ");
					python<<m[i];
				}
				python << _T("])");
			}
			python<<_T("]\n");

			patterns_written.insert(p);
		}

		// write a transform redirector
		python << _T("transform.transform_begin(pattern") << p << _T(")\n");
	}
}

void ApplySurfaceToText(Python &python, CSurface* surface, std::set<CSurface*> &surfaces_written)
{
	if(surfaces_written.find(surface) == surfaces_written.end())
	{
		surfaces_written.insert(surface);
		// get the solids list
		std::list<HeeksObj*> solids;
		for (std::list<int>::iterator It = surface->m_solids.begin(); It != surface->m_solids.end(); It++)
		{
			HeeksObj* object = heeksCAD->GetIDObject(SolidType, *It);
			if (object != NULL)solids.push_back(object);
		} // End for

#if wxCHECK_VERSION(3, 0, 0)
		wxStandardPaths& standard_paths = wxStandardPaths::Get();
#else
		wxStandardPaths standard_paths;
#endif
		wxFileName filepath(standard_paths.GetTempDir().c_str(), wxString::Format(_T("surface%d.stl"), CSurface::number_for_stl_file).c_str());
		CSurface::number_for_stl_file++;

		//write stl file
		heeksCAD->SaveSTLFile(solids, filepath.GetFullPath(), 0.01);

		python << _T("stl") << (int)(surface->m_id) << _T(" = ocl_funcs.STLSurfFromFile(") << PythonString(filepath.GetFullPath()) << _T(")\n");
	}

	python << _T("attach.units = ") << theApp.m_program->m_units << _T("\n");
	python << _T("attach.attach_begin()\n");
	python << _T("nc.creator.stl = stl") << (int)(surface->m_id) << _T("\n");
	python << _T("nc.creator.minz = -10000.0\n");
	python << _T("nc.creator.material_allowance = ") << surface->m_material_allowance << _T("\n");

	theApp.m_attached_to_surface = surface;
}

Python CProgram::RewritePythonProgram()
{
	Python python;

	theApp.m_program_canvas->m_textCtrl->Clear();
	theApp.m_attached_to_surface = NULL;
	CSurface::number_for_stl_file = 1;
	theApp.m_tool_number = 0;

	// call any OnRewritePython functions from other plugins
	for(std::list< void(*)() >::iterator It = theApp.m_OnRewritePython_list.begin(); It != theApp.m_OnRewritePython_list.end(); It++)
	{
		void(*callbackfunc)() = *It;
		(*callbackfunc)();
	}

	bool kurve_funcs_needed = false;
	bool area_module_needed = true;  // area module could be used anywhere
	bool area_funcs_needed = false;
	bool ocl_module_needed = false;
	bool ocl_funcs_needed = false;
	bool nc_attach_needed = false;
	bool transform_module_needed = false;
	bool depths_needed = false;

	typedef std::vector< COp * > OperationsMap_t;
	OperationsMap_t operations;

	if (m_operations == NULL)
	{
		// If there are no operations then there is no GCode.
		// No socks, no shirt, no service.
		return(python);
	} // End if - then

	for(HeeksObj* object = m_operations->GetFirstChild(); object; object = m_operations->GetNextChild())
	{
		operations.push_back( (COp *) object );

		if(((COp*)object)->m_active)
		{
			if(((COp*)object)->m_pattern != 0)transform_module_needed = true;
			if(((COp*)object)->m_surface != 0){nc_attach_needed = true; ocl_module_needed = true; ocl_funcs_needed = true;}

			switch(object->GetType())
			{
			case ProfileType:
				kurve_funcs_needed = true;
				depths_needed = true;
				break;

			case PocketType:
				area_funcs_needed = true;
				depths_needed = true;
				break;

			case DrillingType:
				depths_needed = true;
				break;

			case ScriptOpType:
				ocl_module_needed = true;
				nc_attach_needed = true;
				ocl_funcs_needed = true;
				depths_needed = true;
				break;
			}
		}
	}

	// Language and Windows codepage detection and correction
	#ifndef WIN32
		python << _T("# coding=UTF8\n");
		python << _T("# No troubled Microsoft Windows detected\n");
	#else
		switch((wxLocale::GetSystemLanguage()))
		{
			case wxLANGUAGE_SLOVAK :
				python << _T("# coding=CP1250\n");
				python << _T("# Slovak language detected in Microsoft Windows\n");
				break;
			case wxLANGUAGE_GERMAN:
			case wxLANGUAGE_GERMAN_AUSTRIAN:
			case wxLANGUAGE_GERMAN_BELGIUM:
			case wxLANGUAGE_GERMAN_LIECHTENSTEIN:
			case wxLANGUAGE_GERMAN_LUXEMBOURG:
			case wxLANGUAGE_GERMAN_SWISS  :
				python << _T("# coding=CP1252\n");
				python << _T("# German language or it's variant detected in Microsoft Windows\n");
				break;
			case wxLANGUAGE_DUTCH  :
			case wxLANGUAGE_DUTCH_BELGIAN  :
				python << _T("# coding=CP1252\n");
				python << _T("# Dutch language or it's variant detected in Microsoft Windows\n");
				break;
			case wxLANGUAGE_FRENCH:
			case wxLANGUAGE_FRENCH_BELGIAN:
			case wxLANGUAGE_FRENCH_CANADIAN:
			case wxLANGUAGE_FRENCH_LUXEMBOURG:
			case wxLANGUAGE_FRENCH_MONACO:
			case wxLANGUAGE_FRENCH_SWISS:
				python << _T("# coding=CP1252\n");
				python << _T("# French language or it's variant detected in Microsoft Windows\n");
				break;
			case wxLANGUAGE_ITALIAN:
			case wxLANGUAGE_ITALIAN_SWISS :
				python << _T("# coding=CP1252\n");
				python << _T("#Italian language or it's variant detected in Microsoft Windows\n");
				break;
			case wxLANGUAGE_ENGLISH:
			case wxLANGUAGE_ENGLISH_UK:
			case wxLANGUAGE_ENGLISH_US:
			case wxLANGUAGE_ENGLISH_AUSTRALIA:
			case wxLANGUAGE_ENGLISH_BELIZE:
			case wxLANGUAGE_ENGLISH_BOTSWANA:
			case wxLANGUAGE_ENGLISH_CANADA:
			case wxLANGUAGE_ENGLISH_CARIBBEAN:
			case wxLANGUAGE_ENGLISH_DENMARK:
			case wxLANGUAGE_ENGLISH_EIRE:
			case wxLANGUAGE_ENGLISH_JAMAICA:
			case wxLANGUAGE_ENGLISH_NEW_ZEALAND:
			case wxLANGUAGE_ENGLISH_PHILIPPINES:
			case wxLANGUAGE_ENGLISH_SOUTH_AFRICA:
			case wxLANGUAGE_ENGLISH_TRINIDAD:
			case wxLANGUAGE_ENGLISH_ZIMBABWE:
				python << _T("# coding=CP1252\n");
				python << _T("#English language or it's variant detected in Microsoft Windows\n");
				break;
			default:
				python << _T("# coding=CP1252\n");
				python << _T("#Not supported language detected in Microsoft Windows. Assuming English alphabet\n");
				break;
		}
	#endif
	// add standard stuff at the top
	//hackhack, make it work on unix with FHS
	python << _T("import sys\n");

#ifdef CMAKE_UNIX
	#ifdef RUNINPLACE
	        python << _T("sys.path.insert(0,'") << theApp.GetResFolder() << _T("/')\n");
	#else
	        python << _T("sys.path.insert(0,'/usr/lib/heekscnc/')\n");
	#endif
#else
#ifndef WIN32
#ifndef RUNINPLACE
	python << _T("sys.path.insert(0,") << PythonString(_T("/usr/local/lib/heekscnc/")) << _T(")\n");
#endif
#endif
	python << _T("sys.path.insert(0,") << PythonString(theApp.GetResFolder()) << _T(")\n");
#endif
	python << _T("import math\n");

	// area related things
	if(area_module_needed)
	{
#ifdef WIN32
	python << _T("sys.path.insert(0,") << PythonString(theApp.GetResFolder() + (theApp.m_use_Clipper_not_Boolean ? _T("/Clipper"):_T("/Boolean"))) << _T(")\n");
#endif

		python << _T("import area\n");
		python << _T("area.set_units(") << m_units << _T(")\n");
	}

	// kurve related things
	if(kurve_funcs_needed)
	{
		python << _T("import kurve_funcs\n");
	}

	if(area_funcs_needed)
	{
		python << _T("import area_funcs\n");
	}

	// attach operations
	if(nc_attach_needed)
	{
		python << _T("import nc.attach as attach\n");
	}

	// OpenCamLib stuff
	if(ocl_module_needed)
	{
		python << _T("import ocl\n");
	}
	if(ocl_funcs_needed)
	{
		python << _T("import ocl_funcs\n");
	}

	if(transform_module_needed)
	{
		python << _T("import nc.transform as transform\n");
		python << _T("\n");
	}

	if(depths_needed)
	{
		python << _T("from depth_params import depth_params as depth_params\n");
		python << _T("\n");
	}

	// machine general stuff
	python << _T("from nc.nc import *\n");

	// specific machine
	if (m_machine.post == _T("not found"))
	{
		wxMessageBox(_T("Machine post processor name (defined in Program Properties) not found"));
	} // End if - then
	else

	{
		python << _T("from nc.") + m_machine.post + _T(" import *\n");
		python << _T("\n");
	} // End if - else

	// write the machine's parameters
	for(std::list<PyParam>::iterator It = m_machine.py_params.begin(); It != m_machine.py_params.end(); It++)
	{
		PyParam &p = *It;
		python << _T("nc.creator.") << Ctt(p.m_name.c_str()) << _T(" = ") << Ctt(p.m_value.c_str()) << _T("\n");
	}

	// output file
	python << _T("output(") << PythonString(GetOutputFileName()) << _T(")\n");


#ifdef FREE_VERSION
	python << _T("comment('MADE WITH FREE VERSION OF HEEKSCNC. Please buy full version to remove this text\\n')\n");
	python << _T("comment('***********    MADE WITH FREE VERSION OF HEEKSCNC!   ***********\\n')\n");
	python << _T("comment('***********    MADE WITH FREE VERSION OF HEEKSCNC!   ***********\\n')\n");
	python << _T("comment('***********    MADE WITH FREE VERSION OF HEEKSCNC!   ***********\\n')\n");
	python << _T("comment('***********    MADE WITH FREE VERSION OF HEEKSCNC!   ***********\\n')\n");
	python << _T("comment('***********    MADE WITH FREE VERSION OF HEEKSCNC!   ***********\\n')\n");
	python << _T("comment('***********    MADE WITH FREE VERSION OF HEEKSCNC!   ***********\\n')\n");
	python << _T("comment('***********                    ***********\\n')\n");
	python << _T("comment('***********    www.heeks.net   ***********\\n')\n");
	python << _T("comment('***********                    ***********\\n')\n");
#endif

	// begin program
	python << _T("program_begin(") << wxString::Format(_T("%d"), m_id) << _T(", ") << PythonString(GetShortString()) << _T(")\n");

	// add any stock commands
	std::set<int> stock_ids;
	m_stocks->GetSolidIds(stock_ids);
	for(std::set<int>::iterator It = stock_ids.begin(); It != stock_ids.end(); It++)
	{
		int id = *It;
		HeeksObj* object = heeksCAD->GetIDObject(SolidType, id);
		if(object)
		{
			CBox box;
			object->GetBox(box);
			python << _T("add_stock('BLOCK',[") << box.Width() << _T(", ") << box.Height() << _T(", ") << box.Depth() << _T(", ") << -box.MinX() << _T(", ") << -box.MinY() << _T(", ") << -box.MinZ() << _T("])\n");
		}
	}

	python << _T("absolute()\n");
	if(m_units > 25.0)
	{
		python << _T("imperial()\n");
	}
	else
	{
		python << _T("metric()\n");
	}
	python << _T("set_plane(0)\n");
	python << _T("\n");

	if (m_path_control_mode != ePathControlUndefined)
	{
		python << _T("set_path_control_mode(") << (int) m_path_control_mode << _T(",") << m_motion_blending_tolerance << _T(",") << m_naive_cam_tolerance << _T(")\n");
	}

	// write the tools setup code.
	if (m_tools != NULL)
	{
		// Write the new tool table entries first.
		for(HeeksObj* object = m_tools->GetFirstChild(); object; object = m_tools->GetNextChild())
		{
			switch(object->GetType())
			{
			case ToolType:
				python << ((CTool*)object)->AppendTextToProgram();
				break;
			}
		} // End for
	} // End if - then

	// Write all the operations

	std::set<CSurface*> surfaces_written;
	std::set<int> patterns_written;

	for (OperationsMap_t::const_iterator l_itOperation = operations.begin(); l_itOperation != operations.end(); l_itOperation++)
	{
		HeeksObj *object = (HeeksObj *) *l_itOperation;
		if (object == NULL) continue;

		if(COperations::IsAnOperation(object->GetType()))
		{
			COp* op = (COp*)object;
			if(op->m_active)
			{
				CSurface* surface = (CSurface*)heeksCAD->GetIDObject(SurfaceType, op->m_surface);
				if(surface && !surface->m_same_for_each_pattern_position)ApplySurfaceToText(python, surface, surfaces_written);
				ApplyPatternToText(python, op->m_pattern, patterns_written);
				if(surface && surface->m_same_for_each_pattern_position)ApplySurfaceToText(python, surface, surfaces_written);

				python << op->AppendTextToProgram();

				// end surface attach
				if(surface && surface->m_same_for_each_pattern_position)python << _T("attach.attach_end()\n");
				if(op->m_pattern != 0)python << _T("transform.transform_end()\n");
				if(surface && !surface->m_same_for_each_pattern_position)python << _T("attach.attach_end()\n");
			}
		}
	} // End for - operation

	python << _T("program_end()\n");
	m_python_program = python;
	theApp.m_program_canvas->m_textCtrl->AppendText(python);
	if (python.Length() > theApp.m_program_canvas->m_textCtrl->GetValue().Length())
	{
		// The python program is longer than the text control object can handle.  The maximum
		// length of the text control objects changes depending on the operating system (and its
		// implementation of wxWidgets).  Rather than showing the truncated program, tell the
		// user that it has been truncated and where to find it.

#if wxCHECK_VERSION(3, 0, 0)
		wxStandardPaths& standard_paths = wxStandardPaths::Get();
#else
		wxStandardPaths standard_paths;
#endif
		wxFileName file_str(standard_paths.GetTempDir().c_str(), _T("post.py"));

		theApp.m_program_canvas->m_textCtrl->Clear();
		theApp.m_program_canvas->m_textCtrl->AppendText(_("The Python program is too long \n"));
		theApp.m_program_canvas->m_textCtrl->AppendText(_("to display in this window.\n"));
		theApp.m_program_canvas->m_textCtrl->AppendText(_("Please edit the python program directly at \n"));
		theApp.m_program_canvas->m_textCtrl->AppendText(file_str.GetFullPath());
	}

	return(python);
}

ProgramUserType CProgram::GetUserType()
{
	if((m_nc_code != NULL) && (m_nc_code->m_user_edited)) return ProgramUserTypeNC;
	if(m_script_edited)return ProgramUserTypeScript;
	if((m_operations != NULL) && (m_operations->GetFirstChild()))return ProgramUserTypeTree;
	return ProgramUserTypeUnkown;
}

void CProgram::UpdateFromUserType()
{
#if 0
	switch(GetUserType())
	{
	case ProgramUserTypeUnkown:

	case tree:
   Enable "Operations" icon in tree
     editing the tree, recreates the python script
   Read only "Program" window
     pressing "Post-process" creates the NC code ( and backplots it )
   Read only "Output" window
   Disable all buttons on "Output" window

	}
#endif
}

// static
void CProgram::GetMachines(std::vector<CMachine> &machines)
{
	wxString machines_file = CProgram::alternative_machines_file;
#ifdef CMAKE_UNIX
	#ifdef RUNINPLACE
		if(machines_file.Len() == 0)machines_file = theApp.GetResFolder() + _T("/nc/machines.xml");
	#else
		if(machines_file.Len() == 0)machines_file = _T("/usr/lib/heekscnc/nc/machines.xml");
	#endif
#else
	if(machines_file.Len() == 0)machines_file = theApp.GetResFolder() + _T("/nc/machines.xml");
#endif

	TiXmlDocument doc(Ttc(machines_file.c_str()));
	if (!doc.LoadFile())
	{
		if(doc.Error())
		{
			wxMessageBox(Ctt(doc.ErrorDesc()));
		}
		return;
	}

	char oldlocale[1000];
	strcpy(oldlocale, setlocale(LC_NUMERIC, "C"));

	TiXmlHandle hDoc(&doc);
	TiXmlElement* element;
	TiXmlNode* root = &doc;

	for(element = root->FirstChildElement(); element;	element = element->NextSiblingElement())
	{
		CMachine m;
		for(TiXmlAttribute* a = element->FirstAttribute(); a; a = a->Next())
		{
			std::string name(a->Name());
			if(name == "post")m.post = wxString(Ctt(a->Value()));
			else if(name == "reader")m.reader = wxString(Ctt(a->Value()));
			else if(name == "suffix")m.suffix = wxString(Ctt(a->Value()));
			else if(name == "description")m.description = wxString(Ctt(a->Value()));
			else m.py_params.push_back(PyParam(a->Name(), a->Value()));
		}
		machines.push_back(m);
	}

	setlocale(LC_NUMERIC, oldlocale);
}

// static
void CProgram::GetScriptOps(std::vector< CXmlScriptOp > &script_ops)
{
	wxString script_op_file = theApp.GetResFolder() + _T("/script_ops.xml");

	TiXmlDocument doc(Ttc(script_op_file.c_str()));
	if (!doc.LoadFile())
	{
		if(doc.Error())
		{
			wxMessageBox(Ctt(doc.ErrorDesc()));
		}
		return;
	}

	char oldlocale[1000];
	strcpy(oldlocale, setlocale(LC_NUMERIC, "C"));

	TiXmlHandle hDoc(&doc);
	TiXmlElement* element;
	TiXmlNode* root = &doc;

	for(element = root->FirstChildElement(); element;	element = element->NextSiblingElement())
	{
		wxString label;
		wxString bitmap;
		wxString icon;
		wxString script;

		for(TiXmlAttribute* a = element->FirstAttribute(); a; a = a->Next())
		{
			std::string name(a->Name());
			if(name == "name")label = wxString(Ctt(a->Value()));
			else if(name == "bitmap")bitmap = wxString(Ctt(a->Value()));
			else if(name == "icon")icon = wxString(Ctt(a->Value()));
			else if(name == "script")script = wxString(Ctt(a->Value()));
		}
		script_ops.push_back(CXmlScriptOp(label, bitmap, icon, script));
	}

	setlocale(LC_NUMERIC, oldlocale);
}

// static
CMachine CProgram::GetMachine(const wxString& description)
{
	std::vector<CMachine> machines;
	GetMachines(machines);
	for(unsigned int i = 0; i<machines.size(); i++)
	{
		if(machines[i].description == description)
		{
			return machines[i];
		}
	}

	if(machines.size() > 0)return machines[0];

	CMachine machine;
	machine.post = _T("not found");
	machine.reader = _T("not found");
	machine.description = _T("not found");

	return machine;
}

/**
	If the m_output_file_name_follows_data_file_name flag is true then
	we don't want to use the temporary directory.
 */
wxString CProgram::GetOutputFileName() const
{
	if (m_output_file_name_follows_data_file_name)
	{
		if (heeksCAD->GetFileFullPath())
		{
			wxString path(heeksCAD->GetFileFullPath());
			int offset = -1;
			if ((offset = path.Find('.', true)) != wxNOT_FOUND)
			{
				path.Remove(offset); // chop off the end.
			} // End if - then

			path << m_machine.suffix.c_str();
			return(path);
		} // End if - then
		else
		{
			// The user hasn't assigned a filename yet.  Use the default.
			return GetDefaultOutputFilePath();
		} // End if - else
	} // End if - then
	else
	{
		return(m_output_file);
	} // End if - else
} // End GetOutputFileName() method

wxString CProgram::GetBackplotFilePath() const
{
	// The xml file is created in the temporary folder
#if wxCHECK_VERSION(3, 0, 0)
	wxStandardPaths& standard_paths = wxStandardPaths::Get();
#else
	wxStandardPaths standard_paths;
#endif
	wxFileName file_str(standard_paths.GetTempDir().c_str(), _T("backplot.xml"));
	return file_str.GetFullPath();
}

CNCCode* CProgram::NCCode()
{
    if (m_nc_code == NULL) ReloadPointers();
    return m_nc_code;
}

COperations* CProgram::Operations()
{
    if (m_operations == NULL) ReloadPointers();
    return m_operations;
}

CTools* CProgram::Tools()
{
    if (m_tools == NULL) ReloadPointers();
    return m_tools;
}

CPatterns* CProgram::Patterns()
{
    if (m_patterns == NULL) ReloadPointers();
    return m_patterns;
}

CSurfaces* CProgram::Surfaces()
{
    if (m_surfaces == NULL) ReloadPointers();
    return m_surfaces;
}

CStocks* CProgram::Stocks()
{
    if (m_stocks == NULL) ReloadPointers();
    return m_stocks;
}

void CProgram::ReloadPointers()
{
    for (HeeksObj *child = GetFirstChild(); child != NULL; child = GetNextChild())
	{
	    if (child->GetType() == ToolsType) m_tools = (CTools *) child;
	    if (child->GetType() == PatternsType) m_patterns = (CPatterns *) child;
	    if (child->GetType() == SurfacesType) m_surfaces = (CSurfaces *) child;
	    if (child->GetType() == StocksType) m_stocks = (CStocks *) child;
	    if (child->GetType() == OperationsType) m_operations = (COperations *) child;
	    if (child->GetType() == NCCodeType) m_nc_code = (CNCCode *) child;
	} // End for
}

void CProgram::AddMissingChildren()
{
    // Make sure the pointers are not already amongst existing children.
	ReloadPointers();

	// make sure tools, operations, fixtures, etc. exist
	if(m_tools == NULL){m_tools = new CTools; Add( m_tools, NULL );}
	if(m_patterns == NULL){m_patterns = new CPatterns; Add( m_patterns, NULL );}
	if(m_surfaces == NULL){m_surfaces = new CSurfaces; Add( m_surfaces, NULL );}
	if(m_stocks == NULL){m_stocks = new CStocks; Add( m_stocks, NULL );}
	if(m_operations == NULL){m_operations = new COperations; Add( m_operations, NULL );}
	if(m_nc_code == NULL){m_nc_code = new CNCCode; Add( m_nc_code, NULL );}
}

bool CProgram::operator==( const CProgram & rhs ) const
{
	if (m_machine != rhs.m_machine) return(false);
	if (m_output_file != rhs.m_output_file) return(false);
	if (m_output_file_name_follows_data_file_name != rhs.m_output_file_name_follows_data_file_name) return(false);
	if (m_script_edited != rhs.m_script_edited) return(false);
	if (m_units != rhs.m_units) return(false);

	return(IdNamedObjList::operator==(rhs));
}

bool CMachine::operator==( const CMachine & rhs ) const
{
	if (post != rhs.post) return(false);
	if (reader != rhs.reader) return(false);
	if (suffix != rhs.suffix) return(false);
	if (description != rhs.description) return(false);
	if (py_params.size() != rhs.py_params.size())return false;
	std::list<PyParam>::const_iterator It = py_params.begin(), It2 = rhs.py_params.begin();
	for(;It != py_params.end(); It++, It2++){
		if(!((*It) == (*It2)))return false;
	}

	return(true);
}

void CProgram::WriteDefaultValues()
{
	CNCConfig config;

	config.Write(_T("ProgramMachine"), m_machine.description);
	config.Write(_T("OutputFileNameFollowsDataFileName"), m_output_file_name_follows_data_file_name );
	config.Write(_T("ProgramOutputFile"), m_output_file);
	config.Write(_T("ProgramUnits"), m_units);
	config.Write(_T("ProgramPathControlMode"), (int) m_path_control_mode );
	config.Write(_T("ProgramMotionBlendingTolerance"), m_motion_blending_tolerance );
	config.Write(_T("ProgramNaiveCamTolerance"), m_naive_cam_tolerance );
}

wxString CProgram::GetDefaultOutputFilePath()const
 {
#if wxCHECK_VERSION(3, 0, 0)
	wxStandardPaths& standard_paths = wxStandardPaths::Get();
#else
	wxStandardPaths standard_paths;
#endif
	wxFileName default_path(standard_paths.GetTempDir().c_str(), wxString(_T("test")) + m_machine.suffix);
	return default_path.GetFullPath();
}

void CProgram::ReadDefaultValues()
{
	CNCConfig config;

	wxString machine_description;
	config.Read(_T("ProgramMachine"), &machine_description, _T("LinuxCNC"));
	m_machine = CProgram::GetMachine(machine_description);
	config.Read(_T("OutputFileNameFollowsDataFileName"), &m_output_file_name_follows_data_file_name, true);
	config.Read(_T("ProgramOutputFile"), &m_output_file, GetDefaultOutputFilePath().c_str());
	config.Read(_T("ProgramUnits"), &m_units, 1.0);
	config.Read(_("ProgramPathControlMode"), (int *) &m_path_control_mode, (int) ePathControlUndefined );
	config.Read(_("ProgramMotionBlendingTolerance"), &m_motion_blending_tolerance, 0.0001);
	config.Read(_("ProgramNaiveCamTolerance"), &m_naive_cam_tolerance, 0.0001);
}

static bool OnEdit(HeeksObj* object)
{
	ProgramDlg dlg(heeksCAD->GetMainFrame(), (CProgram*)object);
	if(dlg.ShowModal() == wxID_OK)
	{
		dlg.GetData((CProgram*)object);
		return true;
	}
	return false;
}

void CProgram::GetOnEdit(bool(**callback)(HeeksObj*))
{
	*callback = OnEdit;
}
