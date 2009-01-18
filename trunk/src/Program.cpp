// Program.cpp

#include "stdafx.h"
#include "Program.h"
#include "PythonStuff.h"
#include "../../tinyxml/tinyxml.h"
#include "ProgramCanvas.h"
#include "NCCode.h"
#include "../../interface/MarkedObject.h"
#include "../../interface/PropertyString.h"
#include "Profile.h"
#include "ZigZag.h"

void COperations::WriteXML(TiXmlElement *root)
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

CProgram::CProgram()
{
	m_machine.assign(_T("nc.iso"));
	m_output_file.assign(_T("test.tap"));
	m_nc_code = NULL;
	m_operations = NULL;
}

HeeksObj *CProgram::MakeACopy(void)const
{
	return new CProgram(*this);
}

void CProgram::CopyFrom(const HeeksObj* object)
{
	operator=(*((CProgram*)object));
}

static void on_set_machine(const wxChar* value, HeeksObj* object){((CProgram*)object)->m_machine = value;}
static void on_set_output_file(const wxChar* value, HeeksObj* object){((CProgram*)object)->m_output_file = value;}

void CProgram::GetProperties(std::list<Property *> *list)
{
	list->push_back(new PropertyString(_("machine"), m_machine, this, on_set_machine));
	list->push_back(new PropertyString(_("output file"), m_output_file, this, on_set_output_file));
	HeeksObj::GetProperties(list);
}

bool CProgram::CanAdd(HeeksObj* object)
{
	return object->GetType() == NCCodeType || object->GetType() == OperationsType;
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

void CProgram::WriteXML(TiXmlElement *root)
{
	TiXmlElement * element;
	element = new TiXmlElement( "Program" );
	root->LinkEndChild( element );  
	element->SetAttribute("machine", Ttc(m_machine.c_str()));
	element->SetAttribute("output_file", Ttc(m_output_file.c_str()));
	element->SetAttribute("program", Ttc(theApp.m_program_canvas->m_textCtrl->GetValue()));

	WriteBaseXML(element);
}

bool CProgram::Add(HeeksObj* object, HeeksObj* prev_object)
{
	if(object->GetType() == NCCodeType)m_nc_code = (CNCCode*)object;
	if(object->GetType() == OperationsType)m_operations = (COperations*)object;
	return ObjList::Add(object, prev_object);
}

void CProgram::Remove(HeeksObj* object)
{
	 // these shouldn't happen, though
	if(object == m_nc_code)m_nc_code = NULL;
	else if(object == m_operations)m_operations = NULL;

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
		if(name == "machine"){new_object->m_machine.assign(Ctt(a->Value()));}
		else if(name == "output_file"){new_object->m_output_file.assign(Ctt(a->Value()));}
		else if(name == "program"){theApp.m_program_canvas->m_textCtrl->SetValue(Ctt(a->Value()));}
	}

	new_object->ReadBaseXML(pElem);
	theApp.m_program = new_object;

	return new_object;
}

void CProgram::RewritePythonProgram()
{
	theApp.m_program_canvas->m_textCtrl->Clear();
	CZigZag::number_for_stl_file = 1;

	bool profile_op_exists = false;
	bool zigzag_op_exists = false;
	for(HeeksObj* object = m_operations->GetFirstChild(); object; object = m_operations->GetNextChild())
	{
		if(object->GetType() == ProfileType)
		{
			profile_op_exists = true;
		}
		else if(object->GetType() == ZigZagType)
		{
			zigzag_op_exists = true;
		}
	}

	// add standard stuff at the top

	// kurve related things
	if(profile_op_exists)
	{
		theApp.m_program_canvas->m_textCtrl->AppendText(_T("import kurve\n"));
		theApp.m_program_canvas->m_textCtrl->AppendText(_T("import stdops\n"));
	}

	// pycam stuff
	if(zigzag_op_exists)
	{
		theApp.m_program_canvas->m_textCtrl->AppendText(_T("import sys\n"));
		theApp.m_program_canvas->m_textCtrl->AppendText(_T("sys.path.insert(0,'.')\n"));
		theApp.m_program_canvas->m_textCtrl->AppendText(_T("\n"));
		theApp.m_program_canvas->m_textCtrl->AppendText(_T("from pycam.Geometry import *\n"));
		theApp.m_program_canvas->m_textCtrl->AppendText(_T("from pycam.Cutters.SphericalCutter import *\n"));
		theApp.m_program_canvas->m_textCtrl->AppendText(_T("from pycam.Cutters.CylindricalCutter import *\n"));
		theApp.m_program_canvas->m_textCtrl->AppendText(_T("from pycam.Gui.Visualization import ShowTestScene\n"));
		theApp.m_program_canvas->m_textCtrl->AppendText(_T("from pycam.Importers.TestModel import TestModel\n"));
		theApp.m_program_canvas->m_textCtrl->AppendText(_T("from pycam.PathGenerators.DropCutter import DropCutter\n"));
		theApp.m_program_canvas->m_textCtrl->AppendText(_T("\n"));
		theApp.m_program_canvas->m_textCtrl->AppendText(_T("\n"));
		theApp.m_program_canvas->m_textCtrl->AppendText(_T("\n"));
		theApp.m_program_canvas->m_textCtrl->AppendText(_T("\n"));
		theApp.m_program_canvas->m_textCtrl->AppendText(_T("\n"));
		theApp.m_program_canvas->m_textCtrl->AppendText(_T("\n"));
		theApp.m_program_canvas->m_textCtrl->AppendText(_T("\n"));
	}

	// machine general stuff
	theApp.m_program_canvas->m_textCtrl->AppendText(_T("from nc.nc import *\n"));

	// specific machine
	theApp.m_program_canvas->m_textCtrl->AppendText(_T("import ") + m_machine + _T("\n"));
	theApp.m_program_canvas->m_textCtrl->AppendText(_T("\n"));

	// output file
	theApp.m_program_canvas->m_textCtrl->AppendText(_T("output('") + m_output_file + _T("')\n"));

	// begin program
	theApp.m_program_canvas->m_textCtrl->AppendText(_T("program_begin(123, 'Test program')\n"));
	theApp.m_program_canvas->m_textCtrl->AppendText(_T("absolute()\n"));
	theApp.m_program_canvas->m_textCtrl->AppendText(_T("metric()\n"));
	theApp.m_program_canvas->m_textCtrl->AppendText(_T("set_plane(0)\n"));
	theApp.m_program_canvas->m_textCtrl->AppendText(_T("\n"));

	// write the operations
	for(HeeksObj* object = m_operations->GetFirstChild(); object; object = m_operations->GetNextChild())
	{
		switch(object->GetType())
		{
		case ProfileType:
			((CProfile*)object)->AppendTextToProgram();
			break;
		}
	}

}
