// Tools.cpp
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include "Tools.h"
#include "tinyxml/tinyxml.h"

bool CTools::CanAdd(HeeksObj* object)
{
	return 	(((object->GetType() == CuttingToolType)) || 
		 ((object->GetType() == FixtureType)));
}


void CTools::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element;
	element = new TiXmlElement( "Tools" );
	root->LinkEndChild( element );  
	WriteBaseXML(element);
}

//static
HeeksObj* CTools::ReadFromXMLElement(TiXmlElement* pElem)
{
	CTools* new_object = new CTools;
	new_object->ReadBaseXML(pElem);
	return new_object;
}
