// Tools.h
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "interface/ObjList.h"
#include "HeeksCNCTypes.h"

#pragma once

class CTools: public ObjList{
public:

	// HeeksObj's virtual functions
	bool OneOfAKind(){return true;}
	int GetType()const{return ToolsType;}
	const wxChar* GetTypeString(void)const{return _("Tools");}
	HeeksObj *MakeACopy(void)const{ return new CTools(*this);}
	void GetIcon(int& texture_number, int& x, int& y){GET_ICON(5, 1);}
	bool CanAddTo(HeeksObj* owner){return owner->GetType() == ProgramType;}
	bool CanAdd(HeeksObj* object);
	bool CanBeRemoved(){return false;}
	void WriteXML(TiXmlNode *root);
	bool AutoExpand(){return true;}
	bool UsesID() const { return(false); }

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);

};

