// Tag.cpp
// Copyright (c) 2010, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include "Tag.h"
#include "../../tinyxml/tinyxml.h"
#include "../../interface/PropertyVertex.h"
#include "../../interface/PropertyLength.h"
#include "../../interface/Tool.h"
#include "CNCConfig.h"

CTag::CTag(): m_width(10.0),	m_angle(45.0), m_height(4.0){
	m_pos[0] = m_pos[1] = 0.0;
	ReadDefaultValues();
}

CTag::CTag( const CTag & rhs ) : HeeksObj(rhs)
{
	for (::size_t i=0; i<sizeof(m_pos)/sizeof(m_pos[0]); i++) m_pos[i] = rhs.m_pos[i];
	m_width = rhs.m_width;
	m_angle = rhs.m_angle;
	m_height = rhs.m_height;
}

CTag& CTag::operator= ( const CTag & rhs )
{
	if (this != &rhs)
	{
		HeeksObj::operator=( rhs );

		for (::size_t i=0; i<sizeof(m_pos)/sizeof(m_pos[0]); i++) m_pos[i] = rhs.m_pos[i];
        m_width = rhs.m_width;
        m_angle = rhs.m_angle;
        m_height = rhs.m_height;
	}

	return(*this);
}

const wxBitmap &CTag::GetIcon()
{
	static wxBitmap* icon = NULL;
	if(icon == NULL)icon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/tag.png")));
	return *icon;
}

static unsigned char cross16[32] = {0x80, 0x01, 0x40, 0x02, 0x20, 0x04, 0x10, 0x08, 0x08, 0x10, 0x04, 0x20, 0x02, 0x40, 0x01, 0x80, 0x01, 0x80, 0x02, 0x40, 0x04, 0x20, 0x08, 0x10, 0x10, 0x08, 0x20, 0x04, 0x40, 0x02, 0x80, 0x01};
static unsigned char bmp16[10] = {0x3f, 0xfc, 0x1f, 0xf8, 0x0f, 0xf0, 0x07, 0xe0, 0x03, 0xc0};

void CTag::glCommands(bool select, bool marked, bool no_color)
{
	if(marked)
	{
		glColor3ub(0, 0, 0);
		glRasterPos2dv(m_pos);
		glBitmap(16, 5, 8, 3, 10.0, 0.0, bmp16);

		glColor3ub(0, 0, 255);
		glRasterPos2dv(m_pos);
		glBitmap(16, 16, 8, 8, 10.0, 0.0, cross16);
	}
}

static void on_set_pos(const double* vt, HeeksObj* object){memcpy(((CTag*)object)->m_pos, vt, 2*sizeof(double));}
static void on_set_width(double value, HeeksObj* object){((CTag*)object)->m_width = value; ((CTag*)object)->WriteDefaultValues();}
static void on_set_angle(double value, HeeksObj* object){((CTag*)object)->m_angle = value; ((CTag*)object)->WriteDefaultValues();}
static void on_set_height(double value, HeeksObj* object){((CTag*)object)->m_height = value; ((CTag*)object)->WriteDefaultValues();}

void CTag::GetProperties(std::list<Property *> *list)
{
	list->push_back(new PropertyVertex2d(_("position"), m_pos, this, on_set_pos));
	list->push_back(new PropertyLength(_("width"), m_width, this, on_set_width));
	list->push_back(new PropertyDouble(_("angle"), m_angle, this, on_set_angle));
	list->push_back(new PropertyLength(_("height"), m_height, this, on_set_height));
}

static CTag* object_for_tools = NULL;

class PickPos: public Tool{
public:
	// Tool's virtual functions
	const wxChar* GetTitle(){return _("Pick position");}
	void Run(){
		double pos[3];
		if(heeksCAD->PickPosition(_("Pick position"), pos))
		{
			object_for_tools->m_pos[0] = pos[0];
			object_for_tools->m_pos[1] = pos[1];
		}
		// to do make undoable
	}
	wxString BitmapPath(){ return _T("tagpos");}
};

static PickPos pick_pos;

void CTag::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
	object_for_tools = this;
	t_list->push_back(&pick_pos);

}

void CTag::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element;
	element = heeksCAD->NewXMLElement( "Tag" );
	heeksCAD->LinkXMLEndChild( root,  element );
	element->SetDoubleAttribute( "x", m_pos[0]);
	element->SetDoubleAttribute( "y", m_pos[1]);
	element->SetDoubleAttribute( "width", m_width);
	element->SetDoubleAttribute( "angle", m_angle);
	element->SetDoubleAttribute( "height", m_height);
	WriteBaseXML(element);
}

//static
HeeksObj* CTag::ReadFromXMLElement(TiXmlElement* pElem)
{
	CTag* new_object = new CTag;
	new_object->ReadBaseXML(pElem);
	pElem->Attribute("x", &new_object->m_pos[0]);
	pElem->Attribute("y", &new_object->m_pos[1]);
	pElem->Attribute("width", &new_object->m_width);
	pElem->Attribute("angle", &new_object->m_angle);
	pElem->Attribute("height", &new_object->m_height);
	return new_object;
}

void CTag::WriteDefaultValues()
{
	CNCConfig config;
	config.Write(_T("Width"), m_width);
	config.Write(_T("Angle"), m_angle);
	config.Write(_T("Height"), m_height);
}

void CTag::ReadDefaultValues()
{
	CNCConfig config;
	config.Read(_T("Width"), &m_width);
	config.Read(_T("Angle"), &m_angle);
	config.Read(_T("Height"), &m_height);
}

bool CTag::operator==( const CTag & rhs ) const
{
	for (::size_t i=0; i<sizeof(m_pos)/sizeof(m_pos[0]); i++) if (m_pos[i] != rhs.m_pos[i]) return(false);
	if (m_width != rhs.m_width) return(false);
	if (m_angle != rhs.m_width) return(false);
	if (m_height != rhs.m_width) return(false);
	// return(HeeksObj::operator==(rhs));
	return(true);
}

void CTag::PickPosition(CTag* tag)
{
	object_for_tools = tag;
	pick_pos.Run();
}
