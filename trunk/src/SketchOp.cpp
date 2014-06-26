// SketchOp.cpp
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "stdafx.h"
#include "SketchOp.h"
#include "CNCConfig.h"
#include "ProgramCanvas.h"
#include "Program.h"
#include "interface/PropertyInt.h"
#include "interface/PropertyDouble.h"
#include "interface/PropertyLength.h"
#include "interface/PropertyString.h"
#include "tinyxml/tinyxml.h"
#include "interface/Tool.h"
#include "CTool.h"
#include "Reselect.h"


CSketchOp & CSketchOp::operator= ( const CSketchOp & rhs )
{
	if (this != &rhs)
	{
		m_sketch = rhs.m_sketch;
		CDepthOp::operator=( rhs );
	}

	return(*this);
}

CSketchOp::CSketchOp( const CSketchOp & rhs ) : CDepthOp(rhs)
{
	m_sketch = rhs.m_sketch;
}

void CSketchOp::ReloadPointers()
{
	CDepthOp::ReloadPointers();
}

void CSketchOp::GetBox(CBox &box)
{
	HeeksObj* sketch = heeksCAD->GetIDObject(SketchType, m_sketch);
	if (sketch)sketch->GetBox(box);
}

void CSketchOp::glCommands(bool select, bool marked, bool no_color)
{
	CDepthOp::glCommands(select, marked, no_color);

	if (select || marked)
	{
		// allow sketch operations to be selected
		HeeksObj* sketch = heeksCAD->GetIDObject(SketchType, m_sketch);
		if (sketch)sketch->glCommands(select, marked, no_color);
	}
}

void CSketchOp::WriteBaseXML(TiXmlElement *element)
{
	// write sketch id
	element->SetAttribute( "sketch", m_sketch);

	CDepthOp::WriteBaseXML(element);
}

void CSketchOp::ReadBaseXML(TiXmlElement* element)
{
	element->Attribute("sketch", &m_sketch);

	// get old sketch child item
	TiXmlElement* e = heeksCAD->FirstNamedXMLChildElement(element, "sketch");
	if(e)
	{
		int id = 0;
		if(e->Attribute("id", &id))
			m_sketch = id;
	}

	CDepthOp::ReadBaseXML(element);
}

void on_set_sketch(int value, HeeksObj* object)
{
	((CSketchOp*)object)->m_sketch = value;
}

void CSketchOp::GetProperties(std::list<Property *> *list)
{
	list->push_back(new PropertyInt(_("sketch id"), m_sketch, this, on_set_sketch));
	CDepthOp::GetProperties(list);
}

Python CSketchOp::AppendTextToProgram()
{
	Python python;
    python << CDepthOp::AppendTextToProgram();
	return(python);
}

static ReselectSketch reselect_sketch;

void CSketchOp::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
	reselect_sketch.m_sketch = m_sketch;
	reselect_sketch.m_object = this;
	t_list->push_back(&reselect_sketch);
    CDepthOp::GetTools( t_list, p );
}

bool CSketchOp::operator== ( const CSketchOp & rhs ) const
{
	return(CDepthOp::operator==(rhs));
}
