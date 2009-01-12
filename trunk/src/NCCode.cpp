// NCCode.cpp

#include "stdafx.h"
#include "NCCode.h"
#include "../../tinyxml/tinyxml.h"

void ColouredLineStrips::glCommands()
{
	glColor3ub(r, g, b);
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
	element = new TiXmlElement( "NCCode" );
//	root->LinkEndChild( element );  
//	element->SetAttribute("machine", m_machine.c_str());
//	element->SetAttribute("program", Ttc(theApp.m_program_canvas->m_textCtrl->GetValue()));

	WriteBaseXML(element);
}

//static
HeeksObj* CNCCode::ReadFromXMLElement(TiXmlElement* pElem)
{
	return new CNCCode();
}

void CNCCode::DestroyGLLists(void)
{
	if (m_gl_list)
	{
		glDeleteLists(m_gl_list, 1);
		m_gl_list = 0;
	}
}
