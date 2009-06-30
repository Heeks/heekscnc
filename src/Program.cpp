// Program.cpp

#include "stdafx.h"
#include "Program.h"
#include "PythonStuff.h"
#include "tinyxml/tinyxml.h"
#include "ProgramCanvas.h"
#include "NCCode.h"
#include "interface/MarkedObject.h"
#include "interface/PropertyString.h"
#include "interface/PropertyChoice.h"
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

#include <vector>
#include <algorithm>
#include <fstream>
using namespace std;

bool COperations::CanAdd(HeeksObj* object)
{
	return 	object->GetType() == ProfileType || 
		object->GetType() == PocketType || 
		object->GetType() == ZigZagType || 
		object->GetType() == AdaptiveType || 
		object->GetType() == DrillingType ||
		object->GetType() == CounterBoreType;
}

void COperations::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element;
	element = new TiXmlElement( "Operations" );
	root->LinkEndChild( element );  
	WriteBaseXML(element);
}

//static
HeeksObj* COperations::ReadFromXMLElement(TiXmlElement* pElem)
{
	COperations* new_object = new COperations;
	new_object->ReadBaseXML(pElem);
	return new_object;
}

static COperations* object_for_tools = NULL;

class SetAllActive: public Tool{
	// Tool's virtual functions
	const wxChar* GetTitle(){return _("Set All Operations Active");}
	void Run()
	{
		for(HeeksObj* object = object_for_tools->GetFirstChild(); object; object = object_for_tools->GetNextChild())
		{
			if(COp::IsAnOperation(object->GetType()))
			{
				((COp*)object)->m_active = true;
				heeksCAD->WasModified(object);
			}
		}
	}
	wxString BitmapPath(){ return _T("setactive");}
};

static SetAllActive set_all_active;

class SetAllInactive: public Tool{
	// Tool's virtual functions
	const wxChar* GetTitle(){return _("Set All Operations Inactive");}
	void Run()
	{
		for(HeeksObj* object = object_for_tools->GetFirstChild(); object; object = object_for_tools->GetNextChild())
		{
			if(COp::IsAnOperation(object->GetType()))
			{
				((COp*)object)->m_active = false;
				heeksCAD->WasModified(object);
			}
		}
	}
	wxString BitmapPath(){ return _T("setinactive");}
};

static SetAllInactive set_all_inactive;

void COperations::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
	object_for_tools = this;
	t_list->push_back(&set_all_active);
	t_list->push_back(&set_all_inactive);

	ObjList::GetTools(t_list, p);
}

bool CTools::CanAdd(HeeksObj* object)
{
	return 	((object->GetType() == CuttingToolType));
}

void CTools::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element;
	element = new TiXmlElement( "Tools" );
	root->LinkEndChild( element );  
	WriteBaseXML(element);
}

//static
HeeksObj* CTools::ReadFromXMLElement(TiXmlElement* pElem)
{
	CTools* new_object = new CTools;
	new_object->ReadBaseXML(pElem);
	return new_object;
}

CProgram::CProgram():m_nc_code(NULL), m_operations(NULL), m_tools(NULL), m_script_edited(false)
{
	CNCConfig config;
	wxString machine_file_name;
	config.Read(_T("ProgramMachine"), &machine_file_name, _T("iso"));
	m_machine = CProgram::GetMachine(machine_file_name);

#ifdef WIN32
	config.Read(_T("ProgramOutputFile"), &m_output_file, _T("test.tap"));
#else
	config.Read(_T("ProgramOutputFile"), &m_output_file, _T("/tmp/test.tap"));
#endif
	config.Read(_T("ProgramUnits"), &m_units, 1.0);
}

HeeksObj *CProgram::MakeACopy(void)const
{
	return new CProgram(*this);
}

void CProgram::CopyFrom(const HeeksObj* object)
{
	operator=(*((CProgram*)object));
}

static void on_set_machine(int value, HeeksObj* object)
{
	std::vector<CMachine> machines;
	CProgram::GetMachines(machines);
	((CProgram*)object)->m_machine = machines[value];
	CNCConfig config;
	config.Write(_T("ProgramMachine"), ((CProgram*)object)->m_machine.file_name);
}

static void on_set_output_file(const wxChar* value, HeeksObj* object)
{
	((CProgram*)object)->m_output_file = value;
	CNCConfig config;
	config.Write(_T("ProgramOutputFile"), ((CProgram*)object)->m_output_file);
}

static void on_set_units(int value, HeeksObj* object)
{
	((CProgram*)object)->m_units = (value == 0) ? 1.0:25.4;
	CNCConfig config;
	config.Write(_T("ProgramUnits"), ((CProgram*)object)->m_units);

	// also change HeeksCAD's view units automatically
	heeksCAD->SetViewUnits(((CProgram*)object)->m_units, true);
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

	list->push_back(new PropertyString(_("output file"), m_output_file, this, on_set_output_file));
	{
		std::list< wxString > choices;
		choices.push_back ( wxString ( _("mm") ) );
		choices.push_back ( wxString ( _("inch") ) );
		int choice = 0;
		if(m_units > 25.0)choice = 1;
		list->push_back ( new PropertyChoice ( _("units for nc output"),  choices, choice, this, on_set_units ) );
	}
	HeeksObj::GetProperties(list);
}

bool CProgram::CanAdd(HeeksObj* object)
{
	return object->GetType() == NCCodeType || object->GetType() == OperationsType || object->GetType() == ToolsType;
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
	element->SetAttribute("program", Ttc(theApp.m_program_canvas->m_textCtrl->GetValue()));
	element->SetDoubleAttribute("units", m_units);

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
	}

	return ObjList::Add(object, prev_object);
}

void CProgram::Remove(HeeksObj* object)
{
	 // these shouldn't happen, though
	if(object == m_nc_code)m_nc_code = NULL;
	else if(object == m_operations)m_operations = NULL;
	else if(object == m_tools)m_tools = NULL;

	ObjList::Remove(object);
}

// static member function
HeeksObj* CProgram::ReadFromXMLElement(TiXmlElement* pElem)
{
	CProgram* new_object = new CProgram;

	// get the attributes
	for(TiXmlAttribute* a = pElem->FirstAttribute(); a; a = a->Next())
	{
		std::string name(a->Name());
		if(name == "machine")new_object->m_machine = GetMachine(Ctt(a->Value()));
		else if(name == "output_file"){new_object->m_output_file.assign(Ctt(a->Value()));}
		else if(name == "program"){theApp.m_program_canvas->m_textCtrl->SetValue(Ctt(a->Value()));}
		else if(name == "units"){new_object->m_units = a->DoubleValue();}
	}

	new_object->ReadBaseXML(pElem);
	theApp.m_program = new_object;

	return new_object;
}


/**
	Sort the NC operations by execution order first and then
	by cutting tool number.
 */
struct sort_operations : public std::binary_function< bool, COp *, COp * >
{
	bool operator() ( const COp *lhs, const COp *rhs ) const
	{
		if (lhs->m_execution_order < rhs->m_execution_order) return(true);
		if (lhs->m_execution_order > rhs->m_execution_order) return(false);

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

	bool profile_op_exists = false;
	bool pocket_op_exists = false;
	bool zigzag_op_exists = false;
	bool adaptive_op_exists = false;
	bool drilling_op_exists = false;
	bool counterbore_op_exists = false;

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
			if(((CProfile*)object)->m_active)profile_op_exists = true;
		}
		else if(object->GetType() == PocketType)
		{
			if(((CPocket*)object)->m_active)pocket_op_exists = true;
		}
		else if(object->GetType() == ZigZagType)
		{
			if(((CZigZag*)object)->m_active)zigzag_op_exists = true;
		}
		else if(object->GetType() == AdaptiveType)
		{
			if(((CAdaptive*)object)->m_active)adaptive_op_exists = true;
		}
		else if(object->GetType() == DrillingType)
		{
			if(((CDrilling*)object)->m_active)drilling_op_exists = true;
		}
		else if(object->GetType() == CounterBoreType)
		{
			counterbore_op_exists = true;
		}
	}

	// Sort the operations in order of execution_order and then by cutting_tool_number
	std::sort( operations.begin(), operations.end(), sort_operations() );

	// add standard stuff at the top
	//hackhack, make it work on unix with FHS
#ifndef WIN32
	theApp.m_program_canvas->AppendText(_T("import sys\n"));
	theApp.m_program_canvas->AppendText(_T("sys.path.insert(0,'/usr/local/lib/heekscnc/')\n"));
#endif
	// kurve related things
	if(profile_op_exists)
	{
		theApp.m_program_canvas->AppendText(_T("import kurve\n"));
		theApp.m_program_canvas->AppendText(_T("import kurve_funcs\n"));
	}

	// area related things
	if(pocket_op_exists)
	{
		theApp.m_program_canvas->AppendText(_T("import area\n"));
		theApp.m_program_canvas->AppendText(_T("area.set_units("));
		theApp.m_program_canvas->AppendText(m_units);
		theApp.m_program_canvas->AppendText(_T(")\n"));
		theApp.m_program_canvas->AppendText(_T("import area_funcs\n"));
	}

	// pycam stuff
	if(zigzag_op_exists)
	{
#ifdef WIN32
		theApp.m_program_canvas->AppendText(_T("import sys\n"));
#endif
		theApp.m_program_canvas->AppendText(_T("sys.path.insert(0,'PyCam/trunk')\n"));
		theApp.m_program_canvas->AppendText(_T("\n"));
		theApp.m_program_canvas->AppendText(_T("from pycam.Geometry import *\n"));
		theApp.m_program_canvas->AppendText(_T("from pycam.Cutters.SphericalCutter import *\n"));
		theApp.m_program_canvas->AppendText(_T("from pycam.Cutters.CylindricalCutter import *\n"));
		theApp.m_program_canvas->AppendText(_T("from pycam.Cutters.ToroidalCutter import *\n"));
		theApp.m_program_canvas->AppendText(_T("from pycam.Importers.STLImporter import ImportModel\n"));
		theApp.m_program_canvas->AppendText(_T("from pycam.PathGenerators.DropCutter import DropCutter\n"));
		theApp.m_program_canvas->AppendText(_T("from PyCamToHeeks import HeeksCNCExporter\n"));
		theApp.m_program_canvas->AppendText(_T("\n"));
	}

	// actp
	if(adaptive_op_exists)
	{
		theApp.m_program_canvas->AppendText(_T("import actp_funcs\n"));
		theApp.m_program_canvas->AppendText(_T("import actp\n"));
		theApp.m_program_canvas->AppendText(_T("\n"));
	}

	if(counterbore_op_exists)
	{
		theApp.m_program_canvas->AppendText(_T("import circular_pocket as circular\n"));
		theApp.m_program_canvas->AppendText(_T("\n"));
	}


	// machine general stuff
	theApp.m_program_canvas->AppendText(_T("from nc.nc import *\n"));

	// specific machine
	theApp.m_program_canvas->AppendText(_T("import nc.") + m_machine.file_name + _T("\n"));
	theApp.m_program_canvas->AppendText(_T("\n"));

	// output file
	theApp.m_program_canvas->AppendText(_T("output('") + m_output_file + _T("')\n"));

	// begin program
	theApp.m_program_canvas->AppendText(_T("program_begin(123, 'Test program')\n"));
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


	// And then all the rest of the operations.
	int current_tool = 0;
	for (OperationsMap_t::const_iterator l_itOperation = operations.begin(); l_itOperation != operations.end(); l_itOperation++)
	{
		HeeksObj *object = (HeeksObj *) *l_itOperation;
		if (object == NULL) continue;
		
		if ((((COp *) object)->m_cutting_tool_number > 0) && (current_tool != ((COp *) object)->m_cutting_tool_number))
		{
			// Select the right tool.
			std::ostringstream l_ossValue;

			l_ossValue << "tool_change( id=" << ((COp *) object)->m_cutting_tool_number << ")\n";
			theApp.m_program_canvas->AppendText(Ctt(l_ossValue.str().c_str()));
			current_tool = ((COp *) object)->m_cutting_tool_number;
		} // End if - then

		switch(object->GetType())
		{
		case ProfileType:
			if(((CProfile*)object)->m_active)((CProfile*)object)->AppendTextToProgram();
			break;
		case PocketType:
			if(((CPocket*)object)->m_active)((CPocket*)object)->AppendTextToProgram();
			break;
		case ZigZagType:
			if(((CZigZag*)object)->m_active)((CZigZag*)object)->AppendTextToProgram();
			break;
		case AdaptiveType:
			if(((CAdaptive*)object)->m_active)((CAdaptive*)object)->AppendTextToProgram();
			break;
		case DrillingType:
			if(((CDrilling*)object)->m_active)((CDrilling*)object)->AppendTextToProgram();
			break;
		case CounterBoreType:
			if(((CCounterBore *)object)->m_active)((CCounterBore *)object)->AppendTextToProgram();
			break;
		}
	}
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
	if(!ifs)return;

	char str[1024] = "";

	while(!(ifs.eof()))
	{
		bool space_found = false;
		wxString name;
		wxString description;

		ifs.getline(str, 1024);

		int len = strlen(str);

		for(int i = 0; i<len; i++){
			char c = str[i];
			switch(c)
			{
			case ' ':
				if(space_found)description.Append(c);
				space_found = true;
				break;

			case '\n':
			case '\r':
				break;

			default:
				if(space_found)description.Append(c);
				else name.Append(c);
				break;
			}
		}

		if(name.Length() > 0)
		{
			CMachine m;
			m.file_name = name;
			m.description = description;
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