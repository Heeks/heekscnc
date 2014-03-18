// Tag.h
// Copyright (c) 2010, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "interface/HeeksObj.h"
#include "HeeksCNCTypes.h"

#pragma once

class CTag: public HeeksObj {
public:
	double m_pos[2]; // at middle of tag, x and y
	double m_width;
	double m_angle; // ramp angle, in degrees
	double m_height;

	CTag();
	CTag( const CTag & rhs );
	CTag & operator= ( const CTag & rhs );
	
	bool operator==( const CTag & rhs ) const;
	bool operator!=( const CTag & rhs ) const { return(! (*this == rhs)); }

	bool IsDifferent(HeeksObj *other) { return(*this != (*(CTag *)other)); }

	// HeeksObj's virtual functions
	int GetType()const{return TagType;}
	const wxChar* GetTypeString(void)const{return _("Tag");}
	void glCommands(bool select, bool marked, bool no_color);
	HeeksObj *MakeACopy(void)const{ return new CTag(*this);}
	void GetProperties(std::list<Property *> *list);
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);
	const wxBitmap &GetIcon();
	bool CanAddTo(HeeksObj* owner){return owner->GetType() == TagsType;}
	void WriteXML(TiXmlNode *root);
	bool UsesID() { return(false); }
	void WriteDefaultValues();
	void ReadDefaultValues();

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);

	static void PickPosition(CTag* tag);
};
