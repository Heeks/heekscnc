// Probing.cpp
/*
 * Copyright (c) 2009, Dan Heeks, Perttu Ahola
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "stdafx.h"
#include "Probing.h"
#include "CNCConfig.h"
#include "ProgramCanvas.h"
#include "interface/HeeksObj.h"
#include "interface/PropertyInt.h"
#include "interface/PropertyDouble.h"
#include "interface/PropertyLength.h"
#include "interface/PropertyChoice.h"
#include "interface/Tool.h"
#include "tinyxml/tinyxml.h"
#include "Operations.h"
#include "CuttingTool.h"
#include "Profile.h"
#include "Fixture.h"
#include "CNCPoint.h"
#include "PythonStuff.h"

#include <sstream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <memory>

#include <gp_Ax1.hxx>

extern CHeeksCADInterface* heeksCAD;


static void on_set_starting_angle(double value, HeeksObj* object)
{
	((CProbe_LinearCentre_Outside*)object)->m_starting_angle = value;
}

static void on_set_starting_distance(double value, HeeksObj* object)
{
	((CProbe_LinearCentre_Outside*)object)->m_starting_distance = value;
}

static void on_set_depth(double value, HeeksObj* object)
{
	((CProbe_LinearCentre_Outside*)object)->m_depth = value;
}

void CProbe_LinearCentre_Outside::AppendTextToProgram( const CFixture *pFixture )
{
	CSpeedOp::AppendTextToProgram( pFixture );

#ifdef UNICODE
	std::wostringstream ss;
#else
	std::ostringstream ss;
#endif
	ss.imbue(std::locale("C"));
	ss<<std::setprecision(10);

	// We're going to be working in relative coordinates based on the assumption
	// that the operator has first jogged the machine to the approximate centre point.

	ss << "comment('This program assumes that the machine operator has jogged')\n";
	ss << "comment('the machine to approximatedly the correct location')\n";
	ss << "comment('immediately above the protrusion we are finding the centre of.')\n";
	ss << "comment('This program then jogs out and down in two opposite directions')\n";
	ss << "comment('before probing back towards the centre point looking for the')\n";
	ss << "comment('protrusion.')\n";

	CNCPoint first(m_starting_distance,0,0);
	CNCPoint second(m_starting_distance,0,0);

	gp_Dir x_direction(1,0,0);

	double first_angle_in_radians = (m_starting_angle / 360) * (2 * PI);	// degrees expressed in radians
	while (first_angle_in_radians < 0) first_angle_in_radians += (2 * PI);
	gp_Trsf first_rotation_matrix;
	first_rotation_matrix.SetRotation( gp_Ax1( gp_Pnt(0,0,0), x_direction), first_angle_in_radians );
	first.Transform(first_rotation_matrix);

	double second_angle_in_radians = first_angle_in_radians + PI;	// half a circle around.
	while (second_angle_in_radians < 0) second_angle_in_radians += (2 * PI);
	gp_Trsf second_rotation_matrix;
	second_rotation_matrix.SetRotation( gp_Ax1( gp_Pnt(0,0,0), x_direction), second_angle_in_radians );
	second.Transform(second_rotation_matrix);

	ss << "probe_linear_centre_outside("
		<< "x1=" << first.X(true) << ", "
		<< "y1=" << first.Y(true) << ", "
		<< "depth=" << m_depth / theApp.m_program->m_units << ", "
		<< "x2=" << second.X(true) << ", "
		<< "y2=" << second.Y(true) << ", "
		<< "xml_file_name='" << this->GetOutputFileName(wxString(_(".xml"))).c_str() << "'"
		<< ")\n";

	theApp.m_program_canvas->m_textCtrl->AppendText(ss.str().c_str());
}


void CProbing::GetProperties(std::list<Property *> *list)
{
	CSpeedOp::GetProperties(list);
}

void CProbe_LinearCentre_Outside::GetProperties(std::list<Property *> *list)
{
	list->push_back(new PropertyDouble(_("Starting angle"), m_starting_angle, this, on_set_starting_angle));
	list->push_back(new PropertyLength(_("Starting distance"), m_starting_distance, this, on_set_starting_distance));
	list->push_back(new PropertyLength(_("Depth"), m_depth, this, on_set_depth));
	CProbing::GetProperties(list);
}

HeeksObj *CProbe_LinearCentre_Outside::MakeACopy(void)const
{
	return new CProbe_LinearCentre_Outside(*this);
}

void CProbe_LinearCentre_Outside::CopyFrom(const HeeksObj* object)
{
	operator=(*((CProbe_LinearCentre_Outside*)object));
}

bool CProbe_LinearCentre_Outside::CanAddTo(HeeksObj* owner)
{
	return owner->GetType() == OperationsType;
}

void CProbe_LinearCentre_Outside::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element = new TiXmlElement( "Probe_LinearCentre_Outside" );
	root->LinkEndChild( element );  

	element->SetDoubleAttribute("starting_angle", m_starting_angle);
	element->SetDoubleAttribute("starting_distance", m_starting_distance);
	element->SetDoubleAttribute("depth", m_depth);

	WriteBaseXML(element);
}

// static member function
HeeksObj* CProbe_LinearCentre_Outside::ReadFromXMLElement(TiXmlElement* element)
{
	CProbe_LinearCentre_Outside* new_object = new CProbe_LinearCentre_Outside();

	for(TiXmlElement* pElem = TiXmlHandle(element).FirstChildElement().Element(); pElem; pElem = pElem->NextSiblingElement())
	{
		if (pElem->Attribute("starting_angle")) new_object->m_starting_angle = atof(pElem->Attribute("starting_angle"));
		if (pElem->Attribute("starting_distance")) new_object->m_starting_distance = atof(pElem->Attribute("starting_distance"));
		if (pElem->Attribute("depth")) new_object->m_depth = atof(pElem->Attribute("depth"));
	}

	new_object->ReadBaseXML(element);

	return new_object;
}

void CProbe_LinearCentre_Outside::glCommands(bool select, bool marked, bool no_color)
{
}


wxString CProbe_LinearCentre_Outside::GetOutputFileName(const wxString extension)
{

	wxString file_name = theApp.m_program->GetOutputFileName();

	// Remove the extension.
	int offset = 0;
	if ((offset = file_name.Find(_("."))) != -1)
	{
		file_name.Remove(offset);
	}

	file_name << _("_Probe_Linear_Centre_Outside_id_");
	file_name << m_id;
	file_name << extension;

	return(file_name);
}


class Probe_LinearCentre_Outside_GenerateGCode: public Tool 
{

CProbe_LinearCentre_Outside *m_pThis;

public:
	Probe_LinearCentre_Outside_GenerateGCode() { m_pThis = NULL; }

	// Tool's virtual functions
	const wxChar* GetTitle(){return _("Generate GCode");}

	void Run()
	{
		// We must setup the theApp.m_program_canvas->m_textCtrl variable before
		// calling the HeeksPyPostProcess() routine.  That's the python script
		// that will be executed.

		theApp.m_program_canvas->m_textCtrl->Clear();

		// add standard stuff at the top
		//hackhack, make it work on unix with FHS
#ifndef WIN32
		theApp.m_program_canvas->AppendText(_T("import sys\n"));
		theApp.m_program_canvas->AppendText(_T("sys.path.insert(0,'/usr/local/lib/heekscnc/')\n"));
#endif

		// machine general stuff
		theApp.m_program_canvas->AppendText(_T("from nc.nc import *\n"));

		// specific machine
		if (theApp.m_program->m_machine.file_name == _T("not found"))
		{
			wxMessageBox(_T("Machine name (defined in Program Properties) not found"));
		} // End if - then
		else
		{
			theApp.m_program_canvas->AppendText(_T("import nc.") + theApp.m_program->m_machine.file_name + _T("\n"));
			theApp.m_program_canvas->AppendText(_T("\n"));
		} // End if - else

		// output file
		theApp.m_program_canvas->AppendText(_T("output('") + m_pThis->GetOutputFileName(_(".tap")) + _T("')\n"));

		// begin program
		theApp.m_program_canvas->AppendText(_T("program_begin(123, 'Test program')\n"));
		theApp.m_program_canvas->AppendText(_T("absolute()\n"));

		if(theApp.m_program->m_units > 25.0)
		{
			theApp.m_program_canvas->AppendText(_T("imperial()\n"));
		}
		else
		{
			theApp.m_program_canvas->AppendText(_T("metric()\n"));
		}
		theApp.m_program_canvas->AppendText(_T("set_plane(0)\n"));
		theApp.m_program_canvas->AppendText(_T("\n"));

		CFixture default_fixture(NULL, CFixture::G54 );
		m_pThis->AppendTextToProgram( &default_fixture );

		theApp.m_program_canvas->AppendText(_T("program_end()\n"));

		{
			// clear the output file
			wxFile f(m_pThis->GetOutputFileName(_(".tap")).c_str(), wxFile::write);
			if(f.IsOpened())f.Write(_T("\n"));
		}

		HeeksPyPostProcess(theApp.m_program, m_pThis->GetOutputFileName(_(".tap")), false );
	}
	wxString BitmapPath(){ return _T("export");}
	wxString previous_path;
	void Set( CProbe_LinearCentre_Outside *pThis ) { m_pThis = pThis; }
};

static Probe_LinearCentre_Outside_GenerateGCode generate_gcode;

void CProbe_LinearCentre_Outside::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{

	generate_gcode.Set( this );

	t_list->push_back( &generate_gcode );
}

