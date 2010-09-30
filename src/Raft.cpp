// Raft.cpp
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "stdafx.h"

#ifndef STABLE_OPS_ONLY
#include "Raft.h"
#include "CNCConfig.h"
#include "ProgramCanvas.h"
#include "Program.h"
#include "interface/HeeksObj.h"
#include "interface/PropertyDouble.h"
#include "interface/PropertyLength.h"
#include "interface/PropertyString.h"
#include "interface/PropertyChoice.h"
#include "interface/PropertyVertex.h"
#include "interface/PropertyCheck.h"
#include "tinyxml/tinyxml.h"
#include "CTool.h"
#include "geometry.h"
#include "CNCPoint.h"
#include "Reselect.h"
#include "MachineState.h"

#include <sstream>

// static
double CRaft::max_deviation_for_spline_to_arc = 0.1;

CRaftParams::CRaftParams()
{
	m_step_over = 0.0;
	m_material_allowance = 0.0;
	m_round_corner_factor = 0.0;
	m_starting_place = true;
	m_keep_tool_down_if_poss = true;
	m_use_zig_zag = true;
	m_zig_angle = 0.0;
	
	m_baselayers = 2;
	m_interfacelayers = 2;
	m_baselayerextrusion = 0;
	m_interfacelayerextrusion = 0;
}

void CRaftParams::set_initial_values(const CTool::ToolNumber_t tool_number)
{
    if (tool_number > 0)
    {
        CTool *pTool = CTool::Find(tool_number);
        if (pTool != NULL)
        {
            m_step_over = pTool->CuttingRadius() * 3.0 / 5.0;
        }
    }
}

static void on_set_step_over(double value, HeeksObj* object)
{
	((CRaft*)object)->m_params.m_step_over = value;
	((CRaft*)object)->WriteDefaultValues();
}

static void on_set_material_allowance(double value, HeeksObj* object)
{
	((CRaft*)object)->m_params.m_material_allowance = value;
	((CRaft*)object)->WriteDefaultValues();
}

static void on_set_round_corner_factor(double value, HeeksObj* object)
{
	((CRaft*)object)->m_params.m_round_corner_factor = value;
	((CRaft*)object)->WriteDefaultValues();
}

static void on_set_starting_place(int value, HeeksObj* object)
{
	((CRaft*)object)->m_params.m_starting_place = value;
	((CRaft*)object)->WriteDefaultValues();
}

static void on_set_keep_tool_down(bool value, HeeksObj* object)
{
	((CRaft*)object)->m_params.m_keep_tool_down_if_poss = value;
	((CRaft*)object)->WriteDefaultValues();
}

static void on_set_use_zig_zag(bool value, HeeksObj* object)
{
	((CRaft*)object)->m_params.m_use_zig_zag = value;
	((CRaft*)object)->WriteDefaultValues();
}

static void on_set_zig_angle(double value, HeeksObj* object)
{
	((CRaft*)object)->m_params.m_zig_angle = value;
	((CRaft*)object)->WriteDefaultValues();
}

static void on_set_baselayers(double value, HeeksObj* object)
{
	((CRaft*)object)->m_params.m_baselayers = value;
	((CRaft*)object)->WriteDefaultValues();
}

static void on_set_interfacelayers(double value, HeeksObj* object)
{
	((CRaft*)object)->m_params.m_interfacelayers = value;
	((CRaft*)object)->WriteDefaultValues();
}

static void on_set_baselayerextrusion(int value, HeeksObj* object)
{
	if (value < 0) return;	// An error has occured.

	std::vector< std::pair< int, wxString > > tools = CTool::FindAllTools();

	if ((value >= int(0)) && (value <= int(tools.size()-1)))
	{
                ((CRaft *)object)->m_params.m_baselayerextrusion = tools[value].first;	// Convert the choice offset to the tool number for that choice
	} // End if - then

	((CRaft*)object)->WriteDefaultValues();
}

static void on_set_interfacelayerextrusion(int value, HeeksObj* object)
{
	if (value < 0) return;	// An error has occured.

	std::vector< std::pair< int, wxString > > tools = CTool::FindAllTools();

	if ((value >= int(0)) && (value <= int(tools.size()-1)))
	{
                ((CRaft *)object)->m_params.m_interfacelayerextrusion = tools[value].first;	// Convert the choice offset to the tool number for that choice
	} // End if - then


	((CRaft*)object)->WriteDefaultValues();
}


void CRaftParams::GetProperties(CRaft* parent, std::list<Property *> *list)
{
	list->push_back(new PropertyLength(_("step over"), m_step_over, parent, on_set_step_over));
	list->push_back(new PropertyLength(_("material allowance"), m_material_allowance, parent, on_set_material_allowance));
	list->push_back(new PropertyDouble(_("round corner factor"), m_round_corner_factor, parent, on_set_round_corner_factor));
	list->push_back(new PropertyString(wxString(_T("( ")) + _("for 90 degree corners") + _T(" )"), wxString(_T("( ")) + _("1.5 for square, 1.0 for round")  + _T(" )"), NULL));
	{
		std::list< wxString > choices;
		choices.push_back(_("Boundary"));
		choices.push_back(_("Center"));
		list->push_back(new PropertyChoice(_("starting_place"), choices, m_starting_place, parent, on_set_starting_place));
	}
	list->push_back(new PropertyCheck(_("keep tool down"), m_keep_tool_down_if_poss, parent, on_set_keep_tool_down));
	list->push_back(new PropertyCheck(_("use zig zag"), m_use_zig_zag, parent, on_set_use_zig_zag));
	if(m_use_zig_zag)list->push_back(new PropertyDouble(_("zig angle"), m_zig_angle, parent, on_set_zig_angle));
	
	list->push_back(new PropertyLength(_("baselayers"), m_baselayers, parent, on_set_baselayers));
	list->push_back(new PropertyDouble(_("interfacelayers"), m_interfacelayers, parent, on_set_interfacelayers));
	
	std::vector< std::pair< int, wxString > > tools = CTool::FindAllTools();

	int basechoice = 0;
	int interfacechoice = 0;
        std::list< wxString
         > choices;
	for (std::vector< std::pair< int, wxString > >::size_type i=0; i<tools.size(); i++)
	{
               	choices.push_back(tools[i].second);
			if (m_baselayerextrusion == tools[i].first)
			{
                		basechoice = int(i);
			} // End if - then
			
			if (m_interfacelayerextrusion == tools[i].first)
			{
                		interfacechoice = int(i);
			}
	} // End for

	list->push_back(new PropertyChoice(_("Base Layer Extrusion"), choices, basechoice, parent, on_set_baselayerextrusion));	
	list->push_back(new PropertyChoice(_("Interface Layer Extrusion"), choices, interfacechoice, parent, on_set_interfacelayerextrusion));	
}
void CRaftParams::WriteXMLAttributes(TiXmlNode *root)
{
	TiXmlElement * element;
	element = new TiXmlElement( "params" );
	root->LinkEndChild( element );
	element->SetDoubleAttribute("step", m_step_over);
	element->SetDoubleAttribute("mat", m_material_allowance);
	element->SetDoubleAttribute("rf", m_round_corner_factor);
	element->SetAttribute("from_center", m_starting_place);
	element->SetAttribute("keep_tool_down", m_keep_tool_down_if_poss ? 1:0);
	element->SetAttribute("use_zig_zag", m_use_zig_zag ? 1:0);
	element->SetDoubleAttribute("zig_angle", m_zig_angle);
	
	element->SetDoubleAttribute("baselayers", m_baselayers);
	element->SetDoubleAttribute("interfacelayers", m_interfacelayers);
	element->SetAttribute("Base_Layer_Extrusion", m_baselayerextrusion);
	element->SetAttribute("Interface_Layer_Extrusion", m_interfacelayerextrusion);		
}

void CRaftParams::ReadFromXMLElement(TiXmlElement* pElem)
{
	pElem->Attribute("step", &m_step_over);
	pElem->Attribute("mat", &m_material_allowance);
	pElem->Attribute("rf", &m_round_corner_factor);
	pElem->Attribute("from_center", &m_starting_place);
	int int_for_bool = false;
	pElem->Attribute("keep_tool_down", &int_for_bool);
	m_keep_tool_down_if_poss = (int_for_bool != 0);
	pElem->Attribute("use_zig_zag", &int_for_bool);
	m_use_zig_zag = (int_for_bool != 0);
	pElem->Attribute("zig_angle", &m_zig_angle);
	
	pElem->Attribute("baselayers", &m_baselayers);
	pElem->Attribute("interfacelayers", &m_interfacelayers);
	pElem->Attribute("Base_Layer_Extrusion", &m_baselayerextrusion);
	pElem->Attribute("Interface_Layer_Extrusion", &m_interfacelayerextrusion);	
}
static wxString WriteSketchDefn(HeeksObj* sketch, CMachineState *pMachineState, int id_to_use = 0)
{
#ifdef UNICODE
	std::wostringstream gcode;
#else
    std::ostringstream gcode;
#endif
    gcode.imbue(std::locale("C"));
	gcode << std::setprecision(10);

	wxString area_str = wxString::Format(_T("a%d"), id_to_use > 0 ? id_to_use : sketch->m_id);

	gcode << area_str.c_str() << _T(" = area.Area()\n");

	bool started = false;

	double prev_e[3];

	std::list<HeeksObj*> new_spans;
	for(HeeksObj* span = sketch->GetFirstChild(); span; span = sketch->GetNextChild())
	{
		if(span->GetType() == SplineType)
		{
			heeksCAD->SplineToBiarcs(span, new_spans, CRaft::max_deviation_for_spline_to_arc);
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

			if(type == LineType || type == ArcType)
			{
				span_object->GetStartPoint(s);
				CNCPoint start(pMachineState->Fixture().Adjustment(s));

				if(started && (fabs(s[0] - prev_e[0]) > 0.000000001 || fabs(s[1] - prev_e[1]) > 0.000000001))
				{
					gcode << area_str.c_str() << _T(".append(c)\n");
					started = false;
				}

				if(!started)
				{
					gcode << _T("c = area.Curve()\n");
					gcode << _T("c.append(area.Vertex(0, area.Point(") << start.X(true) << _T(", ") << start.Y(true) << _T("), area.Point(0, 0)))\n");
					started = true;
				}
				span_object->GetEndPoint(e);
				CNCPoint end(pMachineState->Fixture().Adjustment(e));

				if(type == LineType)
				{
					gcode << _T("c.append(area.Vertex(0, area.Point(") << end.X(true) << _T(", ") << end.Y(true) << _T("), area.Point(0, 0)))\n");
				}
				else if(type == ArcType)
				{
					span_object->GetCentrePoint(c);
					CNCPoint centre(pMachineState->Fixture().Adjustment(c));

					double pos[3];
					heeksCAD->GetArcAxis(span_object, pos);
					int span_type = (pos[2] >=0) ? 1:-1;
					gcode << _T("c.append(area.Vertex(") << span_type << _T(", area.Point(") << end.X(true) << _T(", ") << end.Y(true);
					gcode << _T("), area.Point(") << centre.X(true) << _T(", ") << centre.Y(true) << _T(")))\n");
				}
				memcpy(prev_e, e, 3*sizeof(double));
			} // End if - then
			else
			{
				if (type == CircleType)
				{
					std::list< std::pair<int, gp_Pnt > > points;
					span_object->GetCentrePoint(c);

					// Setup the four arcs that will make up the circle using UNadjusted
					// coordinates first so that the offsets align with the X and Y axes.
					double small_amount = 0.001;
					double radius = heeksCAD->CircleGetRadius(span_object);

					points.push_back( std::make_pair(LINEAR, gp_Pnt( c[0] - small_amount, c[1] + radius, c[2] )) ); // north (almost)
					points.push_back( std::make_pair(CW, gp_Pnt( c[0], c[1] + radius, c[2] )) ); // north
					points.push_back( std::make_pair(CW, gp_Pnt( c[0] + radius, c[1], c[2] )) ); // east
					points.push_back( std::make_pair(CW, gp_Pnt( c[0], c[1] - radius, c[2] )) ); // south
					points.push_back( std::make_pair(CW, gp_Pnt( c[0] - radius, c[1], c[2] )) ); // west
					points.push_back( std::make_pair(CW, gp_Pnt( c[0] - small_amount, c[1] + radius, c[2] )) ); // north (almost)

					CNCPoint centre(pMachineState->Fixture().Adjustment(c));

					for (std::list< std::pair<int, gp_Pnt > >::iterator l_itPoint = points.begin(); l_itPoint != points.end(); l_itPoint++)
					{
						CNCPoint pnt = pMachineState->Fixture().Adjustment( l_itPoint->second );

						gcode << _T("c.append(area.Vertex(") << l_itPoint->first << _T(", area.Point(");
						gcode << pnt.X(true) << (_T(", ")) << pnt.Y(true);
						gcode << _T("), area.Point(") << centre.X(true) << _T(", ") << centre.Y(true) << _T(")))\n");
					} // End for
				}
			} // End if - else
		}
	}

	if(started)
	{
		gcode << area_str.c_str() << _T(".append(c)\n");
	}

	// delete the spans made
	for(std::list<HeeksObj*>::iterator It = new_spans.begin(); It != new_spans.end(); It++)
	{
		HeeksObj* span = *It;
		delete span;
	}

	gcode << _T("\n");
	return(wxString(gcode.str().c_str()));
}

const wxBitmap &CRaft::GetIcon()
{
	if(!m_active)return GetInactiveIcon();
	static wxBitmap* icon = NULL;
	if(icon == NULL)icon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/raft.png")));
	return *icon;
}
//This is what happens when the user pushes the post-process button.
Python CRaft::AppendTextToProgram(CMachineState *pMachineState)
{
	Python python;

	ReloadPointers();   // Make sure all the m_sketches values have been converted into children.

	CTool *pTool = CTool::Find( m_tool_number );
	CTool *pBaseTool = CTool::Find( m_params.m_baselayerextrusion );
	CTool *pInterfaceTool = CTool::Find( m_params.m_interfacelayerextrusion );
	if (pTool == NULL)  //fix this to check for base and inteface tools and make sure the type is extrusion.
	{
		wxMessageBox(_T("Please select an extrusion for the base and interface layers"));
		return(python);
	} // End if - then


	python << CDepthOp::AppendTextToProgram(pMachineState);

    for (HeeksObj *object = GetFirstChild(); object != NULL; object = GetNextChild())
    {
		if(object == NULL || object->GetNumChildren() == 0){
			wxMessageBox(wxString::Format(_("Raft operation - Sketch doesn't exist")));
			continue;
		}

		HeeksObj* re_ordered_sketch = NULL;
		SketchOrderType order = heeksCAD->GetSketchOrder(object);
		if( 	(order != SketchOrderTypeCloseCW) &&
			(order != SketchOrderTypeCloseCCW) &&
			(order != SketchOrderTypeMultipleCurves) &&
			(order != SketchOrderHasCircles))
		{
			re_ordered_sketch = object->MakeACopy();
			heeksCAD->ReOrderSketch(re_ordered_sketch, SketchOrderTypeReOrder);
			object = re_ordered_sketch;
			order = heeksCAD->GetSketchOrder(object);
			if(	(order != SketchOrderTypeCloseCW) &&
				(order != SketchOrderTypeCloseCCW) &&
				(order != SketchOrderTypeMultipleCurves) &&
				(order != SketchOrderHasCircles))
			{
				switch(heeksCAD->GetSketchOrder(object))
				{
				case SketchOrderTypeOpen:
					{
						wxMessageBox(wxString::Format(_("Raft operation - Sketch must be a closed shape - sketch %d"), object->m_id));
						delete re_ordered_sketch;
						continue;
					}
					break;

				default:
					{
						wxMessageBox(wxString::Format(_("Raft operation - Badly ordered sketch - sketch %d"), object->m_id));
						delete re_ordered_sketch;
						continue;
					}
					break;
				}
			}
		}
//			python <<  _T("feedrate_hv(") << getfromtool, getfromoperation <<_T(")");
	//		python <<  _T("set_extruder_temp()");  
			
		if(object)
		{
			python << WriteSketchDefn(object, pMachineState, object->m_id);

			// start - assume we are at a suitable clearance height

			// Base Layers

			python << _T("feedrate_hv(") << pBaseTool->m_params.m_feedrate / theApp.m_program->m_units << _T(", ");  //set the feedrate for the base tool.
			python <<  pBaseTool->m_params.m_feedrate / theApp.m_program->m_units << _T(")\n");
			
			python << _T("extruder_temp(") << pBaseTool->m_params.m_temperature << _T(")\n");  //set extruder temp
			python << _T("extruder_on_fwd(") << pBaseTool->m_params.m_flowrate << _T(")\n");  //set flowrate

			int layercount=1 ;  // just keep a count so we can toggle alternate layers 90 degrees perpendicular.
			double starth, finalh;
			double extruderheight = 0.0;
			double layerh = (pBaseTool->m_params.m_layer_height);  //get the layerheight of the extrusion tool to use in the base layer.
			
			while (layercount <= (m_params.m_baselayers))
			{
				starth = extruderheight + layerh;
				finalh = starth - layerh;
				
				python << _T("area_funcs.pocket(a") << (int) object->m_id << _T(", tool_diameter/2, ");
				python << m_params.m_material_allowance / theApp.m_program->m_units;
				python << _T(", " ) << starth << _T(", ") << starth << _T(", ") << finalh << _T(", ");  //rapid down to height, start height, final height
				python << m_params.m_step_over / theApp.m_program->m_units;  //step over
				python << _T(", ") << layerh << _T(", ");  //step Z should equal layerheight
				python << m_params.m_round_corner_factor;
				python << _T(", clearance, ");
				python << m_params.m_starting_place;
				python << _T(", True, ");  //keep the tool down if possible.		    
				python << m_params.m_step_over / theApp.m_program->m_units;			    
			  	if ( layercount % 2 == 0 ){
					python << _T(", ") << m_params.m_zig_angle+90;
					}
  				else {
					python << _T(", ") << m_params.m_zig_angle;
				}
				
				python << _T(")\n");
		    
				// rapid back up to clearance plane
				python << _T("rapid(z = clearance)\n");
				extruderheight = extruderheight + layerh;
	    
				layercount = layercount + 1;
			}
						
			// Interface Layers

			layerh = (pInterfaceTool->m_params.m_layer_height);  //Get the layerheight for the interface extrusion tool.
			python << _T("feedrate_hv(") << pInterfaceTool->m_params.m_feedrate / theApp.m_program->m_units << _T(", ");
			python << pInterfaceTool->m_params.m_feedrate / theApp.m_program->m_units << _T(")\n");
			python << _T("extruder_temp(") << pInterfaceTool->m_params.m_temperature << _T(")\n");  //set extruder temp
			python << _T("extruder_on_fwd(") << pInterfaceTool->m_params.m_flowrate << _T(")\n");  //set flowrate


			while (layercount <= (m_params.m_baselayers + m_params.m_interfacelayers)) 
			{
				starth = extruderheight + layerh;
				finalh = starth - layerh;
				
				python << _T("area_funcs.pocket(a") << (int) object->m_id << _T(", tool_diameter/2, ");
				python << m_params.m_material_allowance / theApp.m_program->m_units;
				python << _T(", " ) << starth << _T(", ") << starth << _T(", ") << finalh << _T(", ");  //rapid down to height, start height, final height
				python << m_params.m_step_over / theApp.m_program->m_units;  //step over
				python << _T(", ") << layerh << _T(", ");  //step Z should equal layerheight
				python << m_params.m_round_corner_factor;
				python << _T(", clearance, ");
				python << m_params.m_starting_place;
				python << _T(", True, ");  //keep the tool down if possible.		    
				python << m_params.m_step_over / theApp.m_program->m_units;			    
			  	if ( layercount % 2 == 0 ){
					python << _T(", ") << m_params.m_zig_angle+90;
					}
  				else {
					python << _T(", ") << m_params.m_zig_angle;
				}
				
				python << _T(")\n");
		    
				// rapid back up to clearance plane
				python << _T("rapid(z = clearance)\n");
				extruderheight = extruderheight + layerh;
	    
				layercount = layercount + 1;
			}	
			

		}

		if(re_ordered_sketch)
		{
			delete re_ordered_sketch;
		}
	} // End for

	return(python);

} // End AppendTextToProgram() method


void CRaft::WriteDefaultValues()
{
	CDepthOp::WriteDefaultValues();

	CNCConfig config(CRaftParams::ConfigScope());
	config.Write(_T("StepOver"), m_params.m_step_over);
	config.Write(_T("MaterialAllowance"), m_params.m_material_allowance);
	config.Write(_T("RoundCornerFactor"), m_params.m_round_corner_factor);
	config.Write(_T("FromCenter"), m_params.m_starting_place);
	config.Write(_T("KeepToolDown"), m_params.m_keep_tool_down_if_poss);
	config.Write(_T("UseZigZag"), m_params.m_use_zig_zag);
	config.Write(_T("ZigAngle"), m_params.m_zig_angle);
	
	config.Write(_T("baselayers"), m_params.m_baselayers);	
	config.Write(_T("interfacelayers"), m_params.m_interfacelayers);
	config.Write(_T("baselayerextrusion"), m_params.m_baselayerextrusion);
	config.Write(_T("interfacelayerextrusion"), m_params.m_interfacelayerextrusion);	
}

void CRaft::ReadDefaultValues()
{
	CDepthOp::ReadDefaultValues();

	CNCConfig config(CRaftParams::ConfigScope());
	config.Read(_T("StepOver"), &m_params.m_step_over, 1.0);
	config.Read(_T("MaterialAllowance"), &m_params.m_material_allowance, 0.2);
	config.Read(_T("RoundCornerFactor"), &m_params.m_round_corner_factor, 1.0);
	config.Read(_T("FromCenter"), &m_params.m_starting_place, 1);
	config.Read(_T("KeepToolDown"), &m_params.m_keep_tool_down_if_poss, true);
	config.Read(_T("UseZigZag"), &m_params.m_use_zig_zag, false);
	config.Read(_T("ZigAngle"), &m_params.m_zig_angle);
	
	config.Read(_T("baselayers"), &m_params.m_baselayers, 2);	// Two base layers
	config.Read(_T("interfacelayers"), &m_params.m_interfacelayers, 2);
	config.Read(_T("baselayerextrusion"), &m_params.m_baselayerextrusion);
	config.Read(_T("interfacelayerextrusion"), m_params.m_interfacelayerextrusion);	

}
void CRaft::glCommands(bool select, bool marked, bool no_color)
{
	CDepthOp::glCommands( select, marked, no_color );
}

void CRaft::GetProperties(std::list<Property *> *list)
{
	AddSketchesProperties(list, this);
	m_params.GetProperties(this, list);
	CDepthOp::GetProperties(list);
}

HeeksObj *CRaft::MakeACopy(void)const
{
	return new CRaft(*this);
}

void CRaft::CopyFrom(const HeeksObj* object)
{
	operator=(*((CRaft*)object));
}

CRaft::CRaft( const CRaft & rhs ) : CDepthOp(rhs)
{
	m_sketches.clear();
	std::copy( rhs.m_sketches.begin(), rhs.m_sketches.end(), std::inserter( m_sketches, m_sketches.begin() ) );
	m_params = rhs.m_params;
}

CRaft & CRaft::operator= ( const CRaft & rhs )
{
	if (this != &rhs)
	{
		CDepthOp::operator=(rhs);
		m_sketches.clear();
		std::copy( rhs.m_sketches.begin(), rhs.m_sketches.end(), std::inserter( m_sketches, m_sketches.begin() ) );

		m_params = rhs.m_params;
		static double max_deviation_for_spline_to_arc;
	}

	return(*this);
}

bool CRaft::CanAddTo(HeeksObj* owner)
{
	return ((owner != NULL) && (owner->GetType() == OperationsType));
}

void CRaft::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element = new TiXmlElement( "Raft" );
	root->LinkEndChild( element );
	m_params.WriteXMLAttributes(element);

	// write sketch ids
	for(std::list<int>::iterator It = m_sketches.begin(); It != m_sketches.end(); It++)
	{
		int sketch = *It;
		TiXmlElement * sketch_element = new TiXmlElement( "sketch" );
		element->LinkEndChild( sketch_element );
		sketch_element->SetAttribute("id", sketch);
	}

	WriteBaseXML(element);
}

// static member function
HeeksObj* CRaft::ReadFromXMLElement(TiXmlElement* element)
{
	CRaft* new_object = new CRaft;

	std::list<TiXmlElement *> elements_to_remove;

	// read profile parameters
	TiXmlElement* params = TiXmlHandle(element).FirstChildElement("params").Element();
	if(params)
	{
		new_object->m_params.ReadFromXMLElement(params);
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

CRaft::CRaft(const std::list<int> &sketches, const int tool_number )
	: CDepthOp(GetTypeString(), &sketches, tool_number ), m_sketches(sketches)
{
	ReadDefaultValues();
	m_params.set_initial_values(tool_number);

	for (Sketches_t::iterator sketch = m_sketches.begin(); sketch != m_sketches.end(); sketch++)
	{
		HeeksObj *object = heeksCAD->GetIDObject( SketchType, *sketch );
		if (object != NULL)
		{
			Add( object, NULL );
		}
	}

	m_sketches.clear();
}

CRaft::CRaft(const std::list<HeeksObj *> &sketches, const int tool_number )
	: CDepthOp(GetTypeString(), sketches, tool_number )
{
	ReadDefaultValues();
	m_params.set_initial_values(tool_number);

	for (std::list<HeeksObj *>::const_iterator sketch = sketches.begin(); sketch != sketches.end(); sketch++)
	{
		Add( *sketch, NULL );
	}
}



/**
	The old version of the CRaft object stored references to graphics as type/id pairs
	that get read into the m_symbols list.  The new version stores these graphics references
	as child elements (based on ObjList).  If we read in an old-format file then the m_symbols
	list will have data in it for which we don't have children.  This routine converts
	these type/id pairs into the HeeksObj pointers as children.
 */
void CRaft::ReloadPointers()
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
	This method adjusts any parameters that don't make sense.  It should report a list
	of changes in the list of strings.
 */
std::list<wxString> CRaft::DesignRulesAdjustment(const bool apply_changes)
{
	std::list<wxString> changes;

	int num_sketches = 0;
	for(HeeksObj *obj = GetFirstChild(); obj != NULL; obj = GetNextChild())
	{
		if (obj->GetType() == SketchType)
		{
		    num_sketches++;
		}
	} // End if - then

	if (num_sketches == 0)
	{
#ifdef UNICODE
			std::wostringstream l_ossChange;
#else
			std::ostringstream l_ossChange;
#endif

			l_ossChange << _("No valid sketches upon which to act for raft operations") << " id='" << m_id << "'\n";
			changes.push_back(l_ossChange.str().c_str());
	} // End if - then


	if (m_tool_number > 0)
	{
		// Make sure the hole depth isn't greater than the tool's cutting depth.
		CTool *pCutter = (CTool *) CTool::Find( m_tool_number );

		if ((pCutter != NULL) && (pCutter->m_params.m_cutting_edge_height < m_depth_op_params.m_final_depth))
		{
			// The tool we've chosen can't cut as deep as we've setup to go.

#ifdef UNICODE
			std::wostringstream l_ossChange;
#else
			std::ostringstream l_ossChange;
#endif

			l_ossChange << _("Adjusting depth of raft") << " id='" << m_id << "' " << _("from") << " '"
				<< m_depth_op_params.m_final_depth << "' " << _("to") << " "
				<< pCutter->m_params.m_cutting_edge_height << " " << _("due to cutting edge length of selected tool") << "\n";
			changes.push_back(l_ossChange.str().c_str());

			if (apply_changes)
			{
				m_depth_op_params.m_final_depth = pCutter->m_params.m_cutting_edge_height;
			} // End if - then
		} // End if - then

		// Also make sure the 'step-over' distance isn't larger than the cutting tool's diameter.
		if ((pCutter != NULL) && ((pCutter->CuttingRadius(false) * 2.0) < m_params.m_step_over))
		{
			wxString change;
			change << _("The step-over distance for raft (id=");
			change << m_id;
			change << _(") is larger than the tool's diameter");
			changes.push_back(change);

			if (apply_changes)
			{
				m_params.m_step_over = (pCutter->CuttingRadius(false) * 2.0);
			} // End if - then
		} // End if - then
	} // End if - then

	std::list<wxString> depth_op_changes = CDepthOp::DesignRulesAdjustment( apply_changes );
	std::copy( depth_op_changes.begin(), depth_op_changes.end(), std::inserter( changes, changes.end() ) );

	return(changes);

} // End DesignRulesAdjustment() method

static void on_set_spline_deviation(double value, HeeksObj* object){
	CRaft::max_deviation_for_spline_to_arc = value;
	CRaft::WriteToConfig();
}

// static
void CRaft::GetOptions(std::list<Property *> *list)
{
	list->push_back ( new PropertyDouble ( _("Pocket spline deviation"), max_deviation_for_spline_to_arc, NULL, on_set_spline_deviation ) );
}

// static
void CRaft::ReadFromConfig()
{
	CNCConfig config(CRaftParams::ConfigScope());
	config.Read(_T("PocketSplineDeviation"), &max_deviation_for_spline_to_arc, 0.1);
}

// static
void CRaft::WriteToConfig()
{
	CNCConfig config(CRaftParams::ConfigScope());
	config.Write(_T("PocketSplineDeviation"), max_deviation_for_spline_to_arc);
}

static ReselectSketches reselect_sketches;

void CRaft::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
	reselect_sketches.m_sketches = &m_sketches;
	reselect_sketches.m_object = this;
	t_list->push_back(&reselect_sketches);

    CDepthOp::GetTools( t_list, p );
}
bool CRaftParams::operator==(const CRaftParams & rhs) const
{
	if (m_starting_place != rhs.m_starting_place) return(false);
	if (m_round_corner_factor != rhs.m_round_corner_factor) return(false);
	if (m_material_allowance != rhs.m_material_allowance) return(false);
	if (m_step_over != rhs.m_step_over) return(false);
	if (m_keep_tool_down_if_poss != rhs.m_keep_tool_down_if_poss) return(false);
	if (m_use_zig_zag != rhs.m_use_zig_zag) return(false);
	if (m_zig_angle != rhs.m_zig_angle) return(false);
	if (m_baselayers != rhs.m_baselayers) return(false);
	if (m_interfacelayers != rhs.m_interfacelayers) return(false);
	if (m_baselayerextrusion != rhs.m_baselayerextrusion) return(false);
	if (m_interfacelayerextrusion != rhs.m_interfacelayerextrusion) return(false);

	return(true);
}

bool CRaft::operator==(const CRaft & rhs) const
{
	if (m_params != rhs.m_params) return(false);

	return(CDepthOp::operator==(rhs));
}

#endif //#ifndef STABLE_OPS_ONLY
