// Pattern.h
/*
 * Copyright (c) 2013, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#pragma once

#include "interface/HeeksObj.h"
#include "HeeksCNCTypes.h"

class CPattern: public HeeksObj {
public:
	wxString m_title;
	bool m_title_made_from_id;
	gp_Trsf m_trsf;
	int m_number_of_copies;
	int m_sub_pattern;

	//	Constructors.
	CPattern();
	CPattern(const wxString &title, const gp_Trsf &trsf, int number_of_copies):m_title(title), m_title_made_from_id(true), m_trsf(trsf), m_number_of_copies(number_of_copies){}

	// HeeksObj's virtual functions
    int GetType()const{return PatternType;}
	const wxChar* GetTypeString(void) const{ return _T("Pattern"); }
    HeeksObj *MakeACopy(void)const;

    void WriteXML(TiXmlNode *root);
    static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);

	void GetProperties(std::list<Property *> *list);
	void CopyFrom(const HeeksObj* object);
	bool CanAddTo(HeeksObj* owner);
	const wxBitmap &GetIcon();
    const wxChar* GetShortString(void)const;
    bool CanEditString(void)const{return true;}
    void OnEditString(const wxChar* str);

	void GetMatrices(std::list<gp_Trsf> &matrices);
}; // End CPattern class definition.

