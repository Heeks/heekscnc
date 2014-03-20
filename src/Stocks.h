// Stocks.h
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "../../interface/ObjList.h"
#include "HeeksCNCTypes.h"

#pragma once

class CStocks: public ObjList{
public:
	// HeeksObj's virtual functions
	bool OneOfAKind(){return true;}
	int GetType()const{return StocksType;}
	const wxChar* GetTypeString(void)const{return _("Stocks");}
	HeeksObj *MakeACopy(void)const;

	CStocks();
	CStocks( const CStocks & rhs );
	CStocks & operator= ( const CStocks & rhs );

	bool operator==( const CStocks & rhs ) const { return(ObjList::operator==(rhs)); }
	bool operator!=( const CStocks & rhs ) const { return(! (*this == rhs)); }

	bool IsDifferent(HeeksObj *other) { return(*this != (*(CStocks *)other)); }
	
	const wxBitmap &GetIcon();
	bool CanAddTo(HeeksObj* owner){return owner->GetType() == ProgramType;}
	bool CanAdd(HeeksObj* object);
	bool CanBeRemoved(){return false;}
	void WriteXML(TiXmlNode *root);
	bool UsesID() { return(false); }
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);

	void GetSolidIds(std::set<int> &ids);

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);
};

