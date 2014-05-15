// Stocks.cpp
// Copyright (c) 2013, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include "Stocks.h"
#include "Program.h"
#include "interface/PropertyChoice.h"
#include "Stock.h"
#include "CNCConfig.h"
#include "tinyxml/tinyxml.h"
#include <wx/stdpaths.h>

bool CStocks::CanAdd(HeeksObj* object)
{
	return 	((object != NULL) && (object->GetType() == StockType));
}


HeeksObj *CStocks::MakeACopy(void) const
{
    return(new CStocks(*this));  // Call the copy constructor.
}


CStocks::CStocks()
{
    CNCConfig config;
}


CStocks::CStocks( const CStocks & rhs ) : ObjList(rhs)
{
}

CStocks & CStocks::operator= ( const CStocks & rhs )
{
    if (this != &rhs)
    {
        ObjList::operator=( rhs );
    }
    return(*this);
}


const wxBitmap &CStocks::GetIcon()
{
	static wxBitmap* icon = NULL;
	if(icon == NULL)icon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/stocks.png")));
	return *icon;
}

void CStocks::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element;
	element = heeksCAD->NewXMLElement( "Stocks" );
	heeksCAD->LinkXMLEndChild( root,  element );
	WriteBaseXML(element);
}

//static
HeeksObj* CStocks::ReadFromXMLElement(TiXmlElement* pElem)
{
	CStocks* new_object = new CStocks;
	new_object->ReadBaseXML(pElem);
	return new_object;
}

void CStocks::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
	CHeeksCNCApp::GetNewStockTools(t_list);

	ObjList::GetTools(t_list, p);
}

void CStocks::GetSolidIds(std::set<int> &ids)
{
	for(HeeksObj* object = this->GetFirstChild(); object; object = this->GetNextChild())
	{
		for(std::list<int>::iterator It = ((CStock*)object)->m_solids.begin(); It != ((CStock*)object)->m_solids.end(); It++)
		{
			int id = *It;
			ids.insert(id);
		}
	}
}