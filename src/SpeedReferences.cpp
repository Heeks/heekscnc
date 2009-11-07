// SpeedReferences.cpp

#include "stdafx.h"
#include "tinyxml/tinyxml.h"
#include "ProgramCanvas.h"
#include "CNCConfig.h"
#include "interface/PropertyString.h"
#include "interface/PropertyChoice.h"
#include "interface/PropertyDouble.h"
#include "interface/PropertyLength.h"
#include "interface/Tool.h"
#include "CuttingTool.h"
#include "Op.h"
#include "SpeedOp.h"
#include "interface/strconv.h"
#include <wx/stdpaths.h>

#include <vector>
#include <algorithm>
#include <fstream>
#include <memory>
using namespace std;

class ExportSpeedReferences: public Tool{
	// Tool's virtual functions
	const wxChar* GetTitle(){return _("Export");}
	void Run()
	{
		wxStandardPaths standard_paths;
		if (previous_path.Length() == 0) previous_path = _T("default.speeds");

		// Prompt the user to select a file to import.
		wxFileDialog fd(heeksCAD->GetMainFrame(), _T("Select a file to export to"),
				standard_paths.GetUserConfigDir().c_str(), previous_path.c_str(),
				wxString(_("Known Files")) + _T(" |*.heeks;*.HEEKS;")
					+ _T("*.speed;*.SPEED;*.Speed;")
					+ _T("*.feed;*.FEED;*.Feed;")
					+ _T("*.feedsnspeeds;*.FEEDSNSPEEDS;"), 
					wxSAVE | wxOVERWRITE_PROMPT );

		fd.SetFilterIndex(1);
		if (fd.ShowModal() == wxID_CANCEL) return;
		previous_path = fd.GetPath().c_str();
		std::list<HeeksObj *> speed_references;
		for (HeeksObj *speed_reference = theApp.m_program->SpeedReferences()->GetFirstChild();
			speed_reference != NULL;
			speed_reference = theApp.m_program->SpeedReferences()->GetNextChild() )
		{
			speed_references.push_back( speed_reference );
		} // End for

		heeksCAD->SaveXMLFile( speed_references, previous_path.c_str(), false );
	}
	wxString BitmapPath(){ return _T("export");}
	wxString previous_path;
};

static ExportSpeedReferences export_speed_references;

class ImportSpeedReferences: public Tool{
	// Tool's virtual functions
	const wxChar* GetTitle(){return _("Import");}
	void Run()
	{
		wxStandardPaths standard_paths;
		if (previous_path.Length() == 0) previous_path = _T("default.speeds");

		// Prompt the user to select a file to import.
		wxFileDialog fd(heeksCAD->GetMainFrame(), _T("Select a file to import"),
				standard_paths.GetUserConfigDir().c_str(), previous_path.c_str(),
				wxString(_("Known Files")) + _T(" |*.heeks;*.HEEKS;")
					+ _T("*.speed;*.SPEED;*.Speed;")
					+ _T("*.feed;*.FEED;*.Feed;")
					+ _T("*.feedsnspeeds;*.FEEDSNSPEEDS;"), 
					wxOPEN | wxFILE_MUST_EXIST );
		fd.SetFilterIndex(1);
		if (fd.ShowModal() == wxID_CANCEL) return;
		previous_path = fd.GetPath().c_str();

		// Delete the speed references that we've already got.  Otherwise we end
		// up with duplicates.  Do this in two passes.  Otherwise we end up
		// traversing the same list that we're modifying.

		std::list<HeeksObj *> speed_references;
		for (HeeksObj *speed_reference = theApp.m_program->SpeedReferences()->GetFirstChild();
			speed_reference != NULL;
			speed_reference = theApp.m_program->SpeedReferences()->GetNextChild() )
		{
			speed_references.push_back( speed_reference );
		} // End for

		for (std::list<HeeksObj *>::iterator l_itObject = speed_references.begin(); l_itObject != speed_references.end(); l_itObject++)
		{
			heeksCAD->Remove( *l_itObject );
		} // End for

		// And read the default speed references.
		// heeksCAD->OpenXMLFile( _T("default.speeds"), true, theApp.m_program->m_speed_references );
		heeksCAD->OpenXMLFile( previous_path.c_str(), theApp.m_program->SpeedReferences() );
	}
	wxString BitmapPath(){ return _T("import");}
	wxString previous_path;
};

static ImportSpeedReferences import_speed_references;

void CSpeedReferences::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
	t_list->push_back(&import_speed_references);
	t_list->push_back(&export_speed_references);

	ObjList::GetTools(t_list, p);
}

void CSpeedReferences::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element;
	element = new TiXmlElement( "SpeedReferences" );
	root->LinkEndChild( element );
	element->SetAttribute("estimate_when_possible", int(m_estimate_when_possible?1:0) );
	WriteBaseXML(element);
}

HeeksObj* CSpeedReferences::ReadFromXMLElement(TiXmlElement* pElem)
{
	CSpeedReferences* new_object = new CSpeedReferences();
	if (pElem->Attribute("estimate_when_possible"))
	{
		new_object->m_estimate_when_possible = (atoi(pElem->Attribute("estimate_when_possible"))!=0);
	} // End if - then
	else
	{
		// It's not in the file.  The default value will have come from the config values anyway.
	} // End if - else
	new_object->ReadBaseXML(pElem);
	return new_object;
}
 
static void on_set_estimate_when_possible(int value, HeeksObj* object)
{
	((CSpeedReferences *)object)->m_estimate_when_possible = (value == 0) ? true: false;

	CNCConfig config;
	config.Write(_T("SpeedReferences_m_estimate_when_possible"), int(((CSpeedReferences *)object)->m_estimate_when_possible?1:0));
}

void CSpeedReferences::GetProperties(std::list<Property *> *list)
{
	{
		std::list< wxString > choices;
		int choice = 0;
		choices.push_back( _("Estimate when possible") );		// true
		choices.push_back( _("Use default values") );		// false

		if (m_estimate_when_possible) 
		{
			choice = 0;
		} // End if - then
		else
		{
			choice = 1;
		} // End if - else

		list->push_back ( new PropertyChoice ( _("Feeds and Speeds"),  choices, choice, this, on_set_estimate_when_possible ) );
	}
	HeeksObj::GetProperties(list);
}


/**
	This method finds a distinct set of material names from the SpeedReferences list.
 */
std::set< wxString > CSpeedReferences::GetMaterials()
{
	std::set< wxString > materials;
	if ((theApp.m_program) && (theApp.m_program->SpeedReferences()))
	{
		for (HeeksObj *material = theApp.m_program->SpeedReferences()->GetFirstChild();
			material != NULL;
			material = theApp.m_program->SpeedReferences()->GetNextChild())
		{
			if (material->GetType() != SpeedReferenceType) continue;

			materials.insert( ((CSpeedReference *)material)->m_material_name );
		} // End for
	} // End if - then

	return(materials);
} // End GetMaterials() method

/**
	Return a set of harndess values that are valid for the chosen material name.
 */
std::set< double > CSpeedReferences::GetHardnessForMaterial( const wxString & material_name )
{
	std::set< double > hardness_values;
	if ((theApp.m_program) && (theApp.m_program->SpeedReferences()))
	{
		for (HeeksObj *material = theApp.m_program->SpeedReferences()->GetFirstChild();
			material != NULL;
			material = theApp.m_program->SpeedReferences()->GetNextChild())
		{
			if (material->GetType() != SpeedReferenceType) continue;

			if (material_name == ((CSpeedReference *) material)->m_material_name)
			{
				hardness_values.insert( ((CSpeedReference *) material)->m_brinell_hardness_of_raw_material );
			} // End if - then
		} // End for
	} // End if - then

	return(hardness_values);

} // End of GetHardnessForMaterial() method

/**
	Return a set of harndess values that have already been configured.
 */
std::set< double > CSpeedReferences::GetAllHardnessValues()
{
	std::set< double > hardness_values;
	if ((theApp.m_program) && (theApp.m_program->SpeedReferences()))
	{
		for (HeeksObj *material = theApp.m_program->SpeedReferences()->GetFirstChild();
			material != NULL;
			material = theApp.m_program->SpeedReferences()->GetNextChild())
		{
			if (material->GetType() != SpeedReferenceType) continue;
			hardness_values.insert( ((CSpeedReference *) material)->m_brinell_hardness_of_raw_material );
		} // End for
	} // End if - then

	return(hardness_values);

} // End of GetAllHardnessValues() method

double CSpeedReferences::GetSurfaceSpeed( 
	const wxString & material_name, 
	const int cutting_tool_material, 
	const double brinell_hardness_of_raw_material )
{
	if (theApp.m_program == NULL) return(-1.0);
	if (theApp.m_program->SpeedReferences() == NULL) return(-1.0);

	for (HeeksObj *material = theApp.m_program->SpeedReferences()->GetFirstChild();
		material != NULL;
		material = theApp.m_program->SpeedReferences()->GetNextChild())
	{
		if (material->GetType() != SpeedReferenceType) continue;
		if ((material_name == ((CSpeedReference *) material)->m_material_name) &&
		    (cutting_tool_material == ((CSpeedReference *) material)->m_cutting_tool_material) &&
		    (brinell_hardness_of_raw_material == ((CSpeedReference *) material)->m_brinell_hardness_of_raw_material))
		{
			return( ((CSpeedReference *) material)->m_surface_speed );
		} // End if - then
	} // End for

	// The data doesn't exist for this combination of inputs.
	return(-1.0);
	
} // End GetSurfaceSpeed() method


