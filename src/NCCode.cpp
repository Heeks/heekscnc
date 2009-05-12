// NCCode.cpp
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "stdafx.h"
#include "NCCode.h"
#include "tinyxml/tinyxml.h"
#include "OutputCanvas.h"
#include "interface/MarkedObject.h"
#include "interface/PropertyColor.h"
#include "interface/PropertyList.h"
#include "CNCConfig.h"

#include <sstream>

void ColouredText::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element;
	element = new TiXmlElement( "text" );
	root->LinkEndChild( element ); 

	// add actual text as a child object
   	TiXmlText* text = new TiXmlText(Ttc(m_str.c_str()));
   	element->LinkEndChild(text);

	if(m_color_type != ColorDefaultType)element->SetAttribute("col", CNCCode::GetColor(m_color_type));
}

void ColouredText::ReadFromXMLElement(TiXmlElement* element)
{
	m_color_type = CNCCode::GetColor(element->Attribute("col"));

	// get the text
	m_str = wxString(Ctt(element->GetText()));
}

void PathLine::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element;
	element = new TiXmlElement( "line" );
	root->LinkEndChild( element ); 
	element->SetDoubleAttribute("x", m_x[0]);
	element->SetDoubleAttribute("y", m_x[1]);
	element->SetDoubleAttribute("z", m_x[2]);
}

void PathLine::ReadFromXMLElement(TiXmlElement* pElem)
{
	pElem->Attribute("x", &m_x[0]);
	pElem->Attribute("y", &m_x[1]);
	pElem->Attribute("z", &m_x[2]);

	m_x[0] *= CNCCodeBlock::multiplier;
	m_x[1] *= CNCCodeBlock::multiplier;
	m_x[2] *= CNCCodeBlock::multiplier;
}

void PathLine::glVertices(const PathObject* prev_po)
{
	if(prev_po)glVertex3dv(prev_po->m_x);
	glVertex3dv(m_x);
}

void PathArc::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element = new TiXmlElement( "arc" );
	root->LinkEndChild( element ); 

	element->SetDoubleAttribute("x", m_x[0]);
	element->SetDoubleAttribute("y", m_x[1]);
	element->SetDoubleAttribute("z", m_x[2]);
	element->SetDoubleAttribute("i", m_c[0]);
	element->SetDoubleAttribute("j", m_c[1]);
	element->SetDoubleAttribute("k", m_c[2]);
	element->SetDoubleAttribute("d", m_dir);
}

void PathArc::ReadFromXMLElement(TiXmlElement* pElem)
{
	// get the attributes
	pElem->Attribute("x", &m_x[0]);
	pElem->Attribute("y", &m_x[1]);
	pElem->Attribute("z", &m_x[2]);
	pElem->Attribute("i", &m_c[0]);
	pElem->Attribute("j", &m_c[1]);
	pElem->Attribute("k", &m_c[2]);
	pElem->Attribute("d", &m_dir);

	m_x[0] *= CNCCodeBlock::multiplier;
	m_x[1] *= CNCCodeBlock::multiplier;
	m_x[2] *= CNCCodeBlock::multiplier;
	m_c[0] *= CNCCodeBlock::multiplier;
	m_c[1] *= CNCCodeBlock::multiplier;
	m_c[2] *= CNCCodeBlock::multiplier;
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

	if(m_dir == 1){
		if(end_angle < start_angle)end_angle += 6.283185307179;
	}
	else{
		if(start_angle < end_angle)start_angle += 6.283185307179;
	}

	double angle_step = (end_angle - start_angle) / 10;
	glVertex3dv(prev_po->m_x);
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
	CNCCode::Color(m_color_type).glColor();
	glBegin(GL_LINE_STRIP);
	for(std::list< PathObject* >::iterator It = m_points.begin(); It != m_points.end(); It++)
	{
		PathObject* po = *It;
		po->glVertices(CNCCode::prev_po);
		CNCCode::prev_po = po;
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

void ColouredPath::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element;
	element = new TiXmlElement( "path" );
	root->LinkEndChild( element ); 

	element->SetAttribute("col", CNCCode::GetColor(m_color_type));
	for(std::list< PathObject* >::iterator It = m_points.begin(); It != m_points.end(); It++)
	{
		PathObject* po = *It;
		po->WriteXML(element);
	}
}

void ColouredPath::ReadFromXMLElement(TiXmlElement* element)
{
	// get the attributes
	m_color_type = CNCCode::GetColor(element->Attribute("col"), ColorRapidType);

	// loop through all the objects
	for(TiXmlElement* pElem = TiXmlHandle(element).FirstChildElement().Element(); pElem; pElem = pElem->NextSiblingElement())
	{
		std::string name(pElem->Value());
		if(name == "line")
		{
			PathLine *new_object = new PathLine;
			new_object->ReadFromXMLElement(pElem);
			m_points.push_back(new_object);
		}
		else if(name == "arc")
		{
			PathArc *new_object = new PathArc;
			new_object->ReadFromXMLElement(pElem);
			m_points.push_back(new_object);
		}
	}
}

double CNCCodeBlock::multiplier = 1.0;

HeeksObj *CNCCodeBlock::MakeACopy(void)const{return new CNCCodeBlock(*this);}

void CNCCodeBlock::glCommands(bool select, bool marked, bool no_color)
{
	if(marked)glLineWidth(3);

	for(std::list<ColouredPath>::iterator It = m_line_strips.begin(); It != m_line_strips.end(); It++)
	{
		ColouredPath& line_strip = *It;
		line_strip.glCommands();
	}

	if(marked)glLineWidth(1);
}

void CNCCodeBlock::GetBox(CBox &box)
{
	for(std::list<ColouredPath>::iterator It = m_line_strips.begin(); It != m_line_strips.end(); It++)
	{
		ColouredPath& line_strip = *It;
		line_strip.GetBox(box);
	}
}

void CNCCodeBlock::WriteXML(TiXmlNode *root)
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
		else if(name == "path")
		{
			ColouredPath l;
			l.ReadFromXMLElement(pElem);
			new_object->m_line_strips.push_back(l);
		}
		else if(name == "mode")
		{
			const char* units = pElem->Attribute("units");
			if(units)pElem->Attribute("units", &CNCCodeBlock::multiplier);
		}
	}

	CNCCode::pos += 1;

	new_object->m_to_pos = CNCCode::pos;

	new_object->ReadBaseXML(element);

	return new_object;
}

void CNCCodeBlock::AppendTextCtrl(wxTextCtrl *textCtrl)
{
	for(std::list<ColouredText>::iterator It = m_text.begin(); It != m_text.end(); It++)
	{
		ColouredText &text = *It;
		HeeksColor &col = CNCCode::Color(text.m_color_type);
		wxColour c(col.red, col.green, col.blue);
		textCtrl->SetDefaultStyle(wxTextAttr(c));
		textCtrl->AppendText(text.m_str);
	}
	textCtrl->AppendText(_T("\n"));
}

void CNCCodeBlock::AppendText(wxString& str)
{
	for(std::list<ColouredText>::iterator It = m_text.begin(); It != m_text.end(); It++)
	{
		ColouredText &text = *It;
		str.append(text.m_str);
	}
	str.append(_T("\n"));
}

void CNCCodeBlock::FormatText(wxTextCtrl *textCtrl)
{
	if (m_formatted) return;
	int i = m_from_pos;
	for(std::list<ColouredText>::iterator It = m_text.begin(); It != m_text.end(); It++)
	{
		ColouredText &text = *It;
		HeeksColor &col = CNCCode::Color(text.m_color_type);
		wxColour c(col.red, col.green, col.blue);
		int len = text.m_str.size();

		wxFont font(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, _T("Lucida Console"), wxFONTENCODING_SYSTEM);
		wxTextAttr ta(c);
		ta.SetFont(font);
		textCtrl->SetStyle(i, i+len, ta);
		i += len;
	}
	m_formatted = true;
}

long CNCCode::pos = 0;
// static
PathObject* CNCCode::prev_po = NULL;

std::map<std::string,ColorEnum> CNCCode::m_colors_s_i;
std::map<ColorEnum,std::string> CNCCode::m_colors_i_s;
std::vector<HeeksColor> CNCCode::m_colors;

void CNCCode::ClearColors(void)
{
	CNCCode::m_colors_s_i.clear();
	CNCCode::m_colors_i_s.clear();
	CNCCode::m_colors.clear();
}

void CNCCode::AddColor(const char* name, const HeeksColor& col)
{
	ColorEnum i = (ColorEnum)ColorCount();
	m_colors_s_i.insert(std::pair<std::string,ColorEnum>(std::string(name), i));
	m_colors_i_s.insert(std::pair<ColorEnum,std::string>(i, std::string(name)));
	m_colors.push_back(col);
}

ColorEnum CNCCode::GetColor(const char* name, ColorEnum def)
{
	if (name == NULL) return def;
	std::map<std::string,ColorEnum>::iterator it = m_colors_s_i.find(std::string(name));
	if (it != m_colors_s_i.end()) return it->second;
	else return def;
}

const char* CNCCode::GetColor(ColorEnum i, const char* def)
{
	std::map<ColorEnum,std::string>::iterator it = m_colors_i_s.find(i);
	if (it != m_colors_i_s.end()) return it->second.c_str();
	else return def;
}

// static
void CNCCode::ReadColorsFromConfig()
{
	CNCConfig config;
	long col;
	ClearColors();
	config.Read(_T("ColorDefaultType"),		&col, HeeksColor(0, 0, 0).COLORREF_color()); AddColor("default", HeeksColor(col));
	config.Read(_T("ColorBlockType"),		&col, HeeksColor(0, 0, 222).COLORREF_color()); AddColor("blocknum", HeeksColor(col));
	config.Read(_T("ColorMiscType"),		&col, HeeksColor(0, 200, 0).COLORREF_color()); AddColor("misc", HeeksColor(col));
	config.Read(_T("ColorProgramType"),		&col, HeeksColor(255, 128, 0).COLORREF_color()); AddColor("program", HeeksColor(col));
	config.Read(_T("ColorToolType"),		&col, HeeksColor(200, 200, 0).COLORREF_color()); AddColor("tool", HeeksColor(col));
	config.Read(_T("ColorCommentType"),		&col, HeeksColor(0, 200, 200).COLORREF_color()); AddColor("comment", HeeksColor(col));
	config.Read(_T("ColorVariableType"),	&col, HeeksColor(164, 88, 188).COLORREF_color()); AddColor("variable", HeeksColor(col));
	config.Read(_T("ColorPrepType"),		&col, HeeksColor(255, 0, 175).COLORREF_color()); AddColor("prep", HeeksColor(col));
	config.Read(_T("ColorAxisType"),		&col, HeeksColor(128, 0, 255).COLORREF_color()); AddColor("axis", HeeksColor(col));
	config.Read(_T("ColorRapidType"),		&col, HeeksColor(222, 0, 0).COLORREF_color()); AddColor("rapid", HeeksColor(col));
	config.Read(_T("ColorFeedType"),		&col, HeeksColor(0, 179, 0).COLORREF_color()); AddColor("feed", HeeksColor(col));
}

// static
void CNCCode::WriteColorsToConfig()
{
	CNCConfig config;

	config.Write(_T("ColorDefaultType"),	CNCCode::m_colors[ColorDefaultType	].COLORREF_color());
	config.Write(_T("ColorBlockType"),		CNCCode::m_colors[ColorBlockType	].COLORREF_color());
	config.Write(_T("ColorMiscType"),		CNCCode::m_colors[ColorMiscType	].COLORREF_color());
	config.Write(_T("ColorProgramType"),	CNCCode::m_colors[ColorProgramType	].COLORREF_color());
	config.Write(_T("ColorToolType"),		CNCCode::m_colors[ColorToolType	].COLORREF_color());
	config.Write(_T("ColorCommentType"),	CNCCode::m_colors[ColorCommentType	].COLORREF_color());
	config.Write(_T("ColorVariableType"),	CNCCode::m_colors[ColorVariableType].COLORREF_color());
	config.Write(_T("ColorPrepType"),		CNCCode::m_colors[ColorPrepType	].COLORREF_color());
	config.Write(_T("ColorAxisType"),		CNCCode::m_colors[ColorAxisType	].COLORREF_color());
	config.Write(_T("ColorRapidType"),		CNCCode::m_colors[ColorRapidType	].COLORREF_color());
	config.Write(_T("ColorFeedType"),		CNCCode::m_colors[ColorFeedType	].COLORREF_color());
}

void on_set_default_color	(HeeksColor value, HeeksObj* object)	{CNCCode::Color(ColorDefaultType    ) = value;}
void on_set_block_color		(HeeksColor value, HeeksObj* object)	{CNCCode::Color(ColorBlockType		) = value;}
void on_set_misc_color		(HeeksColor value, HeeksObj* object)	{CNCCode::Color(ColorMiscType		) = value;}
void on_set_program_color	(HeeksColor value, HeeksObj* object)	{CNCCode::Color(ColorProgramType	) = value;}
void on_set_tool_color		(HeeksColor value, HeeksObj* object)	{CNCCode::Color(ColorToolType		) = value;}
void on_set_comment_color	(HeeksColor value, HeeksObj* object)	{CNCCode::Color(ColorCommentType	) = value;}
void on_set_variable_color	(HeeksColor value, HeeksObj* object)	{CNCCode::Color(ColorVariableType	) = value;}
void on_set_prep_color		(HeeksColor value, HeeksObj* object)	{CNCCode::Color(ColorPrepType		) = value;}
void on_set_axis_color		(HeeksColor value, HeeksObj* object)	{CNCCode::Color(ColorAxisType		) = value;}
void on_set_rapid_color		(HeeksColor value, HeeksObj* object)	{CNCCode::Color(ColorRapidType		) = value;}
void on_set_feed_color		(HeeksColor value, HeeksObj* object)	{CNCCode::Color(ColorFeedType		) = value;}

// static
void CNCCode::GetOptions(std::list<Property *> *list)
{
	PropertyList* nc_options = new PropertyList(_("nc options"));

	PropertyList* text_colors = new PropertyList(_("text colors"));
	text_colors->m_list.push_back ( new PropertyColor ( _("default color"),		CNCCode::Color(ColorDefaultType		), NULL, on_set_default_color	 ) );
	text_colors->m_list.push_back ( new PropertyColor ( _("block color"),		CNCCode::Color(ColorBlockType		), NULL, on_set_block_color		 ) );
	text_colors->m_list.push_back ( new PropertyColor ( _("misc color"),		CNCCode::Color(ColorMiscType		), NULL, on_set_misc_color		 ) );
	text_colors->m_list.push_back ( new PropertyColor ( _("program color	"),	CNCCode::Color(ColorProgramType		), NULL, on_set_program_color	 ) );
	text_colors->m_list.push_back ( new PropertyColor ( _("tool color"),		CNCCode::Color(ColorToolType		), NULL, on_set_tool_color		 ) );
	text_colors->m_list.push_back ( new PropertyColor ( _("comment color	"),	CNCCode::Color(ColorCommentType		), NULL, on_set_comment_color	 ) );
	text_colors->m_list.push_back ( new PropertyColor ( _("variable color"),	CNCCode::Color(ColorVariableType	), NULL, on_set_variable_color	 ) );
	text_colors->m_list.push_back ( new PropertyColor ( _("prep color"),		CNCCode::Color(ColorPrepType		), NULL, on_set_prep_color		 ) );
	text_colors->m_list.push_back ( new PropertyColor ( _("axis color"),		CNCCode::Color(ColorAxisType		), NULL, on_set_axis_color		 ) );
	text_colors->m_list.push_back ( new PropertyColor ( _("rapid color"),		CNCCode::Color(ColorRapidType		), NULL, on_set_rapid_color		 ) );
	text_colors->m_list.push_back ( new PropertyColor ( _("feed color"),		CNCCode::Color(ColorFeedType		), NULL, on_set_feed_color		 ) );
	nc_options->m_list.push_back(text_colors);

	list->push_back(nc_options);
}

CNCCode::CNCCode():m_gl_list(0), m_highlighted_block(NULL), m_user_edited(false){}

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
		CNCCode::prev_po = NULL;

		for(std::list<CNCCodeBlock*>::iterator It = m_blocks.begin(); It != m_blocks.end(); It++)
		{
			CNCCodeBlock* block = *It;
			glPushName((unsigned long)block);
			block->glCommands(true, block == m_highlighted_block, false);
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

void CNCCode::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element;
	element = new TiXmlElement( "nccode" );
	root->LinkEndChild( element ); 

	for(std::list<CNCCodeBlock*>::iterator It = m_blocks.begin(); It != m_blocks.end(); It++)
	{
		CNCCodeBlock* block = *It;
		block->WriteXML(element);
	}

	element->SetAttribute("edited", m_user_edited ? 1:0);

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
					int from_pos = m_highlighted_block->m_from_pos;
					int to_pos = m_highlighted_block->m_to_pos;
					DestroyGLLists();
					theApp.m_output_canvas->m_textCtrl->ShowPosition(from_pos);
					theApp.m_output_canvas->m_textCtrl->SetSelection(from_pos, to_pos);
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

	CNCCodeBlock::multiplier = 1.0;

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

	// loop through the attributes
	int i;
	element->Attribute("edited", &i);
	new_object->m_user_edited = (i != 0);

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

	textCtrl->Freeze();

	wxFont font(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, _T("Lucida Console"), wxFONTENCODING_SYSTEM);
	wxTextAttr ta;
	ta.SetFont(font);
	textCtrl->SetDefaultStyle(ta);

	wxString str;
	for(std::list<CNCCodeBlock*>::iterator It = m_blocks.begin(); It != m_blocks.end(); It++)
	{
		CNCCodeBlock* block = *It;
		block->AppendText(str);
	}
	textCtrl->SetValue(str);

#ifndef WIN32
	// for Windows, this is done in COutputTextCtrl::OnPaint
	for(std::list<CNCCodeBlock*>::iterator It = m_blocks.begin(); It != m_blocks.end(); It++)
	{
		CNCCodeBlock* block = *It;
		block->FormatText(textCtrl);
	}
#endif

	textCtrl->Thaw();
}

void CNCCode::FormatBlocks(wxTextCtrl *textCtrl, int i0, int i1)
{
	textCtrl->Freeze();
	for(std::list<CNCCodeBlock*>::iterator It = m_blocks.begin(); It != m_blocks.end(); It++)
	{
		CNCCodeBlock* block = *It;
		if (i0 <= block->m_from_pos && block->m_from_pos <= i1)
			block->FormatText(textCtrl);
	}
	textCtrl->Thaw();
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
	DestroyGLLists();
}
