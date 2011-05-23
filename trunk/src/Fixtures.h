// Fixtures.h
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#ifndef STABLE_OPS_ONLY
#include "interface/ObjList.h"
#include "HeeksCNCTypes.h"
#include "Fixture.h"

#pragma once

class CFixtures: public ObjList{
public:
	// HeeksObj's virtual functions
	bool OneOfAKind(){return true;}
	int GetType()const{return FixturesType;}
	const wxChar* GetTypeString(void)const{return _("Fixtures");}
	HeeksObj *MakeACopy(void)const{ return new CFixtures(*this);}
	const wxBitmap &GetIcon();
	bool CanAddTo(HeeksObj* owner){return owner->GetType() == ProgramType;}
	bool CanAdd(HeeksObj* object) {	return(object->GetType() == FixtureType); }
	bool CanBeRemoved(){return false;}
	void WriteXML(TiXmlNode *root);
	bool AutoExpand(){return true;}
	bool UsesID() { return(false); }
	void CopyFrom(const HeeksObj* object);

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);

	CFixture *Find( const CFixture::eCoordinateSystemNumber_t coordinate_system_number );
	int GetNextFixture();

	bool operator== ( const CFixtures & rhs ) const { return(ObjList::operator==(rhs)); }
	bool operator!= ( const CFixtures & rhs ) const { return(! (*this == rhs)); }
	bool IsDifferent( HeeksObj *other ) { return( *this != (*(CFixtures *)other) ); }

};

#endif