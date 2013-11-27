// Program.cpp
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include "Program.h"
#include "PythonStuff.h"
#include "tinyxml/tinyxml.h"
#include "ProgramCanvas.h"
#include "NCCode.h"
#include "interface/MarkedObject.h"
#include "interface/PropertyString.h"
#include "interface/PropertyFile.h"
#include "interface/PropertyChoice.h"
#include "interface/PropertyDouble.h"
#include "interface/PropertyLength.h"
#include "interface/PropertyCheck.h"
#include "interface/Tool.h"
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
#include "interface/strconv.h"
#include "MachineState.h"
#include "Pattern.h"
#include "Surface.h"
#include "src/Geom.h"

#include <wx/stdpaths.h>
#include <wx/filename.h>

#include <vector>
#include <algorithm>
#include <fstream>
#include <memory>
using namespace std;

wxString CProgram::alternative_machines_file = _T("");

CProgram::CProgram():m_nc_code(NULL), m_operations(NULL), m_tools(NULL), m_patterns(NULL), m_surfaces(NULL)
, m_script_edited(false)
{
	CNCConfig config(ConfigScope());
	wxString machine_file_name;
	config.Read(_T("ProgramMachine"), &machine_file_name, _T("emc2b"));
	m_machine = CProgram::GetMachine(machine_file_name);

	config.Read(_T("OutputFileNameFollowsDataFileName"), &m_output_file_name_follows_data_file_name, true);

    wxStandardPaths standard_paths;
    wxFileName default_path( standard_paths.GetTempDir().c_str(), _T("test.tap"));

	config.Read(_T("ProgramOutputFile"), &m_output_file, default_path.GetFullPath().c_str());

	config.Read(_T("ProgramUnits"), &m_units, 1.0);
	config.Read(_("ProgramPathControlMode"), (int *) &m_path_control_mode, (int) ePathControlUndefined );
	config.Read(_("ProgramMotionBlendingTolerance"), &m_motion_blending_tolerance, 0.0001);
	config.Read(_("ProgramNaiveCamTolerance"), &m_naive_cam_tolerance, 0.0001);

	config.Read(_("ClearanceSource"), (int *) &m_clearance_source, int(CProgram::eClearanceDefinedByOperation) );
}

const wxBitmap &CProgram::GetIcon()
{
	static wxBitmap* icon = NULL;
	if(icon == NULL)icon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/program.png")));
	return *icon;
}

HeeksObj *CProgram::MakeACopy(void)const
{
	return new CProgram(*this);
}


CProgram::CProgram( const CProgram & rhs ) : ObjList(rhs)
{
    m_nc_code = NULL;
    m_operations = NULL;
    m_tools = NULL;
	m_patterns = NULL;
	m_surfaces = NULL;
    m_script_edited = false;
    m_machine = rhs.m_machine;
    m_output_file = rhs.m_output_file;
    m_output_file_name_follows_data_file_name = rhs.m_output_file_name_follows_data_file_name;

    m_script_edited = rhs.m_script_edited;
    m_units = rhs.m_units;

	m_path_control_mode = rhs.m_path_control_mode;
	m_motion_blending_tolerance = rhs.m_motion_blending_tolerance;
	m_naive_cam_tolerance = rhs.m_naive_cam_tolerance;

	m_clearance_source = rhs.m_clearance_source;

    ReloadPointers();
    AddMissingChildren();

    if ((m_nc_code != NULL) && (rhs.m_nc_code != NULL)) *m_nc_code = *(rhs.m_nc_code);
    if ((m_operations != NULL) && (rhs.m_operations != NULL)) *m_operations = *(rhs.m_operations);
    if ((m_tools != NULL) && (rhs.m_tools != NULL)) *m_tools = *(rhs.m_tools);
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
		// ObjList::operator=(*rhs);	// I don't think this will do anything in this case. But it might one day.

		if ((m_nc_code != NULL) && (rhs->m_nc_code != NULL)) m_nc_code->CopyFrom( rhs->m_nc_code );
		if ((m_operations != NULL) && (rhs->m_operations != NULL)) m_operations->CopyFrom( rhs->m_operations );
		if ((m_tools != NULL) && (rhs->m_tools != NULL)) m_tools->CopyFrom( rhs->m_tools );
		if ((m_patterns != NULL) && (rhs->m_patterns != NULL)) m_patterns->CopyFrom( rhs->m_patterns );
		if ((m_surfaces != NULL) && (rhs->m_surfaces != NULL)) m_surfaces->CopyFrom( rhs->m_surfaces );

		m_machine = rhs->m_machine;
		m_output_file = rhs->m_output_file;
		m_output_file_name_follows_data_file_name = rhs->m_output_file_name_follows_data_file_name;

		m_script_edited = rhs->m_script_edited;
		m_units = rhs->m_units;

		m_path_control_mode = rhs->m_path_control_mode;
		m_motion_blending_tolerance = rhs->m_motion_blending_tolerance;
		m_naive_cam_tolerance = rhs->m_naive_cam_tolerance;

		m_clearance_source = rhs->m_clearance_source;
	}
}

CProgram & CProgram::operator= ( const CProgram & rhs )
{
	if (this != &rhs)
	{
		ObjList::operator=(rhs);
		ReloadPointers();

		if ((m_nc_code != NULL) && (rhs.m_nc_code != NULL)) *m_nc_code = *(rhs.m_nc_code);
		if ((m_operations != NULL) && (rhs.m_operations != NULL)) *m_operations = *(rhs.m_operations);
		if ((m_tools != NULL) && (rhs.m_tools != NULL)) *m_tools = *(rhs.m_tools);
		if ((m_patterns != NULL) && (rhs.m_patterns != NULL)) *m_patterns = *(rhs.m_patterns);
		if ((m_surfaces != NULL) && (rhs.m_surfaces != NULL)) *m_surfaces = *(rhs.m_surfaces);

		m_machine = rhs.m_machine;
		m_output_file = rhs.m_output_file;
		m_output_file_name_follows_data_file_name = rhs.m_output_file_name_follows_data_file_name;

		m_script_edited = rhs.m_script_edited;
		m_units = rhs.m_units;

		m_path_control_mode = rhs.m_path_control_mode;
		m_motion_blending_tolerance = rhs.m_motion_blending_tolerance;
		m_naive_cam_tolerance = rhs.m_naive_cam_tolerance;

		m_clearance_source = rhs.m_clearance_source;
	}

	return(*this);
}


CMachine::CMachine()
{
	m_max_spindle_speed = 0.0;

	CNCConfig config(CMachine::ConfigScope());
	config.Read(_T("safety_height_defined"), &m_safety_height_defined, false );
	config.Read(_T("safety_height"), &m_safety_height, 0.0 );		// in G53 machine units - indicates where to move to for tool changes
	config.Read(_("ClearanceHeight"), (double *) &(m_clearance_height), 50.0 ); // in local coordinate system (G54 etc.) to show how tall clamps and vices are for movement between machine operations.
}

CMachine::CMachine( const CMachine & rhs )
{
	*this = rhs;	// call the assignment operator
}


CMachine & CMachine::operator= ( const CMachine & rhs )
{
	if (this != &rhs)
	{
		configuration_file_name = rhs.configuration_file_name;
		file_name = rhs.file_name;
		description = rhs.description;
		m_max_spindle_speed = rhs.m_max_spindle_speed;
		m_safety_height_defined = rhs.m_safety_height_defined;
		m_safety_height = rhs.m_safety_height;
		m_clearance_height = rhs.m_clearance_height;
	} // End if - then

	return(*this);
} // End assignment operator.


static void on_set_machine(int value, HeeksObj* object, bool from_undo_redo)
{
	std::vector<CMachine> machines;
	CProgram::GetMachines(machines);
	((CProgram*)object)->m_machine = machines[value];
	CNCConfig config(CProgram::ConfigScope());
	config.Write(_T("ProgramMachine"), ((CProgram*)object)->m_machine.file_name);
	heeksCAD->RefreshProperties();
}

static void on_set_output_file(const wxChar* value, HeeksObj* object)
{
	((CProgram*)object)->m_output_file = value;
	CNCConfig config(CProgram::ConfigScope());
	config.Write(_T("ProgramOutputFile"), ((CProgram*)object)->m_output_file);
}

static void on_set_units(int value, HeeksObj* object, bool from_undo_redo)
{
	((CProgram*)object)->m_units = ((value == 0) ? 1.0:25.4);

	CNCConfig config(CProgram::ConfigScope());
	config.Write(_T("ProgramUnits"), ((CProgram*)object)->m_units);

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
	heeksCAD->RefreshProperties();

	CNCConfig config(CProgram::ConfigScope());
	config.Write(_T("OutputFileNameFollowsDataFileName"), pProgram->m_output_file_name_follows_data_file_name );
}

static void on_set_clearance_source(int zero_based_choice, HeeksObj *object, bool from_undo_redo)
{
	CProgram *pProgram = (CProgram *) object;
	pProgram->m_clearance_source = CProgram::eClearanceSource_t(zero_based_choice);

	CNCConfig config(CProgram::ConfigScope());
	config.Write(_T("ClearanceSource"), (int) pProgram->m_clearance_source );
}

static void on_set_path_control_mode(int zero_based_choice, HeeksObj *object, bool from_undo_redo)
{
	CProgram *pProgram = (CProgram *) object;
	pProgram->m_path_control_mode = CProgram::ePathControlMode_t(zero_based_choice);

	CNCConfig config(CProgram::ConfigScope());
	config.Write(_T("ProgramPathControlMode"), (int) pProgram->m_path_control_mode );
}

static void on_set_motion_blending_tolerance(double value, HeeksObj *object)
{
	CProgram *pProgram = (CProgram *) object;
	pProgram->m_motion_blending_tolerance = value;

	CNCConfig config(CProgram::ConfigScope());
	config.Write(_T("ProgramMotionBlendingTolerance"), pProgram->m_motion_blending_tolerance );
}

static void on_set_naive_cam_tolerance(double value, HeeksObj *object)
{
	CProgram *pProgram = (CProgram *) object;
	pProgram->m_naive_cam_tolerance = value;

	CNCConfig config(CProgram::ConfigScope());
	config.Write(_T("ProgramNaiveCamTolerance"), pProgram->m_naive_cam_tolerance );
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
			if(machine.file_name == m_machine.file_name)choice = i;
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

	{
		std::list< wxString > choices;
		choices.push_back ( wxString ( _("By Machine") ) );
		choices.push_back ( wxString ( _("By Fixture") ) );
		choices.push_back ( wxString ( _("By Operation") ) );
		int choice = int(m_clearance_source);

		list->push_back ( new PropertyChoice ( _("Clearance Height Defined"),  choices, choice, this, on_set_clearance_source ) );
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

	HeeksObj::GetProperties(list);
}

static void on_set_max_spindle_speed(double value, HeeksObj* object)
{
    ((CProgram *) object)->m_machine.m_max_spindle_speed = value;
	heeksCAD->RefreshProperties();
}

static void on_set_safety_height_defined(const bool value, HeeksObj *object)
{
    ((CProgram *)object)->m_machine.m_safety_height_defined = value;

	CNCConfig config(CMachine::ConfigScope());
	config.Write(_T("safety_height_defined"), ((CProgram *)object)->m_machine.m_safety_height_defined );

    // to do, make undoable properties
}

static void on_set_safety_height(const double value, HeeksObj *object)
{
    ((CProgram *)object)->m_machine.m_safety_height = value;

	CNCConfig config(CMachine::ConfigScope());
	config.Write(_T("safety_height"), ((CProgram *)object)->m_machine.m_safety_height );

    // to do, make undoable properties
}

static void on_set_clearance_height( const double value, HeeksObj *object)
{
	CMachine *pMachine = &(((CProgram *)object)->m_machine);
	pMachine->m_clearance_height = value;

	CNCConfig config(CMachine::ConfigScope());
	config.Write(_T("ClearanceHeight"), pMachine->m_clearance_height);

	heeksCAD->RefreshProperties();
}

void CMachine::GetProperties(CProgram *parent, std::list<Property *> *list)
{
	list->push_back(new PropertyDouble(_("Maximum Spindle Speed (RPM)"), m_max_spindle_speed, parent, on_set_max_spindle_speed));
	list->push_back(new PropertyCheck(_("Safety Height Defined"), m_safety_height_defined, parent, on_set_safety_height_defined));

	if (theApp.m_program->m_clearance_source == CProgram::eClearanceDefinedByMachine)
	{
		list->push_back(new PropertyLength(_("Clearance Height (for inter-operation movement)"), m_clearance_height, parent, on_set_clearance_height));
	}

    if (m_safety_height_defined)
    {
        list->push_back(new PropertyLength(_("Safety Height (in G53 - Machine - coordinates)"), m_safety_height, parent, on_set_safety_height));
    }
} // End GetProperties() method



bool CProgram::CanAdd(HeeksObj* object)
{
    if (object == NULL) return(false);

	return object->GetType() == NCCodeType ||
		object->GetType() == OperationsType ||
		object->GetType() == ToolsType ||
		object->GetType() == PatternsType ||
		object->GetType() == SurfacesType
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
			if(object && object->GetType() == NCCodeType)
			{
				((CNCCode*)object)->SetClickMarkPoint(marked_object, ray_start, ray_direction);
			}
	}
}

void CProgram::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element;
	element = heeksCAD->NewXMLElement( "Program" );
	heeksCAD->LinkXMLEndChild( root,  element );
	element->SetAttribute( "machine", m_machine.file_name.utf8_str());
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
	}

	return ObjList::Add(object, prev_object);
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

	ObjList::Remove(object);
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
	element->SetDoubleAttribute( "max_spindle_speed", m_max_spindle_speed);
	element->SetAttribute( "safety_height_defined", m_safety_height_defined);
	element->SetDoubleAttribute( "safety_height", m_safety_height);
	element->SetDoubleAttribute( "clearance_height", m_clearance_height);
} // End WriteBaseXML() method

void CMachine::ReadBaseXML(TiXmlElement* element)
{
	if (element->Attribute("max_spindle_speed"))
	{
		element->Attribute("max_spindle_speed", &m_max_spindle_speed);
	} // End if - then

	int flag = 0;
	if (element->Attribute("safety_height_defined")) element->Attribute("safety_height_defined", &flag);
	m_safety_height_defined = (flag != 0);
	if (element->Attribute("safety_height")) element->Attribute("safety_height", &m_safety_height);
	if (element->Attribute("clearance_height")) element->Attribute("clearance_height", &m_clearance_height);

} // End ReadBaseXML() method

void ApplyPatternToText(Python &python, int p)
{
	CPattern* pattern = (CPattern*)heeksCAD->GetIDObject(PatternType, p);
	if(pattern)
	{
		// write a transform redirector
		python << _T("nc.transform.transform_begin([");
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
		python<<_T("])\n");
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

		wxStandardPaths standard_paths;
		wxFileName filepath( standard_paths.GetTempDir().c_str(), wxString::Format(_T("surface%d.stl"), CSurface::number_for_stl_file).c_str() );
		CSurface::number_for_stl_file++;

		//write stl file
		heeksCAD->SaveSTLFile(solids, filepath.GetFullPath(), 0.01);

		python << _T("stl") << (int)(surface->m_id) << _T(" = ocl_funcs.STLSurfFromFile(") << PythonString(filepath.GetFullPath()) << _T(")\n");
	}

	python << _T("nc.attach.units = ") << theApp.m_program->m_units << _T("\n");
	python << _T("nc.attach.attach_begin()\n");
	python << _T("nc.nc.creator.stl = stl") << (int)(surface->m_id) << _T("\n");
	python << _T("nc.nc.creator.minz = ") << surface->m_min_z << _T("\n");
	python << _T("nc.nc.creator.material_allowance = ") << surface->m_material_allowance << _T("\n");

	theApp.machine_state.m_attached_to_surface = surface;
}

Python CProgram::RewritePythonProgram()
{
	Python python;

	theApp.m_program_canvas->m_textCtrl->Clear();
	CSurface::number_for_stl_file = 1;

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
				break;

			case PocketType:
				area_funcs_needed = true;
				break;

			case ScriptOpType:
				ocl_module_needed = true;
				nc_attach_needed = true;
				ocl_funcs_needed = true;
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
		python << _T("import nc.attach\n");
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

	if(CTool::FindFirstByType(CToolParams::eDragKnife) != -1)
	{
		python << _T("import nc.drag_knife\n");
		python << _T("\n");
	}

	if(transform_module_needed)
	{
		python << _T("import nc.transform\n");
		python << _T("\n");
	}

	// machine general stuff
	python << _T("from nc.nc import *\n");

	// specific machine
	if (m_machine.file_name == _T("not found"))
	{
		wxMessageBox(_T("Machine name (defined in Program Properties) not found"));
	} // End if - then
	else
	{
		python << _T("import nc.") + m_machine.file_name + _T("\n");
		python << _T("\n");
	} // End if - else

	// output file
	python << _T("output(") << PythonString(GetOutputFileName()) << _T(")\n");

	// begin program
	python << _T("program_begin(123, ") << PythonString(_T("Test program")) << _T(")\n");
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

	// copied operations ( with same id ) were not being done, so I've removed fixtures completely for the Windows installation
	// Write all the operations
	theApp.machine_state = CMachineState();

	std::set<CSurface*> surfaces_written;

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
				ApplyPatternToText(python, op->m_pattern);
				if(surface && surface->m_same_for_each_pattern_position)ApplySurfaceToText(python, surface, surfaces_written);

				python << op->AppendTextToProgram();

				// end surface attach
				if(surface && surface->m_same_for_each_pattern_position)python << _T("nc.attach.attach_end()\n");
				if(op->m_pattern != 0)python << _T("nc.transform.transform_end()\n");
				if(surface && !surface->m_same_for_each_pattern_position)python << _T("nc.attach.attach_end()\n");
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

		wxStandardPaths standard_paths;
		wxFileName file_str( standard_paths.GetTempDir().c_str(), _T("post.py"));

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
		if(machines_file.Len() == 0)machines_file = theApp.GetResFolder() + _T("/nc/machines.txt");
	#else
		if(machines_file.Len() == 0)machines_file = _T("/usr/lib/heekscnc/nc/machines.txt");
	#endif
#else
	if(machines_file.Len() == 0)machines_file = theApp.GetResFolder() + _T("/nc/machines.txt");
#endif
    ifstream ifs(Ttc(machines_file.c_str()));
	if(!ifs)
	{
#ifdef UNICODE
		std::wostringstream l_ossMessage;
#else
		std::ostringstream l_ossMessage;
#endif

		l_ossMessage << "Could not open '" << machines_file.c_str() << "' for reading";
		wxMessageBox( l_ossMessage.str().c_str() );
		return;
	}

	char str[1024] = "";

	while(!(ifs.eof()))
	{
		CMachine m;

		ifs.getline(str, 1024);

		m.configuration_file_name = machines_file;

		std::vector<wxString> tokens = Tokens( Ctt(str), _T(" \t\n\r") );

		// The first token is the machine name (post processor name)
		if (tokens.size() > 0) {
			m.file_name = tokens[0];
			tokens.erase(tokens.begin());
		} // End if - then

		// If there are other tokens, check the last one to see if it could be a maximum
		// spindle speed.
		if (tokens.size() > 0)
		{
			// We may have a material rate value.
			if (AllNumeric( *tokens.rbegin() ))
			{
				tokens.rbegin()->ToDouble(&(m.m_max_spindle_speed));
				tokens.resize(tokens.size() - 1);	// Remove last token.
			} // End if - then
		} // End if - then

		// Everything else must be a description.
		m.description.clear();
		for (std::vector<wxString>::const_iterator l_itToken = tokens.begin(); l_itToken != tokens.end(); l_itToken++)
		{
			if (l_itToken != tokens.begin()) m.description << _T(" ");
			m.description << *l_itToken;
		} // End for

		if(m.file_name.Length() > 0)
		{
			machines.push_back(m);
		}
	}

}

// static
CMachine CProgram::GetMachine(const wxString& file_name)
{
	std::vector<CMachine> machines;
	GetMachines(machines);
	for(unsigned int i = 0; i<machines.size(); i++)
	{
		if(machines[i].file_name == file_name)
		{
			return machines[i];
		}
	}

	if(machines.size() > 0)return machines[0];

	CMachine machine;
	machine.file_name = _T("not found");
	machine.description = _T("not found");

	CNCConfig config(ConfigScope());
	config.Read(_T("safety_height_defined"), &machine.m_safety_height_defined, false);
	config.Read(_T("safety_height"), &machine.m_safety_height, 0.0);

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

			path << _T(".tap");
			return(path);
		} // End if - then
		else
		{
			// The user hasn't assigned a filename yet.  Use the default.
			return(m_output_file);
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
	wxStandardPaths standard_paths;
	wxFileName file_str( standard_paths.GetTempDir().c_str(), _T("backplot.xml"));
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

void CProgram::ReloadPointers()
{
    for (HeeksObj *child = GetFirstChild(); child != NULL; child = GetNextChild())
	{
	    if (child->GetType() == ToolsType) m_tools = (CTools *) child;
	    if (child->GetType() == PatternsType) m_patterns = (CPatterns *) child;
	    if (child->GetType() == SurfacesType) m_surfaces = (CSurfaces *) child;
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

	return(ObjList::operator==(rhs));
}

bool CMachine::operator==( const CMachine & rhs ) const
{
	if (configuration_file_name != rhs.configuration_file_name) return(false);
	if (file_name != rhs.file_name) return(false);
	if (description != rhs.description) return(false);
	if (m_max_spindle_speed != rhs.m_max_spindle_speed) return(false);
	if (m_safety_height_defined != rhs.m_safety_height_defined) return(false);
	if (m_safety_height != rhs.m_safety_height) return(false);
	if (m_clearance_height != rhs.m_clearance_height) return(false);

	return(true);
}
