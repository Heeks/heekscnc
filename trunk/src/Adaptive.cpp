// Adaptive.cpp
/*
 * Copyright (c) 2009, Dan Heeks, Perttu Ahola
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "stdafx.h"
#include "Adaptive.h"
#include "CNCConfig.h"
#include "ProgramCanvas.h"
#include "interface/HeeksObj.h"
#include "interface/PropertyDouble.h"
#include "interface/PropertyChoice.h"
#include "tinyxml/tinyxml.h"
#include "CuttingTool.h"

#include <sstream>

int CAdaptive::number_for_stl_file = 1;


/**
 * If the cutting_tool_number is positive and relates to an existing
 * CuttingTool object then take the toolcornerrad and toolflatrad
 * from the CuttingTool object's values.
 *
 * If the reference_object_type/id refers to either a point object
 * or a Drilling object then use that object's location as this
 * operation's starting point.  If they're negative or zero then
 * use the existing default values.
 */
void CAdaptiveParams::set_initial_values(
		const std::list<int> &solids, 
		const int cutting_tool_number /* = 0 */,
		const int reference_object_type, /* = -1 */ 
		const unsigned int reference_object_id /* = -1 */ )
{
	CNCConfig config;
	config.Read(_T("m_leadoffdz"), &m_leadoffdz, 0.1);
	config.Read(_T("m_leadofflen"), &m_leadofflen, 1.1);
	config.Read(_T("m_leadoffrad"), &m_leadoffrad, 2.0);
	config.Read(_T("m_retractzheight"), &m_retractzheight, 20.0);
	config.Read(_T("m_leadoffsamplestep"), &m_leadoffsamplestep, 0.6);
	config.Read(_T("m_toolcornerrad"), &m_toolcornerrad, 3.0);
	config.Read(_T("m_toolflatrad"), &m_toolflatrad, 0.0);
	config.Read(_T("m_samplestep"), &m_samplestep, 0.4);
	config.Read(_T("m_stepdown"), &m_stepdown, 5.0);
	config.Read(_T("m_clearcuspheight"), &m_clearcuspheight, 1.67);
	config.Read(_T("m_triangleweaveres"), &m_triangleweaveres, 0.51);
	config.Read(_T("m_flatradweaveres"), &m_flatradweaveres, 0.71);
	config.Read(_T("m_dchangright"), &m_dchangright, 0.17);
	config.Read(_T("m_dchangrightoncontour"), &m_dchangrightoncontour, 0.37);
	config.Read(_T("m_dchangleft"), &m_dchangleft, -0.41);
	config.Read(_T("m_dchangefreespace"), &m_dchangefreespace, -0.6);
	config.Read(_T("m_sidecutdisplch"), &m_sidecutdisplch, 0.0);
	config.Read(_T("m_fcut"), &m_fcut, 1000);
	config.Read(_T("m_fretract"), &m_fretract, 5000);
	config.Read(_T("m_thintol"), &m_thintol, 0.0001);
	config.Read(_T("m_startpoint_x"), &m_startpoint_x, 0);
	config.Read(_T("m_startpoint_y"), &m_startpoint_y, 0);
	config.Read(_T("m_startvel_x"), &m_startvel_x, 1);
	config.Read(_T("m_startvel_y"), &m_startvel_y, 1);
	config.Read(_T("m_minz"), &m_minz, -10000000.0);
	config.Read(_T("m_boundaryclear"), &m_boundaryclear, 21);
	config.Read(_T("m_boundary_x0"), &m_boundary_x0, -20);
	config.Read(_T("m_boundary_x1"), &m_boundary_x1, 20);
	config.Read(_T("m_boundary_y0"), &m_boundary_y0, -20);
	config.Read(_T("m_boundary_y1"), &m_boundary_y1, 20);

	// If the user has selected a cutting tool as part of this operation then use that tool's
	// parameters to set these ones.  If no tool was selected then it's back to default
	// behaviour for this module.

	if ((cutting_tool_number > 0) && (CCuttingTool::FindCuttingTool( cutting_tool_number ) > 0))
	{
		CCuttingTool *pCuttingTool = (CCuttingTool *) heeksCAD->GetIDObject( CuttingToolType, CCuttingTool::FindCuttingTool( cutting_tool_number ) );
		if (pCuttingTool != NULL)
		{
			m_toolcornerrad = pCuttingTool->m_params.m_corner_radius;
			m_toolflatrad = pCuttingTool->m_params.m_flat_radius;
		} // End if - then
	} // End if - then

	// The operator has selected a reference object.  We can use that object
	// to determine the starting point for this operation.  The user can always
	// override this point by updating the properties later on.
	if ((reference_object_type > 0) && (reference_object_id > 0))
	{
		HeeksObj *ref = heeksCAD->GetIDObject( reference_object_type, reference_object_id );
		if (ref != NULL)
		{
			double start[3] = {0.0, 0.0, 0.0};
			switch (reference_object_type)
			{
				case PointType:
					if (ref->GetStartPoint( start ))
					{
						m_start_point_x = start[0];
						m_start_point_y = start[1];
					} // End if - then
					break;

				case DrillingType: {
					std::set<CDrilling::Point3d> locations;
					locations = ref->FindAllLocations();
					if (locations.size() == 1)
					{
						// There must be only one (didn't someone else say that once?) for our purposes.
						m_start_point_x = locations.begin()->x;
						m_start_point_y = locations.begin()->y;
					} // End if - then
				   } // End DrillingType scope
					break;

				default:	// We only support a couple of types.
					break;
			} // End switch
		} // End if - then
	} // End if - then

}

void CAdaptiveParams::write_values_to_config()
{
	CNCConfig config;
	config.Write(_T("m_leadoffdz"), m_leadoffdz);
	config.Write(_T("m_leadofflen"), m_leadofflen);
	config.Write(_T("m_leadoffrad"), m_leadoffrad);
	config.Write(_T("m_retractzheight"), m_retractzheight);
	config.Write(_T("m_leadoffsamplestep"), m_leadoffsamplestep);
	config.Write(_T("m_toolcornerrad"), m_toolcornerrad);
	config.Write(_T("m_toolflatrad"), m_toolflatrad);
	config.Write(_T("m_samplestep"), m_samplestep);
	config.Write(_T("m_stepdown"), m_stepdown);
	config.Write(_T("m_clearcuspheight"), m_clearcuspheight);
	config.Write(_T("m_triangleweaveres"), m_triangleweaveres);
	config.Write(_T("m_flatradweaveres"), m_flatradweaveres);
	config.Write(_T("m_dchangright"), m_dchangright);
	config.Write(_T("m_dchangrightoncontour"), m_dchangrightoncontour);
	config.Write(_T("m_dchangleft"), m_dchangleft);
	config.Write(_T("m_dchangefreespace"), m_dchangefreespace);
	config.Write(_T("m_sidecutdisplch"), m_sidecutdisplch);
	config.Write(_T("m_fcut"), m_fcut);
	config.Write(_T("m_fretract"), m_fretract);
	config.Write(_T("m_thintol"), m_thintol);
	config.Write(_T("m_startpoint_x"), m_startpoint_x);
	config.Write(_T("m_startpoint_y"), m_startpoint_y);
	config.Write(_T("m_startvel_x"), m_startvel_x);
	config.Write(_T("m_startvel_y"), m_startvel_y);
	config.Write(_T("m_minz"), m_minz);
	config.Write(_T("m_boundaryclear"), m_boundaryclear);
	config.Write(_T("m_boundary_x0"), m_boundary_x0);
	config.Write(_T("m_boundary_x1"), m_boundary_x1);
	config.Write(_T("m_boundary_y0"), m_boundary_y0);
	config.Write(_T("m_boundary_y1"), m_boundary_y1);
}

static void on_set_leadoffdz(double value, HeeksObj* object){((CAdaptive*)object)->m_params.m_leadoffdz = value;}
static void on_set_leadofflen(double value, HeeksObj* object){((CAdaptive*)object)->m_params.m_leadofflen = value;}
static void on_set_leadoffrad(double value, HeeksObj* object){((CAdaptive*)object)->m_params.m_leadoffrad = value;}
static void on_set_retractzheight(double value, HeeksObj* object){((CAdaptive*)object)->m_params.m_retractzheight = value;}
static void on_set_leadoffsamplestep(double value, HeeksObj* object){((CAdaptive*)object)->m_params.m_leadoffsamplestep = value;}

static void on_set_toolcornerrad(double value, HeeksObj* object)
{
	if ((((COp *)object)->m_cutting_tool_number > 0) &&
	    (CCuttingTool::FindCuttingTool( ((COp *)object)->m_cutting_tool_number ) > 0))
	{
		wxMessageBox(_T("The corner radius will be taken from the cutting tool definition rather than this value.  Aborting change"));
		return;
	} // End if - then
	else
	{
		((CAdaptive*)object)->m_params.m_toolcornerrad = value;
	} // End if - else
}

static void on_set_toolflatrad(double value, HeeksObj* object)
{
	if ((((COp *)object)->m_cutting_tool_number > 0) &&
	    (CCuttingTool::FindCuttingTool( ((COp *)object)->m_cutting_tool_number ) > 0))
	{
		wxMessageBox(_T("The corner radius will be taken from the cutting tool definition rather than this value.  Aborting change"));
		return;
	} // End if - then
	else
	{
		((CAdaptive*)object)->m_params.m_toolflatrad = value;
	} // End if - else
}

static void on_set_samplestep(double value, HeeksObj* object){((CAdaptive*)object)->m_params.m_samplestep = value;}
static void on_set_stepdown(double value, HeeksObj* object){((CAdaptive*)object)->m_params.m_stepdown = value;}
static void on_set_clearcuspheight(double value, HeeksObj* object){((CAdaptive*)object)->m_params.m_clearcuspheight = value;}
static void on_set_triangleweaveres(double value, HeeksObj* object){((CAdaptive*)object)->m_params.m_triangleweaveres = value;}
static void on_set_flatradweaveres(double value, HeeksObj* object){((CAdaptive*)object)->m_params.m_flatradweaveres = value;}
static void on_set_dchangright(double value, HeeksObj* object){((CAdaptive*)object)->m_params.m_dchangright = value;}
static void on_set_dchangrightoncontour(double value, HeeksObj* object){((CAdaptive*)object)->m_params.m_dchangrightoncontour = value;}
static void on_set_dchangleft(double value, HeeksObj* object){((CAdaptive*)object)->m_params.m_dchangleft = value;}
static void on_set_dchangefreespace(double value, HeeksObj* object){((CAdaptive*)object)->m_params.m_dchangefreespace = value;}
static void on_set_sidecutdisplch(double value, HeeksObj* object){((CAdaptive*)object)->m_params.m_sidecutdisplch = value;}
static void on_set_fcut(double value, HeeksObj* object){((CAdaptive*)object)->m_params.m_fcut = value;}
static void on_set_fretract(double value, HeeksObj* object){((CAdaptive*)object)->m_params.m_fretract = value;}
static void on_set_thintol(double value, HeeksObj* object){((CAdaptive*)object)->m_params.m_thintol = value;}
static void on_set_startpoint_x(double value, HeeksObj* object){((CAdaptive*)object)->m_params.m_startpoint_x = value;}
static void on_set_startpoint_y(double value, HeeksObj* object){((CAdaptive*)object)->m_params.m_startpoint_y = value;}
static void on_set_startvel_x(double value, HeeksObj* object){((CAdaptive*)object)->m_params.m_startvel_x = value;}
static void on_set_startvel_y(double value, HeeksObj* object){((CAdaptive*)object)->m_params.m_startvel_y = value;}
static void on_set_minz(double value, HeeksObj* object){((CAdaptive*)object)->m_params.m_minz = value;}
static void on_set_boundaryclear(double value, HeeksObj* object){((CAdaptive*)object)->m_params.m_boundaryclear = value;}
static void on_set_boundary_x0(double value, HeeksObj* object){((CAdaptive*)object)->m_params.m_boundary_x0 = value;}
static void on_set_boundary_x1(double value, HeeksObj* object){((CAdaptive*)object)->m_params.m_boundary_x1 = value;}
static void on_set_boundary_y0(double value, HeeksObj* object){((CAdaptive*)object)->m_params.m_boundary_y0 = value;}
static void on_set_boundary_y1(double value, HeeksObj* object){((CAdaptive*)object)->m_params.m_boundary_y1 = value;}

void CAdaptiveParams::GetProperties(CAdaptive* parent, std::list<Property *> *list)
{
	list->push_back(new PropertyDouble(_("leadoffdz"), m_leadoffdz, parent, on_set_leadoffdz));
	list->push_back(new PropertyDouble(_("leadofflen"), m_leadofflen, parent, on_set_leadofflen));
	list->push_back(new PropertyDouble(_("leadoffrad"), m_leadoffrad, parent, on_set_leadoffrad));
	list->push_back(new PropertyDouble(_("retractzheight"), m_retractzheight, parent, on_set_retractzheight));
	list->push_back(new PropertyDouble(_("leadoffsamplestep"), m_leadoffsamplestep, parent, on_set_leadoffsamplestep));
	list->push_back(new PropertyDouble(_("toolcornerrad"), m_toolcornerrad, parent, on_set_toolcornerrad));
	list->push_back(new PropertyDouble(_("toolflatrad"), m_toolflatrad, parent, on_set_toolflatrad));
	list->push_back(new PropertyDouble(_("samplestep"), m_samplestep, parent, on_set_samplestep));
	list->push_back(new PropertyDouble(_("stepdown"), m_stepdown, parent, on_set_stepdown));
	list->push_back(new PropertyDouble(_("clearcuspheight"), m_clearcuspheight, parent, on_set_clearcuspheight));
	list->push_back(new PropertyDouble(_("triangleweaveres"), m_triangleweaveres, parent, on_set_triangleweaveres));
	list->push_back(new PropertyDouble(_("flatradweaveres"), m_flatradweaveres, parent, on_set_flatradweaveres));
	list->push_back(new PropertyDouble(_("dchangright"), m_dchangright, parent, on_set_dchangright));
	list->push_back(new PropertyDouble(_("dchangrightoncontour"), m_dchangrightoncontour, parent, on_set_dchangrightoncontour));
	list->push_back(new PropertyDouble(_("dchangleft"), m_dchangleft, parent, on_set_dchangleft));
	list->push_back(new PropertyDouble(_("dchangefreespace"), m_dchangefreespace, parent, on_set_dchangefreespace));
	list->push_back(new PropertyDouble(_("sidecutdisplch"), m_sidecutdisplch, parent, on_set_sidecutdisplch));
	list->push_back(new PropertyDouble(_("fcut"), m_fcut, parent, on_set_fcut));
	list->push_back(new PropertyDouble(_("fretract"), m_fretract, parent, on_set_fretract));
	list->push_back(new PropertyDouble(_("thintol"), m_thintol, parent, on_set_thintol));
	list->push_back(new PropertyDouble(_("startpoint_x"), m_startpoint_x, parent, on_set_startpoint_x));
	list->push_back(new PropertyDouble(_("startpoint_y"), m_startpoint_y, parent, on_set_startpoint_y));
	list->push_back(new PropertyDouble(_("startvel_x"), m_startvel_x, parent, on_set_startvel_x));
	list->push_back(new PropertyDouble(_("startvel_y"), m_startvel_y, parent, on_set_startvel_y));
	list->push_back(new PropertyDouble(_("minz"), m_minz, parent, on_set_minz));
	list->push_back(new PropertyDouble(_("boundaryclear"), m_boundaryclear, parent, on_set_boundaryclear));
	if(parent->m_sketches.empty()){
		list->push_back(new PropertyDouble(_("boundary_x0"), m_boundary_x0, parent, on_set_boundary_x0));
		list->push_back(new PropertyDouble(_("boundary_x1"), m_boundary_x1, parent, on_set_boundary_x1));
		list->push_back(new PropertyDouble(_("boundary_y0"), m_boundary_y0, parent, on_set_boundary_y0));
		list->push_back(new PropertyDouble(_("boundary_y1"), m_boundary_y1, parent, on_set_boundary_y1));
	}

}

void CAdaptiveParams::WriteXMLAttributes(TiXmlNode *root)
{
	TiXmlElement * element;
	element = new TiXmlElement( "params" );
	root->LinkEndChild( element );  
	element->SetDoubleAttribute("leadoffdz", m_leadoffdz);
	element->SetDoubleAttribute("leadofflen", m_leadofflen);
	element->SetDoubleAttribute("leadoffrad", m_leadoffrad);
	element->SetDoubleAttribute("retractzheight", m_retractzheight);
	element->SetDoubleAttribute("leadoffsamplestep", m_leadoffsamplestep);
	element->SetDoubleAttribute("toolcornerrad", m_toolcornerrad);
	element->SetDoubleAttribute("toolflatrad", m_toolflatrad);
	element->SetDoubleAttribute("samplestep", m_samplestep);
	element->SetDoubleAttribute("stepdown", m_stepdown);
	element->SetDoubleAttribute("clearcuspheight", m_clearcuspheight);
	element->SetDoubleAttribute("triangleweaveres", m_triangleweaveres);
	element->SetDoubleAttribute("flatradweaveres", m_flatradweaveres);
	element->SetDoubleAttribute("dchangright", m_dchangright);
	element->SetDoubleAttribute("dchangrightoncontour", m_dchangrightoncontour);
	element->SetDoubleAttribute("dchangleft", m_dchangleft);
	element->SetDoubleAttribute("dchangefreespace", m_dchangefreespace);
	element->SetDoubleAttribute("sidecutdisplch", m_sidecutdisplch);
	element->SetDoubleAttribute("fcut", m_fcut);
	element->SetDoubleAttribute("fretract", m_fretract);
	element->SetDoubleAttribute("thintol", m_thintol);
	element->SetDoubleAttribute("startpoint_x", m_startpoint_x);
	element->SetDoubleAttribute("startpoint_y", m_startpoint_y);
	element->SetDoubleAttribute("startvel_x", m_startvel_x);
	element->SetDoubleAttribute("startvel_y", m_startvel_y);
	element->SetDoubleAttribute("minz", m_minz);
	element->SetDoubleAttribute("boundaryclear", m_boundaryclear);
	element->SetDoubleAttribute("boundary_x0", m_boundary_x0);
	element->SetDoubleAttribute("boundary_x1", m_boundary_x1);
	element->SetDoubleAttribute("boundary_y0", m_boundary_y0);
	element->SetDoubleAttribute("boundary_y1", m_boundary_y1);
}

void CAdaptiveParams::ReadFromXMLElement(TiXmlElement* pElem)
{
	// get the attributes
	for(TiXmlAttribute* a = pElem->FirstAttribute(); a; a = a->Next())
	{
		std::string name(a->Name());
		if(name == "leadoffdz"){m_leadoffdz = a->DoubleValue();}
		else if(name == "leadofflen"){m_leadofflen = a->DoubleValue();}
		else if(name == "leadoffrad"){m_leadoffrad = a->DoubleValue();}
		else if(name == "retractzheight"){m_retractzheight = a->DoubleValue();}
		else if(name == "leadoffsamplestep"){m_leadoffsamplestep = a->DoubleValue();}
		else if(name == "toolcornerrad"){m_toolcornerrad = a->DoubleValue();}
		else if(name == "toolflatrad"){m_toolflatrad = a->DoubleValue();}
		else if(name == "samplestep"){m_samplestep = a->DoubleValue();}
		else if(name == "stepdown"){m_stepdown = a->DoubleValue();}
		else if(name == "clearcuspheight"){m_clearcuspheight = a->DoubleValue();}
		else if(name == "triangleweaveres"){m_triangleweaveres = a->DoubleValue();}
		else if(name == "flatradweaveres"){m_flatradweaveres = a->DoubleValue();}
		else if(name == "dchangright"){m_dchangright = a->DoubleValue();}
		else if(name == "dchangrightoncontour"){m_dchangrightoncontour = a->DoubleValue();}
		else if(name == "dchangleft"){m_dchangleft = a->DoubleValue();}
		else if(name == "dchangefreespace"){m_dchangefreespace = a->DoubleValue();}
		else if(name == "sidecutdisplch"){m_sidecutdisplch = a->DoubleValue();}
		else if(name == "fcut"){m_fcut = a->DoubleValue();}
		else if(name == "fretract"){m_fretract = a->DoubleValue();}
		else if(name == "thintol"){m_thintol = a->DoubleValue();}
		else if(name == "startpoint_x"){m_startpoint_x = a->DoubleValue();}
		else if(name == "startpoint_y"){m_startpoint_y = a->DoubleValue();}
		else if(name == "startvel_x"){m_startvel_x = a->DoubleValue();}
		else if(name == "startvel_y"){m_startvel_y = a->DoubleValue();}
		else if(name == "minz"){m_minz = a->DoubleValue();}
		else if(name == "boundaryclear"){m_boundaryclear = a->DoubleValue();}
		else if(name == "boundary_x0"){m_boundary_x0 = a->DoubleValue();}
		else if(name == "boundary_x1"){m_boundary_x1 = a->DoubleValue();}
		else if(name == "boundary_y0"){m_boundary_y0 = a->DoubleValue();}
		else if(name == "boundary_y1"){m_boundary_y1 = a->DoubleValue();}
	}
}

void CAdaptive::AppendTextToProgram()
{
	COp::AppendTextToProgram();

	//write stl file
	std::list<HeeksObj*> solids;
	for(std::list<int>::iterator It = m_solids.begin(); It != m_solids.end(); It++)
	{
		HeeksObj* object = heeksCAD->GetIDObject(SolidType, *It);
		if(object)solids.push_back(object);
	}


#ifdef WIN32

	wxString filepath = wxString::Format(_T("adaptive%d.stl"), number_for_stl_file);

#else
	wxString filepath = wxString::Format(_T("/tmp/adaptive%d.stl"), number_for_stl_file);
#endif


	number_for_stl_file++;
	heeksCAD->SaveSTLFile(solids, filepath);

#ifdef UNICODE
	std::wostringstream ss;
#else
    std::ostringstream ss;
#endif
    ss.imbue(std::locale("C"));

	ss << "rapid(z=" << m_params.m_retractzheight << ")\n";
	ss << "rapid(x=" << m_params.m_startpoint_x << ", y=" << m_params.m_startpoint_y << ")\n";

	ss << "actp.setleadoffdz(" << m_params.m_leadoffdz << ")\n";

	ss << "actp.setleadofflen(" << m_params.m_leadofflen << ")\n";

	ss << "actp.setleadoffrad(" << m_params.m_leadoffrad << ")\n";

	ss << "actp.setretractzheight(" << m_params.m_retractzheight << ")\n";

	ss << "actp.setleadoffsamplestep(" << m_params.m_leadoffsamplestep << ")\n";

	if ((((COp *)this)->m_cutting_tool_number > 0) &&
	    (CCuttingTool::FindCuttingTool( ((COp *)this)->m_cutting_tool_number ) > 0) )
	{
		// We have a cutting tool to refer to.  Get these values from there instead.

		CCuttingTool *pCuttingTool = (CCuttingTool *) heeksCAD->GetIDObject( CuttingToolType, ((COp *)this)->m_cutting_tool_number );
		if (pCuttingTool != NULL)
		{
			ss << "actp.settoolcornerrad(" << pCuttingTool->m_params.m_corner_radius << ")\n";
			ss << "actp.settoolflatrad(" << pCuttingTool->m_params.m_flat_radius << ")\n";
		} // End if - then
	} // End if - then
	else
	{
		// This object has values and/or we don't have a cutting tool number to refer to.
		// Use these values instead.

		ss << "actp.settoolcornerrad(" << m_params.m_toolcornerrad << ")\n";
		ss << "actp.settoolflatrad(" << m_params.m_toolflatrad << ")\n";
	} // End if - else

	ss << "actp.setsamplestep(" << m_params.m_samplestep << ")\n";

	ss << "actp.setstepdown(" << m_params.m_stepdown << ")\n";

	ss << "actp.setclearcuspheight(" << m_params.m_clearcuspheight << ")\n";

	ss << "actp.settriangleweaveres(" << m_params.m_triangleweaveres << ")\n";

	ss << "actp.setflatradweaveres(" << m_params.m_flatradweaveres << ")\n";

	ss << "actp.setdchangright(" << m_params.m_dchangright << ")\n";

	ss << "actp.setdchangrightoncontour(" << m_params.m_dchangrightoncontour << ")\n";

	ss << "actp.setdchangleft(" << m_params.m_dchangleft << ")\n";

	ss << "actp.setdchangefreespace(" << m_params.m_dchangefreespace << ")\n";

	ss << "actp.setsidecutdisplch(" << m_params.m_sidecutdisplch << ")\n";

	ss << "actp.setfcut(" << m_params.m_fcut << ")\n";

	ss << "actp.setfretract(" << m_params.m_fretract << ")\n";

	ss << "actp.setthintol(" << m_params.m_thintol << ")\n";

	ss << "actp.setstartpoint(" << m_params.m_startpoint_x << ", " << m_params.m_startpoint_y << ", " << m_params.m_startvel_x << ", " << m_params.m_startvel_y << ")\n";

	ss << "actp.setminz(" << m_params.m_minz << ")\n";

	ss << "actp.boundaryclear(" << m_params.m_boundaryclear << ")\n";

	if(!m_sketches.empty())
	{
		std::list<HeeksObj*> sketches;
		for(std::list<int>::iterator It = m_sketches.begin(); It != m_sketches.end(); It++)
		{
			HeeksObj* sketch = heeksCAD->GetIDObject(SketchType, *It);
			if(sketch){
				for(HeeksObj* span_object = sketch->GetFirstChild(); span_object; span_object = sketch->GetNextChild())
				{
					double s[3] = {0, 0, 0};
					double e[3] = {0, 0, 0};
					if(span_object){
						int type = span_object->GetType();
						//TODO: add support for arcs
						if(type == LineType)
						{
							span_object->GetStartPoint(s);
							span_object->GetEndPoint(e);
							ss << "actp.boundaryadd(" << s[0] << ", " << s[1] << ")\n";
							ss << "actp.boundaryadd(" << e[0] << ", " << e[1] << ")\n";
						}
					}
				}
			}
			ss << "actp.boundarybreak()\n";
		}
	}
	else
	{
		ss << "actp.boundaryadd(" << m_params.m_boundary_x0 << ", " << m_params.m_boundary_y0 << ")\n";

		ss << "actp.boundaryadd(" << m_params.m_boundary_x0 << ", " << m_params.m_boundary_y1 << ")\n";

		ss << "actp.boundaryadd(" << m_params.m_boundary_x1 << ", " << m_params.m_boundary_y1 << ")\n";

		ss << "actp.boundaryadd(" << m_params.m_boundary_x1 << ", " << m_params.m_boundary_y0 << ")\n";

		ss << "actp.boundarybreak()\n";

	}

	ss << "actp_funcs.cut('" << filepath.c_str() << "')\n";

	ss << "rapid(z=" << m_params.m_retractzheight << ")\n";

	theApp.m_program_canvas->m_textCtrl->AppendText(ss.str().c_str());
}

void CAdaptive::glCommands(bool select, bool marked, bool no_color)
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

void CAdaptive::GetProperties(std::list<Property *> *list)
{
	m_params.GetProperties(this, list);
	COp::GetProperties(list);
}

HeeksObj *CAdaptive::MakeACopy(void)const
{
	return new CAdaptive(*this);
}

void CAdaptive::CopyFrom(const HeeksObj* object)
{
	operator=(*((CAdaptive*)object));
}

bool CAdaptive::CanAddTo(HeeksObj* owner)
{
	return owner->GetType() == OperationsType;
}

void CAdaptive::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element = new TiXmlElement( "Adaptive" );
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
	// write sketch ids
	for(std::list<int>::iterator It = m_sketches.begin(); It != m_sketches.end(); It++)
	{
		int sketch = *It;
		TiXmlElement * sketch_element = new TiXmlElement( "sketch" );
		element->LinkEndChild( sketch_element );  
		sketch_element->SetAttribute("id", sketch);
	}

	WriteBaseXML(element);
}

// static member function
HeeksObj* CAdaptive::ReadFromXMLElement(TiXmlElement* element)
{
	CAdaptive* new_object = new CAdaptive;

	// read solid and sketch ids
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
		else if(name == "sketch"){
			for(TiXmlAttribute* a = pElem->FirstAttribute(); a; a = a->Next())
			{
				std::string name(a->Name());
				if(name == "id"){
					int id = a->IntValue();
					new_object->m_sketches.push_back(id);
				}
			}
		}
	}

	new_object->ReadBaseXML(element);

	return new_object;
}
