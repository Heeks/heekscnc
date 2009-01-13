// Program.cpp

#include "stdafx.h"
#include "Program.h"
#include "PythonStuff.h"
#include "../../tinyxml/tinyxml.h"
#include "ProgramCanvas.h"
#include "NCCode.h"
#include "../../interface/MarkedObject.h"

CProgram::CProgram()
{
	m_machine.assign("siegkx1");
	m_nc_code = NULL;
}

CProgram::~CProgram()
{
}

const CProgram &CProgram::operator=(const CProgram &p)
{
	m_machine = p.m_machine;
	return *this;
}

void CProgram::GetProperties(std::list<Property *> *list)
{
	HeeksObj::GetProperties(list);
}

void CProgram::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
	HeeksObj::GetTools(t_list, p);
}

HeeksObj *CProgram::MakeACopy(void)const
{
	return new CProgram(*this);
}

void CProgram::CopyFrom(const HeeksObj* object)
{
	operator=(*((CProgram*)object));
}

bool CProgram::CanAdd(HeeksObj* object)
{
	return object->GetType() == NCCodeType;
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
	element->SetAttribute("machine", m_machine.c_str());
	element->SetAttribute("program", Ttc(theApp.m_program_canvas->m_textCtrl->GetValue()));

	WriteBaseXML(element);
}

bool CProgram::Add(HeeksObj* object, HeeksObj* prev_object)
{
	if(object->GetType() == NCCodeType)m_nc_code = (CNCCode*)object;
	return ObjList::Add(object, prev_object);
}

void CProgram::Remove(HeeksObj* object)
{
	if(object == m_nc_code)m_nc_code = NULL;
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
		if(name == "machine"){new_object->m_machine.assign(a->Value());}
		else if(name == "program"){theApp.m_program_canvas->m_textCtrl->SetValue(Ctt(a->Value()));}
	}

	new_object->ReadBaseXML(pElem);
	theApp.m_program = new_object;

	return new_object;
}

