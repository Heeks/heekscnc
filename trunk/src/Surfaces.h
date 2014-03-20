// Surfaces.h
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "../../interface/ObjList.h"
#include "HeeksCNCTypes.h"

#pragma once

class CSurfaces: public ObjList{
public:
	// HeeksObj's virtual functions
	bool OneOfAKind(){return true;}
	int GetType()const{return SurfacesType;}
	const wxChar* GetTypeString(void)const{return _("Surfaces");}
	HeeksObj *MakeACopy(void)const;

	CSurfaces();
	CSurfaces( const CSurfaces & rhs );
	CSurfaces & operator= ( const CSurfaces & rhs );

	bool operator==( const CSurfaces & rhs ) const { return(ObjList::operator==(rhs)); }
	bool operator!=( const CSurfaces & rhs ) const { return(! (*this == rhs)); }

	bool IsDifferent(HeeksObj *other) { return(*this != (*(CSurfaces *)other)); }
	
	const wxBitmap &GetIcon();
	bool CanAddTo(HeeksObj* owner){return owner->GetType() == ProgramType;}
	bool CanAdd(HeeksObj* object);
	bool CanBeRemoved(){return false;}
	void WriteXML(TiXmlNode *root);
	bool UsesID() { return(false); }
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);
};

