// TrsfNCCode.h
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#pragma once

#include "interface/ObjList.h"
#include "HeeksCNCTypes.h"
#include "HeeksCNC.h"

class CTrsfNCCode:public ObjList
{
public:
	double m_x;
	double m_y;

	CTrsfNCCode();
	~CTrsfNCCode();

	void WriteCode(wxTextFile &f);

	// HeeksObj's virtual functions
	int GetType()const{return TrsfNCCodeType;}
	long GetMarkingMask()const{return MARKING_FILTER_UNKNOWN;}
	const wxChar* GetTypeString(void)const{return _T("Transformable NC Code");}
	void GetIcon(int& texture_number, int& x, int& y){GET_ICON(1, 1);}
	const wxBitmap &GetIcon();
	HeeksObj *MakeACopy(void)const;
	void GetProperties(std::list<Property *> *list);
	void WriteXML(TiXmlNode *root);
	bool AutoExpand(){return true;}
	bool OneOfAKind(){return true;}
	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);
	void glCommands(bool select, bool marked, bool no_color);
	void ModifyByMatrix(const double *m);
};
