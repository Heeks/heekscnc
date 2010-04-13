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
#include "Pocket.h"

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
	config.Read(_T("BoarderWidth"), (double *) &m_boarder_width, 25.4);
	config.Read(_T("ClearanceTool"), &m_clearance_tool, 0);
	config.Read(_T("Pass"), (int *) &m_pass, (int) eBoth );
	config.Read(_T("MirrorAxis"), (int *) &m_mirror_axis, (int) eXAxis );
}

void CInlayParams::write_values_to_config()
{
	CNCConfig config(ConfigPrefix());
	config.Write(_T("BoarderWidth"), m_boarder_width);
	config.Write(_T("ClearanceTool"), m_clearance_tool);
	config.Write(_T("Pass"), (int) m_pass );
	config.Write(_T("MirrorAxis"), (int) m_mirror_axis );
}

static void on_set_boarder_width(double value, HeeksObj* object)
{
	((CInlay*)object)->m_params.m_boarder_width = value;
	((CInlay*)object)->WriteDefaultValues();
}

static void on_set_clearance_tool(int value, HeeksObj* object)
{
	((CInlay*)object)->m_params.m_clearance_tool = value;
	((CInlay*)object)->WriteDefaultValues();
}

static void on_set_pass(int value, HeeksObj* object)
{
	((CInlay*)object)->m_params.m_pass = CInlayParams::eInlayPass_t(value);
	((CInlay*)object)->WriteDefaultValues();
}

static void on_set_mirror_axis(int value, HeeksObj* object)
{
	((CInlay*)object)->m_params.m_mirror_axis = CInlayParams::eAxis_t(value);
	((CInlay*)object)->WriteDefaultValues();
}

void CInlayParams::GetProperties(CInlay* parent, std::list<Property *> *list)
{
    list->push_back(new PropertyLength(_("Boarder Width"), m_boarder_width, parent, on_set_boarder_width));

    {
		std::vector< std::pair< int, wxString > > tools = CCuttingTool::FindAllCuttingTools();

		int choice = 0;
        std::list< wxString > choices;
		for (std::vector< std::pair< int, wxString > >::size_type i=0; i<tools.size(); i++)
		{
                	choices.push_back(tools[i].second);

			if (m_clearance_tool == tools[i].first)
			{
                		choice = int(i);
			} // End if - then
		} // End for

		list->push_back(new PropertyChoice(_("Clearance Tool"), choices, choice, parent, on_set_clearance_tool));
	}

	{
		int choice = (int) m_pass;
        std::list< wxString > choices;

		choices.push_back(_("Female half"));
		choices.push_back(_("Male half"));
		choices.push_back(_("Both halves"));

		list->push_back(new PropertyChoice(_("GCode generation"), choices, choice, parent, on_set_pass));
	}

	{
		int choice = (int) m_mirror_axis;
        std::list< wxString > choices;

		choices.push_back(_("X Axis"));
		choices.push_back(_("Y Axis"));

		list->push_back(new PropertyChoice(_("Mirror Axis"), choices, choice, parent, on_set_mirror_axis));
	}

}

void CInlayParams::WriteXMLAttributes(TiXmlNode *root)
{
	TiXmlElement * element;
	element = new TiXmlElement( "params" );
	root->LinkEndChild( element );

	element->SetAttribute("boarder", m_boarder_width);
	element->SetAttribute("clearance_tool", m_clearance_tool);
	element->SetAttribute("pass", (int) m_pass);
	element->SetAttribute("mirror_axis", (int) m_mirror_axis);
}

void CInlayParams::ReadParametersFromXMLElement(TiXmlElement* pElem)
{
	pElem->Attribute("boarder", &m_boarder_width);
	pElem->Attribute("clearance_tool", &m_clearance_tool);
	pElem->Attribute("pass", (int *) &m_pass);
	pElem->Attribute("mirror_axis", (int *) &m_mirror_axis);
}




/**
	This method is called when the CAD operator presses the Python button.  This method generates
	Python source code whose job will be to generate RS-274 GCode.  It's done in two steps so that
	the Python code can be configured to generate GCode suitable for various CNC interpreters.
 */
void CInlay::AppendTextToProgram( const CFixture *pFixture )
{
    wxString gcode = GenerateGCode( pFixture, false );
    theApp.m_program_canvas->m_textCtrl->AppendText(gcode.c_str());
}


/**
    This routine performs two separate functions.  The first is to produce the GCode requied to
    drive the chamfering bit around the material to produce both the male and female halves
    of the inlay operation.  The second function is to generate a set of mirrored sketch
    objects along with their pocketing operations.  These pocket operations are required when
    we generate the male half of the inlay.  We need to machine some material down so that, when
    it is turned upside-down and placed on top of the female piece, the two halves line up
    correctly.  The idea is that these two halves will be glued together and, when the glue
    is dry, the remainder of the male half will be machined off leaving just those sections
    that remain within the female half.

    All this means that the peaks of the male half need to be no higher than the valleys in
    the female half.  This is done on a per-sketch basis because the combination of the
    sketche's geometry and the chamfering tool's geometry means that each sketch may produce
    a pocket whose depth is less than the inlay operation's nominated depth.  In these cases
    we need to create a pocket operation that is bounded by this sketch but will remove most
    of the material directly above the sketch down to the depth that corresponds to the
    depth of the pocket in the female half.

    The other pocket that is produced is a combination of a square boarder sketch and the
    mirrored versions of all the selected sketches.  This module ensures that the boarder
    sketch is oriented clockwise and all the internal sketches are oriented counter-clockwise.
    This will allow the pocket to remove all the material between the selected sketches
    down to a height that will mate with the top-most surface of the female half.

    The boarder is generated based on the bounding box of all the selected sketches as
    well as the boarder width found in the InlayParams object.

    The two functions of this method are enabled by the 'keep_mirrored_sketches' flag.  When
    this flag is true, the extra mirrored sketches and their corresponding pocket operations
    are generated and added to the data model.  This occurs when the right mouse menu option
    (generate male pocket) is manually chosen.  When the normal GCode generation process
    occurs, the 'keep_mirrored_sketches' flag is false so that only the female, male or
    both halves are generated.
 */
wxString CInlay::GenerateGCode( const CFixture *pFixture, const bool keep_mirrored_sketches )
{
	ReloadPointers();

	wxString gcode;
	CDepthOp::AppendTextToProgram( pFixture );

	unsigned int number_of_bad_sketches = 0;
	double tolerance = heeksCAD->GetTolerance();
	std::map<HeeksObj *, double> mirrored_sketches;
	CBox	bounding_box;

	CCuttingTool *pCuttingTool = CCuttingTool::Find( m_cutting_tool_number );
	if (! pCuttingTool)
	{
	    // No shirt, no shoes, no service.
		return(_T(""));
	}

	// NOTE: The determination of the 'max_depth_in_female_pass' assumes that the female
	// pass occurs BEFORE the male pass.
	std::list<CInlayParams::eInlayPass_t> passes;
	passes.push_back( CInlayParams::eFemale );	// Always.
	passes.push_back( CInlayParams::eMale );

    // Use the parameters to determine if we're going to mirror the selected
    // sketches around the X or Y axis.
	gp_Ax1 mirror_axis;
	if (m_params.m_mirror_axis == CInlayParams::eXAxis)
	{
        mirror_axis = gp_Ax1(gp_Pnt(0,0,0), gp_Dir(1,0,0));
	}
	else
	{
	    mirror_axis = gp_Ax1(gp_Pnt(0,0,0), gp_Dir(0,1,0));
	}

	// Record the maximum achieved depth (not the asking depth) for all operations in the female
	// pass of an inlay operation.  When we machine the male part, we will set this as the 'top surface'
	// height and machine from there down.  (NOTE: This is a relative value so a Z value of -5 and
	// a starting_depth of 0 would be assigned a depth value of '5'.
	double max_depth_in_female_pass = 0.0;

    // We need to run through the female pass even if we don't want its gcode output.  This pass generates
    // shapes and statistics that are required when generating the male pass.
	for (std::list<CInlayParams::eInlayPass_t>::const_iterator itPass = passes.begin(); itPass != passes.end(); itPass++)
	{
        // Required for the Contour operation's toolpath generation (which we will use here as well)
		CNCPoint last_position(0.0, 0.0, 0.0);

        // For all selected sketches.
		for (HeeksObj *object = GetFirstChild(); object != NULL; object = GetNextChild())
		{
		    // Convert them to a list of wire objects.
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

				if ((object->GetShortString() != NULL) && ((*itPass == m_params.m_pass) || (m_params.m_pass == CInlayParams::eBoth)))
				{
					gcode << _T("comment(") << PythonString(object->GetShortString()) << _T(")\n");
				}

				try {
				    // For all wires in this sketch...
					for(std::list<TopoDS_Shape>::iterator It2 = wires.begin(); It2 != wires.end(); It2++)
					{
						TopoDS_Shape& wire_to_fix = *It2;
						ShapeFix_Wire fix;
						fix.Load( TopoDS::Wire(wire_to_fix) );
						fix.FixReorder();

						TopoDS_Shape wire = fix.Wire();

                        // Rotate to align with the fixture.
						BRepBuilderAPI_Transform transform(pFixture->GetMatrix());
						transform.Perform(wire, false);
						wire = transform.Shape();

                        // If we're going to produce an inlay toolpath then we're going to need to
                        // create a pocket operation for the areas between the sketches passed in.
                        // (Actually, it will be based on the mirrored version of the sketch passed in)

                        TopoDS_Wire mirrored_wire( TopoDS::Wire(wire) );
                        gp_Trsf rotation;

                        rotation.SetRotation( mirror_axis, PI );
                        BRepBuilderAPI_Transform rotate(rotation);
                        rotate.Perform(mirrored_wire, false); // notice false as second parameter

                        HeeksObj *sketch = NULL;

                        if (*itPass == CInlayParams::eFemale)
                        {
                            // Create a sketch that represents the mirrored sketch and add it to a map.  The
                            // HeeksObj* pointer is the key while the second argument is the maximum depth
                            // achieved during the machining of the female pocket for this sketch.  We will need
                            // this value later when we need to machine to corresponding male half.
                            sketch = heeksCAD->NewSketch();
                            if (heeksCAD->ConvertWireToSketch(TopoDS::Wire(rotate.Shape()), sketch, heeksCAD->GetTolerance()))
                            {
                                for (int i=0; (heeksCAD->GetSketchOrder(sketch) != SketchOrderTypeCloseCCW) && (i<4); i++)
                                {
                                    // At least try to make them all consistently oriented.
                                    heeksCAD->ReOrderSketch( sketch, SketchOrderTypeCloseCCW );
                                } // End for

                                // mirrored_sketches.insert( std::make_pair(sketch, (m_depth_op_params.m_start_depth - m_depth_op_params.m_final_depth)));
                                mirrored_sketches.insert( std::make_pair(sketch, 0));
                            }
                        } // End if - then

						// For all the depths this DepthOp object is configured for...
						std::list<double> depths = GetDepths();

						// If we're machining the male half, we need to start from the deepest setting and move back towards
						// the top.  This doesn't make sense until you realise that the wire we're going to follow will have
						// been moved up the corresponding amount.  i.e. we are still going to machine from the top-down
						// but our depth values will need to be reversed here.
						if (*itPass == CInlayParams::eMale) depths.reverse();

                        // For each depth value (with respect to the m_depth_op_params.m_start_depth value).  Consider this
                        // to be the value 'down' for each female machining operation.  The rotation to produce the male
                        // half makes all this work out correctly.
						for (std::list<double>::iterator itDepth = depths.begin(); itDepth != depths.end(); itDepth++)
						{
						    // What radius does this tapered cutting tool have at this depth of cut.  This is how we can
						    // get right up to the lines in the corners during shallow cuts.
							double radius = pCuttingTool->CuttingRadius(false,m_depth_op_params.m_start_depth - *itDepth);

                            // By default, use the wire we generated earlier.
							TopoDS_Wire tool_path_wire = TopoDS::Wire(wire);

							if (radius > tolerance)
							{
                                try {
                                    // The radius is non-zero.  Try to create an offset wire that is 'radius * -1.0'
                                    // smaller than the original wire.  If this radius is too small, we will end up
                                    // throwing a Standard_Failure exception. In this case, we just assign an empty
                                    // wire and check for this later on.  If the offset shape can be generated, replace
                                    // the original wire with this generated one (our copy of it anyway) and generate
                                    // the toolpath using this instead.
                                    BRepOffsetAPI_MakeOffset offset_wire(TopoDS::Wire(wire));
                                    offset_wire.Perform(radius * -1.0);
                                    if (! offset_wire.IsDone())
                                    {
                                        // It never seems to get here but it sounded nice anyway.
                                        tool_path_wire = TopoDS_Wire(); // Empty wire.
                                    }
                                    else
                                    {
                                        // The offset shape was generated fine.  Use it.
                                        tool_path_wire = TopoDS::Wire(offset_wire.Shape());
                                    }
                                } // End try
                                catch (Standard_Failure & error) {
                                    (void) error;	// Avoid the compiler warning.
                                    Handle_Standard_Failure e = Standard_Failure::Caught();
                                    tool_path_wire = TopoDS_Wire(); // Empty wire.
                                } // End catch
							}

                            // Make sure the wire we ended up with has something inside it.  If it's empty then just
                            // move along to the next depth.
                            int num_edges = 0;
                            for(BRepTools_WireExplorer expEdge(TopoDS::Wire(tool_path_wire)); expEdge.More(); expEdge.Next())
                            {
                                num_edges++;
                            } // End for

							if ((radius > tolerance) && (num_edges > 0))
							{
								gp_Trsf matrix;

                                // Translate this wire down to the depth we're looking at right now.
								matrix.SetTranslation( gp_Vec( gp_Pnt(0,0,0), gp_Pnt( 0,0,*itDepth)));
								BRepBuilderAPI_Transform transform(matrix);
								transform.Perform(tool_path_wire, false); // notice false as second parameter
								tool_path_wire = TopoDS::Wire(transform.Shape());

								if (*itPass == CInlayParams::eMale)
								{
								    // It's the male half we're generating.  Rotate the wire around one
								    // of the two axes so that we end up machining the reverse of the
								    // female half.
									gp_Trsf rotation;

									rotation.SetRotation( mirror_axis, PI );
									BRepBuilderAPI_Transform rotate(rotation);
									rotate.Perform(tool_path_wire, false); // notice false as second parameter
									tool_path_wire = TopoDS::Wire(rotate.Shape());

                                    // And offset the wire 'down' so that the maximum depth reached during the
                                    // female half's processing ends up being at the 'top-most' surface of the
                                    // male half we're producing.
									gp_Trsf translation;
									translation.SetTranslation( gp_Vec( gp_Pnt(0,0,0), gp_Pnt( 0,0,-1.0 * max_depth_in_female_pass)));
									BRepBuilderAPI_Transform translate(translation);
									translate.Perform(tool_path_wire, false); // notice false as second parameter
									tool_path_wire = TopoDS::Wire(translate.Shape());
								}

								if (*itPass == CInlayParams::eFemale)
								{
									// We only want to remember those depths for which a tool path was actually
									// generated.  It's possible that the m_depth_op_params.m_final_depth value is
									// too deep (and hence too large a radius) for a chamfering tool to produce
									// a parallel shape.  For these operations, we don't actually generate a toolpath.
									if ((m_depth_op_params.m_start_depth - *itDepth) > max_depth_in_female_pass)
									{
									    // This is the deepest we went for all shapes.
										max_depth_in_female_pass = m_depth_op_params.m_start_depth - *itDepth;
									}

                                    // Remember which depth we ended up with for this sketch.
									if ((sketch != NULL) && ((m_depth_op_params.m_start_depth - *itDepth) > mirrored_sketches[sketch]))
									{
										mirrored_sketches[sketch] = (m_depth_op_params.m_start_depth - *itDepth);
									}
								}


                                if ((*itPass == m_params.m_pass) || (m_params.m_pass == CInlayParams::eBoth))
                                {
                                    // Actually generate the toolpath from this correctly positioned wire.  We use the
                                    // 'last_position' value to figure out whether we need to raise up and rapidly move
                                    // or just 'stay down' and feed through the material.  At this stage it's just
                                    // GCode that follows a wire so we're just going to use the Contour class's method
                                    // instead of having our own.
                                    gcode << CContour::GeneratePathFromWire(tool_path_wire,
                                                                            last_position,
                                                                            pFixture,
                                                                            m_depth_op_params.m_clearance_height,
                                                                            m_depth_op_params.m_rapid_down_to_height );
                                } // End if - then
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

		if ((*itPass == CInlayParams::eMale) && (keep_mirrored_sketches))
		{
			// Make sure the bounding box is one tool radius larger than all the sketches that
			// have been mirrored.  We need to create one large sketch with the bounding box
			// 'order' in one direction and all the mirrored sketch's orders in the other
			// direction.  We can then create a pocket operation to remove the material between
			// the mirrored sketches down to the inverted 'top surface' depth.

            double boarder_width = m_params.m_boarder_width;
            if ((CCuttingTool::Find(m_params.m_clearance_tool) != NULL) &&
                (boarder_width <= (2.0 * CCuttingTool::Find( m_params.m_clearance_tool)->CuttingRadius())))
            {
                boarder_width = (2.0 * CCuttingTool::Find( m_params.m_clearance_tool)->CuttingRadius());
                boarder_width += 1; // Make sure there really is room.  Add 1mm to be sure.
            }
			HeeksObj* bounding_sketch = heeksCAD->NewSketch();

			double start[3];
			double end[3];

			// left edge
			start[0] = bounding_box.MinX() - boarder_width;
            start[1] = bounding_box.MinY() - boarder_width;
			start[2] = bounding_box.MinZ();

			end[0] = bounding_box.MinX() - boarder_width;
			end[1] = bounding_box.MaxY() + boarder_width;
			end[2] = bounding_box.MinZ();

			bounding_sketch->Add( heeksCAD->NewLine( start, end ), NULL );

			// top edge
			start[0] = bounding_box.MinX() - boarder_width;
			start[1] = bounding_box.MaxY() + boarder_width;
			start[2] = bounding_box.MinZ();

			end[0] = bounding_box.MaxX() + boarder_width;
			end[1] = bounding_box.MaxY() + boarder_width;
			end[2] = bounding_box.MinZ();

			bounding_sketch->Add( heeksCAD->NewLine( start, end ), NULL );

			// right edge
			start[0] = bounding_box.MaxX() + boarder_width;
			start[1] = bounding_box.MaxY() + boarder_width;
			start[2] = bounding_box.MinZ();

			end[0] = bounding_box.MaxX() + boarder_width;
			end[1] = bounding_box.MinY() - boarder_width;
			end[2] = bounding_box.MinZ();

			bounding_sketch->Add( heeksCAD->NewLine( start, end ), NULL );

			// bottom edge
			start[0] = bounding_box.MaxX() + boarder_width;
			start[1] = bounding_box.MinY() - boarder_width;
			start[2] = bounding_box.MinZ();

			end[0] = bounding_box.MinX() - boarder_width;
			end[1] = bounding_box.MinY() - boarder_width;
			end[2] = bounding_box.MinZ();

			bounding_sketch->Add( heeksCAD->NewLine( start, end ), NULL );

            {
                // Mirror the bounding sketch about the nominated axis so that it surrounds the
                // mirrored sketches we've collected.
                gp_Trsf rotation;

                rotation.SetRotation( mirror_axis, PI );
                double m[16];	// A different form of the transformation matrix.
                CFixture::extract( rotation, m );
                bounding_sketch->ModifyByMatrix(m);
            }

            // Make sure this boarder sketch is oriented clockwise.  We will ensure the
            // enclosed sketches are oriented counter-clockwise so that the pocket operation
            // removes the intervening material.
            for (int i=0; (heeksCAD->GetSketchOrder(bounding_sketch) != SketchOrderTypeCloseCW) && (i<4); i++)
            {
                // At least try to make them all consistently oriented.
                heeksCAD->ReOrderSketch( bounding_sketch, SketchOrderTypeCloseCW );
            } // End for

			for (std::map<HeeksObj *,double>::iterator itObject = mirrored_sketches.begin(); itObject != mirrored_sketches.end(); itObject++)
			{
			    std::list<HeeksObj*> new_lines_and_arcs;
			    for (HeeksObj *child = itObject->first->GetFirstChild(); child != NULL; child = itObject->first->GetNextChild())
			    {
					new_lines_and_arcs.push_back(child);
				}

                ((ObjList *)bounding_sketch)->Add( new_lines_and_arcs );
			} // End for

            // This bounding_sketch is now in a form that is suitable for machining with a pocket operation.  We need
            // to reduce the areas between the mirrored sketches down to a level that will eventually mate with the
            // female section's top surface.

            // The pocket operation needs this sketch to be in the main branch so that we can refer to it by
            // ID.  Add it now.
            heeksCAD->Add( bounding_sketch, NULL );

            std::list<int> bounding_sketch_list;
            bounding_sketch_list.push_back( bounding_sketch->m_id );

            CPocket *bounding_sketch_pocket = new CPocket(bounding_sketch_list, m_params.m_clearance_tool);
            bounding_sketch_pocket->m_pocket_params.m_material_allowance = 0.0;
            bounding_sketch_pocket->m_depth_op_params = m_depth_op_params;
            bounding_sketch_pocket->m_depth_op_params.m_start_depth = 0.0;
            bounding_sketch_pocket->m_depth_op_params.m_final_depth = -1.0 * max_depth_in_female_pass;
            bounding_sketch_pocket->m_execution_order = m_execution_order - 1;  // Pocket before inlay
            theApp.m_program->Operations()->Add(bounding_sketch_pocket, NULL);

            // Look through the depths achieved for the individual sketches.  If any of them didn't make
            // the maximum depth then add a pocketing operation to remove material above their peaks
            // so that the chamfering bit can cut from the peak down to the base of the mountain.  The
            // reference depth is the 'max_depth_in_female_pass' as this represents the tallest
            // 'mountain' required in the male half.
            for (std::map<HeeksObj *, double>::iterator itMirroredSketch = mirrored_sketches.begin();
                    itMirroredSketch != mirrored_sketches.end(); itMirroredSketch++)
            {
                if ((itMirroredSketch->second > tolerance) && ((max_depth_in_female_pass - itMirroredSketch->second) > tolerance))
                {
                    // The pocket operation needs this sketch to be in the main branch so that we can refer to it by
                    // ID.  Add it now.
                    heeksCAD->Add( itMirroredSketch->first, NULL );

                    std::list<int> sketch_list;
                    sketch_list.push_back( itMirroredSketch->first->m_id );

                    CPocket *sketch_pocket = new CPocket(sketch_list, m_params.m_clearance_tool);
                    sketch_pocket->m_pocket_params.m_material_allowance = 0.0;
                    sketch_pocket->m_depth_op_params = m_depth_op_params;
                    sketch_pocket->m_depth_op_params.m_start_depth = 0.0;
                    sketch_pocket->m_depth_op_params.m_final_depth = -1.0 * (max_depth_in_female_pass - itMirroredSketch->second);
                    sketch_pocket->m_execution_order = m_execution_order - 1;  // Pocket before inlay
                    theApp.m_program->Operations()->Add(sketch_pocket, NULL);
                } // End if - then
            } // End for
		} // End if - then
	} // End for

	return(gcode);
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




// This tool uses a side-effect of the GenerateGCode() method to
// add the mirrored sketches as well as pocketing operations
// to the main tree.  It discards the GCode that is generated
// but the same process is used for both as the same calculations
// are required for both.
//
// We make the generation of these mirrored sketches and pocket
// operations a manual step so that we don't end up with duplicate
// operations in the model.  It also allows the user to control
// the setup and machining of the male half more easily.
class Inlay_GenerateMalePocket: public Tool
{
CInlay *m_pThis;

public:
	Inlay_GenerateMalePocket() { m_pThis = NULL; }

	// Tool's virtual functions
	const wxChar* GetTitle(){return _("Generate Male Pocket");}

	void Run()
	{
	    CFixture fixture( NULL, CFixture::G54 );
	    m_pThis->GenerateGCode( &fixture, true );
	}
	wxString BitmapPath(){ return _T("opinlay");}
	void Set( CInlay *pThis ) { m_pThis = pThis; }
};

static Inlay_GenerateMalePocket generate_male_pocket;

void CInlay::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
    generate_male_pocket.Set( this );

	t_list->push_back( &generate_male_pocket );

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


