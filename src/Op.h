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

	COp():m_active(true){}

	// HeeksObj's virtual functions
	void GetProperties(std::list<Property *> *list);
	void WriteBaseXML(TiXmlElement *element);
	void ReadBaseXML(TiXmlElement* element);

	virtual void AppendTextToProgram();
};

#endif

