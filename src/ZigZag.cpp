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
#include "Reselect.h"

#include <sstream>

int CZigZag::number_for_stl_file = 1;

static void on_set_minx(double value, HeeksObj* object){((CZigZag*)object)->m_params.m_box.m_x[0] = value;}
static void on_set_maxx(double value, HeeksObj* object){((CZigZag*)object)->m_params.m_box.m_x[3] = value;}
static void on_set_miny(double value, HeeksObj* object){((CZigZag*)object)->m_params.m_box.m_x[1] = value;}
static void on_set_maxy(double value, HeeksObj* object){((CZigZag*)object)->m_params.m_box.m_x[4] = value;}
static void on_set_z0(double value, HeeksObj* object){((CZigZag*)object)->m_params.m_box.m_x[2] = value;}
static void on_set_z1(double value, HeeksObj* object){((CZigZag*)object)->m_params.m_box.m_x[5] = value;}
static void on_set_step_over(double value, HeeksObj* object){((CZigZag*)object)->m_params.m_step_over = value;}
static void on_set_direction(int value, HeeksObj* object){((CZigZag*)object)->m_params.m_direction = value;}
static void on_set_library(int value, HeeksObj* object){((CZigZag*)object)->m_params.m_lib = value;}

void CZigZagParams::GetProperties(CZigZag* parent, std::list<Property *> *list)
{
	list->push_back(new PropertyDouble(_("minimum x"), m_box.m_x[0], parent, on_set_minx));
	list->push_back(new PropertyDouble(_("maximum x"), m_box.m_x[3], parent, on_set_maxx));
	list->push_back(new PropertyDouble(_("minimum y"), m_box.m_x[1], parent, on_set_miny));
	list->push_back(new PropertyDouble(_("maximum y"), m_box.m_x[4], parent, on_set_maxy));
	list->push_back(new PropertyDouble(_("z0"), m_box.m_x[2], parent, on_set_z0));
	list->push_back(new PropertyDouble(_("z1"), m_box.m_x[5], parent, on_set_z1));
	list->push_back(new PropertyDouble(_("step over"), m_step_over, parent, on_set_step_over));
	{
		std::list< wxString > choices;
		choices.push_back(_("X"));
		choices.push_back(_("Y"));
		list->push_back(new PropertyChoice(_("direction"), choices, m_direction, parent, on_set_direction));
	}
	{
		std::list< wxString > choices;
		choices.push_back(_("pycam"));
		choices.push_back(_("OpenCamLib"));
		list->push_back(new PropertyChoice(_("library"), choices, m_lib, parent, on_set_library));
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
	element->SetDoubleAttribute("step_over", m_step_over);
	element->SetAttribute("dir", m_direction);
	element->SetAttribute("lib", m_lib);
}

void CZigZagParams::ReadFromXMLElement(TiXmlElement* pElem)
{
	// get the attributes
	pElem->Attribute("minx", &m_box.m_x[0]);
	pElem->Attribute("maxx", &m_box.m_x[3]);
	pElem->Attribute("miny", &m_box.m_x[1]);
	pElem->Attribute("maxy", &m_box.m_x[4]);
	pElem->Attribute("z0", &m_box.m_x[2]);
	pElem->Attribute("z1", &m_box.m_x[5]);
	pElem->Attribute("step_over", &m_step_over);
	pElem->Attribute("dir", &m_direction);
	pElem->Attribute("lib", &m_lib);
}

CZigZag::CZigZag(const std::list<int> &solids, const int cutting_tool_number)
    :CDepthOp(GetTypeString(), NULL, cutting_tool_number, ZigZagType), m_solids(solids)
{
	ReadDefaultValues();

	// set m_box from the extents of the solids
	for(std::list<int>::const_iterator It = solids.begin(); It != solids.end(); It++)
	{
		int solid = *It;
		HeeksObj* object = heeksCAD->GetIDObject(SolidType, solid);
		if(object)
		{
			object->GetBox(m_params.m_box);
			Add(object, NULL);
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

CZigZag::CZigZag( const CZigZag & rhs ) : CDepthOp(rhs)
{
	*this = rhs;	// Call the assignment operator.
}

CZigZag & CZigZag::operator= ( const CZigZag & rhs )
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

/**
	The old version of the CZigZag object stored references to graphics as type/id pairs
	that get read into the m_solids list.  The new version stores these graphics references
	as child elements (based on ObjList).  If we read in an old-format file then the m_solids
	list will have data in it for which we don't have children.  This routine converts
	these type/id pairs into the HeeksObj pointers as children.
 */
void CZigZag::ReloadPointers()
{
	for (std::list<int>::iterator symbol = m_solids.begin(); symbol != m_solids.end(); symbol++)
	{
		HeeksObj *object = heeksCAD->GetIDObject( SolidType, *symbol );
		if (object != NULL)
		{
			Add( object, NULL );
		}
	}

	CDepthOp::ReloadPointers();
}


void CZigZag::AppendTextToProgram(const CFixture *pFixture)
{
    ReloadPointers();   // Make sure all the solids in m_solids are included as child objects.

	CCuttingTool *pCuttingTool = CCuttingTool::Find(m_cutting_tool_number);
	if(pCuttingTool == NULL)
	{
		return;
	}

	CDepthOp::AppendTextToProgram(pFixture);

	heeksCAD->CreateUndoPoint();

	//write stl file
	std::list<HeeksObj*> solids;
	for (HeeksObj *object = GetFirstChild(); object != NULL; object = GetNextChild())
	{
	    if (object->GetType() != SolidType)
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
				CFixture::extract( pFixture->GetMatrix(), m );

				int type = copy->GetType();
				unsigned int id = copy->m_id;

                copy->ModifyByMatrix(m);
                solids.push_back(copy);
            } // End if - then
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

			// Rotate the coordinates to align with the fixture.
			gp_Pnt min = pFixture->Adjustment( gp_Pnt( m_params.m_box.m_x[0], m_params.m_box.m_x[1], m_params.m_box.m_x[2] ) );
			gp_Pnt max = pFixture->Adjustment( gp_Pnt( m_params.m_box.m_x[3], m_params.m_box.m_x[4], m_params.m_box.m_x[5] ) );

	switch(this->m_params.m_lib)
	{
	case 0: // pycam
		{
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

			// def GenerateToolPath(self, minx, maxx, miny, maxy, z0, z1, dx, dy, direction):
			double dx, dy;
			if(m_params.m_direction)
			{
				dx = m_params.m_step_over;
				dy = 0.1;
			}
			else
			{
				dx = 0.1;
				dy = m_params.m_step_over;
			}
			ss << "pathlist = pg.GenerateToolPath(" << min.X() << ", " << max.X() << ", " << min.Y() << ", " << max.Y() << ", " << min.Z() << ", " << max.Z() << ", " << dx << ", " << dy << "," <<m_params.m_direction<< ")\n";

			ss << "h = HeeksCNCExporter(" << m_params.m_box.m_x[5] << ")\n";

			ss << "h.AddPathList(pathlist)\n";
		} // end of case 0
		break;

	case 1:// OpenCamLib
		{
			ss << "s = ocl.STLSurf('" << filepath.c_str() << "')\n";
			ss << "dcf = ocl.PathDropCutterFinish(s)\n";
			ss << "cutter = ocl.CylCutter(" << pCuttingTool->m_params.m_diameter << ")\n";
			ss << "dcf.setCutter(cutter)\n";
			ss << "dcf.minimumZ = " << min.Z() << "\n";
			if(m_params.m_direction)ss << "steps = " << (int)((max.X() - min.X())/m_params.m_step_over) + 1 << "\n";
			else ss << "steps = " << (int)((max.Y() - min.Y())/m_params.m_step_over) + 1 << "\n";
			if(m_params.m_direction)ss << "step_over = " << (max.X() - min.X()) << " / steps\n";
			else ss << "step_over = " << (max.Y() - min.Y()) << " / steps\n";
			ss << "minx = " << min.X() << "\n";
			ss << "miny = " << min.Y() << "\n";
			ss << "maxx = " << max.X() << "\n";
			ss << "maxy = " << max.Y() << "\n";
			if(m_params.m_direction)ss << "prev_x = " << min.X() << "\n";
			else ss << "prev_y = " << min.Y() << "\n";
			ss << "for i in range(0, steps + 1):\n";
			if(m_params.m_direction)ss << " x = minx + float(i) * step_over\n";
			else ss << " y = miny + float(i) * step_over\n";
			ss << " path = ocl.Path()\n";
			if(m_params.m_direction)ss << " path.append(ocl.Line(ocl.Point(x, miny, 0), ocl.Point(x, maxy, 0)))\n";
			else ss << " path.append(ocl.Line(ocl.Point(minx, y, 0), ocl.Point(maxx, y, 0)))\n";
			ss << " dcf.setPath(path)\n";
			ss << " dcf.run()\n";
			ss << " plist = dcf.getCLPoints()\n";
			ss << " n = 0\n";
			ss << " for p in plist:\n";
			ss << "  if n == 0:\n";
			ss << "   rapid(p.x, p.y)\n";
			ss << "   rapid(z = p.z + 5)\n";
			ss << "   feed(z = p.z)\n";
			ss << "  else:\n";
			ss << "   feed(p.x, p.y, p.z)\n";
			ss << "  n = n + 1\n";
			ss << " rapid(z = " << max.Z() + 5 << ")\n";
		}
		break;

	default:
		break;
	}

	theApp.m_program_canvas->m_textCtrl->AppendText(ss.str().c_str());
}

void CZigZag::glCommands(bool select, bool marked, bool no_color)
{
	CDepthOp::glCommands(select, marked, no_color);
}

void CZigZag::GetProperties(std::list<Property *> *list)
{
	AddSolidsProperties(list, m_solids);
	m_params.GetProperties(this, list);
	CDepthOp::GetProperties(list);
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

	std::list<TiXmlElement *> elements_to_remove;

	// read solid ids
	for(TiXmlElement* pElem = TiXmlHandle(element).FirstChildElement().Element(); pElem; pElem = pElem->NextSiblingElement())
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
		element->RemoveChild(*itElem);
	}

	new_object->ReadBaseXML(element);

	return new_object;
}

bool CZigZag::CanAdd(HeeksObj* object)
{
	switch (object->GetType())
	{
	case SolidType:
	case SketchType:
		return(true);

	default:
		return(false);
	}
}

void CZigZag::WriteDefaultValues()
{
	CDepthOp::WriteDefaultValues();

	CNCConfig config(ConfigScope());
	config.Write(wxString(GetTypeString()) + _T("BoxXMin"), m_params.m_box.m_x[0]);
	config.Write(wxString(GetTypeString()) + _T("BoxYMin"), m_params.m_box.m_x[1]);
	config.Write(wxString(GetTypeString()) + _T("BoxZMin"), m_params.m_box.m_x[2]);
	config.Write(wxString(GetTypeString()) + _T("BoxXMax"), m_params.m_box.m_x[3]);
	config.Write(wxString(GetTypeString()) + _T("BoxYMax"), m_params.m_box.m_x[4]);
	config.Write(wxString(GetTypeString()) + _T("BoxZMax"), m_params.m_box.m_x[5]);
	config.Write(wxString(GetTypeString()) + _T("StepOver"), m_params.m_step_over);
	config.Write(wxString(GetTypeString()) + _T("Direction"), m_params.m_direction);
	config.Write(wxString(GetTypeString()) + _T("Lib"), m_params.m_lib);
}

void CZigZag::ReadDefaultValues()
{
	CDepthOp::ReadDefaultValues();

	CNCConfig config(ConfigScope());
	config.Read(wxString(GetTypeString()) + _T("BoxXMin"), &m_params.m_box.m_x[0], -7.0);
	config.Read(wxString(GetTypeString()) + _T("BoxYMin"), &m_params.m_box.m_x[1], -7.0);
	config.Read(wxString(GetTypeString()) + _T("BoxZMin"), &m_params.m_box.m_x[2], 0.0);
	config.Read(wxString(GetTypeString()) + _T("BoxXMax"), &m_params.m_box.m_x[3], 7.0);
	config.Read(wxString(GetTypeString()) + _T("BoxYMax"), &m_params.m_box.m_x[4], 7.0);
	config.Read(wxString(GetTypeString()) + _T("BoxZMax"), &m_params.m_box.m_x[5], 10.0);
	config.Read(wxString(GetTypeString()) + _T("StepOver"), &m_params.m_step_over, 1.0);
	config.Read(wxString(GetTypeString()) + _T("Direction"), &m_params.m_direction, 0);
	config.Read(wxString(GetTypeString()) + _T("Lib"), &m_params.m_lib, 1);
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
            CCuttingTool *pCuttingTool = CCuttingTool::Find(m_pThis->m_cutting_tool_number);
            if(pCuttingTool)
            {
                double extra = pCuttingTool->m_params.m_diameter/2 + 0.01;
                bounding_box.m_x[0] -= extra;
                bounding_box.m_x[1] -= extra;
                bounding_box.m_x[3] += extra;
                bounding_box.m_x[4] += extra;
            }
        }

	    m_pThis->m_params.m_box = bounding_box;
	}

public:
	void Set( CZigZag *pThis )
	{
	    m_pThis = pThis;
	}

	wxString BitmapPath(){ return _T("import");}
	CZigZag *m_pThis;
};

static ResetBoundary reset_boundary;



static ReselectSolids reselect_solids;

void CZigZag::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
	reselect_solids.m_solids = &m_solids;
	reselect_solids.m_object = this;
	t_list->push_back(&reselect_solids);

	reset_boundary.Set(this);
	t_list->push_back(&reset_boundary);

	CDepthOp::GetTools( t_list, p );
}
