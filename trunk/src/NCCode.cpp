// NCCode.cpp

#include "stdafx.h"
#include "NCCode.h"
#include "../../tinyxml/tinyxml.h"
#include "OutputCanvas.h"
#include "../../interface/MarkedObject.h"

void ColouredText::WriteXML(TiXmlElement *root)
{
	TiXmlElement * element;
	element = new TiXmlElement( "text" );
	root->LinkEndChild( element ); 

	// add actual text as a child object
    	TiXmlText* text = new TiXmlText(Ttc(m_str.c_str()));
    	element->LinkEndChild(text);

	if(m_color_type != TextColorDefaultType)element->SetAttribute("col", CNCCode::m_text_colors_str[m_color_type].c_str());
}

void ColouredText::ReadFromXMLElement(TiXmlElement* element)
{
	m_color_type = TextColorDefaultType;

	// get the attributes
	for(TiXmlAttribute* a = element->FirstAttribute(); a; a = a->Next())
	{
		std::string name(a->Name());
		if(name == "col"){
			std::string coltype(a->Value());
			for(int i = 0; i<MaxTextColorTypes; i++)
			{
				if(coltype == CNCCode::m_text_colors_str[i])
				{
					m_color_type = (TextColorEnum)i;
					break;
				}
			}
		}
	}

	// get the text
	m_str = wxString(Ctt(element->GetText()));
}

void PathLine::WriteXML(TiXmlElement *root)
{
	TiXmlElement * element;
	element = new TiXmlElement( "p" );
	root->LinkEndChild( element ); 
	element->SetDoubleAttribute("x", m_x[0]);
	element->SetDoubleAttribute("y", m_x[1]);
	element->SetDoubleAttribute("z", m_x[2]);
}

void PathLine::ReadFromXMLElement(TiXmlElement* pElem)
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

void PathLine::glVertices(const PathObject* prev_po)
{
	glVertex3dv(m_x);
}

void PathArc::WriteXML(TiXmlElement *root)
{
	TiXmlElement * element;
	if(GetType() == 1)element = new TiXmlElement( "acw" );
	else element = new TiXmlElement( "cw" );
	root->LinkEndChild( element ); 
	element->SetDoubleAttribute("x", m_x[0]);
	element->SetDoubleAttribute("y", m_x[1]);
	element->SetDoubleAttribute("z", m_x[2]);
	element->SetDoubleAttribute("i", m_c[0]);
	element->SetDoubleAttribute("j", m_c[1]);
	element->SetDoubleAttribute("k", m_c[2]);
}

void PathArc::ReadFromXMLElement(TiXmlElement* pElem)
{
	// get the attributes
	for(TiXmlAttribute* a = pElem->FirstAttribute(); a; a = a->Next())
	{
		std::string name(a->Name());
		if(name == "x"){m_x[0] = a->DoubleValue();}
		else if(name == "y"){m_x[1] = a->DoubleValue();}
		else if(name == "z"){m_x[2] = a->DoubleValue();}
		else if(name == "i"){m_c[0] = a->DoubleValue();}
		else if(name == "j"){m_c[1] = a->DoubleValue();}
		else if(name == "k"){m_c[2] = a->DoubleValue();}
	}
}

void PathArc::glVertices(const PathObject* prev_po)
{
	if(prev_po == NULL)return;

	double sx = -m_c[0];
	double sy = -m_c[1];
	// e = cs + se = -c + e - s
	double ex = -m_c[0] + m_x[0] - prev_po->m_x[0];
	double ey = -m_c[1] + m_x[1] - prev_po->m_x[1];
	double rs = sqrt(sx * sx + sy * sy);
	double re = sqrt(ex * ex + ey * ey);

	double start_angle = atan2(sy, sx);
	double end_angle = atan2(ey, ex);

	if(GetType() == 1){
		if(end_angle < start_angle)end_angle += 6.283185307179;
	}
	else{
		if(start_angle < end_angle)start_angle += 6.283185307179;
	}

	double angle_step = (end_angle - start_angle) / 10;
	for(int i = 0; i< 10; i++)
	{
		double angle = start_angle + angle_step * (i + 1);
		double r = rs + ((re - rs) * (i + 1)) /10;
		double x = prev_po->m_x[0] + m_c[0] + r * cos(angle);
		double y = prev_po->m_x[1] + m_c[1] + r * sin(angle);
		double z = prev_po->m_x[2] + ((m_x[2] - prev_po->m_x[2]) * (i+1))/10;
		glVertex3d(x, y, z);
	}
}

ColouredPath::ColouredPath(const ColouredPath& c)
{
	operator=(c);
}

const ColouredPath &ColouredPath::operator=(const ColouredPath& c)
{
	Clear();
	m_color_type = c.m_color_type;
	for(std::list< PathObject* >::const_iterator It = c.m_points.begin(); It != c.m_points.end(); It++)
	{
		PathObject* object = *It;
		m_points.push_back(object->MakeACopy());
	}
	return *this;
}

void ColouredPath::Clear()
{
	for(std::list< PathObject* >::iterator It = m_points.begin(); It != m_points.end(); It++)
	{
		PathObject* object = *It;
		delete object;
	}
	m_points.clear();
}

void ColouredPath::glCommands()
{
	CNCCode::m_lines_colors[m_color_type].glColor();
	glBegin(GL_LINE_STRIP);
	const PathObject* prev_po = NULL;
	for(std::list< PathObject* >::iterator It = m_points.begin(); It != m_points.end(); It++)
	{
		PathObject* po = *It;
		po->glVertices(prev_po);
		prev_po = po;
	}
	glEnd();
}

void ColouredPath::GetBox(CBox &box)
{
	for(std::list< PathObject* >::iterator It = m_points.begin(); It != m_points.end(); It++)
	{
		PathObject* po= *It;
		box.Insert(po->m_x);
	}
}

void ColouredPath::WriteXML(TiXmlElement *root)
{
	TiXmlElement * element;
	element = new TiXmlElement( "path" );
	root->LinkEndChild( element ); 

	element->SetAttribute("col", CNCCode::m_lines_colors_str[m_color_type].c_str());
	for(std::list< PathObject* >::iterator It = m_points.begin(); It != m_points.end(); It++)
	{
		PathObject* po = *It;
		po->WriteXML(element);
	}
}

void ColouredPath::ReadFromXMLElement(TiXmlElement* element)
{
	// get the attributes
	for(TiXmlAttribute* a = element->FirstAttribute(); a; a = a->Next())
	{
		std::string name(a->Name());
		if(name == "col"){
			std::string coltype(a->Value());
			for(int i = 0; i<MaxLinesColorType; i++)
			{
				if(coltype == CNCCode::m_lines_colors_str[i])
				{
					m_color_type = (LinesColorEnum)i;
					break;
				}
			}
		}
	}

	// loop through all the objects
	for(TiXmlElement* pElem = TiXmlHandle(element).FirstChildElement().Element(); pElem; pElem = pElem->NextSiblingElement())
	{
		std::string name(pElem->Value());
		if(name == "p")
		{
			PathLine *new_object = new PathLine;
			new_object->ReadFromXMLElement(pElem);
			m_points.push_back(new_object);
		}
		else if(name == "acw")
		{
			PathAcwArc *new_object = new PathAcwArc;
			new_object->ReadFromXMLElement(pElem);
			m_points.push_back(new_object);
		}
		else if(name == "cw")
		{
			PathCwArc *new_object = new PathCwArc;
			new_object->ReadFromXMLElement(pElem);
			m_points.push_back(new_object);
		}
	}
}

HeeksObj *CNCCodeBlock::MakeACopy(void)const{return new CNCCodeBlock(*this);}

void CNCCodeBlock::glCommands(bool select, bool marked, bool no_color)
{
	for(std::list<ColouredPath>::iterator It = m_line_strips.begin(); It != m_line_strips.end(); It++)
	{
		ColouredPath& line_strip = *It;
		line_strip.glCommands();
	}
}

void CNCCodeBlock::GetBox(CBox &box)
{
	for(std::list<ColouredPath>::iterator It = m_line_strips.begin(); It != m_line_strips.end(); It++)
	{
		ColouredPath& line_strip = *It;
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

	for(std::list<ColouredPath>::iterator It = m_line_strips.begin(); It != m_line_strips.end(); It++)
	{
		ColouredPath &line_strip = *It;
		line_strip.WriteXML(element);
	}

	WriteBaseXML(element);
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
		else if(name == "lines" || name == "path")
		{
			ColouredPath l;
			l.ReadFromXMLElement(pElem);
			new_object->m_line_strips.push_back(l);
		}
	}

#ifdef WIN32
	CNCCode::pos += 2; // 2 for each line ( presumably \r\n )
#else
	CNCCode::pos += 1;
#endif

	new_object->m_to_pos = CNCCode::pos;

	new_object->ReadBaseXML(element);

	return new_object;
}

void CNCCodeBlock::AppendTextCtrl(wxTextCtrl *textCtrl)
{
	for(std::list<ColouredText>::iterator It = m_text.begin(); It != m_text.end(); It++)
	{
		ColouredText &text = *It;
		HeeksColor &col = CNCCode::m_text_colors[text.m_color_type];
		wxColour c(col.red, col.green, col.blue);
		textCtrl->SetDefaultStyle(wxTextAttr(c));
		textCtrl->AppendText(text.m_str);
	}
	textCtrl->AppendText(_T("\n"));
}

long CNCCode::pos = 0;

HeeksColor CNCCode::m_text_colors[MaxTextColorTypes] = {
	HeeksColor(0, 0, 0),
	HeeksColor(0, 255, 0),
	HeeksColor(0, 0, 255),
	HeeksColor(255, 0, 0)
};

std::string CNCCode::m_text_colors_str[MaxTextColorTypes] = {
	std::string("default"), // won't get written
	std::string("block"),
	std::string("prep"),
	std::string("axis")
};

HeeksColor CNCCode::m_lines_colors[MaxLinesColorType] = {
	HeeksColor(255, 0, 0),
	HeeksColor(0, 255, 0)
};
	
std::string CNCCode::m_lines_colors_str[MaxLinesColorType] = {
	std::string("rapid"),
	std::string("feed")
};

CNCCode::CNCCode():m_gl_list(0), m_highlighted_block(NULL){}

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
	m_highlighted_block = NULL;
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

	if(m_highlighted_block)
	{
		glLineWidth(3);
		m_highlighted_block->glCommands(false, false, false);
		glLineWidth(1);
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

	WriteBaseXML(element);
}

bool CNCCode::CanAdd(HeeksObj* object)
{
	return object->GetType() == NCCodeBlockType;
}

bool CNCCode::CanAddTo(HeeksObj* owner)
{
	return owner->GetType() == ProgramType;
}

void CNCCode::SetClickMarkPoint(MarkedObject* marked_object, const double* ray_start, const double* ray_direction)
{
	if(marked_object->m_map.size() > 0)
	{
		MarkedObject* sub_marked_object = marked_object->m_map.begin()->second;
		if(sub_marked_object)
		{
			if(sub_marked_object->m_map.size() > 0)
			{
				HeeksObj* object = sub_marked_object->m_map.begin()->first;
				if(object && object->GetType() == NCCodeBlockType)
				{
					m_highlighted_block = (CNCCodeBlock*)object;
					int pos = m_highlighted_block->m_to_pos;
#ifdef WIN32
					pos -= 2;
#endif
					theApp.m_output_canvas->m_textCtrl->SetInsertionPoint(pos);
					theApp.m_output_canvas->m_textCtrl->SetFocus();
				}
			}
		}
	}
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

	new_object->ReadBaseXML(element);

	new_object->SetTextCtrl(theApp.m_output_canvas->m_textCtrl);

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

void CNCCode::SetTextCtrl(wxTextCtrl *textCtrl)
{
	textCtrl->Clear();

	for(std::list<CNCCodeBlock*>::iterator It = m_blocks.begin(); It != m_blocks.end(); It++)
	{
		CNCCodeBlock* block = *It;
		block->AppendTextCtrl(textCtrl);
	}
}

void CNCCode::HighlightBlock(long pos)
{
	m_highlighted_block = NULL;

	for(std::list<CNCCodeBlock*>::iterator It = m_blocks.begin(); It != m_blocks.end(); It++)
	{
		CNCCodeBlock* block = *It;
		if(pos < block->m_to_pos)
		{
			m_highlighted_block = block;
			break;
		}
	}
}
