// AttachOp.cpp
/*
 * Copyright (c) 2010, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "stdafx.h"
#include "AttachOp.h"
#include "CNCConfig.h"
#include "ProgramCanvas.h"
#include "Program.h"
#include "interface/HeeksObj.h"
#include "interface/PropertyLength.h"
#include "tinyxml/tinyxml.h"
#include "PythonStuff.h"
#include "MachineState.h"
#include "Reselect.h"

#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <sstream>
#include <iomanip>

int CAttachOp::number_for_stl_file = 1;

CAttachOp::CAttachOp():COp(GetTypeString(), 0, AttachOpType), m_tolerance(0.01), m_min_z(0.0)
{
}

CAttachOp::CAttachOp(const std::list<int> &solids, double tol, double min_z):COp(GetTypeString(), 0, AttachOpType), m_solids(solids), m_tolerance(tol), m_min_z(min_z)
{
	ReadDefaultValues();

	for(std::list<int>::const_iterator It = solids.begin(); It != solids.end(); It++)
	{
		int solid = *It;
		HeeksObj* object = heeksCAD->GetIDObject(SolidType, solid);
		if(object)
		{
			Add(object, NULL);
		}
	}
	m_solids.clear();
}

CAttachOp::CAttachOp( const CAttachOp & rhs ) : COp(rhs), m_solids(rhs.m_solids), m_tolerance(rhs.m_tolerance), m_min_z(rhs.m_min_z)
{
}

CAttachOp & CAttachOp::operator= ( const CAttachOp & rhs )
{
	if (this != &rhs)
	{
		COp::operator=( rhs );
		m_solids = rhs.m_solids;
		m_tolerance = rhs.m_tolerance;
		m_min_z = rhs.m_min_z;
	}

	return(*this);
}

const wxBitmap &CAttachOp::GetIcon()
{
	if(!m_active)return GetInactiveIcon();
	static wxBitmap* icon = NULL;
	if(icon == NULL)icon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/attach.png")));
	return *icon;
}

void CAttachOp::ReloadPointers()
{
	for (std::list<int>::iterator symbol = m_solids.begin(); symbol != m_solids.end(); symbol++)
	{
		HeeksObj *object = heeksCAD->GetIDObject( SolidType, *symbol );
		if (object != NULL)
		{
			Add( object, NULL );
		}
	}

	m_solids.clear();	// We don't want to convert them twice.

	COp::ReloadPointers();
}

Python CAttachOp::AppendTextToProgram(CMachineState *pMachineState)
{
	Python python;

    ReloadPointers();   // Make sure all the solids in m_solids are included as child objects.

	python << COp::AppendTextToProgram(pMachineState);

	//write stl file
	std::list<HeeksObj*> solids;
	std::list<HeeksObj*> copies_to_delete;
	for (HeeksObj *object = GetFirstChild(); object != NULL; object = GetNextChild())
	{
	    if (object->GetType() != SolidType && object->GetType() != StlSolidType)
	    {
	        continue;
	    }

		if (object != NULL)
		{
			if(1/* to do pMachineState->Fixture().GetMatrix() == gp_Trsf()*/)
			{
				solids.push_back(object);
			}
			else
			{
				// Need to rotate a COPY of the solid by the fixture settings.
				HeeksObj* copy = object->MakeACopy();
				if (copy != NULL)
				{
					double m[16];	// A different form of the transformation matrix.
					CFixture::extract( pMachineState->Fixture().GetMatrix(), m );
					copy->ModifyByMatrix(m);
					solids.push_back(copy);
					copies_to_delete.push_back(copy);
				}
			}
		} // End if - then
	} // End for


    wxStandardPaths standard_paths;
    wxFileName filepath( standard_paths.GetTempDir().c_str(), wxString::Format(_T("surface%d.stl"), number_for_stl_file).c_str() );
	number_for_stl_file++;

	heeksCAD->SaveSTLFile(solids, filepath.GetFullPath(), 0.01);

	// We don't need the duplicate solids any more.  Delete them.
	for (std::list<HeeksObj*>::iterator l_itSolid = copies_to_delete.begin(); l_itSolid != copies_to_delete.end(); l_itSolid++)
	{
		delete *l_itSolid;
	} // End for

	python << _T("s = ocl_funcs.STLSurfFromFile(") << PythonString(filepath.GetFullPath()) << _T(")\n");
	python << _T("nc.attach.pdcf = ocl.PathDropCutter(s)\n");
	python << _T("nc.attach.pdcf.minimumZ = ") << m_min_z << _T("\n");
	python << _T("nc.attach.units = ") << theApp.m_program->m_units << _T("\n");
	python << _T("nc.attach.attach_begin()\n");

	pMachineState->m_attached_to_surface = true;

	return(python);
} // End AppendTextToProgram() method

static void on_set_tolerance(double value, HeeksObj* object){((CAttachOp*)object)->m_tolerance = value;}
static void on_set_min_z(double value, HeeksObj* object){((CAttachOp*)object)->m_min_z = value;}

void CAttachOp::GetProperties(std::list<Property *> *list)
{
	AddSolidsProperties(list, m_solids);
	list->push_back(new PropertyLength(_("tolerance"), m_tolerance, this, on_set_tolerance));
	list->push_back(new PropertyLength(_("minimum z"), m_min_z, this, on_set_min_z));
	COp::GetProperties(list);
}

static ReselectSolids reselect_solids;

void CAttachOp::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
	reselect_solids.m_solids = &m_solids;
	reselect_solids.m_object = this;
	t_list->push_back(&reselect_solids);

	COp::GetTools( t_list, p );
}

HeeksObj *CAttachOp::MakeACopy(void)const
{
	return new CAttachOp(*this);
}

void CAttachOp::CopyFrom(const HeeksObj* object)
{
	if (object->GetType() == GetType())
	{
		operator=(*((CAttachOp*)object));
	}
}

bool CAttachOp::CanAddTo(HeeksObj* owner)
{
	return ((owner != NULL) && (owner->GetType() == OperationsType));
}

void CAttachOp::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element = new TiXmlElement( "AttachOp" );
	root->LinkEndChild( element );

	element->SetDoubleAttribute("tolerance", m_tolerance);
	element->SetDoubleAttribute("minz", m_min_z);

	// write solid ids
	for(std::list<int>::iterator It = m_solids.begin(); It != m_solids.end(); It++)
	{
		int solid = *It;
		TiXmlElement * solid_element = new TiXmlElement( "solid" );
		element->LinkEndChild( solid_element );
		solid_element->SetAttribute("id", solid);
	}

	COp::WriteBaseXML(element);
}

// static member function
HeeksObj* CAttachOp::ReadFromXMLElement(TiXmlElement* element)
{
	CAttachOp* new_object = new CAttachOp;

	element->Attribute("tolerance", &new_object->m_tolerance);
	element->Attribute("minz", &new_object->m_min_z);

	std::list<TiXmlElement *> elements_to_remove;

	// read solid ids
	for(TiXmlElement* pElem = TiXmlHandle(element).FirstChildElement().Element(); pElem; pElem = pElem->NextSiblingElement())
	{
		std::string name(pElem->Value());
		if(name == "solid"){
			for(TiXmlAttribute* a = pElem->FirstAttribute(); a; a = a->Next())
			{
				std::string name(a->Name());
				if(name == "id"){
					int id = a->IntValue();
					new_object->m_solids.push_back(id);
				}
			}
			elements_to_remove.push_back(pElem);
		}
	}

	for (std::list<TiXmlElement*>::iterator itElem = elements_to_remove.begin(); itElem != elements_to_remove.end(); itElem++)
	{
		element->RemoveChild(*itElem);
	}

	new_object->ReadBaseXML(element);

	return new_object;
}

void CAttachOp::WriteDefaultValues()
{
	COp::WriteDefaultValues();

	CNCConfig config(ConfigScope());
	config.Write(wxString(GetTypeString()) + _T("Tolerance"), m_tolerance);
	config.Write(wxString(GetTypeString()) + _T("MinZ"), m_min_z);
}

void CAttachOp::ReadDefaultValues()
{
	COp::ReadDefaultValues();

	CNCConfig config(ConfigScope());
	config.Read(wxString(GetTypeString()) + _T("Tolerance"), &m_tolerance, 0.01);
	config.Read(wxString(GetTypeString()) + _T("MinZ"), &m_min_z, 0.0);
}

bool CAttachOp::operator==( const CAttachOp & rhs ) const
{
	if (m_tolerance != rhs.m_tolerance) return false;
	if (m_min_z != rhs.m_min_z) return false;
	if (m_solids.size() != rhs.m_solids.size()) return false;

	std::list<int>::const_iterator It = m_solids.begin();
	for(std::list<int>::const_iterator It2 = rhs.m_solids.begin(); It2 != rhs.m_solids.end(); It2++, It++)
	{
		if(*It != *It2)return false;
	}

	return(COp::operator==(rhs));
}

const wxBitmap &CUnattachOp::GetIcon()
{
	if(!m_active)return GetInactiveIcon();
	static wxBitmap* icon = NULL;
	if(icon == NULL)icon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/unattach.png")));
	return *icon;
}

Python CUnattachOp::AppendTextToProgram(CMachineState *pMachineState)
{
	Python python;

	python << _T("nc.attach.attach_end()\n");
	pMachineState->m_attached_to_surface = false;

	return python;
}

HeeksObj *CUnattachOp::MakeACopy(void)const
{
	return new CUnattachOp(*this);
}

void CUnattachOp::CopyFrom(const HeeksObj* object)
{
	if (object->GetType() == GetType())
	{
		operator=(*((CUnattachOp*)object));
	}
}

bool CUnattachOp::CanAddTo(HeeksObj* owner)
{
	return ((owner != NULL) && (owner->GetType() == OperationsType));
}

void CUnattachOp::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element = new TiXmlElement( "UnattachOp" );
	root->LinkEndChild( element );

	COp::WriteBaseXML(element);
}

// static member function
HeeksObj* CUnattachOp::ReadFromXMLElement(TiXmlElement* element)
{
	CUnattachOp* new_object = new CUnattachOp;

	new_object->ReadBaseXML(element);

	return new_object;
}
