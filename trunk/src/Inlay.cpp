// Inlay.cpp
/*
 * Copyright (c) 2009, Dan Heeks, Perttu Ahola
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "stdafx.h"
#include "Inlay.h"
#include "CNCConfig.h"
#include "ProgramCanvas.h"
#include "interface/HeeksObj.h"
#include "interface/ObjList.h"
#include "interface/PropertyInt.h"
#include "interface/PropertyDouble.h"
#include "interface/PropertyLength.h"
#include "interface/PropertyChoice.h"
#include "tinyxml/tinyxml.h"
#include "Operations.h"
#include "CuttingTool.h"
#include "Profile.h"
#include "Fixture.h"
#include "CNCPoint.h"
#include "PythonStuff.h"
#include "Contour.h"

#include <sstream>
#include <iomanip>
#include <vector>
#include <algorithm>

#include <BRepOffsetAPI_MakeOffset.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Shape.hxx>
#include <BRepTools_WireExplorer.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <gp_Circ.hxx>
#include <ShapeFix_Wire.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRep_Tool.hxx>
#include <BRepTools.hxx>
#include <BRepMesh.hxx>
#include <Poly_Polygon3D.hxx>

extern CHeeksCADInterface* heeksCAD;

/* static */ double CInlay::max_deviation_for_spline_to_arc = 0.1;

void CInlayParams::set_initial_values()
{
	CNCConfig config(ConfigPrefix());
	config.Read(_T("ToolOnSide"), (int *) &m_tool_on_side, (int) eRightOrInside);
}

void CInlayParams::write_values_to_config()
{
	CNCConfig config(ConfigPrefix());
	config.Write(_T("ToolOnSide"), (int) m_tool_on_side);
}

static void on_set_tool_on_side(int value, HeeksObj* object){
	switch(value)
	{
	case 0:
		((CInlay*)object)->m_params.m_tool_on_side = CInlayParams::eLeftOrOutside;
		break;
	case 1:
		((CInlay*)object)->m_params.m_tool_on_side = CInlayParams::eRightOrInside;
		break;
	default:
		((CInlay*)object)->m_params.m_tool_on_side = CInlayParams::eOn;
		break;
	}
	((CInlay*)object)->WriteDefaultValues();
}

void CInlayParams::GetProperties(CInlay* parent, std::list<Property *> *list)
{
    std::list< wxString > choices;

    SketchOrderType order = SketchOrderTypeUnknown;

    if(parent->GetNumChildren() > 0)
    {
        HeeksObj* sketch = NULL;

        for (HeeksObj *object = parent->GetFirstChild(); ((sketch == NULL) && (object != NULL)); object = parent->GetNextChild())
        {
            if (object->GetType() == SketchType)
            {
                sketch = object;
            }
        }

        if(sketch != NULL)
        {
            order = heeksCAD->GetSketchOrder(sketch);
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
        }

        choices.push_back(_("On"));

        int choice = int(CInlayParams::eOn);
        switch (parent->m_params.m_tool_on_side)
        {
            case CInlayParams::eRightOrInside:	choice = 1;
                    break;

            case CInlayParams::eOn:	choice = 2;
                    break;

            case CInlayParams::eLeftOrOutside:	choice = 0;
                    break;
        } // End switch

        list->push_back(new PropertyChoice(_("tool on side"), choices, choice, parent, on_set_tool_on_side));
    }


}

void CInlayParams::WriteXMLAttributes(TiXmlNode *root)
{
	TiXmlElement * element;
	element = new TiXmlElement( "params" );
	root->LinkEndChild( element );

	element->SetAttribute("side", m_tool_on_side);
}

void CInlayParams::ReadParametersFromXMLElement(TiXmlElement* pElem)
{
    int int_for_enum = m_tool_on_side;
	pElem->Attribute("side", &int_for_enum);
	m_tool_on_side = (eSide)int_for_enum;
}




/**
	This method is called when the CAD operator presses the Python button.  This method generates
	Python source code whose job will be to generate RS-274 GCode.  It's done in two steps so that
	the Python code can be configured to generate GCode suitable for various CNC interpreters.
 */
void CInlay::AppendTextToProgram( const CFixture *pFixture )
{
	ReloadPointers();

	wxString gcode;
	CDepthOp::AppendTextToProgram( pFixture );

	unsigned int number_of_bad_sketches = 0;
	double tolerance = heeksCAD->GetTolerance();
	std::list<HeeksObj *> mirrored_sketches;
	CBox	bounding_box;

	CCuttingTool *pCuttingTool = CCuttingTool::Find( m_cutting_tool_number );
	if (! pCuttingTool)
	{
		return;
	}

	// NOTE: The determination of the 'max_depth_in_female_pass' assumes that the female
	// pass occurs BEFORE the male pass.
	std::list<eInlayPass_t> passes;
	passes.push_back( eFemale );	// Always.
	passes.push_back( eMale );	// Only for inlay work.

	// Record the maximum achieved depth (not the asking depth) for all operations in the female
	// pass of an inlay operation.  When we machine the male part, we will set this as the 'top surface'
	// height and machine from there down.  (NOTE: This is a relative value so a Z value of -5 and
	// a starting_depth of 0 would be assigned a depth value of '5'.
	double max_depth_in_female_pass = 0.0;

	for (std::list<eInlayPass_t>::const_iterator itPass = passes.begin(); itPass != passes.end(); itPass++)
	{
		CNCPoint last_position(0.0, 0.0, 0.0);

		for (HeeksObj *object = GetFirstChild(); object != NULL; object = GetNextChild())
		{
			std::list<TopoDS_Shape> wires;
			if (! heeksCAD->ConvertSketchToFaceOrWire( object, wires, false))
			{
				number_of_bad_sketches++;
			} // End if - then
			else
			{
				// The wire(s) represent the sketch objects for a tool path.
				CBox box;
				object->GetBox(box);
				bounding_box.Insert(box);

				if (object->GetShortString() != NULL)
				{
					gcode << _T("comment(") << PythonString(object->GetShortString()) << _T(")\n");
				}

				try {
					for(std::list<TopoDS_Shape>::iterator It2 = wires.begin(); It2 != wires.end(); It2++)
					{
						TopoDS_Shape& wire_to_fix = *It2;
						ShapeFix_Wire fix;
						fix.Load( TopoDS::Wire(wire_to_fix) );
						fix.FixReorder();

						TopoDS_Shape wire = fix.Wire();

						BRepBuilderAPI_Transform transform(pFixture->GetMatrix());
						transform.Perform(wire, false);
						wire = transform.Shape();

						if (*itPass == eFemale)
						{
							// If we're going to produce an inlay toolpath then we're going to need to
							// create a pocket operation for the areas between the sketches passed in.
							// (Actually, it will be based on the mirrored version of the sketch passed in)

							TopoDS_Wire mirrored_wire( TopoDS::Wire(wire) );
							gp_Trsf rotation;

							rotation.SetRotation( gp_Ax1(gp_Pnt(0,0,0), gp_Dir(1,0,0)), PI );
							BRepBuilderAPI_Transform rotate(rotation);
							rotate.Perform(mirrored_wire, false); // notice false as second parameter

							HeeksObj *sketch = heeksCAD->NewSketch();
							if (heeksCAD->ConvertWireToSketch(TopoDS::Wire(rotate.Shape()), sketch, heeksCAD->GetTolerance()))
							{
								for (int i=0; (heeksCAD->GetSketchOrder(sketch) != SketchOrderTypeCloseCCW) && (i<4); i++)
								{
									// At least try to make them all consistently oriented.
									heeksCAD->ReOrderSketch( sketch, SketchOrderTypeCloseCCW );
								} // End for

								mirrored_sketches.push_back(sketch);
							} // End if - then
						} // End if - then

						BRepOffsetAPI_MakeOffset offset_wire(TopoDS::Wire(wire));

						// Now generate a toolpath along this wire.
						std::list<double> depths = GetDepths();
						if (*itPass == eMale) depths.reverse();

						for (std::list<double>::iterator itDepth = depths.begin(); itDepth != depths.end(); itDepth++)
						{
							double radius = pCuttingTool->CuttingRadius(false,m_depth_op_params.m_start_depth - *itDepth);

							if (m_params.m_tool_on_side == CInlayParams::eLeftOrOutside) radius *= +1.0;
							if (m_params.m_tool_on_side == CInlayParams::eRightOrInside) radius *= -1.0;
							if (m_params.m_tool_on_side == CInlayParams::eOn) radius = 0.0;

							TopoDS_Wire tool_path_wire(TopoDS::Wire(wire));

							double offset = radius;
							if (offset < 0) offset *= -1.0;

							if (offset > tolerance)
							{
								offset_wire.Perform(radius);
								if (! offset_wire.IsDone())
								{
									break;
								}
								tool_path_wire = TopoDS::Wire(offset_wire.Shape());
							}

							if ((m_params.m_tool_on_side == CInlayParams::eOn) || (offset > tolerance))
							{
								gp_Trsf matrix;

								matrix.SetTranslation( gp_Vec( gp_Pnt(0,0,0), gp_Pnt( 0,0,*itDepth)));
								BRepBuilderAPI_Transform transform(matrix);
								transform.Perform(tool_path_wire, false); // notice false as second parameter
								tool_path_wire = TopoDS::Wire(transform.Shape());

								if (*itPass == eMale)
								{
									gp_Trsf rotation;

									rotation.SetRotation( gp_Ax1(gp_Pnt(0,0,0), gp_Dir(1,0,0)), PI );
									BRepBuilderAPI_Transform rotate(rotation);
									rotate.Perform(tool_path_wire, false); // notice false as second parameter
									tool_path_wire = TopoDS::Wire(rotate.Shape());

									gp_Trsf translation;
									translation.SetTranslation( gp_Vec( gp_Pnt(0,0,0), gp_Pnt( 0,0,-1.0 * max_depth_in_female_pass)));
									BRepBuilderAPI_Transform translate(translation);
									translate.Perform(tool_path_wire, false); // notice false as second parameter
									tool_path_wire = TopoDS::Wire(translate.Shape());
								}

								if (*itPass == eFemale)
								{
									// We only want to remember those depths for which a tool path was actually
									// generated.  It's possible that the m_depth_op_params.m_final_depth value is
									// too deep (and hence too large a radius) for a chamfering tool to produce
									// a parallel shape.  For these operations, we don't actually generate a toolpath.
									if ((m_depth_op_params.m_start_depth - *itDepth) > max_depth_in_female_pass)
									{
										max_depth_in_female_pass = m_depth_op_params.m_start_depth - *itDepth;
									}
								}

								gcode << CContour::GeneratePathFromWire(	tool_path_wire,
																last_position,
																pFixture,
																m_depth_op_params.m_clearance_height,
																m_depth_op_params.m_rapid_down_to_height );
							} // End if - then
						} // End for
					} // End for
				} // End try
				catch (Standard_Failure & error) {
					(void) error;	// Avoid the compiler warning.
					Handle_Standard_Failure e = Standard_Failure::Caught();
					number_of_bad_sketches++;
				} // End catch
			} // End if - else
		} // End for

		if (*itPass == eMale)
		{
			// Make sure the bounding box is one tool radius larger than all the sketches that
			// have been mirrored.  We need to create one large sketch with the bounding box
			// 'order' in one direction and all the mirrored sketch's orders in the other
			// direction.  We can then create a pocket operation to remove the material between
			// the mirrored sketches down to the inverted 'top surface' depth.

			HeeksObj* bounding_sketch = heeksCAD->NewSketch();

			double start[3];
			double end[3];

			// left edge
			start[0] = bounding_box.MinX() - (bounding_box.Width()/2.0);
            start[1] = bounding_box.MinY() - (bounding_box.Height()/2.0);
			start[2] = bounding_box.MinZ();

			end[0] = bounding_box.MinX() - (bounding_box.Width()/2.0);
			end[1] = bounding_box.MaxY() + (bounding_box.Height()/2.0);
			end[2] = bounding_box.MinZ();

			bounding_sketch->Add( heeksCAD->NewLine( start, end ), NULL );

			// top edge
			start[0] = bounding_box.MinX() - (bounding_box.Width()/2.0);
			start[1] = bounding_box.MaxY() + (bounding_box.Height()/2.0);
			start[2] = bounding_box.MinZ();

			end[0] = bounding_box.MaxX() + (bounding_box.Width()/2.0);
			end[1] = bounding_box.MaxY() + (bounding_box.Height()/2.0);
			end[2] = bounding_box.MinZ();

			bounding_sketch->Add( heeksCAD->NewLine( start, end ), NULL );

			// right edge
			start[0] = bounding_box.MaxX() + (bounding_box.Width()/2.0);
			start[1] = bounding_box.MaxY() + (bounding_box.Height()/2.0);
			start[2] = bounding_box.MinZ();

			end[0] = bounding_box.MaxX() + (bounding_box.Width()/2.0);
			end[1] = bounding_box.MinY() - (bounding_box.Height()/2.0);
			end[2] = bounding_box.MinZ();

			bounding_sketch->Add( heeksCAD->NewLine( start, end ), NULL );

			// bottom edge
			start[0] = bounding_box.MaxX() + (bounding_box.Width()/2.0);
			start[1] = bounding_box.MinY() - (bounding_box.Height()/2.0);
			start[2] = bounding_box.MinZ();

			end[0] = bounding_box.MinX() - (bounding_box.Width()/2.0);
			end[1] = bounding_box.MinY() - (bounding_box.Height()/2.0);
			end[2] = bounding_box.MinZ();

			bounding_sketch->Add( heeksCAD->NewLine( start, end ), NULL );




            {
                // Mirror the bounding sketch about the X axis so that it surrounds the
                // mirrored sketches we've collected.
                gp_Trsf rotation;

                rotation.SetRotation( gp_Ax1(gp_Pnt(0,0,0), gp_Dir(1,0,0)), PI );
                double m[16];	// A different form of the transformation matrix.
                CFixture::extract( rotation, m );
                bounding_sketch->ModifyByMatrix(m);
            }

            {
                // Mirror the bounding sketch about the X axis so that it surrounds the
                // mirrored sketches we've collected.
                gp_Trsf scale;

                scale.SetScaleFactor(1.1);
                double m[16];	// A different form of the transformation matrix.
                CFixture::extract( scale, m );
                bounding_sketch->ModifyByMatrix(m);
            }

            for (int i=0; (heeksCAD->GetSketchOrder(bounding_sketch) != SketchOrderTypeCloseCW) && (i<4); i++)
            {
                // At least try to make them all consistently oriented.
                heeksCAD->ReOrderSketch( bounding_sketch, SketchOrderTypeCloseCW );
            } // End for

			for (std::list<HeeksObj *>::iterator itObject = mirrored_sketches.begin(); itObject != mirrored_sketches.end(); itObject++)
			{
			    std::list<HeeksObj*> new_lines_and_arcs;
			    for (HeeksObj *child = (*itObject)->GetFirstChild(); child != NULL; child = (*itObject)->GetNextChild())
			    {
					new_lines_and_arcs.push_back(child);
				}

                ((ObjList *)bounding_sketch)->Add( new_lines_and_arcs );
			} // End for

			heeksCAD->Add( bounding_sketch, NULL );
		}
	} // End for

	theApp.m_program_canvas->m_textCtrl->AppendText(gcode.c_str());
}




/**
	This is the Graphics Library Commands (from the OpenGL set).  This method calls the OpenGL
	routines to paint the drill action in the graphics window.  The graphics is transient.

	Part of its job is to re-paint the elements that this CInlay object refers to so that
	we know what CAD objects this CNC operation is referring to.
 */
void CInlay::glCommands(bool select, bool marked, bool no_color)
{
	CDepthOp::glCommands( select, marked, no_color );
}




void CInlay::GetProperties(std::list<Property *> *list)
{
	m_params.GetProperties(this, list);
	CDepthOp::GetProperties(list);
}

HeeksObj *CInlay::MakeACopy(void)const
{
	return new CInlay(*this);
}

void CInlay::CopyFrom(const HeeksObj* object)
{
	operator=(*((CInlay*)object));
}

bool CInlay::CanAddTo(HeeksObj* owner)
{
	return owner->GetType() == OperationsType;
}

void CInlay::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element = new TiXmlElement( "Inlay" );
	root->LinkEndChild( element );
	m_params.WriteXMLAttributes(element);

    if (m_symbols.size() > 0)
    {
        TiXmlElement * symbols;
        symbols = new TiXmlElement( "symbols" );
        element->LinkEndChild( symbols );

        for (Symbols_t::const_iterator l_itSymbol = m_symbols.begin(); l_itSymbol != m_symbols.end(); l_itSymbol++)
        {
            TiXmlElement * symbol = new TiXmlElement( "symbol" );
            symbols->LinkEndChild( symbol );
            symbol->SetAttribute("type", l_itSymbol->first );
            symbol->SetAttribute("id", l_itSymbol->second );
        } // End for
    } // End if - then

	WriteBaseXML(element);
}

// static member function
HeeksObj* CInlay::ReadFromXMLElement(TiXmlElement* element)
{
	CInlay* new_object = new CInlay;
	std::list<TiXmlElement *> elements_to_remove;

	// read point and circle ids
	for(TiXmlElement* pElem = TiXmlHandle(element).FirstChildElement().Element(); pElem; pElem = pElem->NextSiblingElement())
	{
		std::string name(pElem->Value());
		if(name == "params"){
			new_object->m_params.ReadParametersFromXMLElement(pElem);
			elements_to_remove.push_back(pElem);
		}
		else if(name == "symbols"){
			for(TiXmlElement* child = TiXmlHandle(pElem).FirstChildElement().Element(); child; child = child->NextSiblingElement())
			{
				if (child->Attribute("type") && child->Attribute("id"))
				{
					new_object->AddSymbol( atoi(child->Attribute("type")), atoi(child->Attribute("id")) );
				}
			} // End for
			elements_to_remove.push_back(pElem);
		} // End if
	}

	for (std::list<TiXmlElement*>::iterator itElem = elements_to_remove.begin(); itElem != elements_to_remove.end(); itElem++)
	{
		element->RemoveChild(*itElem);
	}

	new_object->ReadBaseXML(element);

	return new_object;
}


/**
	This method adjusts any parameters that don't make sense.  It should report a list
	of changes in the list of strings.
 */
std::list<wxString> CInlay::DesignRulesAdjustment(const bool apply_changes)
{
	std::list<wxString> changes;

	return(changes);

} // End DesignRulesAdjustment() method


/**
    This method returns TRUE if the type of symbol is suitable for reference as a source of location
 */
bool CInlay::CanAdd( HeeksObj *object )
{
    switch (object->GetType())
    {
        case CircleType:
        case SketchType:
		case LineType:
            return(true);

        default:
            return(false);
    }
}


void CInlay::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
    CDepthOp::GetTools( t_list, p );
}

static void on_set_spline_deviation(double value, HeeksObj* object){
	CInlay::max_deviation_for_spline_to_arc = value;
	CInlay::WriteToConfig();
}

// static
void CInlay::GetOptions(std::list<Property *> *list)
{
	list->push_back ( new PropertyDouble ( _("Inlay spline deviation"), max_deviation_for_spline_to_arc, NULL, on_set_spline_deviation ) );
}


void CInlay::ReloadPointers()
{
	for (Symbols_t::iterator l_itSymbol = m_symbols.begin(); l_itSymbol != m_symbols.end(); l_itSymbol++)
	{
		HeeksObj *object = heeksCAD->GetIDObject( l_itSymbol->first, l_itSymbol->second );
		if (CanAdd(object))
		{
			Add( object, NULL );
		}
	}

	m_symbols.clear();
}


CInlay::CInlay( const CInlay & rhs ) : CDepthOp( rhs )
{
    m_params.set_initial_values();
	*this = rhs;	// Call the assignment operator.
}

CInlay & CInlay::operator= ( const CInlay & rhs )
{
	if (this != &rhs)
	{
		m_params = rhs.m_params;
		m_symbols.clear();
		std::copy( rhs.m_symbols.begin(), rhs.m_symbols.end(), std::inserter( m_symbols, m_symbols.begin() ) );

		CDepthOp::operator=( rhs );
	}

	return(*this);
}


