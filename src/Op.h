// Op.h
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#ifndef OP_HEADER
#define OP_HEADER

#include "interface/HeeksObj.h"

class COp : public HeeksObj
{
public:
	wxString m_comment;
	bool m_active; // don't make NC code, if this is not active
	wxString m_title;

	COp(const wxString& title):m_active(true), m_title(title){}

	// HeeksObj's virtual functions
	void GetProperties(std::list<Property *> *list);
	void WriteBaseXML(TiXmlElement *element);
	void ReadBaseXML(TiXmlElement* element);
	wxString GetIcon(){return theApp.GetResFolder() + _T("/icons/noentry");}
	const wxChar* GetShortString(void)const{return m_title.c_str();}
	bool CanEditString(void)const{return true;}
	void OnEditString(const wxChar* str);

	virtual void AppendTextToProgram();

	static bool IsAnOperation(int object_type);
};

#endif

