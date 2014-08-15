// Pattern.h
/*
 * Copyright (c) 2013, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#pragma once

#include "interface/IdNamedObj.h"
#include "HeeksCNCTypes.h"

class CPattern: public IdNamedObj {
public:
	int m_copies1;
	double m_x_shift1;
	double m_y_shift1;
	int m_copies2;
	double m_x_shift2;
	double m_y_shift2;

	//	Constructors.
	CPattern();
	CPattern(int copies1, double x_shift1, double y_shift1, int copies2, double x_shift2, double y_shift2):m_copies1(copies1), m_x_shift1(x_shift1), m_y_shift1(y_shift1), m_copies2(copies2), m_x_shift2(x_shift2), m_y_shift2(y_shift2){}

	// HeeksObj's virtual functions
	int GetType() const { return PatternType; }
	const wxChar* GetTypeString(void) const { return _("Pattern"); }
	HeeksObj *MakeACopy(void) const;
	void WriteXML(TiXmlNode *root);
	void GetProperties(std::list<Property *> *list);
	void CopyFrom(const HeeksObj* object);
	bool CanAddTo(HeeksObj* owner);
	const wxBitmap &GetIcon();
	void GetOnEdit(bool(**callback)(HeeksObj*));
	HeeksObj* PreferredPasteTarget();

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);

	void GetMatrices(std::list<gp_Trsf> &matrices);
}; // End CPattern class definition.
