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
#include "interface/Tool.h"
#include "Profile.h"
#include "Pocket.h"
#include "ZigZag.h"
#include "Adaptive.h"
#include "Drilling.h"
#include "CuttingTool.h"
#include "Op.h"
#include "CNCConfig.h"
#include "CounterBore.h"
#include "Fixture.h"
#include "SpeedOp.h"
#include "Operations.h"
#include "Fixtures.h"
#include "Tools.h"
#include "interface/strconv.h"
#include <wx/stdpaths.h>
#include <wx/filename.h>

#include <vector>
#include <algorithm>
#include <fstream>
#include <memory>
using namespace std;

CProgram::CProgram():m_nc_code(NULL), m_operations(NULL), m_tools(NULL), m_speed_references(NULL), m_fixtures(NULL), m_script_edited(false)
{
	CNCConfig config(ConfigScope());
	wxString machine_file_name;
	config.Read(_T("ProgramMachine"), &machine_file_name, _T("iso"));
	m_machine = CProgram::GetMachine(machine_file_name);

	config.Read(_T("OutputFileNameFollowsDataFileName"), &m_output_file_name_follows_data_file_name, false);

    wxStandardPaths standard_paths;
    wxFileName default_path( standard_paths.GetTempDir().c_str(), _T("test.tap"));

	config.Read(_T("ProgramOutputFile"), &m_output_file, default_path.GetFullPath().c_str());

	config.Read(_T("ProgramUnits"), &m_units, 1.0);
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


CProgram::CProgram( const CProgram & rhs )
{
    m_nc_code = NULL;
    m_operations = NULL;
    m_tools = NULL;
    m_speed_references = NULL;
    m_fixtures = NULL;
    m_script_edited = false;

	*this = rhs;    // And finish off with the assignment operator.
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
		if ((m_speed_references != NULL) && (rhs->m_speed_references != NULL)) m_speed_references->CopyFrom(rhs->m_speed_references);
		if ((m_fixtures != NULL) && (rhs->m_fixtures != NULL)) m_fixtures->CopyFrom(rhs->m_fixtures);

		m_raw_material = rhs->m_raw_material;
		m_machine = rhs->m_machine;
		m_output_file = rhs->m_output_file;
		m_output_file_name_follows_data_file_name = rhs->m_output_file_name_follows_data_file_name;

		m_script_edited = rhs->m_script_edited;
		m_units = rhs->m_units;
	}
}

CProgram & CProgram::operator= ( const CProgram & rhs )
{
	if (this != &rhs)
	{
		ObjList::operator=(rhs);

		if ((m_nc_code != NULL) && (rhs.m_nc_code != NULL)) *m_nc_code = *(rhs.m_nc_code);
		if ((m_operations != NULL) && (rhs.m_operations != NULL)) *m_operations = *(rhs.m_operations);
		if ((m_tools != NULL) && (rhs.m_tools != NULL)) *m_tools = *(rhs.m_tools);
		if ((m_speed_references != NULL) && (rhs.m_speed_references != NULL)) *m_speed_references = *(rhs.m_speed_references);
		if ((m_fixtures != NULL) && (rhs.m_fixtures != NULL)) *m_fixtures = *(rhs.m_fixtures);

		m_raw_material = rhs.m_raw_material;
		m_machine = rhs.m_machine;
		m_output_file = rhs.m_output_file;
		m_output_file_name_follows_data_file_name = rhs.m_output_file_name_follows_data_file_name;

		m_script_edited = rhs.m_script_edited;
		m_units = rhs.m_units;
	}

	return(*this);
}

static void on_set_machine(int value, HeeksObj* object)
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

static void on_set_units(int value, HeeksObj* object)
{
	((CProgram*)object)->ChangeUnits((value == 0) ? 1.0:25.4);

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

static void on_set_output_file_name_follows_data_file_name(int zero_based_choice, HeeksObj *object)
{
	CProgram *pProgram = (CProgram *) object;
	pProgram->m_output_file_name_follows_data_file_name = (zero_based_choice != 0);
	heeksCAD->RefreshProperties();

	CNCConfig config(CProgram::ConfigScope());
	config.Write(_T("OutputFileNameFollowsDataFileName"), pProgram->m_output_file_name_follows_data_file_name );
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
	m_raw_material.GetProperties(this, list);
	HeeksObj::GetProperties(list);
}

static void on_set_max_spindle_speed(double value, HeeksObj* object)
{
#ifdef UNICODE
	std::wostringstream l_ossMessage;
#else
	std::ostringstream l_ossMessage;
#endif

	l_ossMessage << _T("This value is read-only.  Settings must be adjusted in the corresponding machine definition file\n")
			<< ((CProgram *)object)->m_machine.configuration_file_name.c_str();

	wxMessageBox(l_ossMessage.str().c_str());
	heeksCAD->RefreshProperties();
}

void CMachine::GetProperties(CProgram *parent, std::list<Property *> *list)
{
	list->push_back(new PropertyDouble(_("Maximum Spindle Speed (RPM)"), m_max_spindle_speed, parent, on_set_max_spindle_speed));
} // End GetProperties() method



bool CProgram::CanAdd(HeeksObj* object)
{
	return object->GetType() == NCCodeType ||
		object->GetType() == OperationsType ||
		object->GetType() == ToolsType ||
		object->GetType() == SpeedReferencesType ||
		object->GetType() == FixturesType;
}

bool CProgram::CanAddTo(HeeksObj* owner)
{
	return owner->GetType() == DocumentType;
}

void CProgram::SetClickMarkPoint(MarkedObject* marked_object, const double* ray_start, const double* ray_direction)
{
	if(marked_object->m_map.size() > 0)
	{
		MarkedObject* sub_marked_object = marked_object->m_map.begin()->second;
		if(sub_marked_object)
		{
			HeeksObj* object = sub_marked_object->m_map.begin()->first;
			if(object && object->GetType() == NCCodeType)
			{
				((CNCCode*)object)->SetClickMarkPoint(sub_marked_object, ray_start, ray_direction);
			}
		}
	}
}

void CProgram::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element;
	element = new TiXmlElement( "Program" );
	root->LinkEndChild( element );
	element->SetAttribute("machine", Ttc(m_machine.file_name.c_str()));
	element->SetAttribute("output_file", Ttc(m_output_file.c_str()));

#ifdef UNICODE
	std::wostringstream l_ossValue;
#else
	std::ostringstream l_ossValue;
#endif

	l_ossValue << (m_output_file_name_follows_data_file_name?1:0);
	element->SetAttribute("output_file_name_follows_data_file_name", Ttc(l_ossValue.str().c_str()));

	element->SetAttribute("program", Ttc(theApp.m_program_canvas->m_textCtrl->GetValue()));
	element->SetDoubleAttribute("units", m_units);

	m_raw_material.WriteBaseXML(element);
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

	case SpeedReferencesType:
		m_speed_references = (CSpeedReferences*)object;
		break;

	case FixturesType:
		m_fixtures = (CFixtures*)object;
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
	else if(object == m_speed_references)m_speed_references = NULL;
	else if(object == m_fixtures)m_fixtures = NULL;

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
	}

	new_object->ReadBaseXML(pElem);
	new_object->m_raw_material.ReadBaseXML(pElem);

	new_object->AddMissingChildren();

	return new_object;
}

/**
	Sort the NC operations by;
		- execution order
		- centre drilling operations
		- drilling operations
		- all other operations (sorted by tool number to avoid unnecessary tool changes)
 */
struct sort_operations : public std::binary_function< bool, COp *, COp * >
{
	bool operator() ( const COp *lhs, const COp *rhs ) const
	{
		if (lhs->m_execution_order < rhs->m_execution_order) return(true);
		if (lhs->m_execution_order > rhs->m_execution_order) return(false);

		// We want to run through all the centre drilling, then drilling, then milling then chamfering.

		if ((((HeeksObj *)lhs)->GetType() == ChamferType) && (((HeeksObj *)rhs)->GetType() != ChamferType)) return(false);
		if ((((HeeksObj *)lhs)->GetType() != ChamferType) && (((HeeksObj *)rhs)->GetType() == ChamferType)) return(true);

		if ((((HeeksObj *)lhs)->GetType() == DrillingType) && (((HeeksObj *)rhs)->GetType() != DrillingType)) return(true);
		if ((((HeeksObj *)lhs)->GetType() != DrillingType) && (((HeeksObj *)rhs)->GetType() == DrillingType)) return(false);

		if ((((HeeksObj *)lhs)->GetType() == DrillingType) && (((HeeksObj *)rhs)->GetType() == DrillingType))
		{
			// They're both drilling operations.  Select centre drilling over normal drilling.
			CCuttingTool *lhsPtr = (CCuttingTool *) CCuttingTool::Find( lhs->m_cutting_tool_number );
			CCuttingTool *rhsPtr = (CCuttingTool *) CCuttingTool::Find( rhs->m_cutting_tool_number );

			if ((lhsPtr != NULL) && (rhsPtr != NULL))
			{
				if ((lhsPtr->m_params.m_type == CCuttingToolParams::eCentreDrill) &&
				    (rhsPtr->m_params.m_type != CCuttingToolParams::eCentreDrill)) return(true);

				if ((lhsPtr->m_params.m_type != CCuttingToolParams::eCentreDrill) &&
				    (rhsPtr->m_params.m_type == CCuttingToolParams::eCentreDrill)) return(false);

				// There is no preference for centre drill.  Neither tool is a centre drill.  Give preference
				// to a normal drill bit over a milling bit now.

				if ((lhsPtr->m_params.m_type == CCuttingToolParams::eDrill) &&
				    (rhsPtr->m_params.m_type != CCuttingToolParams::eDrill)) return(true);

				if ((lhsPtr->m_params.m_type != CCuttingToolParams::eDrill) &&
				    (rhsPtr->m_params.m_type == CCuttingToolParams::eDrill)) return(false);

				// Finally, give preference to a milling bit over a chamfer bit.
				if ((lhsPtr->m_params.m_type == CCuttingToolParams::eChamfer) &&
				    (rhsPtr->m_params.m_type != CCuttingToolParams::eChamfer)) return(false);

				if ((lhsPtr->m_params.m_type != CCuttingToolParams::eChamfer) &&
				    (rhsPtr->m_params.m_type == CCuttingToolParams::eChamfer)) return(true);
			} // End if - then
		} // End if - then

		// The execution orders are the same.  Let's group on tool number so as
		// to avoid unnecessary tool change operations.

		if (lhs->m_cutting_tool_number < rhs->m_cutting_tool_number) return(true);
		if (lhs->m_cutting_tool_number > rhs->m_cutting_tool_number) return(false);

		return(false);
	} // End operator
};

void CProgram::RewritePythonProgram()
{
	theApp.m_program_canvas->m_textCtrl->Clear();
	CZigZag::number_for_stl_file = 1;
	CAdaptive::number_for_stl_file = 1;

	bool kurve_module_needed = false;
	bool kurve_funcs_needed = false;
	bool area_module_needed = false;
	bool area_funcs_needed = false;
	bool ocl_funcs_needed = false;
	bool adaptive_op_exists = false;
	bool drilling_op_exists = false;
	bool rough_turning_op_exists = false;

	typedef std::vector< COp * > OperationsMap_t;
	OperationsMap_t operations;

	if (m_operations == NULL)
	{
		// If there are no operations then there is no GCode.
		// No socks, no shirt, no service.
		return;
	} // End if - then

	for(HeeksObj* object = m_operations->GetFirstChild(); object; object = m_operations->GetNextChild())
	{
		operations.push_back( (COp *) object );

		if(object->GetType() == ProfileType)
		{
			if(((CProfile*)object)->m_active)
			{
				kurve_module_needed = true;
				kurve_funcs_needed = true;
			}
		}
		else if((object->GetType() == PocketType) || (object->GetType() == InlayType))
		{
			if(((COp*)object)->m_active)
			{
				area_module_needed = true;
				area_funcs_needed = true;
			}
		}
		else if(object->GetType() == ZigZagType)
		{
			if(((CZigZag*)object)->m_active)
			{
				ocl_funcs_needed = true;
			}
		}
		else if(object->GetType() == AdaptiveType)
		{
			if(((CAdaptive*)object)->m_active)adaptive_op_exists = true;
		}
		else if(object->GetType() == DrillingType)
		{
			if(((CDrilling*)object)->m_active)drilling_op_exists = true;
		}
		else if(object->GetType() == TurnRoughType)
		{
			if(((CProfile*)object)->m_active)
			{
				kurve_module_needed = true;
				area_module_needed = true;
				rough_turning_op_exists = true;
			}
		}
	}

	// Sort the operations in order of execution_order and then by cutting_tool_number
	std::sort( operations.begin(), operations.end(), sort_operations() );

	// add standard stuff at the top
	//hackhack, make it work on unix with FHS
	theApp.m_program_canvas->AppendText(_T("import sys\n"));

#ifndef WIN32
	theApp.m_program_canvas->AppendText(wxString(wxString(_T("sys.path.insert(0,")) + PythonString(_T("/usr/local/lib/heekscnc/")) + wxString(_T(")\n"))).c_str());
#endif

	theApp.m_program_canvas->AppendText(wxString(_T("sys.path.insert(0,")) + PythonString(theApp.GetDllFolder()).c_str() + wxString(_T(")\n")));
	theApp.m_program_canvas->AppendText(_T("import math\n"));

	// kurve related things
	if(kurve_module_needed)
	{
		theApp.m_program_canvas->AppendText(_T("import kurve\n"));
	}

	if(kurve_funcs_needed)
	{
		theApp.m_program_canvas->AppendText(_T("import kurve_funcs\n"));
	}

	// area related things
	if(area_module_needed)
	{
		theApp.m_program_canvas->AppendText(_T("import area\n"));
		theApp.m_program_canvas->AppendText(_T("area.set_units("));
		theApp.m_program_canvas->AppendText(m_units);
		theApp.m_program_canvas->AppendText(_T(")\n"));
	}

	if(area_funcs_needed)
	{
		theApp.m_program_canvas->AppendText(_T("import area_funcs\n"));
	}

	// OpenCamLib stuff
	if(ocl_funcs_needed)
	{
		theApp.m_program_canvas->AppendText(_T("import ocl_funcs\n"));
	}

	// actp
	if(adaptive_op_exists)
	{
		theApp.m_program_canvas->AppendText(_T("import actp_funcs\n"));
		theApp.m_program_canvas->AppendText(_T("import actp\n"));
		theApp.m_program_canvas->AppendText(_T("\n"));
	}

	if(rough_turning_op_exists)
	{
		theApp.m_program_canvas->AppendText(_T("import turning\n"));
		theApp.m_program_canvas->AppendText(_T("\n"));
	}


	// machine general stuff
	theApp.m_program_canvas->AppendText(_T("from nc.nc import *\n"));

	// specific machine
	if (m_machine.file_name == _T("not found"))
	{
		wxMessageBox(_T("Machine name (defined in Program Properties) not found"));
	} // End if - then
	else
	{
		theApp.m_program_canvas->AppendText(_T("import nc.") + m_machine.file_name + _T("\n"));
		theApp.m_program_canvas->AppendText(_T("\n"));
	} // End if - else

	// output file
	theApp.m_program_canvas->AppendText(wxString(_T("output(")) + PythonString(GetOutputFileName()).c_str() + wxString(_T(")\n")));

	// begin program
	theApp.m_program_canvas->AppendText(wxString((_T("program_begin(123, ")) + PythonString(_T("Test program")) + wxString(_T(")\n"))).c_str());
	theApp.m_program_canvas->AppendText(_T("absolute()\n"));
	if(m_units > 25.0)
	{
		theApp.m_program_canvas->AppendText(_T("imperial()\n"));
	}
	else
	{
		theApp.m_program_canvas->AppendText(_T("metric()\n"));
	}
	theApp.m_program_canvas->AppendText(_T("set_plane(0)\n"));
	theApp.m_program_canvas->AppendText(_T("\n"));

	m_raw_material.AppendTextToProgram();

	// write the tools setup code.
	if (m_tools != NULL)
	{
		// Write the new tool table entries first.
		for(HeeksObj* object = m_tools->GetFirstChild(); object; object = m_tools->GetNextChild())
		{
			switch(object->GetType())
			{
			case CuttingToolType:
				((CCuttingTool*)object)->AppendTextToProgram();
				break;
			}
		} // End for
	} // End if - then

	// this was too slow for me

	// Write all the operations once for each fixture.
	std::list<CFixture *> fixtures;
	std::auto_ptr<CFixture> default_fixture = std::auto_ptr<CFixture>(new CFixture( NULL, CFixture::G54 ) );

	for (int fixture = int(CFixture::G54); fixture <= int(CFixture::G59_3); fixture++)
	{
		CFixture *pFixture = CFixture::Find( CFixture::eCoordinateSystemNumber_t( fixture ) );
		if (pFixture != NULL)
		{
			fixtures.push_back( pFixture );
		} // End if - then
	} // End for

	if (fixtures.size() == 0)
	{
		// We need at least one fixture definition to generate any GCode.  Generate one
		// that provides no rotation at all.

		fixtures.push_back( default_fixture.get() );
	} // End if - then

	for (std::list<CFixture *>::const_iterator l_itFixture = fixtures.begin(); l_itFixture != fixtures.end(); l_itFixture++)
	{
		(*l_itFixture)->AppendTextToProgram();

		// And then all the rest of the operations.
		int current_tool = 0;
		for (OperationsMap_t::const_iterator l_itOperation = operations.begin(); l_itOperation != operations.end(); l_itOperation++)
		{
			HeeksObj *object = (HeeksObj *) *l_itOperation;
			if (object == NULL) continue;

			if(COp::IsAnOperation(object->GetType()))
			{
				if(((COp*)object)->m_active)
				{
					if ((((COp *) object)->m_cutting_tool_number > 0) && (current_tool != ((COp *) object)->m_cutting_tool_number))
					{
						// Select the right tool.
#ifdef UNICODE
						std::wostringstream l_ossValue;
#else
						std::ostringstream l_ossValue;
#endif
                        l_ossValue.imbue(std::locale("C"));
                        l_ossValue<<std::setprecision(10);

						CCuttingTool *pCuttingTool = (CCuttingTool *) heeksCAD->GetIDObject( CuttingToolType, ((COp *) object)->m_cutting_tool_number );
						if (pCuttingTool != NULL)
						{
							l_ossValue << _T("comment(") << PythonString(_T("tool change to ") + pCuttingTool->m_title).c_str() << ")\n";
						} // End if - then


						l_ossValue << _T("tool_change( id=") << ((COp *) object)->m_cutting_tool_number << _T(")\n");
						theApp.m_program_canvas->AppendText(l_ossValue.str().c_str());
						current_tool = ((COp *) object)->m_cutting_tool_number;
					} // End if - then

					((COp*)object)->AppendTextToProgram( *l_itFixture );
				}
			}
		} // End for - operation
	} // End for - fixture

	theApp.m_program_canvas->AppendText(_T("program_end()\n"));
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
	wxString machines_file = theApp.GetResFolder() + _T("/nc/machines.txt");
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
				m.m_max_spindle_speed = atof( Ttc(tokens.rbegin()->c_str()) );
				tokens.resize(tokens.size() - 1);	// Remove last token.
			} // End if - then
		} // End if - then

		// Everything else must be a description.
#ifdef UNICODE
		std::wostringstream l_ossDescription;
#else
		std::ostringstream l_ossDescription;
#endif
		for (std::vector<wxString>::const_iterator l_itToken = tokens.begin(); l_itToken != tokens.end(); l_itToken++)
		{
			if (l_itToken != tokens.begin()) l_ossDescription << _T(" ");
			l_ossDescription << l_itToken->c_str();
		} // End for
		m.description = l_ossDescription.str().c_str();

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
#ifdef UNICODE
			std::wostringstream l_ossPath;
			std::wstring l_ssPath;
			std::wstring::size_type i;
#else
			std::ostringstream l_ossPath;
			std::string l_ssPath;
			std::string::size_type i;
#endif

			l_ssPath.assign(heeksCAD->GetFileFullPath());
			if ( (i=l_ssPath.find_last_of('.')) != l_ssPath.npos)
			{
				l_ssPath.erase(i);	// chop off the end.
			} // End if - then

			l_ossPath << l_ssPath.c_str() << _T(".tap");
			return(l_ossPath.str().c_str());
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

void CProgram::AddMissingChildren()
{
	// make sure tools, operations, fixtures, etc. exist
	if(m_tools == NULL){m_tools = new CTools; Add( m_tools, NULL );}
	if(m_fixtures == NULL){m_fixtures = new CFixtures; Add( m_fixtures, NULL );}
	if(m_operations == NULL){m_operations = new COperations; Add( m_operations, NULL );}
	if(m_speed_references == NULL){m_speed_references = new CSpeedReferences; Add( m_speed_references, NULL );}
	if(m_nc_code == NULL){m_nc_code = new CNCCode; Add( m_nc_code, NULL );}
}

void CProgram::ChangeUnits( const double units )
{
    m_units = units;
    Tools()->OnChangeUnits(units);
    Operations()->OnChangeUnits(units);
}
