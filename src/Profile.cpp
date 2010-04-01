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
#include "tinyxml/tinyxml.h"
#include "interface/Tool.h"
#include "CuttingTool.h"
#include "CNCPoint.h"
#include "Reselect.h"

#include <gp_Pnt.hxx>
#include <gp_Ax1.hxx>
#include <gp_Trsf.hxx>

#include "geometry.h"	// from the kurve directory.

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
	m_sort_sketches = 1;
	m_num_tags = 0;
	m_tag_width = 5;
	m_tag_angle = 45;
	m_offset_extra = 0.0;
}

static void on_set_tool_on_side(int value, HeeksObj* object){
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

static void on_set_cut_mode(int value, HeeksObj* object)
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
static void on_set_sort_sketches(const int value, HeeksObj* object)
{
	((CProfile*)object)->m_profile_params.m_sort_sketches = value;
	((CProfile*)object)->WriteDefaultValues();
}
static void on_set_num_tags(const int value, HeeksObj* object){((CProfile*)object)->m_profile_params.m_num_tags = value;}
static void on_set_tag_width(const double value, HeeksObj* object){((CProfile*)object)->m_profile_params.m_tag_width = value;}
static void on_set_tag_angle(const double value, HeeksObj* object){((CProfile*)object)->m_profile_params.m_tag_angle = value;}
static void on_set_offset_extra(const double value, HeeksObj* object){((CProfile*)object)->m_profile_params.m_offset_extra = value;}

void CProfileParams::GetProperties(CProfile* parent, std::list<Property *> *list)
{
	{
		std::list< wxString > choices;

		SketchOrderType order = SketchOrderTypeUnknown;

		if(parent->GetNumChildren() == 1)
		{
			HeeksObj* sketch = parent->GetFirstChild();
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

	{
		std::list< wxString > choices;
		choices.push_back(_("Conventional"));
		choices.push_back(_("Climb"));
		list->push_back(new PropertyChoice(_("cut mode"), choices, m_cut_mode, parent, on_set_cut_mode));
	}

	if(parent->GetNumChildren() == 1) // multiple sketches must use auto roll on, and can not have start and end points specified
	{
		list->push_back(new PropertyCheck(_("auto roll on"), m_auto_roll_on, parent, on_set_auto_roll_on));
		if(!m_auto_roll_on)list->push_back(new PropertyVertex(_("roll on point"), m_roll_on_point, parent, on_set_roll_on_point));
		list->push_back(new PropertyCheck(_("auto roll off"), m_auto_roll_off, parent, on_set_auto_roll_off));
		if(!m_auto_roll_off)list->push_back(new PropertyVertex(_("roll off point"), m_roll_off_point, parent, on_set_roll_off_point));
		if(m_auto_roll_on || m_auto_roll_off)list->push_back(new PropertyLength(_("roll radius"), m_auto_roll_radius, parent, on_set_roll_radius));
		list->push_back(new PropertyCheck(_("use start point"), m_start_given, parent, on_set_start_given));
		if(m_start_given)list->push_back(new PropertyVertex(_("start point"), m_start, parent, on_set_start));
		list->push_back(new PropertyCheck(_("use end point"), m_end_given, parent, on_set_end_given));
		if(m_end_given)list->push_back(new PropertyVertex(_("end point"), m_end, parent, on_set_end));

	}
	else
	{
		std::list< wxString > choices;

		choices.push_back(_("Respect existing order"));	// Must be 'false' (0)
		choices.push_back(_("True"));			// Must be 'true' (non-zero)

		int choice = int(m_sort_sketches);
		list->push_back(new PropertyChoice(_("sort_sketches"), choices, choice, parent, on_set_sort_sketches));
	} // End if - else

	list->push_back(new PropertyInt(_("number of tags"), m_num_tags, parent, on_set_num_tags));
	if(m_num_tags)
	{
		list->push_back(new PropertyLength(_("tag width"), m_tag_width, parent, on_set_tag_width));
		list->push_back(new PropertyDouble(_("tag angle"), m_tag_angle, parent, on_set_tag_angle));
	}
	list->push_back(new PropertyLength(_("offset_extra"), m_offset_extra, parent, on_set_offset_extra));
}

void CProfileParams::WriteXMLAttributes(TiXmlNode *root)
{
	TiXmlElement * element;
	element = new TiXmlElement( "params" );
	root->LinkEndChild( element );
	element->SetAttribute("side", m_tool_on_side);
	element->SetAttribute("cut_mode", m_cut_mode);
	element->SetAttribute("auto_roll_on", m_auto_roll_on ? 1:0);
	if(!m_auto_roll_on)
	{
		element->SetDoubleAttribute("roll_onx", m_roll_on_point[0]);
		element->SetDoubleAttribute("roll_ony", m_roll_on_point[1]);
		element->SetDoubleAttribute("roll_onz", m_roll_on_point[2]);
	}
	element->SetAttribute("auto_roll_off", m_auto_roll_off ? 1:0);
	if(!m_auto_roll_off)
	{
		element->SetDoubleAttribute("roll_offx", m_roll_off_point[0]);
		element->SetDoubleAttribute("roll_offy", m_roll_off_point[1]);
		element->SetDoubleAttribute("roll_offz", m_roll_off_point[2]);
	}
	if(m_auto_roll_on || m_auto_roll_off)
	{
		element->SetDoubleAttribute("roll_radius", m_auto_roll_radius);
	}
	element->SetAttribute("start_given", m_start_given ? 1:0);
	if(m_start_given)
	{
		element->SetDoubleAttribute("startx", m_start[0]);
		element->SetDoubleAttribute("starty", m_start[1]);
		element->SetDoubleAttribute("startz", m_start[2]);
	}
	element->SetAttribute("end_given", m_end_given ? 1:0);
	if(m_end_given)
	{
		element->SetDoubleAttribute("endx", m_end[0]);
		element->SetDoubleAttribute("endy", m_end[1]);
		element->SetDoubleAttribute("endz", m_end[2]);
	}

	std::ostringstream l_ossValue;
	l_ossValue << m_sort_sketches;
	element->SetAttribute("sort_sketches", l_ossValue.str().c_str());

	if(m_num_tags)
	{
		element->SetAttribute("num_tags", m_num_tags);
		element->SetDoubleAttribute("tag_width", m_tag_width);
		element->SetDoubleAttribute("tag_angle", m_tag_angle);
	}

	element->SetDoubleAttribute("offset_extra", m_offset_extra);
}

void CProfileParams::ReadFromXMLElement(TiXmlElement* pElem)
{
	int int_for_bool;
	int int_for_enum;;

	int_for_enum = m_tool_on_side;
	pElem->Attribute("side", &int_for_enum);
	m_tool_on_side = (eSide)int_for_enum;

	int_for_enum = m_cut_mode;
	pElem->Attribute("cut_mode", &int_for_enum);
	m_cut_mode = (eCutMode)int_for_enum;

	pElem->Attribute("auto_roll_on", &int_for_bool); m_auto_roll_on = (int_for_bool != 0);
	pElem->Attribute("roll_onx", &m_roll_on_point[0]);
	pElem->Attribute("roll_ony", &m_roll_on_point[1]);
	pElem->Attribute("roll_onz", &m_roll_on_point[2]);
	pElem->Attribute("auto_roll_off", &int_for_bool); m_auto_roll_off = (int_for_bool != 0);
	pElem->Attribute("roll_offx", &m_roll_off_point[0]);
	pElem->Attribute("roll_offy", &m_roll_off_point[1]);
	pElem->Attribute("roll_offz", &m_roll_off_point[2]);
	pElem->Attribute("roll_radius", &m_auto_roll_radius);
	pElem->Attribute("start_given", &int_for_bool); m_start_given = (int_for_bool != 0);
	pElem->Attribute("startx", &m_start[0]);
	pElem->Attribute("starty", &m_start[1]);
	pElem->Attribute("startz", &m_start[2]);
	pElem->Attribute("end_given", &int_for_bool); m_end_given = (int_for_bool != 0);
	pElem->Attribute("endx", &m_end[0]);
	pElem->Attribute("endy", &m_end[1]);
	pElem->Attribute("endz", &m_end[2]);
	if (pElem->Attribute("sort_sketches"))
	{
		m_sort_sketches = atoi(pElem->Attribute("sort_sketches"));
	} // End if - then
	else
	{
		m_sort_sketches = 1;	// Default.
	} // End if - else
	pElem->Attribute("num_tags", &m_num_tags);
	pElem->Attribute("tag_width", &m_tag_width);
	pElem->Attribute("tag_angle", &m_tag_angle);
	pElem->Attribute("offset_extra", &m_offset_extra);
}

CProfile::CProfile( const CProfile & rhs ) : CDepthOp(rhs)
{
	*this = rhs;	 // Call the assignment operator.
}

CProfile & CProfile::operator= ( const CProfile & rhs )
{
	if (this != &rhs)
	{
		CDepthOp::operator=( rhs );
		m_sketches.clear();
		std::copy( rhs.m_sketches.begin(), rhs.m_sketches.end(), std::inserter( m_sketches, m_sketches.begin() ) );

		m_profile_params = rhs.m_profile_params;

		// static double max_deviation_for_spline_to_arc;
	}

	return(*this);
}


void CProfile::GetRollOnPos(HeeksObj* sketch, double &x, double &y)
{
	// roll on
	if(m_profile_params.m_auto_roll_on)
	{
		if(sketch)
		{
			HeeksObj* first_child = sketch->GetAtIndex(0);
			if(first_child)
			{
				double s[3];
				if(!(first_child->GetStartPoint(s)))return;
				x = s[0];
				y = s[1];
				if(m_profile_params.m_tool_on_side == CProfileParams::eOn)return;
				double v[3];
				if(heeksCAD->GetSegmentVector(first_child, 0.0, v))
				{
					double off_vec[3] = {-v[1], v[0], 0.0};
					if(m_profile_params.m_tool_on_side == CProfileParams::eRightOrInside){off_vec[0] = -off_vec[0]; off_vec[1] = -off_vec[1];}

					CCuttingTool *pCuttingTool = CCuttingTool::Find( m_cutting_tool_number );
					if (pCuttingTool != NULL)
					{
						x = s[0] + off_vec[0] * (pCuttingTool->CuttingRadius() + m_profile_params.m_auto_roll_radius) - v[0] * m_profile_params.m_auto_roll_radius;
						y = s[1] + off_vec[1] * (pCuttingTool->CuttingRadius() + m_profile_params.m_auto_roll_radius) - v[1] * m_profile_params.m_auto_roll_radius;
					} // End if - then
				}
			}
		}
	}
	else
	{
		x = m_profile_params.m_roll_on_point[0];
		y = m_profile_params.m_roll_on_point[1];
	}
}

void CProfile::GetRollOffPos(HeeksObj* sketch, double &x, double &y)
{
	// roll off
	if(m_profile_params.m_auto_roll_off)
	{
			int num_spans = sketch->GetNumChildren();
			if(num_spans > 0)
			{
				HeeksObj* last_child = sketch->GetAtIndex(num_spans - 1);
				if(last_child)
				{
					double e[3];
					if(!(last_child->GetEndPoint(e)))return;
					x = e[0];
					y = e[1];
					if(m_profile_params.m_tool_on_side == CProfileParams::eOn)return;
					double v[3];
					if(heeksCAD->GetSegmentVector(last_child, 0.0, v))
					{
						CCuttingTool *pCuttingTool = CCuttingTool::Find( m_cutting_tool_number );
						if (pCuttingTool != NULL)
						{
							double off_vec[3] = {-v[1], v[0], 0.0};
							if(m_profile_params.m_tool_on_side == CProfileParams::eRightOrInside){off_vec[0] = -off_vec[0]; off_vec[1] = -off_vec[1];}
							x = e[0] + off_vec[0] * (pCuttingTool->CuttingRadius() + m_profile_params.m_auto_roll_radius) + v[0] * m_profile_params.m_auto_roll_radius;
							y = e[1] + off_vec[1] * (pCuttingTool->CuttingRadius() + m_profile_params.m_auto_roll_radius) + v[1] * m_profile_params.m_auto_roll_radius;
						} // End if - then
					}
				}
			}
	}
	else
	{
		x = m_profile_params.m_roll_off_point[0];
		y = m_profile_params.m_roll_off_point[1];
	}
}


/**
	This is the duplicate of the kurve_funcs.py->make_smaller() method.  It's the one we can
	call from C++
 */
void CProfile::make_smaller( geoff_geometry::Kurve *pKurve, double *pStartx, double *pStarty, double *pFinishx, double *pFinishy ) const
{
	if (pStartx && pStarty)
	{
		int sp;
		double sx, sy, ex, ey, cx, cy;
		geoff_geometry::kurve_get_span(pKurve, 0, sp, sx, sy, ex, ey, cx, cy);

		if (pStartx) *pStartx = sx;
		if (pStarty) *pStarty = sy;

		geoff_geometry::kurve_change_start(pKurve, *pStartx, *pStarty);
	} // End if - then

	if (pFinishx && pFinishy)
	{
		int sp;
		double sx, sy, ex, ey, cx, cy;
		geoff_geometry::kurve_get_span(pKurve, 0, sp, sx, sy, ex, ey, cx, cy);

		if (pFinishx) *pFinishx = sx;
		if (pFinishy) *pFinishy = sy;

		geoff_geometry::kurve_change_start(pKurve, *pFinishx, *pFinishy);
	} // End if - then

} // End of make_smaller() method



/**
	This is the duplicate of the kurve_funcs.py->roll_on_point() method.  It's the one we can
	call from C++
 */
bool CProfile::roll_on_point( geoff_geometry::Kurve *pKurve, const wxString &direction, const double tool_radius, const double roll_radius, double *pRoll_on_x, double *pRoll_on_y) const
{
	*pRoll_on_x = double(0.0);
	*pRoll_on_y = double(0.0);

	double offset = tool_radius;
	if (direction == _T("right")) offset = -offset;
	Kurve *offset_k = geoff_geometry::kurve_new();

	bool offset_success = geoff_geometry::kurve_offset(pKurve, offset_k, offset);
	if (offset_success == false)
	{
       		// raise "couldn't offset kurve %d" % (k)
		return(false);
	} // End if - then

	int sp;
	double sx, sy, ex, ey, cx, cy;
	double vx, vy;
	double off_vx, off_vy;

	if (geoff_geometry::kurve_num_spans(offset_k) > 0)
	{
		geoff_geometry::kurve_get_span(offset_k, 0, sp, sx, sy, ex, ey, cx, cy);
		geoff_geometry::kurve_get_span_dir(offset_k, 0, 0, vx, vy); // get start direction
		off_vx = -vy;
		off_vy = vx;

		if (direction == _T("right"))
		{
			off_vx = -off_vx;
			off_vy = -off_vy;
		} // End if - then

	        *pRoll_on_x = sx + off_vx * roll_radius - vx * roll_radius;
        	*pRoll_on_y = sy + off_vy * roll_radius - vy * roll_radius;
	} // End if - then

	return(true);

} // End roll_on_point() method

wxString CProfile::WriteSketchDefn(HeeksObj* sketch, int id_to_use, geoff_geometry::Kurve *pKurve, const CFixture *pFixture, bool reversed )
{
	// write the python code for the sketch
#ifdef UNICODE
	std::wostringstream l_ossPythonCode;
#else
	std::ostringstream l_ossPythonCode;
#endif
	l_ossPythonCode<<std::setprecision(10);

	if ((sketch->GetShortString() != NULL) && (wxString(sketch->GetShortString()).size() > 0))
	{
		l_ossPythonCode << (wxString::Format(_T("comment('%s')\n"), wxString(sketch->GetShortString()).c_str())).c_str(); }

	l_ossPythonCode << (wxString::Format(_T("k%d = kurve.new()\n"), id_to_use > 0 ? id_to_use : sketch->m_id)).c_str();

	bool started = false;
	int sketch_id = (id_to_use > 0 ? id_to_use : sketch->m_id);

	std::list<HeeksObj*> spans;

	for(HeeksObj* span_object = sketch->GetFirstChild(); span_object; span_object = sketch->GetNextChild())
	{
		if(reversed)spans.push_front(span_object);
		else spans.push_back(span_object);
	}

	std::list<HeeksObj*> new_spans;
	for(std::list<HeeksObj*>::iterator It = spans.begin(); It != spans.end(); It++)
	{
		HeeksObj* span = *It;
		if(span->GetType() == SplineType)
		{
			std::list<HeeksObj*> new_spans2;
			heeksCAD->SplineToBiarcs(span, new_spans2, CProfile::max_deviation_for_spline_to_arc);
			for(std::list<HeeksObj*>::iterator It2 = new_spans2.begin(); It2 != new_spans2.end(); It2++)
			{
				HeeksObj* s = *It2;
				if(reversed)new_spans.push_front(s);
				else new_spans.push_back(s);
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
					CNCPoint start(pFixture->Adjustment(s));

					l_ossPythonCode << _T("kurve.add_point(k");
					l_ossPythonCode << sketch_id;
					l_ossPythonCode << _T(", " << LINEAR << ", ");
					l_ossPythonCode << start.X(true);
					l_ossPythonCode << _T(", ");
					l_ossPythonCode << start.Y(true);
					l_ossPythonCode << _T(", 0.0, 0.0)\n");
					started = true;

					geoff_geometry::kurve_add_point(pKurve,
									0,
									start.X(true),
									start.Y(true),
									0.0, 0.0);
				}
				if(reversed)span_object->GetStartPoint(e);
				else span_object->GetEndPoint(e);
				CNCPoint end(pFixture->Adjustment( e ));

				if(type == LineType)
				{
					l_ossPythonCode << _T("kurve.add_point(k");
					l_ossPythonCode << sketch_id;
					l_ossPythonCode << _T(", " << LINEAR << ", ");
					l_ossPythonCode << end.X(true);
					l_ossPythonCode << (_T(", "));
					l_ossPythonCode << end.Y(true);
					l_ossPythonCode << (_T(", 0.0, 0.0)\n"));

					geoff_geometry::kurve_add_point(pKurve,
									0,
									end.X(true),
									end.Y(true),
									0.0, 0.0);
				}
				else if(type == ArcType)
				{
					span_object->GetCentrePoint(c);
					CNCPoint centre(pFixture->Adjustment(c));

					double pos[3];
					heeksCAD->GetArcAxis(span_object, pos);
					int span_type = ((pos[2] >=0) != reversed) ? ACW: CW;
					l_ossPythonCode << (_T("kurve.add_point(k"));
					l_ossPythonCode << (sketch_id);
					l_ossPythonCode << (_T(", "));
					l_ossPythonCode << (span_type);
					l_ossPythonCode << (_T(", "));
					l_ossPythonCode << end.X(true);
					l_ossPythonCode << (_T(", "));
					l_ossPythonCode << end.Y(true);
					l_ossPythonCode << (_T(", "));
					l_ossPythonCode << centre.X(true);
					l_ossPythonCode << (_T(", "));
					l_ossPythonCode << centre.Y(true);
					l_ossPythonCode << (_T(")\n"));

					geoff_geometry::kurve_add_point(pKurve,
									span_type,
									end.X(true),
									end.Y(true),
									centre.X(true),
									centre.Y(true) );
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
					points.push_back( std::make_pair(LINEAR, gp_Pnt( c[0], c[1] + radius, c[2] )) ); // north
					if(reversed)
					{
						points.push_back( std::make_pair(ACW, gp_Pnt( c[0] - radius, c[1], c[2] )) ); // west
						points.push_back( std::make_pair(ACW, gp_Pnt( c[0], c[1] - radius, c[2] )) ); // south
						points.push_back( std::make_pair(ACW, gp_Pnt( c[0] + radius, c[1], c[2] )) ); // east
						points.push_back( std::make_pair(ACW, gp_Pnt( c[0], c[1] + radius, c[2] )) ); // north
					}
					else
					{
						points.push_back( std::make_pair(CW, gp_Pnt( c[0] + radius, c[1], c[2] )) ); // east
						points.push_back( std::make_pair(CW, gp_Pnt( c[0], c[1] - radius, c[2] )) ); // south
						points.push_back( std::make_pair(CW, gp_Pnt( c[0] - radius, c[1], c[2] )) ); // west
						points.push_back( std::make_pair(CW, gp_Pnt( c[0], c[1] + radius, c[2] )) ); // north
					}

					pFixture->Adjustment(c);
					CNCPoint centre(c);

					for (std::list< std::pair<int, gp_Pnt > >::iterator l_itPoint = points.begin(); l_itPoint != points.end(); l_itPoint++)
					{
						CNCPoint pnt = pFixture->Adjustment( l_itPoint->second );

						l_ossPythonCode << (_T("kurve.add_point(k"));
						l_ossPythonCode << (sketch_id);
						l_ossPythonCode << _T(", ") << l_itPoint->first << _T(", ");
						l_ossPythonCode << pnt.X(true);
						l_ossPythonCode << (_T(", "));
						l_ossPythonCode << pnt.Y(true);
						l_ossPythonCode << (_T(", "));
						l_ossPythonCode << centre.X(true);
						l_ossPythonCode << (_T(", "));
						l_ossPythonCode << centre.Y(true);
						l_ossPythonCode << (_T(")\n"));

						geoff_geometry::kurve_add_point(pKurve,
										l_itPoint->first,
										pnt.X(true),
										pnt.Y(true),
										centre.X(true),
										centre.Y(true) );
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

	l_ossPythonCode << _T("\n");

	if(GetNumChildren() == 1 && (m_profile_params.m_start_given || m_profile_params.m_end_given))
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

			starting = pFixture->Adjustment( starting );

			startx = starting.X();
			starty = starting.Y();

			ss<<std::setprecision(10);
			ss << ", startx = " << startx << ", starty = " << starty;
			start_string = ss.str().c_str();
		}


		wxString finish_string;
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

			finish = pFixture->Adjustment( finish );

			finishx = finish.X();
			finishy = finish.Y();

			ss<<std::setprecision(10);
			ss << ", finishx = " << finishx << ", finishy = " << finishy;
			finish_string = ss.str().c_str();
		}

		l_ossPythonCode << (wxString::Format(_T("kurve_funcs.make_smaller( k%d%s%s)\n"), sketch_id, start_string.c_str(), finish_string.c_str())).c_str();
		make_smaller( 	pKurve,
				(m_profile_params.m_start_given)?&startx:NULL,
				(m_profile_params.m_start_given)?&starty:NULL,
				(m_profile_params.m_end_given)?&finishx:NULL,
				(m_profile_params.m_end_given)?&finishy:NULL );
	}

	return(l_ossPythonCode.str().c_str());
}

wxString CProfile::AppendTextForOneSketch(HeeksObj* object, int sketch, double *pRollOnPointX, double *pRollOnPointY, const CFixture *pFixture)
{
#ifdef UNICODE
	std::wostringstream l_ossPythonCode;
#else
	std::ostringstream l_ossPythonCode;
#endif

	if(object)
	{
		// decide if we need to reverse the kurve
		bool reversed = false;
		bool initially_ccw = false;
		if(m_profile_params.m_tool_on_side != CProfileParams::eOn)
		{
			SketchOrderType order = SketchOrderTypeUnknown;
			HeeksObj* object = heeksCAD->GetIDObject(SketchType, sketch);
			if(object)order = heeksCAD->GetSketchOrder(object);
			if(order == SketchOrderTypeCloseCCW)initially_ccw = true;
			if(m_speed_op_params.m_spindle_speed<0)reversed = !reversed;
			if(m_profile_params.m_cut_mode == CProfileParams::eConventional)reversed = !reversed;
			if(m_profile_params.m_tool_on_side == CProfileParams::eRightOrInside)reversed = !reversed;
		}

		// write the kurve definition
		geoff_geometry::Kurve *pKurve = geoff_geometry::kurve_new();
		l_ossPythonCode << WriteSketchDefn(object, sketch, pKurve, pFixture, initially_ccw != reversed).c_str();

		double total_to_cut = m_depth_op_params.m_start_depth - m_depth_op_params.m_final_depth;
		int num_step_downs = (int)(total_to_cut / fabs(m_depth_op_params.m_step_down) + 1.0 - heeksCAD->GetTolerance());

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

		CCuttingTool *pCuttingTool = CCuttingTool::Find( m_cutting_tool_number );

		// get roll on string
		wxString roll_on_string;
		switch(m_profile_params.m_tool_on_side)
		{
		case CProfileParams::eLeftOrOutside:
		case CProfileParams::eRightOrInside:
			{
				if(m_profile_params.m_auto_roll_on || (GetNumChildren() > 1))
				{
					l_ossPythonCode << wxString::Format(_T("roll_on_x, roll_on_y = kurve_funcs.roll_on_point(k%d, '%s', tool_diameter/2 + offset_extra, roll_radius)\n"), sketch, side_string.c_str()).c_str();

					if ((pRollOnPointX != NULL) && (pRollOnPointY != NULL) && (pCuttingTool != NULL))
					{
						roll_on_point( pKurve, side_string.c_str(), pCuttingTool->CuttingRadius(), m_profile_params.m_auto_roll_radius, pRollOnPointX, pRollOnPointY);
					} // End if - then

					roll_on_string = wxString(_T("roll_on_x, roll_on_y"));
				}
				else
				{
#ifdef UNICODE
					std::wostringstream ss;
#else
					std::ostringstream ss;
#endif
					ss.imbue(std::locale("C"));
					ss<<std::setprecision(10);
					ss << m_profile_params.m_roll_on_point[0] / theApp.m_program->m_units << ", " << m_profile_params.m_roll_on_point[1] / theApp.m_program->m_units;
					roll_on_string = ss.str().c_str();

					if ((pRollOnPointX != NULL) && (pRollOnPointY != NULL))
					{
						*pRollOnPointX = m_profile_params.m_roll_on_point[0];
						*pRollOnPointY = m_profile_params.m_roll_on_point[1];
					}
				}
			}
			break;
		default:
			{
				l_ossPythonCode << wxString::Format(_T("sp, span1sx, span1sy, ex, ey, cx, cy = kurve.get_span(k%d, 0)\n"), sketch).c_str();
				roll_on_string = _T("span1sx, span1sy");
			}
			break;
		}

		// rapid across to it
		l_ossPythonCode << wxString::Format(_T("rapid(%s)\n"), roll_on_string.c_str()).c_str();

		wxString roll_off_string;
		switch(m_profile_params.m_tool_on_side)
		{
		case CProfileParams::eLeftOrOutside:
		case CProfileParams::eRightOrInside:
			{
			if(m_profile_params.m_auto_roll_off || (GetNumChildren() > 1))
			{
				l_ossPythonCode << wxString::Format(_T("roll_off_x, roll_off_y = kurve_funcs.roll_off_point(k%d, '%s', tool_diameter/2 + offset_extra, roll_radius)\n"), sketch, side_string.c_str()).c_str();
				roll_off_string = wxString(_T("roll_off_x, roll_off_y"));
			}
			else
			{
#ifdef UNICODE
				std::wostringstream ss;
#else
				std::ostringstream ss;
#endif
				ss<<std::setprecision(10);
				ss << m_profile_params.m_roll_off_point[0] / theApp.m_program->m_units << ", " << m_profile_params.m_roll_off_point[1] / theApp.m_program->m_units;
				roll_off_string = ss.str().c_str();
			}
		}
			break;
		default:
		{
			l_ossPythonCode << wxString::Format(_T("sp, sx, sy, ex, ey, cx, cy = kurve.get_span(k%d, kurve.num_spans(k%d) - 1)\n"), sketch, sketch).c_str();
			roll_off_string = _T("ex, ey");
		}
		}

		if(m_profile_params.m_num_tags > 0)
		{
			l_ossPythonCode << _T("tag_width = ") << m_profile_params.m_tag_width / theApp.m_program->m_units << _T("\n");
			l_ossPythonCode << _T("tag_angle = ") << m_profile_params.m_tag_angle * PI/180 << _T("\n");
			l_ossPythonCode << _T("tag_height = float(tag_width)/2 * math.tan(tag_angle)\n");
			l_ossPythonCode << _T("tag_depth = final_depth + tag_height\n");
			l_ossPythonCode << _T("if tag_depth > start_depth: tag_depth = start_depth\n");
		}

		if(num_step_downs > 1)
		{
			l_ossPythonCode << wxString(_T("incremental_rapid_height = rapid_down_to_height - start_depth\n")).c_str();
			l_ossPythonCode << _T("prev_depth = start_depth\n");
			l_ossPythonCode << wxString::Format(_T("for step in range(0, %d):\n"), num_step_downs).c_str();
			l_ossPythonCode << wxString::Format(_T(" depth_of_cut = ( start_depth - final_depth ) * ( step + 1 ) / %d\n"), num_step_downs).c_str();
			l_ossPythonCode << _T(" depth = start_depth - depth_of_cut\n";

			// rapid across to roll on point
			l_ossPythonCode << wxString::Format(_T(" if step != 0:\n  rapid(%s)\n"), roll_on_string.c_str()).c_str();
			if(m_profile_params.m_num_tags > 0)
			{
				// rapid down to just above the material, which might be at the top of the tag
				l_ossPythonCode << wxString(_T(" mat_depth = prev_depth\n")).c_str();
				l_ossPythonCode << wxString(_T(" if tag_depth > mat_depth: mat_depth = tag_depth\n")).c_str();
				l_ossPythonCode << wxString(_T(" rapid(z = mat_depth + incremental_rapid_height)\n")).c_str();
				// feed down to depth
				l_ossPythonCode << wxString(_T(" mat_depth = depth\n")).c_str();
				l_ossPythonCode << wxString(_T(" if tag_depth > mat_depth: mat_depth = tag_depth\n")).c_str();
				l_ossPythonCode << wxString(_T(" feed(z = mat_depth)\n")).c_str();
			}
			else
			{
				// rapid down to just above the material
				l_ossPythonCode << wxString(_T(" rapid(z = prev_depth + incremental_rapid_height)\n")).c_str();
				// feed down to depth
				l_ossPythonCode << wxString(_T(" feed(z = depth)\n")).c_str();
			}

			// set up the tag parameters
			wxString tag_string;

			if(m_profile_params.m_num_tags > 0)
			{
				l_ossPythonCode << _T(" sub_tag_height = tag_height - ( depth - final_depth )\n");
				l_ossPythonCode << _T(" if sub_tag_height < 0: sub_tag_height = 0\n");
				l_ossPythonCode << _T(" sub_tag_width = 2.0 * float(sub_tag_height) / math.tan (tag_angle) \n");
				l_ossPythonCode << _T(" tag = ") << m_profile_params.m_num_tags << ", sub_tag_width, tag_angle\n");
				tag_string = _T(", start_depth, depth, tag");
			}

			// profile the kurve
			l_ossPythonCode << wxString::Format(_T(" kurve_funcs.profile(k%d, '%s', tool_diameter/2, offset_extra, %s, %s%s)\n"), sketch, side_string.c_str(), roll_on_string.c_str(), roll_off_string.c_str(), tag_string.c_str()).c_str();

			// rapid back up to clearance plane
			l_ossPythonCode << wxString(_T(" rapid(z = clearance)\n")).c_str();

			// set prev_depth
			l_ossPythonCode << _T(" prev_depth = depth\n");
		}
		else
		{
			// rapid down to just above the material
			l_ossPythonCode << wxString(_T("rapid(z = rapid_down_to_height)\n")).c_str();

			// feed down to final depth
			l_ossPythonCode << wxString::Format(_T("feed(z = final_depth%s)\n"), (m_profile_params.m_num_tags > 0) ? _T(" + tag_height") : _T("")).c_str();

			// set up the tag parameters
			wxString tag_string;

			if(m_profile_params.m_num_tags > 0)
			{
			    l_ossPythonCode << wxString::Format(_T("depth_of_cut = ( start_depth - final_depth )\n")).c_str();
				l_ossPythonCode << _T("tag = ") << m_profile_params.m_num_tags << _T(", tag_width, tag_angle\n");
				tag_string = _T(", start_depth, depth_of_cut, tag");
			}

			// profile the kurve
			l_ossPythonCode << (wxString::Format(_T("kurve_funcs.profile(k%d, '%s', tool_diameter/2, offset_extra, %s, %s%s)\n"), sketch, side_string.c_str(), roll_on_string.c_str(), roll_off_string.c_str(), tag_string.c_str()).c_str());

			// rapid back up to clearance plane
			l_ossPythonCode << (wxString(_T("rapid(z = clearance)\n"))).c_str();
		}
	}

	return(l_ossPythonCode.str().c_str());
}

void CProfile::WriteDefaultValues()
{
	CDepthOp::WriteDefaultValues();

	CNCConfig config(CProfileParams::ConfigScope());
	config.Write(_T("ToolOnSide"), m_profile_params.m_tool_on_side);
	config.Write(_T("CutMode"), m_profile_params.m_cut_mode);
	config.Write(_T("RollRadius"), m_profile_params.m_auto_roll_radius);
}

void CProfile::ReadDefaultValues()
{
	CDepthOp::ReadDefaultValues();

	CNCConfig config(CProfileParams::ConfigScope());
	int int_side = m_profile_params.m_tool_on_side;
	config.Read(_T("ToolOnSide"), &int_side, CProfileParams::eLeftOrOutside);
	m_profile_params.m_tool_on_side = (CProfileParams::eSide)int_side;
	int int_mode = m_profile_params.m_cut_mode;
	config.Read(_T("CutMode"), &int_mode, CProfileParams::eConventional);
	m_profile_params.m_cut_mode = (CProfileParams::eCutMode)int_mode;
	config.Read(_T("RollRadius"), &m_profile_params.m_auto_roll_radius, 2.0);

	ConfirmAutoRollRadius(true);

}





void CProfile::AppendTextToProgram(const CFixture *pFixture)
{
	CCuttingTool *pCuttingTool = CCuttingTool::Find( m_cutting_tool_number );
	if (pCuttingTool == NULL)
	{
		wxMessageBox(_T("Cannot generate GCode for profile without a cutting tool assigned"));
		return;
	} // End if - then

	for (HeeksObj *child = GetFirstChild(); child != NULL; child = GetNextChild())
	{
	    if (child->GetType() == SketchType)
	    {
	        m_sketches.push_back( child->m_id );
	    }
	}

	std::vector<CNCPoint> starting_points;
	wxString python_code = AppendTextToProgram( starting_points, pFixture );

	CDepthOp::AppendTextToProgram(pFixture);
	theApp.m_program_canvas->m_textCtrl->AppendText( python_code.c_str() );

	m_sketches.clear();

} // End AppendTextToProgram() method

struct sort_sketches : public std::binary_function< const int, const int, bool >
{
	CNCPoint m_reference_point;
	CProfile *m_pThis;

	sort_sketches( CProfile *pThis, const CNCPoint & reference_point )
	{
		m_reference_point = reference_point;
		m_pThis = pThis;
	} // End constructor

	// Return true if dist(lhs to ref) < dist(rhs to ref)
	bool operator()( const int lhs, const int rhs ) const
	{
		HeeksObj *lhsPtr = heeksCAD->GetIDObject( SketchType, lhs );
		HeeksObj *rhsPtr = heeksCAD->GetIDObject( SketchType, rhs );

		if ((lhsPtr != NULL) && (rhsPtr != NULL))
		{
			double x,y;
			m_pThis->GetRollOnPos(lhsPtr, x, y );
			CNCPoint lhsPoint( x, y, 0.0 );

			m_pThis->GetRollOnPos(rhsPtr, x, y );
			CNCPoint rhsPoint( x, y, 0.0 );

			return( lhsPoint.Distance( m_reference_point ) < rhsPoint.Distance( m_reference_point ) );
		} // End if - then
		else
		{
			return(false);
		} // End if - else
	} // End operator() method
}; // End sort_sketches() method



wxString CProfile::AppendTextToProgram( std::vector<CNCPoint> & starting_points, const CFixture *pFixture )
{
	if(m_profile_params.m_auto_roll_on || m_profile_params.m_auto_roll_off)
	{
		theApp.m_program_canvas->AppendText(_T("roll_radius = float("));
		theApp.m_program_canvas->AppendText(m_profile_params.m_auto_roll_radius / theApp.m_program->m_units);
		theApp.m_program_canvas->AppendText(_T(")\n"));
	}

#ifdef UNICODE
	std::wostringstream l_ossPythonCode;
#else
	std::ostringstream l_ossPythonCode;
#endif
	l_ossPythonCode<<std::setprecision(10);
	l_ossPythonCode<<_T("offset_extra = ")<<m_profile_params.m_offset_extra<<_T("\n");

	// Make a local copy so that we can either sort it or leave it alone.  We don't want
	// to affect the member list itself.

	std::list<int> sketches;
	std::copy( m_sketches.begin(), m_sketches.end(), std::inserter( sketches, sketches.begin() ) );

	if (m_profile_params.m_sort_sketches)
	{
		std::vector<int> sorted;
		std::copy( m_sketches.begin(), m_sketches.end(), std::inserter( sorted, sorted.begin() ) );
		for (std::vector<int>::iterator l_itSketch = sorted.begin(); l_itSketch != sorted.end(); l_itSketch++)
		{
			if (l_itSketch == sorted.begin())
			{
				HeeksObj *ref = heeksCAD->GetIDObject( SketchType, *l_itSketch );
				if (ref != NULL)
				{
					sort_sketches compare( this, CNCPoint( 0.0, 0.0, 0.0 ) );
					std::sort( l_itSketch, sorted.end(), compare );
				} // End if - then
			} // End if - then
			else
			{
				std::vector<int>::iterator l_itNextSketch = l_itSketch;
				l_itNextSketch++;

				if (l_itNextSketch != sorted.end())
				{
					HeeksObj *ref = heeksCAD->GetIDObject( SketchType, *l_itSketch );
					if (ref != NULL)
					{
						double x,y;
						GetRollOffPos( ref, x, y );
						sort_sketches compare( this, CNCPoint( x, y, 0.0 ) );
						std::sort( l_itNextSketch, sorted.end(), compare );
					} // End if - then
				} // End if - then
			} // End if - else
		} // End for

		sketches.erase( sketches.begin(), sketches.end() );
		std::copy( sorted.begin(), sorted.end(), std::inserter( sketches, sketches.begin() ) );
	} // End if - then

	for(std::list<int>::iterator It = sketches.begin(); It != sketches.end(); It++)
	{
		int sketch = *It;

		// write a kurve definition
		HeeksObj* object = heeksCAD->GetIDObject(SketchType, sketch);
		if(object == NULL || object->GetNumChildren() == 0)continue;

		HeeksObj* re_ordered_sketch = NULL;
		SketchOrderType sketch_order = heeksCAD->GetSketchOrder(object);
		if(sketch_order == SketchOrderTypeBad)
		{
			re_ordered_sketch = object->MakeACopy();
			heeksCAD->ReOrderSketch(re_ordered_sketch, SketchOrderTypeReOrder);
			object = re_ordered_sketch;
		}

		double roll_on_point_x, roll_on_point_y;
		if(sketch_order == SketchOrderTypeMultipleCurves || sketch_order == SketchOrderHasCircles)
		{
			std::list<HeeksObj*> new_separate_sketches;
			heeksCAD->ExtractSeparateSketches(object, new_separate_sketches);
			for(std::list<HeeksObj*>::iterator It = new_separate_sketches.begin(); It != new_separate_sketches.end(); It++)
			{
				HeeksObj* one_curve_sketch = *It;
				l_ossPythonCode << AppendTextForOneSketch(one_curve_sketch, sketch, &roll_on_point_x, &roll_on_point_y, pFixture).c_str();
				CBox bbox;
				one_curve_sketch->GetBox(bbox);
				starting_points.push_back( CNCPoint( roll_on_point_x, roll_on_point_y, bbox.MaxZ() ) );
				delete one_curve_sketch;
			}
		}
		else
		{
			l_ossPythonCode << AppendTextForOneSketch(object, sketch, &roll_on_point_x, &roll_on_point_y, pFixture).c_str();
			CBox bbox;
			object->GetBox(bbox);
			starting_points.push_back( CNCPoint( roll_on_point_x, roll_on_point_y, bbox.MaxZ() ) );
		}

		if(re_ordered_sketch)
		{
			delete re_ordered_sketch;
		}
	}

	return( l_ossPythonCode.str().c_str() );
}

static unsigned char cross16[32] = {0x80, 0x01, 0x40, 0x02, 0x20, 0x04, 0x10, 0x08, 0x08, 0x10, 0x04, 0x20, 0x02, 0x40, 0x01, 0x80, 0x01, 0x80, 0x02, 0x40, 0x04, 0x20, 0x08, 0x10, 0x10, 0x08, 0x20, 0x04, 0x40, 0x02, 0x80, 0x01};

void CProfile::glCommands(bool select, bool marked, bool no_color)
{
	CDepthOp::glCommands(select, marked, no_color);

	if(marked && !no_color)
	{
		if(GetNumChildren() == 1)
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
	AddSketchesProperties(list, m_sketches);
	m_profile_params.GetProperties(this, list);

	CDepthOp::GetProperties(list);
}

static CProfile* object_for_pick = NULL;

class PickStart: public Tool{
	// Tool's virtual functions
	const wxChar* GetTitle(){return _("Pick Start");}
	void Run(){if(heeksCAD->PickPosition(_("Pick new start point"), object_for_pick->m_profile_params.m_start))object_for_pick->m_profile_params.m_start_given = true;}
	wxString BitmapPath(){ return _T("pickstart");}
};

static PickStart pick_start;

class PickEnd: public Tool{
	// Tool's virtual functions
	const wxChar* GetTitle(){return _("Pick End");}
	void Run(){if(heeksCAD->PickPosition(_("Pick new end point"), object_for_pick->m_profile_params.m_end))object_for_pick->m_profile_params.m_end_given = true;}
	wxString BitmapPath(){ return _T("pickend");}
};

static PickEnd pick_end;

class PickRollOn: public Tool{
	// Tool's virtual functions
	const wxChar* GetTitle(){return _("Pick roll on point");}
	void Run(){if(heeksCAD->PickPosition(_("Pick roll on point"), object_for_pick->m_profile_params.m_roll_on_point))object_for_pick->m_profile_params.m_auto_roll_on = false;}
	wxString BitmapPath(){ return _T("rollon");}
};

static PickRollOn pick_roll_on;

class PickRollOff: public Tool{
	// Tool's virtual functions
	const wxChar* GetTitle(){return _("Pick roll off point");}
	void Run(){if(heeksCAD->PickPosition(_("Pick roll off point"), object_for_pick->m_profile_params.m_roll_off_point))object_for_pick->m_profile_params.m_auto_roll_off = false;}
	wxString BitmapPath(){ return _T("rolloff");}
};

static PickRollOff pick_roll_off;

static ReselectSketches reselect_sketches;

void CProfile::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
	object_for_pick = this;
	t_list->push_back(&pick_start);
	t_list->push_back(&pick_end);
	t_list->push_back(&pick_roll_on);
	t_list->push_back(&pick_roll_off);
	reselect_sketches.m_sketches = &m_sketches;
	reselect_sketches.m_object = this;
	t_list->push_back(&reselect_sketches);

	CDepthOp::GetTools(t_list, p);
}

HeeksObj *CProfile::MakeACopy(void)const
{
	return new CProfile(*this);
}

void CProfile::CopyFrom(const HeeksObj* object)
{
	if (object->GetType() == GetType())
	{
		operator=(*((CProfile*)object));
	}
}

bool CProfile::CanAddTo(HeeksObj* owner)
{
	return owner->GetType() == OperationsType;
}

void CProfile::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element = new TiXmlElement( "Profile" );
	root->LinkEndChild( element );
	m_profile_params.WriteXMLAttributes(element);

	/*
	// write sketch ids
	for(std::list<int>::iterator It = m_sketches.begin(); It != m_sketches.end(); It++)
	{
		int sketch = *It;
		TiXmlElement * sketch_element = new TiXmlElement( "sketch" );
		element->LinkEndChild( sketch_element );
		sketch_element->SetAttribute("id", sketch);
	}
	*/

	CDepthOp::WriteBaseXML(element);
}

// static member function
HeeksObj* CProfile::ReadFromXMLElement(TiXmlElement* element)
{
	CProfile* new_object = new CProfile;

	std::list<TiXmlElement *> elements_to_remove;

	// read profile parameters
	TiXmlElement* params = TiXmlHandle(element).FirstChildElement("params").Element();
	if(params)
	{
		new_object->m_profile_params.ReadFromXMLElement(params);
		elements_to_remove.push_back(params);
	}

	// read sketch ids
	for(TiXmlElement* sketch = TiXmlHandle(element).FirstChildElement("sketch").Element(); sketch; sketch = sketch->NextSiblingElement())
	{
		if ((wxString(Ctt(sketch->Value())) == wxString(_T("sketch"))) &&
			(sketch->Attribute("id") != NULL) &&
			(sketch->Attribute("title") == NULL))
		{
			int id = 0;
			sketch->Attribute("id", &id);
			if(id)
			{
				new_object->m_sketches.push_back(id);
			}

			elements_to_remove.push_back(sketch);
		} // End if - then
	}

	for (std::list<TiXmlElement*>::iterator itElem = elements_to_remove.begin(); itElem != elements_to_remove.end(); itElem++)
	{
		element->RemoveChild(*itElem);
	}

	// read common parameters
	new_object->ReadBaseXML(element);

	return new_object;
}


/**
	The old version of the CDrilling object stored references to graphics as type/id pairs
	that get read into the m_symbols list.  The new version stores these graphics references
	as child elements (based on ObjList).  If we read in an old-format file then the m_symbols
	list will have data in it for which we don't have children.  This routine converts
	these type/id pairs into the HeeksObj pointers as children.
 */
void CProfile::ReloadPointers()
{
	for (Sketches_t::iterator symbol = m_sketches.begin(); symbol != m_sketches.end(); symbol++)
	{
		HeeksObj *object = heeksCAD->GetIDObject( SketchType, *symbol );
		if (object != NULL)
		{
			Add( object, NULL );
		}
	}

	m_sketches.clear();	// We don't want to convert them twice.

	CDepthOp::ReloadPointers();
}


/**
	If it's an 'inside' profile then we need to make sure the auto_roll_radius is not so large
	that it's going to gouge the part outside the sketch's area.  This routine only
	reduces the auto_roll_radius.  Its value is not changed unless a gouge scenario is detected.
 */
std::list<wxString> CProfile::ConfirmAutoRollRadius(const bool apply_changes)
{
#ifdef UNICODE
			std::wostringstream l_ossChange;
#else
			std::ostringstream l_ossChange;
#endif

	std::list<wxString> changes;

	if (m_profile_params.m_tool_on_side == CProfileParams::eRightOrInside)
	{
		// Look at the dimensions of the sketches as well as the diameter of the cutting bit to decide if
		// our existing m_auto_roll_radius is too big for this profile.  If so, reduce it now.
		CCuttingTool *pCuttingTool = NULL;
		if ((m_cutting_tool_number > 0) && ((pCuttingTool = CCuttingTool::Find(m_cutting_tool_number)) != NULL))
		{
			for (std::list<int>::iterator l_itSketchId = m_sketches.begin(); l_itSketchId != m_sketches.end(); l_itSketchId++)
			{
				HeeksObj *sketch = heeksCAD->GetIDObject( SketchType, *l_itSketchId );
				if (sketch != NULL)
				{
					CBox bounding_box;
					sketch->GetBox( bounding_box );

					double min_distance_across = (bounding_box.Height() < bounding_box.Width())?bounding_box.Height():bounding_box.Width();
					double max_roll_radius = (min_distance_across - (pCuttingTool->CuttingRadius() * 2.0)) / 2.0;

					if (max_roll_radius < m_profile_params.m_auto_roll_radius)
					{
						l_ossChange << "Need to adjust auto_roll_radius for profile id=" << m_id << " from "
								<< m_profile_params.m_auto_roll_radius << " to " << max_roll_radius << "\n";
						changes.push_back(l_ossChange.str().c_str());

						if (apply_changes)
						{
							m_profile_params.m_auto_roll_radius = max_roll_radius;
						} // End if - then
					}
				} // End if - then
			} // End for
		} // End if - then
	} // End if - then

	return(changes);

} // End ConfirmAutoRollRadius() method

/**
	This method adjusts any parameters that don't make sense.  It should report a list
	of changes in the list of strings.
 */
std::list<wxString> CProfile::DesignRulesAdjustment(const bool apply_changes)
{
	std::list<wxString> changes;

	std::list<int> invalid_sketches;
	for(std::list<int>::iterator l_itSketch = m_sketches.begin(); l_itSketch != m_sketches.end(); l_itSketch++)
	{
		HeeksObj *obj = heeksCAD->GetIDObject( SketchType, *l_itSketch );
		if (obj == NULL)
		{
#ifdef UNICODE
			std::wostringstream l_ossChange;
#else
			std::ostringstream l_ossChange;
#endif

			l_ossChange << _("Invalid reference to sketch") << " id='" << *l_itSketch << "' " << _("in profile operations") << " id='" << m_id << "'\n";
			changes.push_back(l_ossChange.str().c_str());

			if (apply_changes)
			{
				invalid_sketches.push_back( *l_itSketch );
			} // End if - then
		} // End if - then
	} // End for

	if (apply_changes)
	{
		for(std::list<int>::iterator l_itSketch = invalid_sketches.begin(); l_itSketch != invalid_sketches.end(); l_itSketch++)
		{
			std::list<int>::iterator l_itToRemove = std::find( m_sketches.begin(), m_sketches.end(), *l_itSketch );
			if (l_itToRemove != m_sketches.end())
			{
				m_sketches.erase(l_itToRemove);
			} // End while
		} // End for
	} // End if - then

	if (GetNumChildren() == 0)
	{
#ifdef UNICODE
			std::wostringstream l_ossChange;
#else
			std::ostringstream l_ossChange;
#endif

			l_ossChange << _("No valid sketches upon which to act for profile operations") << " id='" << m_id << "'\n";
			changes.push_back(l_ossChange.str().c_str());
	} // End if - then


	if (m_cutting_tool_number > 0)
	{
		// Make sure the hole depth isn't greater than the tool's cutting depth.
		CCuttingTool *pCutter = (CCuttingTool *) CCuttingTool::Find( m_cutting_tool_number );
		if ((pCutter != NULL) && (pCutter->m_params.m_cutting_edge_height < m_depth_op_params.m_final_depth))
		{
			// The tool we've chosen can't cut as deep as we've setup to go.

			std::wostringstream l_ossChange;

			l_ossChange << _("Adjusting depth of profile") << " id='" << m_id << "' " << _("from") << " '"
				<< m_depth_op_params.m_final_depth << " " << _("to") << " "
				<< pCutter->m_params.m_cutting_edge_height << " " << _("due to cutting edge length of selected tool") << "\n";
			changes.push_back(l_ossChange.str().c_str());

			if (apply_changes)
			{
				m_depth_op_params.m_final_depth = pCutter->m_params.m_cutting_edge_height;
			} // End if - then
		} // End if - then
	} // End if - then

	std::list<wxString> roll_radius_changes = ConfirmAutoRollRadius(apply_changes);
	std::copy( roll_radius_changes.begin(), roll_radius_changes.end(), std::inserter( changes, changes.end() ) );

	std::list<wxString> depth_op_changes = CDepthOp::DesignRulesAdjustment( apply_changes );
	std::copy( depth_op_changes.begin(), depth_op_changes.end(), std::inserter( changes, changes.end() ) );

	return(changes);

} // End DesignRulesAdjustment() method

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
	CNCConfig config(CProfileParams::ConfigScope());
	config.Read(_T("ProfileSplineDeviation"), &max_deviation_for_spline_to_arc, 0.01);
}

// static
void CProfile::WriteToConfig()
{
	CNCConfig config(CProfileParams::ConfigScope());
	config.Write(_T("ProfileSplineDeviation"), max_deviation_for_spline_to_arc);
}
