// Stock.cpp

#include <stdafx.h>

#include "Stock.h"
#include "Stocks.h"
#include "Program.h"
#include "CNCConfig.h"
#include "tinyxml/tinyxml.h"
#include "interface/PropertyLength.h"
#include "interface/PropertyCheck.h"
#include "Reselect.h"
#include "StockDlg.h"

CStock::CStock()
{
	ReadDefaultValues();
}

HeeksObj *CStock::MakeACopy(void)const
{
	return new CStock(*this);
}

void CStock::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element = heeksCAD->NewXMLElement( "Stock" );
	heeksCAD->LinkXMLEndChild( root,  element );

	// write solid ids
	for (std::list<int>::iterator It = m_solids.begin(); It != m_solids.end(); It++)
    {
		int solid = *It;

		TiXmlElement * solid_element = heeksCAD->NewXMLElement( "solid" );
		heeksCAD->LinkXMLEndChild( element, solid_element );
		solid_element->SetAttribute("id", solid);
	}

	IdNamedObj::WriteBaseXML(element);
}

// static member function
HeeksObj* CStock::ReadFromXMLElement(TiXmlElement* element)
{
	CStock* new_object = new CStock;

	// read solid ids
	for(TiXmlElement* pElem = heeksCAD->FirstXMLChildElement( element ) ; pElem; pElem = pElem->NextSiblingElement())
	{
		std::string name(pElem->Value());
		if(name == "solid"){
			for(TiXmlAttribute* a = pElem->FirstAttribute(); a; a = a->Next())
			{
				std::string name(a->Name());
				if(name == "id"){
					int id = a->IntValue();
					new_object->m_solids.push_back(id);
				}
			}
		}
	}

	new_object->ReadBaseXML(element);

	return new_object;
}

void CStock::WriteDefaultValues()
{
}

void CStock::ReadDefaultValues()
{
}

void CStock::GetProperties(std::list<Property *> *list)
{
	AddSolidsProperties(list, m_solids);

	IdNamedObj::GetProperties(list);
}

void CStock::CopyFrom(const HeeksObj* object)
{
	if (object->GetType() == GetType())
	{
		operator=(*((CStock*)object));
	}
}

bool CStock::CanAddTo(HeeksObj* owner)
{
	return ((owner != NULL) && (owner->GetType() == StocksType));
}

const wxBitmap &CStock::GetIcon()
{
	static wxBitmap* icon = NULL;
	if(icon == NULL)icon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/stock.png")));
	return *icon;
}

static bool OnEdit(HeeksObj* object)
{
	return StockDlg::Do((CStock*)object);
}

void CStock::GetOnEdit(bool(**callback)(HeeksObj*))
{
	*callback = OnEdit;
}

HeeksObj* CStock::PreferredPasteTarget()
{
	return theApp.m_program->Stocks();
}
