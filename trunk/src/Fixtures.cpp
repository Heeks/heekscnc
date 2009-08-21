// Fixtures.cpp
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include "Fixtures.h"
#include "tinyxml/tinyxml.h"

void CFixtures::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element;
	element = new TiXmlElement( "Fixtures" );
	root->LinkEndChild( element );  
	WriteBaseXML(element);
}

HeeksObj* CFixtures::ReadFromXMLElement(TiXmlElement* pElem)
{
	CFixtures* new_object = new CFixtures();
	new_object->ReadBaseXML(pElem);
	return new_object;
}
