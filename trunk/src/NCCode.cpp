// NCCode.cpp
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "stdafx.h"
#include <math.h>
#include "NCCode.h"
#include "OutputCanvas.h"
#include "../../interface/MarkedObject.h"
#include "../../interface/PropertyColor.h"
#include "../../interface/PropertyList.h"
#include "../../interface/PropertyInt.h"
#include "../../interface/Tool.h"
#include "CNCConfig.h"
#include "CTool.h"
#include "../../src/Geom.h"
#include "Program.h"

#include <TopoDS_Shape.hxx>
#include <TopoDS_Solid.hxx>
#include <TopoDS_Compound.hxx>
#include <BRep_Builder.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <Standard_Failure.hxx>
#include <StdFail_NotDone.hxx>
#include "../../tinyxml/tinyxml.h"

#include <wx/progdlg.h>

#include <memory>
#include <sstream>

int CNCCode::s_arc_interpolation_count = 20;

void ColouredText::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element;
	element = heeksCAD->NewXMLElement( "text" );
	heeksCAD->LinkXMLEndChild( root,  element );

	// add actual text as a child object
   	TiXmlText* text = heeksCAD->NewXMLText(m_str.utf8_str());
   	element->LinkEndChild(text);

	if(m_color_type != ColorDefaultType)element->SetAttribute( "col", CNCCode::GetColor(m_color_type));
}

void ColouredText::ReadFromXMLElement(TiXmlElement* element)
{
	m_color_type = CNCCode::GetColor(element->Attribute("col"));

	// get the text
	const char* text = element->GetText();
	if(text)m_str = wxString(Ctt(text));
}

// static
double PathObject::m_current_x[3] = {0, 0, 0};
double PathObject::m_prev_x[3] = {0, 0, 0};

void PathObject::WriteBaseXML(TiXmlElement *pElem)
{
	pElem->SetAttribute("tool_number", m_tool_number);

	pElem->SetDoubleAttribute("x", m_x[0]);
	pElem->SetDoubleAttribute("y", m_x[1]);
	pElem->SetDoubleAttribute("z", m_x[2]);

} // End WriteXML() method

void PathObject::ReadFromXMLElement(TiXmlElement* pElem)
{
	memcpy(m_prev_x, m_current_x, 3*sizeof(double));

	double x;
	if(pElem->Attribute("x", &x))m_current_x[0] = x * CNCCodeBlock::multiplier;
	if(pElem->Attribute("y", &x))m_current_x[1] = x * CNCCodeBlock::multiplier;
	if(pElem->Attribute("z", &x))m_current_x[2] = x * CNCCodeBlock::multiplier;

	memcpy(m_x, m_current_x, 3*sizeof(double));

	if (pElem->Attribute("tool_number"))
	{
		pElem->Attribute("tool_number", &m_tool_number);
	} // End if - then
	else
	{
		m_tool_number = 0;	// No tool selected.
	} // End if - else
}

void PathLine::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element;
	element = heeksCAD->NewXMLElement( "line" );
	heeksCAD->LinkXMLEndChild( root,  element );

	PathObject::WriteBaseXML(element);
}

void PathLine::ReadFromXMLElement(TiXmlElement* pElem)
{
	PathObject::ReadFromXMLElement(pElem);
}

void PathLine::glVertices(const PathObject* prev_po)
{
	if(prev_po)glVertex3dv(prev_po->m_x);
	glVertex3dv(m_x);
}

void PathArc::GetBox(CBox &box,const PathObject* prev_po)
{
	PathObject::GetBox(box,prev_po);

	if(IsIncluded(gp_Pnt(0,1,0),prev_po))
		box.Insert(prev_po->m_x[0]+m_c[0],prev_po->m_x[1]+m_c[1]+m_radius,0);
	if(IsIncluded(gp_Pnt(0,-1,0),prev_po))
		box.Insert(prev_po->m_x[0]+m_c[0],prev_po->m_x[1]+m_c[1]-m_radius,0);
	if(IsIncluded(gp_Pnt(1,0,0),prev_po))
		box.Insert(prev_po->m_x[0]+m_c[0]+m_radius,prev_po->m_x[1]+m_c[1],0);
	if(IsIncluded(gp_Pnt(-1,0,0),prev_po))
		box.Insert(prev_po->m_x[0]+m_c[0]-m_radius,prev_po->m_x[1]+m_c[1],0);

}

bool PathArc::IsIncluded(gp_Pnt pnt,const PathObject* prev_po)
{
	double sx = -m_c[0];
	double sy = -m_c[1];
	// e = cs + se = -c + e - s
	double ex = -m_c[0] + m_x[0] - prev_po->m_x[0];
	double ey = -m_c[1] + m_x[1] - prev_po->m_x[1];
	double rs = sqrt(sx * sx + sy * sy);
	// double re = sqrt(ex * ex + ey * ey);
	m_radius = rs;

	double start_angle = atan2(sy, sx);
	double end_angle = atan2(ey, ex);

	if(m_dir == 1){
		if(end_angle < start_angle)end_angle += 6.283185307179;
	}
	else{
		if(start_angle < end_angle)start_angle += 6.283185307179;
	}

	// double angle_step = 0;

	if (start_angle == end_angle)
		// It's a full circle.
		return true;

	double the_angle = atan2(pnt.Y(),pnt.X());
	double the_angle2 = the_angle + 2*M_PI;
	return (the_angle >= start_angle && the_angle <= end_angle) || (the_angle2 >= start_angle && the_angle2 <= end_angle);
}

void PathArc::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element = heeksCAD->NewXMLElement( "arc" );
	heeksCAD->LinkXMLEndChild( root,  element );

	element->SetDoubleAttribute( "i", m_c[0]);
	element->SetDoubleAttribute( "j", m_c[1]);
	element->SetDoubleAttribute( "k", m_c[2]);
	element->SetDoubleAttribute( "d", m_dir);

	PathObject::WriteBaseXML(element);
}

void PathArc::ReadFromXMLElement(TiXmlElement* pElem)
{
	// get the attributes
	bool radius_set = false;
	if (pElem->Attribute("r"))
	{
		pElem->Attribute("r", &m_radius);
		m_radius *= CNCCodeBlock::multiplier;
		radius_set = true;
	}
	else
	{
		if (pElem->Attribute("i")) pElem->Attribute("i", &m_c[0]);
		if (pElem->Attribute("j")) pElem->Attribute("j", &m_c[1]);
		if (pElem->Attribute("k")) pElem->Attribute("k", &m_c[2]);
		if (pElem->Attribute("d")) pElem->Attribute("d", &m_dir);

		m_c[0] *= CNCCodeBlock::multiplier;
		m_c[1] *= CNCCodeBlock::multiplier;
		m_c[2] *= CNCCodeBlock::multiplier;
	}

	PathObject::ReadFromXMLElement(pElem);

	if(radius_set)
	{
		// set ij and direction from radius
		SetFromRadius();
	}
}

void PathArc::SetFromRadius()
{
	// make a circle at start point and end point
	gp_Pnt ps(m_prev_x[0], m_prev_x[1], m_prev_x[2]);
	gp_Pnt pe(m_x[0], m_x[1], m_x[2]);
	double r = fabs(m_radius);
	gp_Circ c1(gp_Ax2(ps, gp_Dir(0, 0, 1)), r);
	gp_Circ c2(gp_Ax2(pe, gp_Dir(0, 0, 1)), r);
	std::list<gp_Pnt> plist;
	intersect(c1, c2, plist);
	if(plist.size() == 2)
	{
		gp_Pnt p1 = plist.front();
		gp_Pnt p2 = plist.back();
		gp_Vec along(ps, pe);
		gp_Vec right = gp_Vec(0, 0, 1).Crossed(along);
		gp_Vec vc(p1, p2);
		bool left = vc.Dot(right) < 0;
		if((m_radius < 0) == left)
		{
			extract(gp_Vec(ps, p1), this->m_c);
			this->m_dir = 1;
		}
		else
		{
			extract(gp_Vec(ps, p2), this->m_c);
			this->m_dir = -1;
		}
		m_radius = r;
	}
}

void PathArc::glVertices(const PathObject* prev_po)
{
	if (prev_po == NULL) return;

	std::list<gp_Pnt> vertices = Interpolate( prev_po, CNCCode::s_arc_interpolation_count );
	glVertex3dv(prev_po->m_x);
	for (std::list<gp_Pnt>::const_iterator l_itVertex = vertices.begin(); l_itVertex != vertices.end(); l_itVertex++)
	{
		glVertex3d(l_itVertex->X(), l_itVertex->Y(), l_itVertex->Z());
	} // End for
}

std::list<gp_Pnt> PathArc::Interpolate( const PathObject *prev_po, const unsigned int number_of_points ) const
{
	std::list<gp_Pnt> points;

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

	double angle_step = 0;

	if (start_angle == end_angle)
	{
		// It's a full circle.
		angle_step = (2 * M_PI) / number_of_points;
		if (m_dir == -1)
		{
			angle_step = -angle_step; // fix preview of full cw arcs
		}
	} // End if - then
	else
	{
		// It's an arc.
		angle_step = (end_angle - start_angle) / number_of_points;
	} // End if - else

	points.push_back( gp_Pnt( prev_po->m_x[0], prev_po->m_x[1], prev_po->m_x[2] ) );

	for(unsigned int i = 0; i< number_of_points; i++)
	{
		double angle = start_angle + angle_step * (i + 1);
		double r = rs + ((re - rs) * (i + 1)) /number_of_points;
		double x = prev_po->m_x[0] + m_c[0] + r * cos(angle);
		double y = prev_po->m_x[1] + m_c[1] + r * sin(angle);
		double z = prev_po->m_x[2] + ((m_x[2] - prev_po->m_x[2]) * (i+1))/number_of_points;

		points.push_back( gp_Pnt( x, y, z ) );
	}

	return(points);
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
		po->GetBox(box,CNCCode::prev_po);
		CNCCode::prev_po = po;
	}
}

void ColouredPath::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element;
	element = heeksCAD->NewXMLElement( "path" );
	heeksCAD->LinkXMLEndChild( root,  element );

	element->SetAttribute( "col", CNCCode::GetColor(m_color_type));
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
	for(TiXmlElement* pElem = heeksCAD->FirstXMLChildElement( element ) ; pElem; pElem = pElem->NextSiblingElement())
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

void CNCCodeBlock::WriteNCCode(wxTextFile &f, double ox, double oy)
{
	//TODO: offset is always in millimeters, but this gcode block could be in anything
	//I used inches, so I hacked it into working.
	wxString movement;
	std::list<ColouredText>::iterator it;
	for(it = m_text.begin(); it != m_text.end(); it++)
	{
		ColouredText ct = *it;
		switch(ct.m_color_type)
		{
			case ColorPrepType:
				f.AddLine(ct.m_str);
				break;
			case ColorRapidType:
			case ColorFeedType:
				movement = ct.m_str;
				break;
			case ColorAxisType:
				{
					wxString str = ct.m_str;
					wxChar axis = ct.m_str[0];
					double pos=0;
					ct.m_str.SubString(1,ct.m_str.size()-1).ToDouble(&pos);
					if(axis == 'X' || axis == 'x')
					{
						str = wxString::Format(_T("%c%f"),axis,pos+ox/25.4);
					}
					if(axis == 'Y' || axis == 'y')
					{
						str = wxString::Format(_T("%c%f"),axis,pos+oy/25.4);
					}
					movement.append(str);
				}
				break;
			default:
				break;
		}
	}

	if(movement.size())
		f.AddLine(movement);
}

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
	element = heeksCAD->NewXMLElement( "ncblock" );
	heeksCAD->LinkXMLEndChild( root,  element );

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
	for(TiXmlElement* pElem = heeksCAD->FirstXMLChildElement( element ) ; pElem; pElem = pElem->NextSiblingElement())
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

	if(new_object->m_text.size() > 0)CNCCode::pos++;

	new_object->m_to_pos = CNCCode::pos;

	new_object->ReadBaseXML(element);

	return new_object;
}

void CNCCodeBlock::AppendText(wxString& str)
{
	if(m_text.size() == 0)return;

	for(std::list<ColouredText>::iterator It = m_text.begin(); It != m_text.end(); It++)
	{
		ColouredText &text = *It;
		str.append(text.m_str);
	}
	str.append(_T("\n"));
}

void CNCCodeBlock::FormatText(wxTextCtrl *textCtrl, bool highlighted, bool force_format)
{
	if (m_formatted && !force_format) return;
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
		if(highlighted)ta.SetBackgroundColour(wxColour(218, 242, 142));
		else ta.SetBackgroundColour(wxColour(255, 255, 255));
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

const wxBitmap &CNCCode::GetIcon()
{
	static wxBitmap* icon = NULL;
	if(icon == NULL)icon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/nccode.png")));
	return *icon;
}

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
	config.Read(_T("ColorDefaultType"),		&col, HeeksColor(0, 0, 0).COLORREF_color()); AddColor("default", HeeksColor((long)col));
	config.Read(_T("ColorBlockType"),		&col, HeeksColor(0, 0, 222).COLORREF_color()); AddColor("blocknum", HeeksColor((long)col));
	config.Read(_T("ColorMiscType"),		&col, HeeksColor(0, 200, 0).COLORREF_color()); AddColor("misc", HeeksColor((long)col));
	config.Read(_T("ColorProgramType"),		&col, HeeksColor(255, 128, 0).COLORREF_color()); AddColor("program", HeeksColor((long)col));
	config.Read(_T("ColorToolType"),		&col, HeeksColor(200, 200, 0).COLORREF_color()); AddColor("tool", HeeksColor((long)col));
	config.Read(_T("ColorCommentType"),		&col, HeeksColor(0, 200, 200).COLORREF_color()); AddColor("comment", HeeksColor((long)col));
	config.Read(_T("ColorVariableType"),	&col, HeeksColor(164, 88, 188).COLORREF_color()); AddColor("variable", HeeksColor((long)col));
	config.Read(_T("ColorPrepType"),		&col, HeeksColor(255, 0, 175).COLORREF_color()); AddColor("prep", HeeksColor((long)col));
	config.Read(_T("ColorAxisType"),		&col, HeeksColor(128, 0, 255).COLORREF_color()); AddColor("axis", HeeksColor((long)col));
	config.Read(_T("ColorRapidType"),		&col, HeeksColor(222, 0, 0).COLORREF_color()); AddColor("rapid", HeeksColor((long)col));
	config.Read(_T("ColorFeedType"),		&col, HeeksColor(0, 179, 0).COLORREF_color()); AddColor("feed", HeeksColor((long)col));
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

CNCCode::CNCCode():m_gl_list(0), m_highlighted_block(NULL), m_user_edited(false)
{
	CNCConfig config;
	config.Read(_T("CNCCode_ArcInterpolationCount"), &CNCCode::s_arc_interpolation_count, 20);
}

CNCCode::~CNCCode()
{
	Clear();
}

const CNCCode &CNCCode::operator=(const CNCCode &rhs)
{
	HeeksObj::operator =(rhs);
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
			glPushName(block->GetIndex());
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

void on_set_arc_interpolation_count(int value, HeeksObj*object)
{
	CNCCode::s_arc_interpolation_count = value;
	CNCConfig config;
	config.Write(_T("CNCCode_ArcInterpolationCount"), CNCCode::s_arc_interpolation_count);
}

void CNCCode::GetProperties(std::list<Property *> *list)
{
	list->push_back( new PropertyInt(_("Arc Interpolation Count"), CNCCode::s_arc_interpolation_count, this, on_set_arc_interpolation_count) );
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
	element = heeksCAD->NewXMLElement( "nccode" );
	heeksCAD->LinkXMLEndChild( root,  element );

	for(std::list<CNCCodeBlock*>::iterator It = m_blocks.begin(); It != m_blocks.end(); It++)
	{
		CNCCodeBlock* block = *It;
		block->WriteXML(element);
	}

	element->SetAttribute( "edited", m_user_edited ? 1:0);

	WriteBaseXML(element);
}

bool CNCCode::CanAdd(HeeksObj* object)
{
	return ((object != NULL) && (object->GetType() == NCCodeBlockType));
}

bool CNCCode::CanAddTo(HeeksObj* owner)
{
	return ((owner != NULL) && (owner->GetType() == ProgramType));
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
					SetHighlightedBlock((CNCCodeBlock*)object);
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
	PathObject::m_current_x[0] = PathObject::m_current_x[1] = PathObject::m_current_x[2]  = 0.0;

	// loop through all the objects
	for(TiXmlElement* pElem = heeksCAD->FirstXMLChildElement( element ) ; pElem;	pElem = pElem->NextSiblingElement())
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
		block->FormatText(textCtrl, block == m_highlighted_block, false);
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
			block->FormatText(textCtrl, block == m_highlighted_block, false);
	}
	textCtrl->Thaw();
}

void CNCCode::HighlightBlock(long pos)
{
	SetHighlightedBlock(NULL);

	for(std::list<CNCCodeBlock*>::iterator It = m_blocks.begin(); It != m_blocks.end(); It++)
	{
		CNCCodeBlock* block = *It;
		if(pos < block->m_to_pos)
		{
			SetHighlightedBlock(block);
			break;
		}
	}
	DestroyGLLists();
}



static double Distance( const gp_Pnt start, const gp_Pnt end )
{
	double x_squared = (start.X() - end.X()) * (start.X() - end.X());
	double y_squared = (start.Y() - end.Y()) * (start.Y() - end.Y());
	double z_squared = (start.Z() - end.Z()) * (start.Z() - end.Z());

	return( sqrt( x_squared + y_squared + z_squared ) );
} // End Distance() routine


/**
	Generate as many points as is necessary such that the tool turns to the next
	cutting edge and, in that time (based on the spindle speed) advances at the
	feed rate.  We want to calculate material removal rate on a per-cutting edge
	basis.
 */
std::list<gp_Pnt> PathLine::Interpolate(
	const gp_Pnt & start_point,
	const gp_Pnt & end_point,
	const double feed_rate,
	const double spindle_rpm,
	const unsigned int number_of_cutting_edges) const
{
	std::list<gp_Pnt> points;

	double spindle_rps = spindle_rpm / 60.0;	// Revolutions Per Second.
	double time_between_cutting_edges = (1 / spindle_rps) / number_of_cutting_edges;

	double advance_distance = (feed_rate / 60.0) * time_between_cutting_edges;
	double number_of_interpolated_points = Distance( start_point, end_point ) / advance_distance;

	points.push_back( start_point );

	for ( int i=0; i < int(floor(number_of_interpolated_points)); i++)
	{
		double x = (((start_point.X() - end_point.X()) / number_of_interpolated_points) * i) + start_point.X();
		double y = (((start_point.Y() - end_point.Y()) / number_of_interpolated_points) * i) + start_point.Y();
		double z = (((start_point.Z() - end_point.Z()) / number_of_interpolated_points) * i) + start_point.Z();

		points.push_back( gp_Pnt( x, y, z ) );
	} // End for

	points.push_back( end_point );

	return(points);
} // End Interpolate() method


std::list<gp_Pnt> PathArc::Interpolate(
	const PathObject *previous_point,
	const double feed_rate,
	const double spindle_rpm,
	const unsigned int number_of_cutting_edges) const
{
	std::list<gp_Pnt> points;

	double spindle_rps = spindle_rpm / 60.0;	// Revolutions Per Second.
	double time_between_cutting_edges = (1 / spindle_rps) / number_of_cutting_edges;

	double advance_distance = (feed_rate / 60.0) * time_between_cutting_edges;

	// This distance is wrong for arcs.  We're doing a straight line distance but we really want a distance
	// around the arc.  TODO Fix this.
	double number_of_interpolated_points = Distance( gp_Pnt( previous_point->m_x[0], previous_point->m_x[1], previous_point->m_x[2] ),
							gp_Pnt( m_x[0], m_x[1], m_x[2] ) ) / advance_distance;

	points = Interpolate( previous_point, (unsigned int) (floor(number_of_interpolated_points)) );

	return(points);

} // End Interpolate() method


std::list< std::pair<PathObject *, CTool *> > CNCCode::GetPaths() const
{
	std::list< std::pair<PathObject *, CTool *> > paths;

	for(std::list<CNCCodeBlock*>::const_iterator l_itCodeBlock = m_blocks.begin(); l_itCodeBlock != m_blocks.end(); l_itCodeBlock++)
	{
		for (std::list<ColouredPath>::const_iterator l_itColouredPath = (*l_itCodeBlock)->m_line_strips.begin();
			l_itColouredPath != (*l_itCodeBlock)->m_line_strips.end(); l_itColouredPath++)
		{
			for (std::list< PathObject* >::const_iterator l_itPoint = l_itColouredPath->m_points.begin();
				l_itPoint != l_itColouredPath->m_points.end(); l_itPoint++)
			{
				CTool *pTool = CTool::Find( (*l_itPoint)->m_tool_number );
				if (pTool != NULL)
				{
					paths.push_back( std::make_pair( *l_itPoint, pTool ) );
				} // End if - then
			} // End for
		} // End for
	} // End for

	return(paths);
} // End GetPaths() method

void CNCCode::SetHighlightedBlock(CNCCodeBlock* block)
{
	if(m_highlighted_block)m_highlighted_block->FormatText(theApp.m_output_canvas->m_textCtrl, false, true);
	m_highlighted_block = block;
	if(m_highlighted_block)m_highlighted_block->FormatText(theApp.m_output_canvas->m_textCtrl, true, true);
}