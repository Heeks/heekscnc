// NCCode.cpp

#include "stdafx.h"
#include "NCCode.h"
#include "../../tinyxml/tinyxml.h"

void ColouredText::WriteXML(TiXmlElement *root)
{
	TiXmlElement * element;
	element = new TiXmlElement( "text" );
	root->LinkEndChild( element ); 

	element->SetAttribute("str", m_str);
	element->SetAttribute("col", m_col.COLORREF_color());
}

void ColouredText::ReadFromXMLElement(TiXmlElement* pElem)
{
	// get the attributes
	for(TiXmlAttribute* a = pElem->FirstAttribute(); a; a = a->Next())
	{
		std::string name(a->Name());
		if(name == "col"){m_col = HeeksColor(a->IntValue());}
		else if(name == "str"){m_str = wxString(Ctt(a->Value()));}
	}
}

void threedoubles::WriteXML(TiXmlElement *root)
{
	TiXmlElement * element;
	element = new TiXmlElement( "p" );
	root->LinkEndChild( element ); 
	element->SetDoubleAttribute("x", m_x[0]);
	element->SetDoubleAttribute("y", m_x[1]);
	element->SetDoubleAttribute("z", m_x[2]);
}

void threedoubles::ReadFromXMLElement(TiXmlElement* pElem)
{
	// get the attributes
	for(TiXmlAttribute* a = pElem->FirstAttribute(); a; a = a->Next())
	{
		std::string name(a->Name());
		if(name == "x"){m_x[0] = a->DoubleValue();}
		else if(name == "y"){m_x[1] = a->DoubleValue();}
		else if(name == "z"){m_x[2] = a->DoubleValue();}
	}
}

void ColouredLineStrips::glCommands()
{
	m_col.glColor();
	glBegin(GL_LINE_STRIP);
	for(std::list< threedoubles >::iterator It = m_points.begin(); It != m_points.end(); It++)
	{
		threedoubles &td = *It;
		glVertex3dv(td.m_x);
	}
	glEnd();
}

void ColouredLineStrips::GetBox(CBox &box)
{
	for(std::list< threedoubles >::iterator It = m_points.begin(); It != m_points.end(); It++)
	{
		threedoubles &td = *It;
		box.Insert(td.m_x);
	}
}

void ColouredLineStrips::WriteXML(TiXmlElement *root)
{
	TiXmlElement * element;
	element = new TiXmlElement( "lines" );
	root->LinkEndChild( element ); 

	element->SetAttribute("col", m_col.COLORREF_color());
	for(std::list< threedoubles >::iterator It = m_points.begin(); It != m_points.end(); It++)
	{
		threedoubles &td = *It;
		td.WriteXML(element);
	}
}

void ColouredLineStrips::ReadFromXMLElement(TiXmlElement* element)
{
	// get the attributes
	for(TiXmlAttribute* a = element->FirstAttribute(); a; a = a->Next())
	{
		std::string name(a->Name());
		if(name == "col"){m_col = HeeksColor(a->IntValue());}
	}

	// loop through all the objects
	for(TiXmlElement* pElem = TiXmlHandle(element).FirstChildElement().Element(); pElem; pElem = pElem->NextSiblingElement())
	{
		std::string name(pElem->Value());
		if(name == "p")
		{
			threedoubles td;
			td.ReadFromXMLElement(pElem);
			m_points.push_back(td);
		}
	}
}

HeeksObj *CNCCodeBlock::MakeACopy(void)const{return new CNCCodeBlock(*this);}

void CNCCodeBlock::glCommands(bool select, bool marked, bool no_color)
{
	for(std::list<ColouredLineStrips>::iterator It = m_line_strips.begin(); It != m_line_strips.end(); It++)
	{
		ColouredLineStrips& line_strip = *It;
		line_strip.glCommands();
	}
}

void CNCCodeBlock::GetBox(CBox &box)
{
	for(std::list<ColouredLineStrips>::iterator It = m_line_strips.begin(); It != m_line_strips.end(); It++)
	{
		ColouredLineStrips& line_strip = *It;
		line_strip.glCommands();
	}
}

void CNCCodeBlock::WriteXML(TiXmlElement *root)
{
	TiXmlElement * element;
	element = new TiXmlElement( "ncblock" );
	root->LinkEndChild( element ); 

	for(std::list<ColouredText>::iterator It = m_text.begin(); It != m_text.end(); It++)
	{
		ColouredText &text = *It;
		text.WriteXML(element);
	}

	for(std::list<ColouredLineStrips>::iterator It = m_line_strips.begin(); It != m_line_strips.end(); It++)
	{
		ColouredLineStrips &line_strip = *It;
		line_strip.WriteXML(element);
	}

	//WriteBaseXML(element); // write HeeksObj::m_id
}

// static 
HeeksObj* CNCCodeBlock::ReadFromXMLElement(TiXmlElement* element)
{
	CNCCodeBlock* new_object = new CNCCodeBlock;
	new_object->m_from_pos = CNCCode::pos;

	// loop through all the objects
	for(TiXmlElement* pElem = TiXmlHandle(element).FirstChildElement().Element(); pElem; pElem = pElem->NextSiblingElement())
	{
		std::string name(pElem->Value());
		if(name == "text")
		{
			ColouredText t;
			t.ReadFromXMLElement(pElem);
			new_object->m_text.push_back(t);
			CNCCode::pos += t.m_str.Len();
		}
		else if(name == "lines")
		{
			ColouredLineStrips l;
			l.ReadFromXMLElement(pElem);
			new_object->m_line_strips.push_back(l);
		}
	}

	new_object->m_to_pos = CNCCode::pos;

	//HeeksObj::ReadBaseXML(element); // read HeeksObj::m_id

	return new_object;
}

long CNCCode::pos = 0;

CNCCode::CNCCode():m_gl_list(0){}

CNCCode::~CNCCode()
{
	Clear();
}

const CNCCode &CNCCode::operator=(const CNCCode &rhs)
{
	Clear();
	for(std::list<CNCCodeBlock*>::const_iterator It = rhs.m_blocks.begin(); It != rhs.m_blocks.end(); It++)
	{
		CNCCodeBlock* block = *It;
		CNCCodeBlock* new_block = new CNCCodeBlock(*block);
		m_blocks.push_back(new_block);
	}
	return *this;
}

void CNCCode::Clear()
{
	for(std::list<CNCCodeBlock*>::iterator It = m_blocks.begin(); It != m_blocks.end(); It++)
	{
		CNCCodeBlock* block = *It;
		delete block;
	}
	m_blocks.clear();
	DestroyGLLists();
	m_box = CBox();
}

void CNCCode::glCommands(bool select, bool marked, bool no_color)
{
	if(m_gl_list)
	{
		glCallList(m_gl_list);
	}
	else{
		m_gl_list = glGenLists(1);
		glNewList(m_gl_list, GL_COMPILE_AND_EXECUTE);

		// render all the blocks
		for(std::list<CNCCodeBlock*>::iterator It = m_blocks.begin(); It != m_blocks.end(); It++)
		{
			CNCCodeBlock* block = *It;
			glPushName((unsigned long)block);
			block->glCommands(true, false, false);
			glPopName();
		}

		glEndList();
	}
}

void CNCCode::GetBox(CBox &box)
{
	if(!m_box.m_valid)
	{
		for(std::list<CNCCodeBlock*>::iterator It = m_blocks.begin(); It != m_blocks.end(); It++)
		{
			CNCCodeBlock* block = *It;
			block->GetBox(m_box);
		}
	}

	box.Insert(m_box);
}

void CNCCode::GetProperties(std::list<Property *> *list)
{
	HeeksObj::GetProperties(list);
}

void CNCCode::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
	HeeksObj::GetTools(t_list, p);
}

HeeksObj *CNCCode::MakeACopy(void)const{return new CNCCode(*this);}

void CNCCode::CopyFrom(const HeeksObj* object){operator=(*((CNCCode*)object));}

void CNCCode::WriteXML(TiXmlElement *root)
{
	TiXmlElement * element;
	element = new TiXmlElement( "nccode" );
	root->LinkEndChild( element ); 

	for(std::list<CNCCodeBlock*>::iterator It = m_blocks.begin(); It != m_blocks.end(); It++)
	{
		CNCCodeBlock* block = *It;
		block->WriteXML(element);
	}

	//WriteBaseXML(element); // write HeeksObj::m_id
}

//static
HeeksObj* CNCCode::ReadFromXMLElement(TiXmlElement* element)
{
	CNCCode* new_object = new CNCCode;
	pos = 0;

	// loop through all the objects
	for(TiXmlElement* pElem = TiXmlHandle(element).FirstChildElement().Element(); pElem;	pElem = pElem->NextSiblingElement())
	{
		std::string name(pElem->Value());
		if(name == "ncblock")
		{
			HeeksObj* object = CNCCodeBlock::ReadFromXMLElement(pElem);
			new_object->m_blocks.push_back((CNCCodeBlock*)object);
		}
	}

	//HeeksObj::ReadBaseXML(element); // read HeeksObj::m_id

	return new_object;
}

void CNCCode::DestroyGLLists(void)
{
	if (m_gl_list)
	{
		glDeleteLists(m_gl_list, 1);
		m_gl_list = 0;
	}
}

void CNCCode::Test()
{
	CNCCode::pos = 0;
	{
		CNCCodeBlock* new_block = new CNCCodeBlock;
		{
			ColouredText t;
			t.m_col = HeeksColor(255, 0, 0);
			t.m_str = wxString(_T("G0"));
			new_block->m_text.push_back(t);
		}
		{
			ColouredText t;
			t.m_col = HeeksColor(0, 0, 0);
			t.m_str = wxString(_T("X10"));
			new_block->m_text.push_back(t);
		}
		{
			ColouredLineStrips s;
			s.m_col = HeeksColor(255, 0, 0);
			{
				threedoubles td;
				td.m_x[0] = 0.0;
				td.m_x[1] = 0.0;
				td.m_x[2] = 0.0;
				s.m_points.push_back(td);
			}
			{
				threedoubles td;
				td.m_x[0] = 10.0;
				td.m_x[1] = 0.0;
				td.m_x[2] = 0.0;
				s.m_points.push_back(td);
			}
			new_block->m_line_strips.push_back(s);
		}
		m_blocks.push_back(new_block);
	}
	{
		CNCCodeBlock* new_block = new CNCCodeBlock;
		{
			ColouredText t;
			t.m_col = HeeksColor(0, 255, 0);
			t.m_str = wxString(_T("G1"));
			new_block->m_text.push_back(t);
		}
		{
			ColouredText t;
			t.m_col = HeeksColor(0, 0, 0);
			t.m_str = wxString(_T("X20"));
			new_block->m_text.push_back(t);
		}
		{
			ColouredText t;
			t.m_col = HeeksColor(0, 0, 0);
			t.m_str = wxString(_T("Y10"));
			new_block->m_text.push_back(t);
		}
		{
			ColouredLineStrips s;
			s.m_col = HeeksColor(0, 255, 0);
			{
				threedoubles td;
				td.m_x[0] = 10.0;
				td.m_x[1] = 0.0;
				td.m_x[2] = 0.0;
				s.m_points.push_back(td);
			}
			{
				threedoubles td;
				td.m_x[0] = 20.0;
				td.m_x[1] = 10.0;
				td.m_x[2] = 0.0;
				s.m_points.push_back(td);
			}
			new_block->m_line_strips.push_back(s);
		}
		m_blocks.push_back(new_block);
	}
}