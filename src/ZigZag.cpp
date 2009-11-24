// ZigZag.cpp
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "stdafx.h"
#include "ZigZag.h"
#include "CNCConfig.h"
#include "ProgramCanvas.h"
#include "interface/PropertyDouble.h"
#include "interface/PropertyChoice.h"
#include "tinyxml/tinyxml.h"

#include <sstream>

int CZigZag::number_for_stl_file = 1;

static void on_set_minx(double value, HeeksObj* object){((CZigZag*)object)->m_params.m_box.m_x[0] = value;}
static void on_set_maxx(double value, HeeksObj* object){((CZigZag*)object)->m_params.m_box.m_x[3] = value;}
static void on_set_miny(double value, HeeksObj* object){((CZigZag*)object)->m_params.m_box.m_x[1] = value;}
static void on_set_maxy(double value, HeeksObj* object){((CZigZag*)object)->m_params.m_box.m_x[4] = value;}
static void on_set_z0(double value, HeeksObj* object){((CZigZag*)object)->m_params.m_box.m_x[2] = value;}
static void on_set_z1(double value, HeeksObj* object){((CZigZag*)object)->m_params.m_box.m_x[5] = value;}
static void on_set_dx(double value, HeeksObj* object){((CZigZag*)object)->m_params.m_dx = value;}
static void on_set_dy(double value, HeeksObj* object){((CZigZag*)object)->m_params.m_dy = value;}
static void on_set_direction(int value, HeeksObj* object){((CZigZag*)object)->m_params.m_direction = value;}

void CZigZagParams::GetProperties(CZigZag* parent, std::list<Property *> *list)
{
	list->push_back(new PropertyDouble(_("minimum x"), m_box.m_x[0], parent, on_set_minx));
	list->push_back(new PropertyDouble(_("maximum x"), m_box.m_x[3], parent, on_set_maxx));
	list->push_back(new PropertyDouble(_("minimum y"), m_box.m_x[1], parent, on_set_miny));
	list->push_back(new PropertyDouble(_("maximum y"), m_box.m_x[4], parent, on_set_maxy));
	list->push_back(new PropertyDouble(_("z0"), m_box.m_x[2], parent, on_set_z0));
	list->push_back(new PropertyDouble(_("z1"), m_box.m_x[5], parent, on_set_z1));
	list->push_back(new PropertyDouble(_("dx"), m_dx, parent, on_set_dx));
	list->push_back(new PropertyDouble(_("dy"), m_dy, parent, on_set_dy));
	{
		std::list< wxString > choices;
		choices.push_back(_("X"));
		choices.push_back(_("Y"));
		list->push_back(new PropertyChoice(_("direction"), choices, m_direction, parent, on_set_direction));
	}
}

void CZigZagParams::WriteXMLAttributes(TiXmlNode *root)
{
	TiXmlElement * element;
	element = new TiXmlElement( "params" );
	root->LinkEndChild( element );  
	element->SetDoubleAttribute("minx", m_box.m_x[0]);
	element->SetDoubleAttribute("maxx", m_box.m_x[3]);
	element->SetDoubleAttribute("miny", m_box.m_x[1]);
	element->SetDoubleAttribute("maxy", m_box.m_x[4]);
	element->SetDoubleAttribute("z0", m_box.m_x[2]);
	element->SetDoubleAttribute("z1", m_box.m_x[5]);
	element->SetDoubleAttribute("dx", m_dx);
	element->SetDoubleAttribute("dy", m_dy);
	element->SetAttribute("dir", m_direction);
}

void CZigZagParams::ReadFromXMLElement(TiXmlElement* pElem)
{
	// get the attributes
	for(TiXmlAttribute* a = pElem->FirstAttribute(); a; a = a->Next())
	{
		std::string name(a->Name());
		if(name == "minx"){m_box.m_x[0] = a->DoubleValue();}
		else if(name == "maxx"){m_box.m_x[3] = a->DoubleValue();}
		else if(name == "miny"){m_box.m_x[1] = a->DoubleValue();}
		else if(name == "maxy"){m_box.m_x[4] = a->DoubleValue();}
		else if(name == "z0"){m_box.m_x[2] = a->DoubleValue();}
		else if(name == "z1"){m_box.m_x[5] = a->DoubleValue();}
		else if(name == "dx"){m_dx = a->DoubleValue();}
		else if(name == "dy"){m_dy = a->DoubleValue();}
		else if(name == "dir"){m_direction = a->IntValue();}
	}
}

CZigZag::CZigZag(const std::list<int> &solids, const int cutting_tool_number):CSpeedOp(GetTypeString(), cutting_tool_number), m_solids(solids)
{
	ReadDefaultValues();

	// set m_box from the extents of the solids
	for(std::list<int>::const_iterator It = solids.begin(); It != solids.end(); It++)
	{
		int solid = *It;
		HeeksObj* object = heeksCAD->GetIDObject(SolidType, solid);
		if(object)
		{
			double extents[6];
			if(heeksCAD->BodyGetExtents(object, extents))
			{
				m_params.m_box.Insert(CBox(extents));
			}
		}
	}

	// add tool radius all around the box
	if(m_params.m_box.m_valid)
	{
		CCuttingTool *pCuttingTool = CCuttingTool::Find(m_cutting_tool_number);
		if(pCuttingTool)
		{
			double extra = pCuttingTool->m_params.m_diameter/2 + 0.01;
			m_params.m_box.m_x[0] -= extra;
			m_params.m_box.m_x[1] -= extra;
			m_params.m_box.m_x[3] += extra;
			m_params.m_box.m_x[4] += extra;
		}
	}
}

void CZigZag::AppendTextToProgram(const CFixture *pFixture)
{
	CCuttingTool *pCuttingTool = CCuttingTool::Find(m_cutting_tool_number);
	if(pCuttingTool == NULL)
	{
		return;
	}

	CSpeedOp::AppendTextToProgram(pFixture);

	heeksCAD->CreateUndoPoint();

	//write stl file
	std::list<HeeksObj*> solids;
	for(std::list<int>::iterator It = m_solids.begin(); It != m_solids.end(); It++)
	{
		HeeksObj* object = heeksCAD->GetIDObject(SolidType, *It);
		if (object != NULL)
		{
			// Need to rotate a COPY of the solid by the fixture settings.
			HeeksObj* copy = object->MakeACopy();
			if (copy != NULL)
			{
				double m[16];	// A different form of the transformation matrix.
				CFixture::extract( pFixture->GetMatrix(), m );

				int type = copy->GetType();
				unsigned int id = copy->m_id;

				if (copy->ModifyByMatrix(m))
				{
					// The modification has resulted in a new HeeksObj that uses
					// the same ID as the old one.  We just need to renew our
					// HeeksObj pointer so that we use (and delete) the right one later on.

					copy = heeksCAD->GetIDObject( type, id );
				} // End if - then
			} // End if - then

			if(copy) solids.push_back(copy);
		} // End if - then
	} // End for

#ifdef WIN32
	wxString filepath = wxString::Format(_T("zigzag%d.stl"), number_for_stl_file);
#else
        wxString filepath = wxString::Format(_T("/tmp/zigzag%d.stl"), number_for_stl_file);
#endif
	number_for_stl_file++;

	heeksCAD->SaveSTLFile(solids, filepath);

	// We don't need the duplicate solids any more.  Delete them.
	for (std::list<HeeksObj*>::iterator l_itSolid = solids.begin(); l_itSolid != solids.end(); l_itSolid++)
	{
		heeksCAD->Remove( *l_itSolid );
	} // End for
	heeksCAD->Changed();

#ifdef UNICODE
	std::wostringstream ss;
#else
    std::ostringstream ss;
#endif
    ss.imbue(std::locale("C"));

	switch(pCuttingTool->m_params.m_type){
	case CCuttingToolParams::eBallEndMill:
	case CCuttingToolParams::eSlotCutter:
	case CCuttingToolParams::eEndmill:
		if(fabs(pCuttingTool->m_params.m_corner_radius - pCuttingTool->m_params.m_diameter/2) < 0.0000001)
		{
			ss << "c = SphericalCutter(" << pCuttingTool->m_params.m_diameter/2 << ", Point(0,0,7))\n";
		}
		else if(fabs(pCuttingTool->m_params.m_corner_radius) < 0.0000001)
		{
			ss << "c = CylindricalCutter(" << pCuttingTool->m_params.m_diameter/2 << ", Point(0,0,7))\n";
		}
		else
		{
			ss << "c = ToroidalCutter(" << pCuttingTool->m_params.m_diameter/2 << ", " << pCuttingTool->m_params.m_corner_radius << ", Point(0,0,7))\n";
		}
		break;
	default:
		wxMessageBox(_("invalid tool type"));
		break;
	};
 
    ss << "model = ImportModel('" << filepath.c_str() << "')\n";

    ss << "pg = DropCutter(c, model)\n";

	// Rotate the coordinates to align with the fixture.
	gp_Pnt min = pFixture->Adjustment( gp_Pnt( m_params.m_box.m_x[0], m_params.m_box.m_x[1], m_params.m_box.m_x[2] ) );
	gp_Pnt max = pFixture->Adjustment( gp_Pnt( m_params.m_box.m_x[3], m_params.m_box.m_x[4], m_params.m_box.m_x[5] ) );

	// def GenerateToolPath(self, minx, maxx, miny, maxy, z0, z1, dx, dy, direction):
    ss << "pathlist = pg.GenerateToolPath(" << min.X() << ", " << max.X() << ", " << min.Y() << ", " << max.Y() << ", " << min.Z() << ", " << max.Z() << ", " << m_params.m_dx << ", " << m_params.m_dy << "," <<m_params.m_direction<< ")\n";

    ss << "h = HeeksCNCExporter(" << m_params.m_box.m_x[5] << ")\n";

    ss << "h.AddPathList(pathlist)\n";

	theApp.m_program_canvas->m_textCtrl->AppendText(ss.str().c_str());
}

void CZigZag::glCommands(bool select, bool marked, bool no_color)
{
	if(0 && marked && !no_color)
	{
		for(std::list<int>::iterator It = m_solids.begin(); It != m_solids.end(); It++)
		{
			int solid = *It;
			HeeksObj* object = heeksCAD->GetIDObject(SolidType, solid);
			if(object)object->glCommands(false, true, false);
		}
	}
}

void CZigZag::GetProperties(std::list<Property *> *list)
{
	m_params.GetProperties(this, list);
	COp::GetProperties(list);
}

HeeksObj *CZigZag::MakeACopy(void)const
{
	return new CZigZag(*this);
}

void CZigZag::CopyFrom(const HeeksObj* object)
{
	operator=(*((CZigZag*)object));
}

bool CZigZag::CanAddTo(HeeksObj* owner)
{
	return owner->GetType() == OperationsType;
}

void CZigZag::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element = new TiXmlElement( "ZigZag" );
	root->LinkEndChild( element );  
	m_params.WriteXMLAttributes(element);

	// write solid ids
	for(std::list<int>::iterator It = m_solids.begin(); It != m_solids.end(); It++)
	{
		int solid = *It;
		TiXmlElement * solid_element = new TiXmlElement( "solid" );
		element->LinkEndChild( solid_element );  
		solid_element->SetAttribute("id", solid);
	}

	WriteBaseXML(element);
}

// static member function
HeeksObj* CZigZag::ReadFromXMLElement(TiXmlElement* element)
{
	CZigZag* new_object = new CZigZag;

	// read solid ids
	for(TiXmlElement* pElem = TiXmlHandle(element).FirstChildElement().Element(); pElem; pElem = pElem->NextSiblingElement())
	{
		std::string name(pElem->Value());
		if(name == "params"){
			new_object->m_params.ReadFromXMLElement(pElem);
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
		}
	}

	new_object->ReadBaseXML(element);

	return new_object;
}

void CZigZag::WriteDefaultValues()
{
	CSpeedOp::WriteDefaultValues();

	CNCConfig config;
	config.Write(wxString(GetTypeString()) + _T("BoxXMin"), m_params.m_box.m_x[0]);
	config.Write(wxString(GetTypeString()) + _T("BoxYMin"), m_params.m_box.m_x[1]);
	config.Write(wxString(GetTypeString()) + _T("BoxZMin"), m_params.m_box.m_x[2]);
	config.Write(wxString(GetTypeString()) + _T("BoxXMax"), m_params.m_box.m_x[3]);
	config.Write(wxString(GetTypeString()) + _T("BoxYMax"), m_params.m_box.m_x[4]);
	config.Write(wxString(GetTypeString()) + _T("BoxZMax"), m_params.m_box.m_x[5]);
	config.Write(wxString(GetTypeString()) + _T("DX"), m_params.m_dx);
	config.Write(wxString(GetTypeString()) + _T("DY"), m_params.m_dy);
	config.Write(wxString(GetTypeString()) + _T("Direction"), m_params.m_direction);
}

void CZigZag::ReadDefaultValues()
{
	CSpeedOp::ReadDefaultValues();

	CNCConfig config;
	config.Read(wxString(GetTypeString()) + _T("BoxXMin"), &m_params.m_box.m_x[0], -7.0);
	config.Read(wxString(GetTypeString()) + _T("BoxYMin"), &m_params.m_box.m_x[1], -7.0);
	config.Read(wxString(GetTypeString()) + _T("BoxZMin"), &m_params.m_box.m_x[2], 0.0);
	config.Read(wxString(GetTypeString()) + _T("BoxXMax"), &m_params.m_box.m_x[3], 7.0);
	config.Read(wxString(GetTypeString()) + _T("BoxYMax"), &m_params.m_box.m_x[4], 7.0);
	config.Read(wxString(GetTypeString()) + _T("BoxZMax"), &m_params.m_box.m_x[5], 10.0);
	config.Read(wxString(GetTypeString()) + _T("DX"), &m_params.m_dx, 1.0);
	config.Read(wxString(GetTypeString()) + _T("DY"), &m_params.m_dy, 1.0);
	config.Read(wxString(GetTypeString()) + _T("Direction"), &m_params.m_direction, 0);
}
