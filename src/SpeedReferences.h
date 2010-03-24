// SpeedReferences.h
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

// one of these stores all the operations, and which machine it is for, and stuff like clamping info if I ever get round to it

#pragma once

#include "interface/Tool.h"
#include "interface/ObjList.h"
#include "HeeksCNCTypes.h"
#include "RawMaterial.h"
#include "HeeksCNC.h"
#include "SpeedReference.h"
#include "CNCConfig.h"

class CSpeedReferences: public ObjList{
public:
	bool m_estimate_when_possible;	// flag to turn feeds and speeds estimation on and off.
	static wxString ConfigScope(void) {return _T("SpeedReferences");}

	CSpeedReferences()
	{
		CNCConfig config(ConfigScope());
                int value;
                config.Read(_T("SpeedReferences_m_estimate_when_possible"), &value, 1);
                m_estimate_when_possible = (value != 0);
	}

	// HeeksObj's virtual functions
	bool OneOfAKind(){return true;}
	int GetType()const{return SpeedReferencesType;}
	const wxChar* GetTypeString(void)const{return _("Feeds and Speeds");}
	HeeksObj *MakeACopy(void)const{ return new CSpeedReferences(*this);}
	void GetIcon(int& texture_number, int& x, int& y){GET_ICON(3, 1);}
	bool CanAddTo(HeeksObj* owner){return owner->GetType() == ProgramType;}
	bool CanAdd(HeeksObj* object) {	return((object->GetType() == SpeedReferenceType) || (object->GetType() == CuttingRateType)); }
	bool CanBeRemoved(){return false;}
	void WriteXML(TiXmlNode *root);
	bool AutoExpand(){return false;}
	void GetProperties(std::list<Property *> *list);
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);
	void CopyFrom(const HeeksObj* object);

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);

	static std::set< wxString > GetMaterials();
	static std::set< double > GetHardnessForMaterial( const wxString & material_name );
	static std::set< double > GetAllHardnessValues();

	static double GetSurfaceSpeed( const wxString & material_name,
					const int cutting_tool_material,
					const double brinell_hardness_of_raw_material );
};

