// Surface.h
/*
 * Copyright (c) 2013, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#pragma once

#include "interface/HeeksObj.h"
#include "HeeksCNCTypes.h"

class CSurface: public HeeksObj {
public:
	wxString m_title;
	bool m_title_made_from_id;
	std::list<int> m_solids;
	double m_tolerance;
	double m_min_z;
	double m_material_allowance;
	bool m_same_for_each_pattern_position;
	static int number_for_stl_file;

	//	Constructors.
	CSurface();
	CSurface(const std::list<int> &solids, double tol, double min_z):m_title_made_from_id(true), m_solids(solids), m_tolerance(tol), m_min_z(min_z), m_same_for_each_pattern_position(true){}

	// HeeksObj's virtual functions
    int GetType()const{return SurfaceType;}
	const wxChar* GetTypeString(void) const{ return _T("Surface"); }
    HeeksObj *MakeACopy(void)const;

    void WriteXML(TiXmlNode *root);
    static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);

	void GetProperties(std::list<Property *> *list);
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);
	void CopyFrom(const HeeksObj* object);
	bool CanAddTo(HeeksObj* owner);
	const wxBitmap &GetIcon();
    const wxChar* GetShortString(void)const;
    bool CanEditString(void)const{return true;}
    void OnEditString(const wxChar* str);

	const wxString ConfigScope(void)const{return _T("Surface");}

	void WriteDefaultValues();
	void ReadDefaultValues();

}; // End CSurface class definition.

