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
	wxString GetIcon(){return theApp.GetResFolder() + _T("/icons/program");}
	HeeksObj *MakeACopy(void)const;
	void GetProperties(std::list<Property *> *list);
	void WriteXML(TiXmlNode *root);
	bool AutoExpand(){return true;}
	bool OneOfAKind(){return true;}
	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);
	void glCommands(bool select, bool marked, bool no_color);
	bool ModifyByMatrix(const double *m);
};
