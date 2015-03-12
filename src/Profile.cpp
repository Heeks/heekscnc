// Profile.cpp
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "stdafx.h"
#include "Profile.h"
#include "CNCConfig.h"
#include "ProgramCanvas.h"
#include "Program.h"
#include "interface/HeeksObj.h"
#include "interface/PropertyLength.h"
#include "interface/PropertyChoice.h"
#include "interface/PropertyVertex.h"
#include "interface/PropertyCheck.h"
#include "interface/PropertyInt.h"
#include "interface/InputMode.h"
#include "interface/LeftAndRight.h"
#include "tinyxml/tinyxml.h"
#include "interface/Tool.h"
#include "CTool.h"
#include "CNCPoint.h"
#include "PythonStuff.h"
#include "Tags.h"
#include "Tag.h"
#include "ProfileDlg.h"

#include <gp_Pnt.hxx>
#include <gp_Ax1.hxx>
#include <gp_Trsf.hxx>

#include <sstream>
#include <iomanip>

// static
double CProfile::max_deviation_for_spline_to_arc = 0.1;

CProfileParams::CProfileParams()
{
	m_tool_on_side = eOn;
	m_cut_mode = eConventional;
	m_auto_roll_radius = 2.0;
	m_auto_roll_on = true;
	m_auto_roll_off = true;
	m_roll_on_point[0] = m_roll_on_point[1] = m_roll_on_point[2] = 0.0;
	m_roll_off_point[0] = m_roll_off_point[1] = m_roll_off_point[2] = 0.0;
	m_start_given = false;
	m_end_given = false;
	m_start[0] = m_start[1] = m_start[2] = 0.0;
	m_end[0] = m_end[1] = m_end[2] = 0.0;

    m_extend_at_start = 0.0; 
    m_extend_at_end = 0.0; 

    m_lead_in_line_len = 1.0;
    m_lead_out_line_len= 1.0;

	m_end_beyond_full_profile = false;
	m_sort_sketches = 1;
	m_offset_extra = 0.0;
	m_do_finishing_pass = false;
	m_only_finishing_pass = false;
	m_finishing_h_feed_rate = 0.0;
	m_finishing_cut_mode = eConventional;
	m_finishing_step_down = 1.0;
}

static void on_set_tool_on_side(int value, HeeksObj* object, bool from_undo_redo){
	switch(value)
	{
	case 0:
		((CProfile*)object)->m_profile_params.m_tool_on_side = CProfileParams::eLeftOrOutside;
		break;
	case 1:
		((CProfile*)object)->m_profile_params.m_tool_on_side = CProfileParams::eRightOrInside;
		break;
	default:
		((CProfile*)object)->m_profile_params.m_tool_on_side = CProfileParams::eOn;
		break;
	}
	((CProfile*)object)->WriteDefaultValues();
}

static void on_set_cut_mode(int value, HeeksObj* object, bool from_undo_redo)
{
	((CProfile*)object)->m_profile_params.m_cut_mode = (CProfileParams::eCutMode)value;
	((CProfile*)object)->WriteDefaultValues();
}

static void on_set_auto_roll_on(bool value, HeeksObj* object){((CProfile*)object)->m_profile_params.m_auto_roll_on = value; heeksCAD->RefreshProperties();}
static void on_set_roll_on_point(const double* vt, HeeksObj* object){memcpy(((CProfile*)object)->m_profile_params.m_roll_on_point, vt, 3*sizeof(double));}
static void on_set_roll_radius(double value, HeeksObj* object){((CProfile*)object)->m_profile_params.m_auto_roll_radius = value; ((CProfile*)object)->WriteDefaultValues();}
static void on_set_auto_roll_off(bool value, HeeksObj* object){((CProfile*)object)->m_profile_params.m_auto_roll_off = value; heeksCAD->RefreshProperties();}
static void on_set_roll_off_point(const double* vt, HeeksObj* object){memcpy(((CProfile*)object)->m_profile_params.m_roll_off_point, vt, 3*sizeof(double));}
static void on_set_start_given(bool value, HeeksObj* object){((CProfile*)object)->m_profile_params.m_start_given = value; heeksCAD->RefreshProperties();}
static void on_set_start(const double* vt, HeeksObj* object){memcpy(((CProfile*)object)->m_profile_params.m_start, vt, 3*sizeof(double));}
static void on_set_end_given(bool value, HeeksObj* object){((CProfile*)object)->m_profile_params.m_end_given = value; heeksCAD->RefreshProperties();}
static void on_set_end(const double* vt, HeeksObj* object){memcpy(((CProfile*)object)->m_profile_params.m_end, vt, 3*sizeof(double));}

static void on_set_extend_at_start(double value, HeeksObj* object){((CProfile*)object)->m_profile_params.m_extend_at_start = value; ((CProfile*)object)->WriteDefaultValues();}
static void on_set_extend_at_end(double value, HeeksObj* object){((CProfile*)object)->m_profile_params.m_extend_at_end = value; ((CProfile*)object)->WriteDefaultValues();}

//lead in lead out line length
static void on_set_lead_in_line_len(double value, HeeksObj* object){((CProfile*)object)->m_profile_params.m_lead_in_line_len = value; ((CProfile*)object)->WriteDefaultValues();}
static void on_set_lead_out_line_len(double value, HeeksObj* object){((CProfile*)object)->m_profile_params.m_lead_out_line_len = value; ((CProfile*)object)->WriteDefaultValues();}


static void on_set_end_beyond_full_profile(bool value, HeeksObj* object){((CProfile*)object)->m_profile_params.m_end_beyond_full_profile = value;}
static void on_set_sort_sketches(const int value, HeeksObj* object, bool from_undo_redo)
{
	((CProfile*)object)->m_profile_params.m_sort_sketches = value;
	((CProfile*)object)->WriteDefaultValues();
}
static void on_set_offset_extra(const double value, HeeksObj* object){((CProfile*)object)->m_profile_params.m_offset_extra = value;((CProfile*)object)->WriteDefaultValues();}
static void on_set_do_finishing_pass(bool value, HeeksObj* object){((CProfile*)object)->m_profile_params.m_do_finishing_pass = value; heeksCAD->RefreshProperties();((CProfile*)object)->WriteDefaultValues();}
static void on_set_only_finishing_pass(bool value, HeeksObj* object){((CProfile*)object)->m_profile_params.m_only_finishing_pass = value; heeksCAD->RefreshProperties();((CProfile*)object)->WriteDefaultValues();}
static void on_set_finishing_h_feed_rate(double value, HeeksObj* object)
{
	((CProfile*)object)->m_profile_params.m_finishing_h_feed_rate = value;
	((CProfile*)object)->WriteDefaultValues();
}

static void on_set_finish_cut_mode(int value, HeeksObj* object, bool from_undo_redo)
{
	((CProfile*)object)->m_profile_params.m_finishing_cut_mode = (CProfileParams::eCutMode)value;
	((CProfile*)object)->WriteDefaultValues();
}

static void on_set_finish_step_down(double value, HeeksObj* object)
{
	((CProfile*)object)->m_profile_params.m_finishing_step_down = value;
	((CProfile*)object)->WriteDefaultValues();
}

void CProfileParams::GetProperties(CProfile* parent, std::list<Property *> *list)
{
	CToolParams::eToolType tool_type = CTool::FindToolType(parent->m_tool_number);

	if(CTool::IsMillingToolType(tool_type)){
		std::list< wxString > choices;

		SketchOrderType order = SketchOrderTypeUnknown;

		{
			HeeksObj* sketch = heeksCAD->GetIDObject(SketchType, parent->m_sketch);
			if((sketch) && (sketch->GetType() == SketchType))
			{
				order = heeksCAD->GetSketchOrder(sketch);
			}
		}

		switch(order)
		{
		case SketchOrderTypeOpen:
			choices.push_back(_("Left"));
			choices.push_back(_("Right"));
			break;

		case SketchOrderTypeCloseCW:
		case SketchOrderTypeCloseCCW:
			choices.push_back(_("Outside"));
			choices.push_back(_("Inside"));
			break;

		default:
			choices.push_back(_("Outside or Left"));
			choices.push_back(_("Inside or Right"));
			break;
		}
		choices.push_back(_("On"));

		int choice = int(eOn);
		switch (m_tool_on_side)
		{
			case eRightOrInside:	choice = 1;
					break;

			case eOn:	choice = 2;
					break;

			case eLeftOrOutside:	choice = 0;
					break;
		} // End switch

		list->push_back(new PropertyChoice(_("tool on side"), choices, choice, parent, on_set_tool_on_side));
	}

	if(CTool::IsMillingToolType(tool_type)){
		std::list< wxString > choices;
		choices.push_back(_("Conventional"));
		choices.push_back(_("Climb"));
		list->push_back(new PropertyChoice(_("cut mode"), choices, m_cut_mode, parent, on_set_cut_mode));
	}

	{
		list->push_back(new PropertyCheck(_("auto roll on"), m_auto_roll_on, parent, on_set_auto_roll_on));
		if(!m_auto_roll_on)list->push_back(new PropertyVertex(_("roll on point"), m_roll_on_point, parent, on_set_roll_on_point));
		list->push_back(new PropertyCheck(_("auto roll off"), m_auto_roll_off, parent, on_set_auto_roll_off));
		if(!m_auto_roll_off)list->push_back(new PropertyVertex(_("roll off point"), m_roll_off_point, parent, on_set_roll_off_point));
		if(m_auto_roll_on || m_auto_roll_off)list->push_back(new PropertyLength(_("roll radius"), m_auto_roll_radius, parent, on_set_roll_radius));
		list->push_back(new PropertyCheck(_("use start point"), m_start_given, parent, on_set_start_given));
		if(m_start_given)list->push_back(new PropertyVertex(_("start point"), m_start, parent, on_set_start));
		list->push_back(new PropertyCheck(_("use end point"), m_end_given, parent, on_set_end_given));
		if(m_end_given)
		{
			list->push_back(new PropertyVertex(_("end point"), m_end, parent, on_set_end));
			list->push_back(new PropertyCheck(_("end beyond full profile"), m_end_beyond_full_profile, parent, on_set_end_beyond_full_profile));
		}
	}
    
    list->push_back(new PropertyLength(_("extend before start"), m_extend_at_start, parent, on_set_extend_at_start));
    list->push_back(new PropertyLength(_("extend past end"), m_extend_at_end, parent, on_set_extend_at_end));

    //lead in lead out line length
    list->push_back(new PropertyLength(_("lead in line length"), m_lead_in_line_len, parent, on_set_lead_in_line_len));
    list->push_back(new PropertyLength(_("lead out line length"), m_lead_out_line_len, parent, on_set_lead_out_line_len));

	list->push_back(new PropertyLength(_("offset_extra"), m_offset_extra, parent, on_set_offset_extra));
	if(CTool::IsMillingToolType(tool_type))
	{
		list->push_back(new PropertyCheck(_("do finishing pass"), m_do_finishing_pass, parent, on_set_do_finishing_pass));
		if(m_do_finishing_pass)
		{
			list->push_back(new PropertyCheck(_("only finishing pass"), m_only_finishing_pass, parent, on_set_only_finishing_pass));
			list->push_back(new PropertyLength(_("finishing feed rate"), m_finishing_h_feed_rate, parent, on_set_finishing_h_feed_rate));

			{
				std::list< wxString > choices;
				choices.push_back(_("Conventional"));
				choices.push_back(_("Climb"));
				list->push_back(new PropertyChoice(_("finish cut mode"), choices, m_finishing_cut_mode, parent, on_set_finish_cut_mode));
			}
			list->push_back(new PropertyLength(_("finishing step down"), m_finishing_step_down, parent, on_set_finish_step_down));
		}
	}
}

void CProfileParams::WriteXMLAttributes(TiXmlNode *root)
{
	TiXmlElement * element;
	element = heeksCAD->NewXMLElement( "params" );
	heeksCAD->LinkXMLEndChild( root,  element );
	element->SetAttribute( "side", m_tool_on_side);
	element->SetAttribute( "cut_mode", m_cut_mode);
	element->SetAttribute( "auto_roll_on", m_auto_roll_on ? 1:0);
	if(!m_auto_roll_on)
	{
		element->SetDoubleAttribute( "roll_onx", m_roll_on_point[0]);
		element->SetDoubleAttribute( "roll_ony", m_roll_on_point[1]);
		element->SetDoubleAttribute( "roll_onz", m_roll_on_point[2]);
	}
	element->SetAttribute( "auto_roll_off", m_auto_roll_off ? 1:0);
	if(!m_auto_roll_off)
	{
		element->SetDoubleAttribute( "roll_offx", m_roll_off_point[0]);
		element->SetDoubleAttribute( "roll_offy", m_roll_off_point[1]);
		element->SetDoubleAttribute( "roll_offz", m_roll_off_point[2]);
	}
	if(m_auto_roll_on || m_auto_roll_off)
	{
		element->SetDoubleAttribute( "roll_radius", m_auto_roll_radius);
	}
	element->SetAttribute( "start_given", m_start_given ? 1:0);
	if(m_start_given)
	{
		element->SetDoubleAttribute( "startx", m_start[0]);
		element->SetDoubleAttribute( "starty", m_start[1]);
		element->SetDoubleAttribute( "startz", m_start[2]);
	}
	element->SetAttribute( "end_given", m_end_given ? 1:0);
	if(m_end_given)
	{
		element->SetDoubleAttribute( "endx", m_end[0]);
		element->SetDoubleAttribute( "endy", m_end[1]);
		element->SetDoubleAttribute( "endz", m_end[2]);
		element->SetAttribute( "end_beyond_full_profile", m_end_beyond_full_profile ? 1:0);
	}

	std::ostringstream l_ossValue;
	l_ossValue << m_sort_sketches;
	element->SetAttribute( "sort_sketches", l_ossValue.str().c_str());
	element->SetDoubleAttribute( "extend_at_start", m_extend_at_start);
    element->SetDoubleAttribute( "extend_at_end",m_extend_at_end);

	element->SetDoubleAttribute( "lead_in_line_len", m_lead_in_line_len);
    element->SetDoubleAttribute( "lead_out_line_len",m_lead_out_line_len);

	element->SetDoubleAttribute( "offset_extra", m_offset_extra);
	element->SetAttribute( "do_finishing_pass", m_do_finishing_pass ? 1:0);
	element->SetAttribute( "only_finishing_pass", m_only_finishing_pass ? 1:0);
	element->SetDoubleAttribute( "finishing_feed_rate", m_finishing_h_feed_rate);
	element->SetAttribute( "finish_cut_mode", m_finishing_cut_mode);
	element->SetDoubleAttribute( "finishing_step_down", m_finishing_step_down);
}

void CProfileParams::ReadFromXMLElement(TiXmlElement* pElem)
{
	int int_for_bool;
	int int_for_enum;

	if(pElem->Attribute("side", &int_for_enum))m_tool_on_side = (eSide)int_for_enum;
	if(pElem->Attribute("cut_mode", &int_for_enum))m_cut_mode = (eCutMode)int_for_enum;
	if(pElem->Attribute("auto_roll_on", &int_for_bool))m_auto_roll_on = (int_for_bool != 0);
	pElem->Attribute("roll_onx", &m_roll_on_point[0]);
	pElem->Attribute("roll_ony", &m_roll_on_point[1]);
	pElem->Attribute("roll_onz", &m_roll_on_point[2]);
	if(pElem->Attribute("auto_roll_off", &int_for_bool))m_auto_roll_off = (int_for_bool != 0);
	pElem->Attribute("roll_offx", &m_roll_off_point[0]);
	pElem->Attribute("roll_offy", &m_roll_off_point[1]);
	pElem->Attribute("roll_offz", &m_roll_off_point[2]);
	pElem->Attribute("roll_radius", &m_auto_roll_radius);
	if(pElem->Attribute("start_given", &int_for_bool))m_start_given = (int_for_bool != 0);
	pElem->Attribute("startx", &m_start[0]);
	pElem->Attribute("starty", &m_start[1]);
	pElem->Attribute("startz", &m_start[2]);
	if(pElem->Attribute("end_given", &int_for_bool))m_end_given = (int_for_bool != 0);
	pElem->Attribute("endx", &m_end[0]);
	pElem->Attribute("endy", &m_end[1]);
	pElem->Attribute("endz", &m_end[2]);
	if(pElem->Attribute("end_beyond_full_profile", &int_for_bool))m_end_beyond_full_profile = (int_for_bool != 0);
	if(pElem->Attribute("sort_sketches"))m_sort_sketches = atoi(pElem->Attribute("sort_sketches"));
	pElem->Attribute("offset_extra", &m_offset_extra);
	if(pElem->Attribute("do_finishing_pass", &int_for_bool))m_do_finishing_pass = (int_for_bool != 0);
	if(pElem->Attribute("only_finishing_pass", &int_for_bool))m_only_finishing_pass = (int_for_bool != 0);
	pElem->Attribute("finishing_feed_rate", &m_finishing_h_feed_rate);
	if(pElem->Attribute("finish_cut_mode", &int_for_enum))m_finishing_cut_mode = (eCutMode)int_for_enum;
	pElem->Attribute("finishing_step_down", &m_finishing_step_down);
    pElem->Attribute("extend_at_start", &m_extend_at_start);
    pElem->Attribute("extend_at_end",&m_extend_at_end);

    pElem->Attribute("lead_in_line_len", &m_lead_in_line_len);
    pElem->Attribute("lead_out_line_len",&m_lead_out_line_len);

}

CProfile::CProfile( const CProfile & rhs ) : CSketchOp(rhs)
{
	m_tags = new CTags;
	Add( m_tags, NULL );
	if (rhs.m_tags != NULL) *m_tags = *(rhs.m_tags);
	m_sketch = rhs.m_sketch;
	m_profile_params = rhs.m_profile_params;
}

CProfile::CProfile(int sketch, const int tool_number )
		: 	CSketchOp(sketch, tool_number, ProfileType), m_tags(NULL)
{
    ReadDefaultValues();
} // End constructor

CProfile & CProfile::operator= ( const CProfile & rhs )
{
	if (this != &rhs)
	{
		CSketchOp::operator=( rhs );
		if ((m_tags != NULL) && (rhs.m_tags != NULL)) *m_tags = *(rhs.m_tags);
		m_sketch = rhs.m_sketch;

		m_profile_params = rhs.m_profile_params;
	}

	return(*this);
}

const wxBitmap &CProfile::GetIcon()
{
	if(!m_active)return GetInactiveIcon();
	static wxBitmap* icon = NULL;
	if(icon == NULL)icon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/profile.png")));
	return *icon;
}

bool CProfile::Add(HeeksObj* object, HeeksObj* prev_object)
{
	switch(object->GetType())
	{
	case TagsType:
		m_tags = (CTags*)object;
		break;
	}

	return CSketchOp::Add(object, prev_object);
}

void CProfile::Remove(HeeksObj* object)
{
	// set the m_tags pointer to NULL, when Tags is removed from here
	if(object == m_tags)m_tags = NULL;

	CSketchOp::Remove(object);
}

Python CProfile::WriteSketchDefn(HeeksObj* sketch, bool reversed )
{
	// write the python code for the sketch
	Python python;

	if ((sketch->GetShortString() != NULL) && (wxString(sketch->GetShortString()).size() > 0))
	{
		python << (wxString::Format(_T("comment(%s)\n"), PythonString(sketch->GetShortString()).c_str()));
	}

	python << _T("curve = area.Curve()\n");

	bool started = false;
	std::list<HeeksObj*> spans;
	switch(sketch->GetType())
	{
	case SketchType:
		for(HeeksObj* span_object = sketch->GetFirstChild(); span_object; span_object = sketch->GetNextChild())
		{
			if(reversed)spans.push_front(span_object);
			else spans.push_back(span_object);
		}
		break;
	case CircleType:
		spans.push_back(sketch);
		break;
	case AreaType:
		break;
	}

	std::list<HeeksObj*> new_spans;
	for(std::list<HeeksObj*>::iterator It = spans.begin(); It != spans.end(); It++)
	{
		HeeksObj* span = *It;
		if(span->GetType() == SplineType)
		{
			std::list<HeeksObj*> new_spans2;
			heeksCAD->SplineToBiarcs(span, new_spans2, CProfile::max_deviation_for_spline_to_arc);
			if(reversed)
			{
				for(std::list<HeeksObj*>::reverse_iterator It2 = new_spans2.rbegin(); It2 != new_spans2.rend(); It2++)
				{
					HeeksObj* s = *It2;
					new_spans.push_back(s);
				}
			}
			else
			{
				for(std::list<HeeksObj*>::iterator It2 = new_spans2.begin(); It2 != new_spans2.end(); It2++)
				{
					HeeksObj* s = *It2;
					new_spans.push_back(s);
				}
			}
		}
		else
		{
			new_spans.push_back(span->MakeACopy());
		}
	}

	for(std::list<HeeksObj*>::iterator It = new_spans.begin(); It != new_spans.end(); It++)
	{
		HeeksObj* span_object = *It;
		double s[3] = {0, 0, 0};
		double e[3] = {0, 0, 0};
		double c[3] = {0, 0, 0};


		if(span_object){
			int type = span_object->GetType();
			if(type == LineType || type == ArcType || type == CircleType)
			{
				if(!started && type != CircleType)
				{
					if(reversed)span_object->GetEndPoint(s);
					else span_object->GetStartPoint(s);
					CNCPoint start(s);

					python << _T("curve.append(area.Point(");
					python << start.X(true);
					python << _T(", ");
					python << start.Y(true);
					python << _T("))\n");
					started = true;
				}
				if(reversed)span_object->GetStartPoint(e);
				else span_object->GetEndPoint(e);
				CNCPoint end(e);

				if(type == LineType)
				{
					python << _T("curve.append(area.Point(");
					python << end.X(true);
					python << _T(", ");
					python << end.Y(true);
					python << _T("))\n");
				}
				else if(type == ArcType)
				{
					span_object->GetCentrePoint(c);
					CNCPoint centre(c);

					double pos[3];
					heeksCAD->GetArcAxis(span_object, pos);
					int span_type = ((pos[2] >=0) != reversed) ? 1: -1;
					python << _T("curve.append(area.Vertex(");
					python << (span_type);
					python << (_T(", area.Point("));
					python << end.X(true);
					python << (_T(", "));
					python << end.Y(true);
					python << (_T("), area.Point("));
					python << centre.X(true);
					python << (_T(", "));
					python << centre.Y(true);
					python << (_T(")))\n"));
				}
				else if(type == CircleType)
				{
					std::list< std::pair<int, gp_Pnt > > points;
					span_object->GetCentrePoint(c);

					double radius = heeksCAD->CircleGetRadius(span_object);

					// Setup the four arcs to make up the full circle using UNadjusted
					// coordinates.  We do this so that the offsets are expressed along the
					// X and Y axes.  We will adjust the resultant points later.

					// The kurve code needs a start point first.
					points.push_back( std::make_pair(0, gp_Pnt( c[0], c[1] + radius, c[2] )) ); // north
					if(reversed)
					{
						points.push_back( std::make_pair(1, gp_Pnt( c[0] - radius, c[1], c[2] )) ); // west
						points.push_back( std::make_pair(1, gp_Pnt( c[0], c[1] - radius, c[2] )) ); // south
						points.push_back( std::make_pair(1, gp_Pnt( c[0] + radius, c[1], c[2] )) ); // east
						points.push_back( std::make_pair(1, gp_Pnt( c[0], c[1] + radius, c[2] )) ); // north
					}
					else
					{
						points.push_back( std::make_pair(-1, gp_Pnt( c[0] + radius, c[1], c[2] )) ); // east
						points.push_back( std::make_pair(-1, gp_Pnt( c[0], c[1] - radius, c[2] )) ); // south
						points.push_back( std::make_pair(-1, gp_Pnt( c[0] - radius, c[1], c[2] )) ); // west
						points.push_back( std::make_pair(-1, gp_Pnt( c[0], c[1] + radius, c[2] )) ); // north
					}

					CNCPoint centre(c);

					for (std::list< std::pair<int, gp_Pnt > >::iterator l_itPoint = points.begin(); l_itPoint != points.end(); l_itPoint++)
					{
						CNCPoint pnt( l_itPoint->second );

						python << (_T("curve.append(area.Vertex("));
						python << l_itPoint->first << _T(", area.Point(");
						python << pnt.X(true);
						python << (_T(", "));
						python << pnt.Y(true);
						python << (_T("), area.Point("));
						python << centre.X(true);
						python << (_T(", "));
						python << centre.Y(true);
						python << (_T(")))\n"));
					} // End for
				}
			}
		}
	}

	// delete the spans made
	for(std::list<HeeksObj*>::iterator It = new_spans.begin(); It != new_spans.end(); It++)
	{
		HeeksObj* span = *It;
		delete span;
	}

	python << _T("\n");

	if(m_profile_params.m_start_given || m_profile_params.m_end_given)
	{
		double startx, starty, finishx, finishy;

		wxString start_string;
		if(m_profile_params.m_start_given)
		{
#ifdef UNICODE
			std::wostringstream ss;
#else
			std::ostringstream ss;
#endif

			gp_Pnt starting(m_profile_params.m_start[0] / theApp.m_program->m_units,
					m_profile_params.m_start[1] / theApp.m_program->m_units,
					0.0 );

			startx = starting.X();
			starty = starting.Y();

			ss.imbue(std::locale("C"));
			ss<<std::setprecision(10);
			ss << ", start = area.Point(" << startx << ", " << starty << ")";
			start_string = ss.str().c_str();
		}


		wxString finish_string;
		wxString beyond_string;
		if(m_profile_params.m_end_given)
		{
#ifdef UNICODE
			std::wostringstream ss;
#else
			std::ostringstream ss;
#endif

			gp_Pnt finish(m_profile_params.m_end[0] / theApp.m_program->m_units,
					m_profile_params.m_end[1] / theApp.m_program->m_units,
					0.0 );

			finishx = finish.X();
			finishy = finish.Y();

			ss.imbue(std::locale("C"));
			ss<<std::setprecision(10);
			ss << ", finish = area.Point(" << finishx << ", " << finishy << ")";
			finish_string = ss.str().c_str();

			if(m_profile_params.m_end_beyond_full_profile)beyond_string = _T(", end_beyond = True");
		}

		python << (wxString::Format(_T("kurve_funcs.make_smaller( curve%s%s%s)\n"), start_string.c_str(), finish_string.c_str(), beyond_string.c_str())).c_str();
	}

	return(python);
}

Python CProfile::AppendTextForSketch(HeeksObj* object, CProfileParams::eCutMode cut_mode)
{
    Python python;

	if(object)
	{
		// decide if we need to reverse the kurve
		bool reversed = false;
		bool initially_ccw = false;
		if(m_profile_params.m_tool_on_side != CProfileParams::eOn)
		{
			if(object)
			{
				switch(object->GetType())
				{
				case CircleType:
				case AreaType:
					initially_ccw = true;
					break;
				case SketchType:
					SketchOrderType order = heeksCAD->GetSketchOrder(object);
					if(order == SketchOrderTypeCloseCCW)initially_ccw = true;
					break;
				}
			}
			if(m_speed_op_params.m_spindle_speed<0)reversed = !reversed;
			if(cut_mode == CProfileParams::eConventional)reversed = !reversed;
			if(m_profile_params.m_tool_on_side == CProfileParams::eRightOrInside)reversed = !reversed;
		}

		// write the kurve definition
		python << WriteSketchDefn(object, initially_ccw != reversed);

		if((m_profile_params.m_start_given == false) && (m_profile_params.m_end_given == false))
		{
			python << _T("kurve_funcs.set_good_start_point(curve, ") << (reversed ? _T("True") : _T("False")) << _T(")\n");
		}

		// start - assume we are at a suitable clearance height

		// get offset side string
		wxString side_string;
		switch(m_profile_params.m_tool_on_side)
		{
		case CProfileParams::eLeftOrOutside:
			if(reversed)side_string = _T("right");
			else side_string = _T("left");
			break;
		case CProfileParams::eRightOrInside:
			if(reversed)side_string = _T("left");
			else side_string = _T("right");
			break;
		default:
			side_string = _T("on");
			break;
		}

		// roll on
		switch(m_profile_params.m_tool_on_side)
		{
		case CProfileParams::eLeftOrOutside:
		case CProfileParams::eRightOrInside:
			{
				if(m_profile_params.m_auto_roll_on)
				{
					python << wxString(_T("roll_on = 'auto'\n"));
				}
				else
				{
					python << wxString(_T("roll_on = area.Point(")) << m_profile_params.m_roll_on_point[0] / theApp.m_program->m_units << wxString(_T(", ")) << m_profile_params.m_roll_on_point[1] / theApp.m_program->m_units << wxString(_T(")\n"));
				}
			}
			break;
		default:
			{
				python << _T("roll_on = None\n");
			}
			break;
		}

		// rapid across to it
		//python << wxString::Format(_T("rapid(%s)\n"), roll_on_string.c_str()).c_str();

		switch(m_profile_params.m_tool_on_side)
		{
		case CProfileParams::eLeftOrOutside:
		case CProfileParams::eRightOrInside:
			{
				if(m_profile_params.m_auto_roll_off)
				{
					python << wxString(_T("roll_off = 'auto'\n"));
				}
				else
				{
					python << wxString(_T("roll_off = area.Point(")) << m_profile_params.m_roll_off_point[0] / theApp.m_program->m_units << wxString(_T(", ")) << m_profile_params.m_roll_off_point[1] / theApp.m_program->m_units << wxString(_T(")\n"));
				}
			}
			break;
		default:
			{
				python << _T("roll_off = None\n");
			}
			break;
		}

		bool tags_cleared = false;
		for(CTag* tag = (CTag*)(m_tags->GetFirstChild()); tag; tag = (CTag*)(m_tags->GetNextChild()))
		{
			if(!tags_cleared)python << _T("kurve_funcs.clear_tags()\n");
			tags_cleared = true;
			python << _T("kurve_funcs.add_tag(area.Point(") << tag->m_pos[0] / theApp.m_program->m_units << _T(", ") << tag->m_pos[1] / theApp.m_program->m_units << _T("), ") << tag->m_width / theApp.m_program->m_units << _T(", ") << tag->m_angle * M_PI/180 << _T(", ") << tag->m_height / theApp.m_program->m_units << _T(")\n");
		}
        //extend_at_start, extend_at_end
        python << _T("extend_at_start= ") << m_profile_params.m_extend_at_start / theApp.m_program->m_units << _T("\n");
        python << _T("extend_at_end= ") << m_profile_params.m_extend_at_end / theApp.m_program->m_units<< _T("\n");

        //lead in lead out line length
        python << _T("lead_in_line_len= ") << m_profile_params.m_lead_in_line_len / theApp.m_program->m_units << _T("\n");
        python << _T("lead_out_line_len= ") << m_profile_params.m_lead_out_line_len / theApp.m_program->m_units<< _T("\n");

		// profile the kurve
		python << wxString::Format(_T("kurve_funcs.profile(curve, '%s', tool_diameter/2, offset_extra, roll_radius, roll_on, roll_off, depthparams, extend_at_start,extend_at_end,lead_in_line_len,lead_out_line_len )\n"), side_string.c_str());
	}
	python << _T("absolute()\n");
	return(python);
}

void CProfile::WriteDefaultValues()
{
	CSketchOp::WriteDefaultValues();

	CNCConfig config;
	config.Write(_T("ToolOnSide"), (int)(m_profile_params.m_tool_on_side));
	config.Write(_T("CutMode"), (int)(m_profile_params.m_cut_mode));
	config.Write(_T("RollRadius"), m_profile_params.m_auto_roll_radius);
	config.Write(_T("OffsetExtra"), m_profile_params.m_offset_extra);
	config.Write(_T("DoFinishPass"), m_profile_params.m_do_finishing_pass);
	config.Write(_T("OnlyFinishPass"), m_profile_params.m_only_finishing_pass);
	config.Write(_T("FinishFeedRate"), m_profile_params.m_finishing_h_feed_rate);
	config.Write(_T("FinishCutMode"), (int)(m_profile_params.m_finishing_cut_mode));
	config.Write(_T("FinishStepDown"), m_profile_params.m_finishing_step_down);
	config.Write(_T("EndBeyond"), m_profile_params.m_end_beyond_full_profile);

	config.Write(_T("ExtendAtStart"), m_profile_params.m_extend_at_start);
	config.Write(_T("ExtendAtEnd"), m_profile_params.m_extend_at_end);
	config.Write(_T("LeadInLineLen"), m_profile_params.m_lead_in_line_len);
	config.Write(_T("LeadOutLineLen"), m_profile_params.m_lead_out_line_len);

}

void CProfile::ReadDefaultValues()
{
	CSketchOp::ReadDefaultValues();

	CNCConfig config;
	int int_side = m_profile_params.m_tool_on_side;
	config.Read(_T("ToolOnSide"), &int_side, CProfileParams::eLeftOrOutside);
	m_profile_params.m_tool_on_side = (CProfileParams::eSide)int_side;
	int int_mode = m_profile_params.m_cut_mode;
	config.Read(_T("CutMode"), &int_mode, CProfileParams::eConventional);
	m_profile_params.m_cut_mode = (CProfileParams::eCutMode)int_mode;
	config.Read(_T("RollRadius"), &m_profile_params.m_auto_roll_radius, 2.0);
	config.Read(_T("OffsetExtra"), &m_profile_params.m_offset_extra, 0.0);
	config.Read(_T("DoFinishPass"), &m_profile_params.m_do_finishing_pass, false);
	config.Read(_T("OnlyFinishPass"), &m_profile_params.m_only_finishing_pass, false);
	config.Read(_T("FinishFeedRate"), &m_profile_params.m_finishing_h_feed_rate, 100.0);
	config.Read(_T("FinishCutMode"), &int_mode, CProfileParams::eConventional);
	m_profile_params.m_finishing_cut_mode = (CProfileParams::eCutMode)int_mode;
	config.Read(_T("FinishStepDown"), &m_profile_params.m_finishing_step_down, 1.0);
	config.Read(_T("EndBeyond"), &m_profile_params.m_end_beyond_full_profile, false);

	config.Read(_T("ExtendAtStart"), &m_profile_params.m_extend_at_start, 0.0);
	config.Read(_T("ExtendAtEnd"), &m_profile_params.m_extend_at_end, 0.0);
	config.Read(_T("LeadInLineLen"), &m_profile_params.m_lead_in_line_len, 0.0);
	config.Read(_T("LeadOutLineLen"), &m_profile_params.m_lead_out_line_len, 0.0);
}

Python CProfile::AppendTextToProgram()
{
	Python python;

	// only do finish pass for non milling cutters
	if(!CTool::IsMillingToolType(CTool::FindToolType(m_tool_number)))
	{
		this->m_profile_params.m_only_finishing_pass = true;
	}

	// roughing pass
	if(!this->m_profile_params.m_do_finishing_pass || !this->m_profile_params.m_only_finishing_pass)
	{
		python << AppendTextToProgram(false);
	}

	// finishing pass
	if(this->m_profile_params.m_do_finishing_pass)
	{
		python << AppendTextToProgram(true);
	}

	return python;
}

Python CProfile::AppendTextToProgram(bool finishing_pass)
{
	Python python;

	CTool *pTool = CTool::Find( m_tool_number );
	if (pTool == NULL)
	{
		if(!finishing_pass) wxMessageBox(_("Cannot generate G-Code for profile without a tool assigned"));
		return(python);
	} // End if - then

	if(!finishing_pass || m_profile_params.m_only_finishing_pass)
	{
		python << CSketchOp::AppendTextToProgram();

		if(m_profile_params.m_auto_roll_on || m_profile_params.m_auto_roll_off)
		{
			python << _T("roll_radius = float(");
			python << m_profile_params.m_auto_roll_radius / theApp.m_program->m_units;
			python << _T(")\n");
		}
	}

	if(finishing_pass)
	{
		python << _T("feedrate_hv(") << m_profile_params.m_finishing_h_feed_rate / theApp.m_program->m_units << _T(", ");
		python << m_speed_op_params.m_vertical_feed_rate / theApp.m_program->m_units << _T(")\n");
		python << _T("flush_nc()\n");
		python << _T("offset_extra = 0.0\n");
		python << _T("depthparams.step_down = ") << m_profile_params.m_finishing_step_down << _T("\n");
		python << _T("depthparams.z_finish_depth = 0.0\n");
	}
	else
	{
		python << _T("offset_extra = ") << m_profile_params.m_offset_extra / theApp.m_program->m_units << _T("\n");
	}

	CProfileParams::eCutMode cut_mode = finishing_pass ? m_profile_params.m_finishing_cut_mode : m_profile_params.m_cut_mode;

	HeeksObj* object = heeksCAD->GetIDObject(SketchType, m_sketch);
	if(object)
	{
		HeeksObj* sketch_to_be_deleted = NULL;
		if(object->GetType() == AreaType)
		{
			object = heeksCAD->NewSketchFromArea(object);
			sketch_to_be_deleted = object;
		}

		if(object->GetType() == SketchType)
		{
			HeeksObj* re_ordered_sketch = NULL;
			SketchOrderType sketch_order = heeksCAD->GetSketchOrder(object);
			if(sketch_order == SketchOrderTypeBad)
			{
				re_ordered_sketch = object->MakeACopy();
				heeksCAD->ReOrderSketch(re_ordered_sketch, SketchOrderTypeReOrder);
				object = re_ordered_sketch;
			}

			if(sketch_order == SketchOrderTypeMultipleCurves || sketch_order == SketchOrderHasCircles)
			{
				std::list<HeeksObj*> new_separate_sketches;
				heeksCAD->ExtractSeparateSketches(object, new_separate_sketches, false);
				for(std::list<HeeksObj*>::iterator It = new_separate_sketches.begin(); It != new_separate_sketches.end(); It++)
				{
					HeeksObj* one_curve_sketch = *It;
					python << AppendTextForSketch(one_curve_sketch, cut_mode).c_str();
					delete one_curve_sketch;
				}
			}
			else
			{
				python << AppendTextForSketch(object, cut_mode).c_str();
			}

			if(re_ordered_sketch)
			{
				delete re_ordered_sketch;
			}
		}
		else
		{
			python << AppendTextForSketch(object, cut_mode).c_str();
		}

		delete sketch_to_be_deleted;
	}

	return python;
} // End AppendTextToProgram() method

static unsigned char cross16[32] = {0x80, 0x01, 0x40, 0x02, 0x20, 0x04, 0x10, 0x08, 0x08, 0x10, 0x04, 0x20, 0x02, 0x40, 0x01, 0x80, 0x01, 0x80, 0x02, 0x40, 0x04, 0x20, 0x08, 0x10, 0x10, 0x08, 0x20, 0x04, 0x40, 0x02, 0x80, 0x01};

void CProfile::glCommands(bool select, bool marked, bool no_color)
{
	CSketchOp::glCommands(select, marked, no_color);

	if(marked && !no_color)
	{
		{
			// draw roll on point
			if(!m_profile_params.m_auto_roll_on)
			{
				glColor3ub(0, 200, 200);
				glRasterPos3dv(m_profile_params.m_roll_on_point);
				glBitmap(16, 16, 8, 8, 10.0, 0.0, cross16);
			}
			// draw roll off point
			if(!m_profile_params.m_auto_roll_on)
			{
				glColor3ub(255, 128, 0);
				glRasterPos3dv(m_profile_params.m_roll_off_point);
				glBitmap(16, 16, 8, 8, 10.0, 0.0, cross16);
			}
			// draw start point
			if(m_profile_params.m_start_given)
			{
				glColor3ub(128, 0, 255);
				glRasterPos3dv(m_profile_params.m_start);
				glBitmap(16, 16, 8, 8, 10.0, 0.0, cross16);
			}
			// draw end point
			if(m_profile_params.m_end_given)
			{
				glColor3ub(200, 200, 0);
				glRasterPos3dv(m_profile_params.m_end);
				glBitmap(16, 16, 8, 8, 10.0, 0.0, cross16);
			}
		}
	}
}

void CProfile::GetProperties(std::list<Property *> *list)
{
	m_profile_params.GetProperties(this, list);

	CSketchOp::GetProperties(list);
}

static CProfile* object_for_tools = NULL;

class PickStart: public Tool{
	// Tool's virtual functions
	const wxChar* GetTitle(){return _("Pick Start");}
	void Run(){if(heeksCAD->PickPosition(_("Pick new start point"), object_for_tools->m_profile_params.m_start))object_for_tools->m_profile_params.m_start_given = true; heeksCAD->RefreshProperties();}
	wxString BitmapPath(){ return _T("pickstart");}
};

static PickStart pick_start;

class PickEnd: public Tool{
	// Tool's virtual functions
	const wxChar* GetTitle(){return _("Pick End");}
	void Run(){if(heeksCAD->PickPosition(_("Pick new end point"), object_for_tools->m_profile_params.m_end))object_for_tools->m_profile_params.m_end_given = true; heeksCAD->RefreshProperties();}
	wxString BitmapPath(){ return _T("pickend");}
};

static PickEnd pick_end;

class PickRollOn: public Tool{
	// Tool's virtual functions
	const wxChar* GetTitle(){return _("Pick roll on point");}
	void Run(){if(heeksCAD->PickPosition(_("Pick roll on point"), object_for_tools->m_profile_params.m_roll_on_point))object_for_tools->m_profile_params.m_auto_roll_on = false;}
	wxString BitmapPath(){ return _T("rollon");}
};

static PickRollOn pick_roll_on;

class PickRollOff: public Tool{
	// Tool's virtual functions
	const wxChar* GetTitle(){return _("Pick roll off point");}
	void Run(){if(heeksCAD->PickPosition(_("Pick roll off point"), object_for_tools->m_profile_params.m_roll_off_point))object_for_tools->m_profile_params.m_auto_roll_off = false;}
	wxString BitmapPath(){ return _T("rolloff");}
};

static PickRollOff pick_roll_off;



class TagAddingMode: public CInputMode, CLeftAndRight{
private:
	wxPoint clicked_point;

public:
	// virtual functions for InputMode
	const wxChar* GetTitle(){return _("Add tags by clicking");}
	void OnMouse( wxMouseEvent& event );
	void OnKeyDown(wxKeyEvent& event);
	void GetTools(std::list<Tool*> *f_list, const wxPoint *p);
};


void TagAddingMode::OnMouse( wxMouseEvent& event )
{
	bool event_used = false;

	if(LeftAndRightPressed(event, event_used))
	{
		heeksCAD->SetInputMode(heeksCAD->GetSelectMode());
	}

	if(!event_used){
		if(event.MiddleIsDown() || event.GetWheelRotation() != 0)
		{
			heeksCAD->GetSelectMode()->OnMouse(event);
		}
		else{
			if(event.LeftDown()){
				double pos[3];
				heeksCAD->Digitize(event.GetPosition(), pos);
			}
			else if(event.LeftUp()){
				double pos[3];
				if(heeksCAD->GetLastDigitizePosition(pos))
				{
					CTag* new_object = new CTag();
					new_object->m_pos[0] = pos[0];
					new_object->m_pos[1] = pos[1];
					heeksCAD->StartHistory();
					heeksCAD->AddUndoably(new_object, object_for_tools->Tags());
					heeksCAD->EndHistory();
				}
			}
			else if(event.RightUp()){
				// do context menu same as select mode
				heeksCAD->GetSelectMode()->OnMouse(event);
			}
		}
	}
}

void TagAddingMode::OnKeyDown(wxKeyEvent& event)
{
	switch(event.GetKeyCode()){
	case WXK_F1:
	case WXK_RETURN:
	case WXK_ESCAPE:
		// end drawing mode
		heeksCAD->SetInputMode(heeksCAD->GetSelectMode());
	}
}

class EndDrawing:public Tool{
public:
	void Run(){heeksCAD->SetInputMode(heeksCAD->GetSelectMode());}
	const wxChar* GetTitle(){return _("Stop adding tags");}
	wxString BitmapPath(){return _T("enddraw");}
};

static EndDrawing end_drawing;

void TagAddingMode::GetTools(std::list<Tool*> *f_list, const wxPoint *p){
	f_list->push_back(&end_drawing);
}


TagAddingMode tag_adding_mode;

class AddTagTool: public Tool
{
public:
	// Tool's virtual functions
	const wxChar* GetTitle(){return _("Add Tags");}

	void Run()
	{
		heeksCAD->SetInputMode(&tag_adding_mode);
	}
	wxString BitmapPath(){ return _T("addtag");}
};

static AddTagTool add_tag_tool;

void CProfile::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
	object_for_tools = this;
	t_list->push_back(&pick_start);
	t_list->push_back(&pick_end);
	t_list->push_back(&pick_roll_on);
	t_list->push_back(&pick_roll_off);
	t_list->push_back(&add_tag_tool);

	CSketchOp::GetTools(t_list, p);
}

HeeksObj *CProfile::MakeACopy(void)const
{
	return new CProfile(*this);
}

void CProfile::CopyFrom(const HeeksObj* object)
{
	if (object->GetType() == GetType())
	{
		CProfile *rhs = (CProfile *) object;

		if ((m_tags != NULL) && (rhs->m_tags != NULL)) m_tags->CopyFrom( rhs->m_tags );

		m_sketch = rhs->m_sketch;
		m_profile_params = rhs->m_profile_params;
		m_depth_op_params = rhs->m_depth_op_params;
		m_speed_op_params = rhs->m_speed_op_params;
		m_comment = rhs->m_comment;
		m_active = rhs->m_active;
		m_tool_number = rhs->m_tool_number;
		m_operation_type = rhs->m_operation_type;
		m_pattern = rhs->m_pattern;
		m_surface = rhs->m_surface;
	}
}

bool CProfile::CanAdd(HeeksObj* object)
{
	return ((object != NULL) && (object->GetType() == TagsType || object->GetType() == SketchType));
}

bool CProfile::CanAddTo(HeeksObj* owner)
{
	return ((owner != NULL) && (owner->GetType() == OperationsType));
}

void CProfile::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element = heeksCAD->NewXMLElement( "Profile" );
	heeksCAD->LinkXMLEndChild( root,  element );
	m_profile_params.WriteXMLAttributes(element);

	CSketchOp::WriteBaseXML(element);
}

// static member function
HeeksObj* CProfile::ReadFromXMLElement(TiXmlElement* element)
{
	CProfile* new_object = new CProfile;

	std::list<TiXmlElement *> elements_to_remove;

	// read profile parameters
	TiXmlElement* params = heeksCAD->FirstNamedXMLChildElement(element, "params");
	if(params)
	{
		new_object->m_profile_params.ReadFromXMLElement(params);
		elements_to_remove.push_back(params);
	}

	for (std::list<TiXmlElement*>::iterator itElem = elements_to_remove.begin(); itElem != elements_to_remove.end(); itElem++)
	{
		heeksCAD->RemoveXMLChild( element, *itElem);
	}

	// read common parameters
	new_object->ReadBaseXML(element);

	new_object->AddMissingChildren();

	return new_object;
}

void CProfile::AddMissingChildren()
{
	// make sure "tags" exists
	if(m_tags == NULL){m_tags = new CTags; Add( m_tags, NULL );}
}

static void on_set_spline_deviation(double value, HeeksObj* object){
	CProfile::max_deviation_for_spline_to_arc = value;
	CProfile::WriteToConfig();
}

// static
void CProfile::GetOptions(std::list<Property *> *list)
{
	list->push_back ( new PropertyDouble ( _("Profile spline deviation"), max_deviation_for_spline_to_arc, NULL, on_set_spline_deviation ) );
}

// static
void CProfile::ReadFromConfig()
{
	CNCConfig config;
	config.Read(_T("ProfileSplineDeviation"), &max_deviation_for_spline_to_arc, 0.01);
}

// static
void CProfile::WriteToConfig()
{
	CNCConfig config;
	config.Write(_T("ProfileSplineDeviation"), max_deviation_for_spline_to_arc);
}
bool CProfileParams::operator==( const CProfileParams & rhs ) const
{
	if (m_auto_roll_on != rhs.m_auto_roll_on) return(false);
	if (m_auto_roll_off != rhs.m_auto_roll_off) return(false);
	if (m_auto_roll_radius != rhs.m_auto_roll_radius) return(false);
	for (::size_t i=0; i<sizeof(m_roll_on_point)/sizeof(m_roll_on_point[0]); i++) if (m_roll_on_point[i] != rhs.m_roll_on_point[i]) return(false);
	for (::size_t i=0; i<sizeof(m_roll_off_point)/sizeof(m_roll_off_point[0]); i++) if (m_roll_off_point[i] != rhs.m_roll_off_point[i]) return(false);
	if (m_start_given != rhs.m_start_given) return(false);
	if (m_end_given != rhs.m_end_given) return(false);
	if (m_end_beyond_full_profile != rhs.m_end_beyond_full_profile) return(false);
	for (::size_t i=0; i<sizeof(m_start)/sizeof(m_start[0]); i++) if (m_start[i] != rhs.m_start[i]) return(false);
	for (::size_t i=0; i<sizeof(m_end)/sizeof(m_end[0]); i++) if (m_end[i] != rhs.m_end[i]) return(false);
	if (m_sort_sketches != rhs.m_sort_sketches) return(false);
	if (m_offset_extra != rhs.m_offset_extra) return(false);
	if (m_do_finishing_pass != rhs.m_do_finishing_pass) return(false);
	if (m_finishing_h_feed_rate != rhs.m_finishing_h_feed_rate) return(false);
	if (m_finishing_cut_mode != rhs.m_finishing_cut_mode) return(false);

	return(true);
}

bool CProfile::operator==( const CProfile & rhs ) const
{
	if (m_profile_params != rhs.m_profile_params) return(false);

	if(m_sketch != rhs.m_sketch)return false;

	return(CSketchOp::operator==(rhs));
}

static bool OnEdit(HeeksObj* object)
{
	return ProfileDlg::Do((CProfile*)object);
}

void CProfile::GetOnEdit(bool(**callback)(HeeksObj*))
{
	*callback = OnEdit;
}

void CProfile::Clear()
{
	if (m_tags != NULL)m_tags->Clear();
}
