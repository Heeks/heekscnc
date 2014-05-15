// Tools.h
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "interface/ObjList.h"
#include "HeeksCNCTypes.h"

#pragma once

class CTools: public ObjList{
public:
    typedef enum {
        eGuageReplacesSize = 0,
        eIncludeGuageAndSize
	} TitleFormat_t;

    TitleFormat_t m_title_format;

public:

	// HeeksObj's virtual functions
	bool OneOfAKind(){return true;}
	int GetType()const{return ToolsType;}
	const wxChar* GetTypeString(void)const{return _("Tools");}
	HeeksObj *MakeACopy(void)const;

	CTools();
	CTools( const CTools & rhs );
	CTools & operator= ( const CTools & rhs );

	bool operator==( const CTools & rhs ) const { return(ObjList::operator==(rhs)); }
	bool operator!=( const CTools & rhs ) const { return(! (*this == rhs)); }

	bool IsDifferent(HeeksObj *other) { return(*this != (*(CTools *)other)); }
	

	const wxBitmap &GetIcon();
	bool CanAddTo(HeeksObj* owner){return owner->GetType() == ProgramType;}
	bool CanAdd(HeeksObj* object);
	bool CanBeRemoved(){return false;}
	void WriteXML(TiXmlNode *root);
	bool UsesID() { return(false); }
	void CopyFrom(const HeeksObj* object);

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);

	void GetProperties(std::list<Property *> *list);
};

