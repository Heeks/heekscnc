// Fixtures.h
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "interface/ObjList.h"
#include "HeeksCNCTypes.h"

#pragma once

class CFixtures: public ObjList{
public:
	// HeeksObj's virtual functions
	bool OneOfAKind(){return true;}
	int GetType()const{return FixturesType;}
	const wxChar* GetTypeString(void)const{return _("Fixtures");}
	HeeksObj *MakeACopy(void)const{ return new CFixtures(*this);}
	void GetIcon(int& texture_number, int& x, int& y){GET_ICON(9, 0);}
	bool CanAddTo(HeeksObj* owner){return owner->GetType() == ProgramType;}
	bool CanAdd(HeeksObj* object) {	return(object->GetType() == FixtureType); }
	bool CanBeRemoved(){return false;}
	void WriteXML(TiXmlNode *root);
	bool AutoExpand(){return true;}
	bool UsesID() const { return(false); }

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);

};
