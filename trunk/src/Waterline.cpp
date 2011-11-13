// Waterline.cpp
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "stdafx.h"

#ifndef STABLE_OPS_ONLY
#include "Waterline.h"
#include "CNCConfig.h"
#include "ProgramCanvas.h"
#include "interface/PropertyLength.h"
#include "interface/PropertyChoice.h"
#include "tinyxml/tinyxml.h"
#include "Reselect.h"
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include "PythonStuff.h"
#include "CTool.h"
#include "MachineState.h"
#include "Program.h"

#include <sstream>

int CWaterline::number_for_stl_file = 1;

static void on_set_minx(double value, HeeksObj* object){((CWaterline*)object)->m_params.m_box.m_x[0] = value; heeksCAD->Changed();}
static void on_set_maxx(double value, HeeksObj* object){((CWaterline*)object)->m_params.m_box.m_x[3] = value;heeksCAD->Changed();}
static void on_set_miny(double value, HeeksObj* object){((CWaterline*)object)->m_params.m_box.m_x[1] = value;heeksCAD->Changed();}
static void on_set_maxy(double value, HeeksObj* object){((CWaterline*)object)->m_params.m_box.m_x[4] = value;heeksCAD->Changed();}
static void on_set_step_over(double value, HeeksObj* object){((CWaterline*)object)->m_params.m_step_over = value;heeksCAD->Changed();}
static void on_set_material_allowance(double value, HeeksObj* object){((CWaterline*)object)->m_params.m_material_allowance = value;heeksCAD->Changed();}
static void on_set_tolerance(double value, HeeksObj* object){((CWaterline*)object)->m_params.m_tolerance = value;heeksCAD->Changed();}

void CWaterlineParams::GetProperties(CWaterline* parent, std::list<Property *> *list)
{
	list->push_back(new PropertyLength(_("minimum x"), m_box.m_x[0], parent, on_set_minx));
	list->push_back(new PropertyLength(_("maximum x"), m_box.m_x[3], parent, on_set_maxx));
	list->push_back(new PropertyLength(_("minimum y"), m_box.m_x[1], parent, on_set_miny));
	list->push_back(new PropertyLength(_("maximum y"), m_box.m_x[4], parent, on_set_maxy));
	list->push_back(new PropertyLength(_("step over"), m_step_over, parent, on_set_step_over));
	list->push_back(new PropertyLength(_("material allowance"), m_material_allowance, parent, on_set_material_allowance));
	list->push_back(new PropertyLength(_("tolerance"), m_tolerance, parent, on_set_tolerance));
}

void CWaterlineParams::WriteXMLAttributes(TiXmlNode *root)
{
	TiXmlElement * element;
	element = heeksCAD->NewXMLElement( "params" );
	heeksCAD->LinkXMLEndChild( root,  element );
	element->SetDoubleAttribute( "minx", m_box.m_x[0]);
	element->SetDoubleAttribute( "maxx", m_box.m_x[3]);
	element->SetDoubleAttribute( "miny", m_box.m_x[1]);
	element->SetDoubleAttribute( "maxy", m_box.m_x[4]);
	element->SetDoubleAttribute( "step_over", m_step_over);
	element->SetDoubleAttribute( "material_allowance", m_material_allowance);
	element->SetDoubleAttribute( "tolerance", m_tolerance);
}

void CWaterlineParams::ReadFromXMLElement(TiXmlElement* pElem)
{
	// get the attributes
	pElem->Attribute("minx", &m_box.m_x[0]);
	pElem->Attribute("maxx", &m_box.m_x[3]);
	pElem->Attribute("miny", &m_box.m_x[1]);
	pElem->Attribute("maxy", &m_box.m_x[4]);
	pElem->Attribute("step_over", &m_step_over);
	pElem->Attribute("material_allowance", &m_material_allowance);
	pElem->Attribute("tolerance", &m_tolerance);
}

CWaterline::CWaterline(const std::list<int> &solids, const int tool_number)
    :CDepthOp(GetTypeString(), NULL, tool_number, WaterlineType), m_solids(solids)
{
	ReadDefaultValues();

	// set m_box from the extents of the solids
	for(std::list<int>::const_iterator It = solids.begin(); It != solids.end(); It++)
	{
		int solid = *It;
		HeeksObj* object = heeksCAD->GetIDObject(SolidType, solid);
		if(object)
		{
			if(object->GetType() == StlSolidType)
			{
				object->GetBox(m_params.m_box);
			}
			else
			{
				double extents[6];
				if(heeksCAD->BodyGetExtents(object, extents))
				{
					m_params.m_box.Insert(CBox(extents));
				}
			}

			Add(object, NULL);
		}
	}
	m_solids.clear();

	SetDepthOpParamsFromBox();

	// add tool radius all around the box
	if(m_params.m_box.m_valid)
	{
		CTool *pTool = CTool::Find(m_tool_number);
		if(pTool)
		{
			double extra = pTool->m_params.m_diameter/2 + 0.01;
			m_params.m_box.m_x[0] -= extra;
			m_params.m_box.m_x[1] -= extra;
			m_params.m_box.m_x[3] += extra;
			m_params.m_box.m_x[4] += extra;
		}
	}
}

CWaterline::CWaterline( const CWaterline & rhs ) : CDepthOp(rhs)
{
	m_solids.clear();
    std::copy( rhs.m_solids.begin(), rhs.m_solids.end(), std::inserter( m_solids, m_solids.begin() ) );

    m_params = rhs.m_params;
    // static int number_for_stl_file;
}

CWaterline & CWaterline::operator= ( const CWaterline & rhs )
{
	if (this != &rhs)
	{
		CDepthOp::operator =(rhs);

		m_solids.clear();
		std::copy( rhs.m_solids.begin(), rhs.m_solids.end(), std::inserter( m_solids, m_solids.begin() ) );

		m_params = rhs.m_params;
		// static int number_for_stl_file;
	}

	return(*this);
}

const wxBitmap &CWaterline::GetIcon()
{
	if(!m_active)return GetInactiveIcon();
	static wxBitmap* icon = NULL;
	if(icon == NULL)icon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/waterline.png")));
	return *icon;
}

/**
	The old version of the CWaterline object stored references to graphics as type/id pairs
	that get read into the m_solids list.  The new version stores these graphics references
	as child elements (based on ObjList).  If we read in an old-format file then the m_solids
	list will have data in it for which we don't have children.  This routine converts
	these type/id pairs into the HeeksObj pointers as children.
 */
void CWaterline::ReloadPointers()
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

	CDepthOp::ReloadPointers();
}

void CWaterline::SetDepthOpParamsFromBox()
{
	m_depth_op_params.m_start_depth = m_params.m_box.MaxZ();
	m_depth_op_params.ClearanceHeight( m_params.m_box.MaxZ() + 5.0 );
	m_depth_op_params.m_final_depth = m_params.m_box.MinZ();
	m_depth_op_params.m_rapid_safety_space = m_params.m_box.MaxZ() + 2.0;
	m_depth_op_params.m_step_down = m_params.m_box.Depth(); // set it to a finishing pass
}

Python CWaterline::AppendTextToProgram(CMachineState *pMachineState)
{
	Python python;

    ReloadPointers();   // Make sure all the solids in m_solids are included as child objects.

	CTool *pTool = CTool::Find(m_tool_number);
	if(pTool == NULL)
	{
		return(python);
	}

	python << CDepthOp::AppendTextToProgram(pMachineState);

	// write the corner radius
	python << _T("corner_radius = float(");
	double cr = pTool->m_params.m_corner_radius - pTool->m_params.m_flat_radius;
	if(cr<0)cr = 0.0;
	python << ( cr / theApp.m_program->m_units ) << _T(")\n");

	heeksCAD->CreateUndoPoint();

	//write stl file
	std::list<HeeksObj*> solids;
	for (HeeksObj *object = GetFirstChild(); object != NULL; object = GetNextChild())
	{
	    if (object->GetType() != SolidType && object->GetType() != StlSolidType)
	    {
	        continue;
	    }

		if (object != NULL)
		{
			// Need to rotate a COPY of the solid by the fixture settings.
			HeeksObj* copy = object->MakeACopy();
			if (copy != NULL)
			{
				double m[16];	// A different form of the transformation matrix.
				CFixture::extract( pMachineState->Fixture().GetMatrix(CFixture::YZ), m );
                copy->ModifyByMatrix(m);

                CFixture::extract( pMachineState->Fixture().GetMatrix(CFixture::XZ), m );
                copy->ModifyByMatrix(m);

                CFixture::extract( pMachineState->Fixture().GetMatrix(CFixture::XY), m );
                copy->ModifyByMatrix(m);

                solids.push_back(copy);
            } // End if - then
        } // End if - then
	} // End for


    wxStandardPaths standard_paths;
    wxFileName filepath( standard_paths.GetTempDir().c_str(), wxString::Format(_T("waterline%d.stl"), number_for_stl_file).c_str() );
	number_for_stl_file++;

	heeksCAD->SaveSTLFile(solids, filepath.GetFullPath(), m_params.m_tolerance);

	// We don't need the duplicate solids any more.  Delete them.
	for (std::list<HeeksObj*>::iterator l_itSolid = solids.begin(); l_itSolid != solids.end(); l_itSolid++)
	{
		heeksCAD->Remove( *l_itSolid );
	} // End for
	heeksCAD->Changed();

    python << _T("ocl_funcs.waterline( filepath = ") << PythonString(filepath.GetFullPath()) << _T(", ")
            << _T("tool_diameter = ") << pTool->CuttingRadius() * 2.0 << _T(", ")
            << _T("corner_radius = ") << pTool->m_params.m_corner_radius / theApp.m_program->m_units << _T(", ")
            << _T("step_over = ") << m_params.m_step_over / theApp.m_program->m_units << _T(", ")
            << _T("mat_allowance = ") << m_params.m_material_allowance / theApp.m_program->m_units << _T(", ")
            << _T("clearance = clearance, ")
            << _T("rapid_safety_space = rapid_safety_space, ")
            << _T("start_depth = start_depth, ")
            << _T("step_down = step_down, ")
            << _T("final_depth = final_depth, ")
            << _T("units = ") << theApp.m_program->m_units << _T(", ")
            << _T("x0 = ") << m_params.m_box.m_x[0] / theApp.m_program->m_units << _T(", ")
            << _T("y0 = ") << m_params.m_box.m_x[1] / theApp.m_program->m_units << _T(", ")
            << _T("x1 = ") << m_params.m_box.m_x[3] / theApp.m_program->m_units << _T(", ")
            << _T("y1 = ") << m_params.m_box.m_x[4] / theApp.m_program->m_units << _T(", ")
            << _T("tolerance = ") << m_params.m_tolerance << _T(")\n");

	return(python);
}

void CWaterline::glCommands(bool select, bool marked, bool no_color)
{
	CDepthOp::glCommands(select, marked, no_color);
}

void CWaterline::GetProperties(std::list<Property *> *list)
{
	AddSolidsProperties(list, this);
	m_params.GetProperties(this, list);
	CDepthOp::GetProperties(list);
}

HeeksObj *CWaterline::MakeACopy(void)const
{
	return new CWaterline(*this);
}

void CWaterline::CopyFrom(const HeeksObj* object)
{
	operator=(*((CWaterline*)object));
}

bool CWaterline::CanAddTo(HeeksObj* owner)
{
	return ((owner != NULL) && (owner->GetType() == OperationsType));
}

void CWaterline::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element = heeksCAD->NewXMLElement( "Waterline" );
	heeksCAD->LinkXMLEndChild( root,  element );
	m_params.WriteXMLAttributes(element);

	// write solid ids
	for (HeeksObj *object = GetFirstChild(); object != NULL; object = GetNextChild())
	{
		if (object->GetIDGroupType() != SolidType)continue;
		int solid = object->GetID();
		TiXmlElement * solid_element = heeksCAD->NewXMLElement( "solid" );
		heeksCAD->LinkXMLEndChild( element, solid_element );
		solid_element->SetAttribute("id", solid);
	}

	WriteBaseXML(element);
}

// static member function
HeeksObj* CWaterline::ReadFromXMLElement(TiXmlElement* element)
{
	CWaterline* new_object = new CWaterline;

	std::list<TiXmlElement *> elements_to_remove;

	// read solid ids
	for(TiXmlElement* pElem = heeksCAD->FirstXMLChildElement( element ) ; pElem; pElem = pElem->NextSiblingElement())
	{
		std::string name(pElem->Value());
		if(name == "params"){
			new_object->m_params.ReadFromXMLElement(pElem);
			elements_to_remove.push_back(pElem);
		}
		else if(name == "solid"){
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
		heeksCAD->RemoveXMLChild( element, *itElem);
	}

	new_object->ReadBaseXML(element);

	return new_object;
}

bool CWaterline::CanAdd(HeeksObj* object)
{
    if (object == NULL) return(false);

	switch (object->GetType())
	{
	case SolidType:
	case SketchType:
	case FixtureType:
		return(true);

	default:
		return(false);
	}
}

void CWaterline::WriteDefaultValues()
{
	CDepthOp::WriteDefaultValues();

	CNCConfig config(ConfigScope());
	config.Write(wxString(GetTypeString()) + _T("BoxXMin"), m_params.m_box.m_x[0]);
	config.Write(wxString(GetTypeString()) + _T("BoxYMin"), m_params.m_box.m_x[1]);
	config.Write(wxString(GetTypeString()) + _T("BoxXMax"), m_params.m_box.m_x[3]);
	config.Write(wxString(GetTypeString()) + _T("BoxYMax"), m_params.m_box.m_x[4]);
	config.Write(wxString(GetTypeString()) + _T("StepOver"), m_params.m_step_over);
	config.Write(wxString(GetTypeString()) + _T("MatAllowance"), m_params.m_material_allowance);
	config.Write(wxString(GetTypeString()) + _T("Tolerance"), m_params.m_tolerance);
}

void CWaterline::ReadDefaultValues()
{
	CDepthOp::ReadDefaultValues();

	CNCConfig config(ConfigScope());
	config.Read(wxString(GetTypeString()) + _T("BoxXMin"), &m_params.m_box.m_x[0], -7.0);
	config.Read(wxString(GetTypeString()) + _T("BoxYMin"), &m_params.m_box.m_x[1], -7.0);
	config.Read(wxString(GetTypeString()) + _T("BoxXMax"), &m_params.m_box.m_x[3], 7.0);
	config.Read(wxString(GetTypeString()) + _T("BoxYMax"), &m_params.m_box.m_x[4], 7.0);
	config.Read(wxString(GetTypeString()) + _T("StepOver"), &m_params.m_step_over, 1.0);
	config.Read(wxString(GetTypeString()) + _T("MatAllowance"), &m_params.m_material_allowance, 0.0);
	config.Read(wxString(GetTypeString()) + _T("Tolerance"), &m_params.m_tolerance, 0.01);
}


class ResetBoundary: public Tool {
public:
	// Tool's virtual functions
	const wxChar* GetTitle(){return _("Reset Boundary");}
	void Run()
	{
	    CBox bounding_box;

	    for (HeeksObj *object = m_pThis->GetFirstChild(); object != NULL; object = m_pThis->GetNextChild())
	    {
                object->GetBox(bounding_box);
		}

        // add tool radius all around the box
        if(bounding_box.m_valid)
        {
            CTool *pTool = CTool::Find(m_pThis->m_tool_number);
            if(pTool)
            {
                double extra = pTool->m_params.m_diameter/2 + 0.01;
                bounding_box.m_x[0] -= extra;
                bounding_box.m_x[1] -= extra;
                bounding_box.m_x[3] += extra;
                bounding_box.m_x[4] += extra;
            }
        }

	    m_pThis->m_params.m_box = bounding_box;
		m_pThis->SetDepthOpParamsFromBox();
	}

public:
	void Set( CWaterline *pThis )
	{
	    m_pThis = pThis;
	}

	wxString BitmapPath(){ return _T("import");}
	CWaterline *m_pThis;
};

static ResetBoundary reset_boundary;



static ReselectSolids reselect_solids;

void CWaterline::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
	reselect_solids.m_solids = &m_solids;
	reselect_solids.m_object = this;
	t_list->push_back(&reselect_solids);

	reset_boundary.Set(this);
	t_list->push_back(&reset_boundary);

	CDepthOp::GetTools( t_list, p );
}
bool CWaterlineParams::operator==( const CWaterlineParams & rhs ) const
{
	if (m_box != rhs.m_box) return(false);
	if (m_step_over != rhs.m_step_over) return(false);
	if (m_material_allowance != rhs.m_material_allowance) return(false);

	return(true);
}

bool CWaterline::operator==( const CWaterline & rhs ) const
{
	if (m_params != rhs.m_params) return(false);

	// std::list<int> m_solids;

	return(CDepthOp::operator==(rhs));
}
#endif
