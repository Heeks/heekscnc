
#include "stdafx.h"
#ifndef STABLE_OPS_ONLY

#include "Chamfer.h"
#include "interface/PropertyLength.h"
#include "CNCPoint.h"
#include "Drilling.h"
#include "CounterBore.h"
#include "ProgramCanvas.h"
#include "MachineState.h"
#include "CNCConfig.h"
#include "Program.h"
#include "Profile.h"
#include "Contour.h"
#include "Inlay.h"

#include <BRepOffsetAPI_MakeOffset.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Shape.hxx>
#include <BRepTools_WireExplorer.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <gp_Circ.hxx>
#include <ShapeFix_Wire.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRep_Tool.hxx>
#include <BRepTools.hxx>
#include <BRepMesh.hxx>
#include <Poly_Polygon3D.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <Handle_BRepAdaptor_HCurve.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <Adaptor3d_HCurve.hxx>
#include <Adaptor3d_Curve.hxx>

void CChamferParams::set_initial_values()
{
	CNCConfig config(ConfigScope());

	config.Read(_T("m_chamfer_width"), &m_chamfer_width, 1.0);	// 1.0 mm
	config.Read(_T("ToolOnSide"), (int *) &m_tool_on_side, (int) CChamferParams::eLeftOrOutside);
}

void CChamferParams::write_values_to_config()
{
	// We always want to store the parameters in mm and convert them back later on.

	CNCConfig config(ConfigScope());

	// These values are in mm.
	config.Write(_T("m_chamfer_width"), m_chamfer_width);
	config.Write(_T("ToolOnSide"), (int) m_tool_on_side);
}

static void on_set_chamfer_width(double value, HeeksObj* object)
{
	((CChamfer*)object)->m_params.m_chamfer_width = value;
	((CChamfer*)object)->m_params.write_values_to_config();
}


static void on_set_tool_on_side(int value, HeeksObj* object){
	switch(value)
	{
	case 0:
		((CChamfer*)object)->m_params.m_tool_on_side = CChamferParams::eLeftOrOutside;
		break;
	case 1:
		((CChamfer*)object)->m_params.m_tool_on_side = CChamferParams::eRightOrInside;
		break;
	default:
		((CChamfer*)object)->m_params.m_tool_on_side = CChamferParams::eOn;
		break;
	}
	((CChamfer*)object)->m_params.write_values_to_config();
}

void CChamferParams::GetProperties(CChamfer * parent, std::list<Property *> *list)
{
	list->push_back(new PropertyLength(_("Chamfer Width"), m_chamfer_width, parent, on_set_chamfer_width));

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

				choices.push_back(_("On"));

				int choice = int(CChamferParams::eOn);
				switch (parent->m_params.m_tool_on_side)
				{
					case CChamferParams::eRightOrInside:	choice = 1;
							break;

					case CChamferParams::eOn:	choice = 2;
							break;

					case CChamferParams::eLeftOrOutside:	choice = 0;
							break;
				} // End switch

				list->push_back(new PropertyChoice(_("tool on side"), choices, choice, parent, on_set_tool_on_side));
            }
        }
    }
}

void CChamferParams::WriteXMLAttributes(TiXmlNode *root)
{
	TiXmlElement * element;
	element = heeksCAD->NewXMLElement( "params" );
	heeksCAD->LinkXMLEndChild( root,  element );

	element->SetDoubleAttribute( "m_chamfer_width", m_chamfer_width);
	element->SetAttribute( "side", m_tool_on_side);
}

void CChamferParams::ReadParametersFromXMLElement(TiXmlElement* pElem)
{
	if (pElem->Attribute("m_chamfer_width")) pElem->Attribute("m_chamfer_width", &m_chamfer_width);
	if(pElem->Attribute("side")) pElem->Attribute("side", (int *) &m_tool_on_side);
}


CChamfer::CChamfer(	const Symbols_t &symbols,
			const int tool_number )
		: CDepthOp(GetTypeString(), NULL, tool_number, ChamferType), m_symbols(symbols)
{
    for (Symbols_t::iterator symbol = m_symbols.begin(); symbol != m_symbols.end(); symbol++)
    {
        HeeksObj *object = heeksCAD->GetIDObject( symbol->first, symbol->second );
        if (object != NULL)
        {
            if (CanAdd(object)) Add(object,NULL);
        } // End if - then
    } // End for
    m_symbols.clear();
    m_params.set_initial_values();
}

CChamfer::CChamfer( const CChamfer & rhs ) : CDepthOp(rhs)
{
	std::copy( rhs.m_symbols.begin(), rhs.m_symbols.end(), std::inserter( m_symbols, m_symbols.begin() ) );
}

CChamfer & CChamfer::operator= ( const CChamfer & rhs )
{
	if (this != &rhs)
	{
		m_symbols.clear();
		std::copy( rhs.m_symbols.begin(), rhs.m_symbols.end(), std::inserter( m_symbols, m_symbols.begin() ) );
		CDepthOp::operator=(rhs);
	}

	return(*this);
}

const wxBitmap &CChamfer::GetIcon()
{
	if(!m_active)return GetInactiveIcon();
	static wxBitmap* icon = NULL;
	if(icon == NULL)icon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/chamfmill.png")));
	return *icon;
}

void CChamfer::glCommands(bool select, bool marked, bool no_color)
{
	CDepthOp::glCommands( select, marked, no_color );
}

HeeksObj *CChamfer::MakeACopy(void)const
{
	CChamfer *new_object = new CChamfer(*this);
	return(new_object);
}

void CChamfer::CopyFrom(const HeeksObj* object)
{
	if (object->GetType() == ChamferType)
	{
		*this = *((CChamfer *) object);
	}
}


bool CChamfer::CanAddTo(HeeksObj* owner)
{
	return((owner != NULL) && (owner->GetType() == OperationsType));
}

bool CChamfer::CanAdd(HeeksObj* object)
{
    if (object == NULL) return(false);

	switch (object->GetType())
	{
		case ProfileType:
		case PocketType:
		case ContourType:
		case CounterBoreType:
		case DrillingType:
		case FixtureType:
		case SketchType:
			return(true);

		default:
			return(false);
	} // End switch
}

void CChamfer::GetProperties(std::list<Property *> *list)
{
	m_params.GetProperties( this, list );
	CDepthOp::GetProperties(list);
}


void CChamfer::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
	CDepthOp::GetTools( t_list, p );
}


static double drawing_units( const double value )
{
	return(value / theApp.m_program->m_units);
}

Python CChamfer::AppendTextForCircularChildren(
	CMachineState *pMachineState,
	const double theta,
	HeeksObj *child,
	CTool *pChamferingBit )
{
	Python python;

	// See what the maximum possible depth is for this chamfering bit.  We want to figure
	// out whether we can cut with the middle part of the chamfering bit rather than
	// cutting using the very tip (we don't want to break it off).  In fact, we should
	// really cut with the top-most cutting edge so that it's as strong as it can be.  This
	// depends on the area available for fitting the chamfering bit.

	double min_chamfer_diameter = pChamferingBit->m_params.m_flat_radius * 2.0;
	double stand_off = m_depth_op_params.m_rapid_safety_space;
	double clearance_height = m_depth_op_params.ClearanceHeight();

	Circles_t circles;

	if (child->GetType() == DrillingType)
	{
		// Get the size of the drilled holes.  We need to know whether we need to just plunge
		// the chamfering bit directly down into the hole or whether we need to run
		// around the edge.

		CDrilling *pDrilling = (CDrilling *) child;
		CTool *pDrillBit = CTool::Find( pDrilling->m_tool_number );
		if (pDrillBit == NULL)
		{
			// It's difficult to drill a hole without a drill bit but apparently this file does.
			printf("Ignoring drilling operation (id=%d) with no  tool defined\n", pDrilling->m_id );
			return(python);	// Empty.
		}

        stand_off = pDrilling->m_params.m_standoff;
        clearance_height = pDrilling->m_params.ClearanceHeight();

		double hole_diameter = pDrillBit->CuttingRadius(false) * 2.0;

		if (hole_diameter < min_chamfer_diameter)
		{
			printf("Ignoring chamfer for drilled hole due to geometry of selected chamfering bit\n");
			return(python);	// Empty.
		}

		std::vector<CNCPoint> locations = CDrilling::FindAllLocations(pDrilling, pMachineState->Location(), true, NULL);
        for (std::vector<CNCPoint>::const_iterator l_itLocation = locations.begin(); l_itLocation != locations.end(); l_itLocation++)
		{
			CNCPoint point = pMachineState->Fixture().Adjustment( *l_itLocation );
			circles.push_back( Circle( point, hole_diameter, pDrilling->m_params.m_depth ) );
		} // End for
	} // End if - then

	if (child->GetType() == CounterBoreType)
	{
		CCounterBore *pCounterBore = ((CCounterBore *) child);

		stand_off = pCounterBore->m_depth_op_params.m_rapid_safety_space;
        clearance_height = pCounterBore->m_depth_op_params.ClearanceHeight();

		std::vector<CNCPoint> locations = CDrilling::FindAllLocations(pCounterBore, pMachineState->Location(), pCounterBore->m_params.m_sort_locations != 0, NULL);
		for (std::vector<CNCPoint>::const_iterator l_itLocation = locations.begin(); l_itLocation != locations.end(); l_itLocation++)
		{
			CNCPoint point = pMachineState->Fixture().Adjustment( *l_itLocation );
			double max_depth = pCounterBore->m_depth_op_params.m_start_depth - pCounterBore->m_depth_op_params.m_final_depth;
			circles.push_back( Circle( point, pCounterBore->m_params.m_diameter, max_depth ) );
		} // End for
	}

	for (Circles_t::iterator l_itCircle = circles.begin(); l_itCircle != circles.end(); l_itCircle++)
	{
		// We want to select a depth such that we're cutting with the top-most part of the chamfering
		// bit as that is the strongest part.  We don't want to break off the tip unless we really can't
		// get into the hole without doing so.

		double max_hole_depth = l_itCircle->MaxDepth();
		double max_bit_plunge_depth = pChamferingBit->m_params.m_cutting_edge_height * cos(theta);

		double min_bit_radius = pChamferingBit->m_params.m_flat_radius;

		double hole_radius = l_itCircle->Diameter() / 2.0;
		double required_bit_plunge_depth =  (m_params.m_chamfer_width * cos( theta ));

		if ((required_bit_plunge_depth >= max_hole_depth) ||
			(required_bit_plunge_depth >= max_bit_plunge_depth) ||
			(hole_radius < min_bit_radius))
		{
			// It's too deep for one pass.
			return(python);	// Empty.
		}

		double plunge_depth = (max_hole_depth<=max_bit_plunge_depth)?max_hole_depth:max_bit_plunge_depth;
		double bit_radius_at_plunge_depth = pChamferingBit->m_params.m_flat_radius + (plunge_depth / tan(theta));

		// This is the gap between the bit and the hole when the bit's bottom is at the top surface.
		double gap_radius = hole_radius - min_bit_radius;

		// We need to figure out how far down to move before this gap is closed by the slope of the cutting edge.
		double gap_closure_depth = gap_radius / tan(theta);

		if ( hole_radius <= bit_radius_at_plunge_depth )
		{
			// We can plunge straight down at the hole's location.

			// If the chamfering bit is at the top of the hole then the diameter of
			// cut is equal to the flat radius.  How far should we plunge down before
			// the edge of the chamfering bit touches the top of the hole?

            CNCPoint point(l_itCircle->Location());

			python << _T("drill(")
				<< _T("x=") << point.X(true) << _T(", ")
				<< _T("y=") << point.Y(true) << _T(", ")
				<< _T("z=") << drawing_units(point.Z(false) - gap_closure_depth) << _T(", ")
				<< _T("depth=") << drawing_units(required_bit_plunge_depth) << _T(", ")
				<< _T("standoff=") << drawing_units(stand_off) << _T(", ")
				<< _T("dwell=") << 0.0 << _T(", ")
				<< _T("peck_depth=") << 0.0 << _T(", ")
                << _T("clearance_height=") << drawing_units(clearance_height)
				<< _T(")\n");
		}
		else
		{
			// We will have to run around the edge of the large hole.  Figure out the offset
			// in from the edge and generate the corresponding tool path.

			CNCPoint centre(l_itCircle->Location());
			CNCPoint point(l_itCircle->Location());

			double radius_of_spiral = hole_radius - bit_radius_at_plunge_depth + (m_params.m_chamfer_width * sin(theta));

			python << _T("rapid( x=") << centre.X(true) << _T(", ")
						<< _T("y=") << centre.Y(true) << _T(", ")
						<< _T("z=") << drawing_units(clearance_height) << _T(")\n");

            python << _T("rapid(z=") << drawing_units(stand_off) << _T(")\n");

			double cutting_depth = point.Z(false) - plunge_depth;

			// Move to 12 O'Clock.
			python << _T("feed( x=") << centre.X(true) << _T(", ")
						_T("y=") << drawing_units(centre.Y(false) + radius_of_spiral) << _T(", ")
						_T("z=") << drawing_units(cutting_depth) << _T(")\n");
			point.SetX( centre.X(false) );
			point.SetY( centre.Y(false) + radius_of_spiral );

			// First quadrant (12 O'Clock to 9 O'Clock)
			python << _T("arc_ccw( x=") << drawing_units(centre.X(false) - radius_of_spiral) << _T(", ") <<
						_T("y=") << centre.Y(true) << _T(", ") <<
						_T("z=") << drawing_units(cutting_depth) << _T(", ") <<	// full depth
						_T("i=") << drawing_units(centre.X(false)) << _T(", ") <<
						_T("j=") << drawing_units(centre.Y(false)) << _T(")\n");
			point.SetX( centre.X(false) - radius_of_spiral );
			point.SetY( centre.Y(false) );

			// Second quadrant (9 O'Clock to 6 O'Clock)
			python << _T("arc_ccw( x=") << centre.X(true) << _T(", ") <<
						_T("y=") << drawing_units(centre.Y(false) - radius_of_spiral) << _T(", ") <<
						_T("z=") << drawing_units(cutting_depth) << _T(", ") <<	// full depth now
						_T("i=") << drawing_units(centre.X(false)) << _T(", ") <<
						_T("j=") << drawing_units(centre.Y(false)) << _T(")\n");
			point.SetX( centre.X(false) );
			point.SetY( centre.Y(false) - radius_of_spiral );

			// Third quadrant (6 O'Clock to 3 O'Clock)
			python << _T("arc_ccw( x=") << drawing_units(centre.X(false) + radius_of_spiral) << _T(", ") <<
						_T("y=") << centre.Y(true) << _T(", ") <<
						_T("z=") << drawing_units(cutting_depth) << _T(", ") <<	// full depth now
						_T("i=") << drawing_units(centre.X(false)) << _T(", ") <<
						_T("j=") << drawing_units(centre.Y(false)) << _T(")\n");
			point.SetX( centre.X(false) + radius_of_spiral );
			point.SetY( centre.Y(false) );

			// Fourth quadrant (3 O'Clock to 12 O'Clock)
			python << _T("arc_ccw( x=") << centre.X(true) << _T(", ") <<
						_T("y=") << drawing_units(centre.Y(false) + radius_of_spiral) << _T(", ") <<
						_T("z=") << drawing_units(cutting_depth) << _T(", ") <<	// full depth now
						_T("i=") << drawing_units(centre.X(false)) << _T(", ") <<
						_T("j=") << drawing_units(centre.Y(false)) << _T(")\n");
			point.SetX( centre.X(false) );
			point.SetY( centre.Y(false) + radius_of_spiral );

			python << _T("rapid( z=") << drawing_units(m_depth_op_params.ClearanceHeight()) << _T(")\n");
		}
	} // End for

	return(python);
}


/**
	Figure out what depths we want to apply for the child profile (or contour) operation.
	We need to go as deep as either the profile did or as the chamfering bit will allow.
	We also need to allow for the chamfering width when calculating this.

	The depth we add here should NOT be to cut into the material but to just touch
	the outside corner.  We will move it left or right enough to cut into the material
	in later processing.  This seems confusing but we can either go deeper to cut into the
	material or move the chamfering bit left or right to do so.  It depends on the
	geometry of the tool used to cut the profile, the tool used to chamfer and how deep
	the profile is.  To make it more simple, let this routine figure out the maximum
	depth we can go while still cutting with the chamfering bit and not hitting the bottom
	or the edges of the profile (or contour).
 */
std::list<double> CChamfer::GetProfileChamferingDepths(HeeksObj *child) const
{
    std::list<double> depths;

	double child_depth = 0.0;
	double child_start_depth = 0.0;
	double child_final_depth = 0.0;

	CTool *pChamferingBit = CTool::Find(m_tool_number);
	if (pChamferingBit == NULL)
	{
		return(depths);	// Empty.
	}

	switch (child->GetType())
	{
	case ProfileType:
	case ContourType:
	case PocketType:
	case SketchType:
		break;

	default:
		return(depths);	// Empty.
	} // End switch

	CDepthOp *pDepthOp = dynamic_cast<CDepthOp *>(child);
	if (pDepthOp == NULL)
	{
		if (child->GetType() == SketchType)
		{
			// It's a sketch child.  Assume that the area around the sketch is big enough to fit the
			// chamfering bit.

			depths.push_back( m_depth_op_params.m_start_depth - pChamferingBit->m_params.m_cutting_edge_height );
			return(depths);
		}
		else
		{
			// It's not a sketch either.  We can't handle this.
			return(depths);	// empty.
		}
	}
	else
	{
		// It's a CDepthOp based operation. (contour, profile or pocket)
		child_start_depth = pDepthOp->m_depth_op_params.m_start_depth;
		child_final_depth = pDepthOp->m_depth_op_params.m_final_depth;
		child_depth = child_start_depth - child_final_depth;

		CTool *pProfileTool = CTool::Find(pDepthOp->m_tool_number);
		if (pProfileTool == NULL)
		{
			return(depths);	// empty.
		}

		if (pChamferingBit->m_params.m_diameter < pProfileTool->m_params.m_diameter)
		{
			// The chamfering bit fits between the walls formed by the profile's bit.
			// Set the depth so that we're either at the chamfering bit's greatest depth
			// or the profile's greatest depth (whichever is less)
			// NOTE: We don't want to go more than 90% of the profile's depth.  We don't
			// want to drag on the bottom.

			if (pChamferingBit->m_params.m_cutting_edge_height < (0.9 * child_depth))
			{
				depths.push_back( pDepthOp->m_depth_op_params.m_start_depth - pChamferingBit->m_params.m_cutting_edge_height );
				return(depths);
			}
			else
			{
				depths.push_back( pDepthOp->m_depth_op_params.m_start_depth - (child_depth * 0.9) );
				return(depths);
			}
		}
		else
		{
			// The chamfering bit is larger than the profile's width.  We need to figure out
			// how deep we can go before the chamfering bit just touches the edges of the
			// profile.

			double theta = pChamferingBit->m_params.m_cutting_edge_angle / 360.0 * 2.0 * PI;
			double radius = pProfileTool->m_params.m_diameter / 2.0;
			double depth = radius / tan(theta);
			depths.push_back( pDepthOp->m_depth_op_params.m_start_depth - depth );
			return(depths);
		}
	} // End if - else
}



/**
	We can see what diameter tool was used to create the pocket or contour child operation.  We assume
	that this has already occured.  We want to fit down into the slot cut by these operations as far
	as possible but only touching the appropriate side of the material (inside or outside).
	We also need to make sure we don't go too deep.  If the chamfering bit is small while the endmill
	used to cut the profile or contour was large then we may need to limit the depth of cut.
 */
Python CChamfer::AppendTextForProfileChildren(
	CMachineState *pMachineState,
	const double theta,
	HeeksObj *child,
	CTool *pChamferingBit )
{
	Python python;

	unsigned int number_of_bad_sketches = 0;
	double tolerance = heeksCAD->GetTolerance();

	double start_depth = 0.0;
	double final_depth = 0.0;
	double clearance_height = 0.0;
	double rapid_safety_space = 0.0;
	std::list<HeeksObj *> sketches;

	CDepthOp *pDepthOp = dynamic_cast<CDepthOp *>(child);
	if (pDepthOp == NULL)
	{
		start_depth = m_depth_op_params.m_start_depth;
		final_depth = m_depth_op_params.m_final_depth;
		clearance_height = m_depth_op_params.ClearanceHeight();
		rapid_safety_space = m_depth_op_params.m_rapid_safety_space;

		if (child->GetType() == SketchType)
		{
			sketches.push_back(child);
		}
	}
	else
	{
		start_depth = pDepthOp->m_depth_op_params.m_start_depth;
		final_depth = pDepthOp->m_depth_op_params.m_final_depth;
		clearance_height = pDepthOp->m_depth_op_params.ClearanceHeight();
		rapid_safety_space = pDepthOp->m_depth_op_params.m_rapid_safety_space;

		for (HeeksObj *object = child->GetFirstChild(); object != NULL; object = child->GetNextChild())
		{
			if (object->GetType() == SketchType)
			{
				sketches.push_back(object);
			}
		}		
	}

	

    // for (HeeksObj *object = child->GetFirstChild(); object != NULL; object = child->GetNextChild())
	for (std::list<HeeksObj *>::iterator itChild = sketches.begin(); itChild != sketches.end(); itChild++)
    {
		HeeksObj *object = *itChild;

        std::list<TopoDS_Shape> wires;
        if (! heeksCAD->ConvertSketchToFaceOrWire( object, wires, false))
        {
            number_of_bad_sketches++;
        } // End if - then
        else
        {
            // The wire(s) represent the sketch objects for a tool path.
            if (object->GetShortString() != NULL)
            {
				wxString comment;
				comment << _T("Chamfering of ") << object->GetShortString();
                python << _T("comment(") << PythonString(comment).c_str() << _T(")\n");
            }

            try {
                for(std::list<TopoDS_Shape>::iterator It2 = wires.begin(); It2 != wires.end(); It2++)
                {
                    TopoDS_Shape& wire_to_fix = *It2;
                    ShapeFix_Wire fix;
                    fix.Load( TopoDS::Wire(wire_to_fix) );
                    fix.FixReorder();

                    TopoDS_Shape wire = fix.Wire();

                    BRepBuilderAPI_Transform transform1(pMachineState->Fixture().GetMatrix(CFixture::YZ));
                    transform1.Perform(wire, false);
                    wire = transform1.Shape();

                    BRepBuilderAPI_Transform transform2(pMachineState->Fixture().GetMatrix(CFixture::XZ));
                    transform2.Perform(wire, false);
                    wire = transform2.Shape();

                    BRepBuilderAPI_Transform transform3(pMachineState->Fixture().GetMatrix(CFixture::XY));
                    transform3.Perform(wire, false);
                    wire = transform3.Shape();

                    BRepOffsetAPI_MakeOffset offset_wire(TopoDS::Wire(wire));

                    // Now generate a toolpath along this wire.
                    std::list<double> depths = GetProfileChamferingDepths(child);

                    for (std::list<double>::iterator itDepth = depths.begin(); itDepth != depths.end(); itDepth++)
                    {
                        double radius = pChamferingBit->CuttingRadius(false,fabs(*itDepth - start_depth));

						// We know what offset we'd really like.  See how far we can offset the shape before we start
                        // getting cross-over of graphics.
                        double max_offset = CInlay::FindMaxOffset( radius, TopoDS::Wire(wire), radius / 10.0 );

						if (radius > max_offset) radius = max_offset;

						// Now move the tool slightly less than this offset so that the chamfering width is
						// produced.
						double theta = pChamferingBit->m_params.m_cutting_edge_angle / 360.0 * 2.0 * PI;
						radius -= this->m_params.m_chamfer_width * sin(theta);

						switch (child->GetType())
						{
						case ProfileType:
							if (((CProfile *) child)->m_profile_params.m_tool_on_side == CProfileParams::eLeftOrOutside) radius *= +1.0;
							if (((CProfile *) child)->m_profile_params.m_tool_on_side == CProfileParams::eRightOrInside) radius *= -1.0;
							if (((CProfile *) child)->m_profile_params.m_tool_on_side == CProfileParams::eOn) radius *= +1.0;
							break;

						case ContourType:
							if (((CContour *) child)->m_params.m_tool_on_side == CContourParams::eLeftOrOutside) radius *= +1.0;
							if (((CContour *) child)->m_params.m_tool_on_side == CContourParams::eRightOrInside) radius *= -1.0;
							if (((CContour *) child)->m_params.m_tool_on_side == CContourParams::eOn) radius *= +1.0;
							break;

						case PocketType:
							radius *= -1.0;
							break;

						default:
							if (m_params.m_tool_on_side == CContourParams::eLeftOrOutside) radius *= +1.0;
							if (m_params.m_tool_on_side == CContourParams::eRightOrInside) radius *= -1.0;
							if (m_params.m_tool_on_side == CContourParams::eOn) radius *= +1.0;
							break;
						} // End switch

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

                        if (offset > tolerance)
                        {
                            gp_Trsf matrix;

                            matrix.SetTranslation( gp_Vec( gp_Pnt(0,0,0), gp_Pnt( 0,0,*itDepth)));
                            BRepBuilderAPI_Transform transform(matrix);
                            transform.Perform(tool_path_wire, false); // notice false as second parameter
                            tool_path_wire = TopoDS::Wire(transform.Shape());

							python << CContour::GeneratePathFromWire(	tool_path_wire,
                                                            pMachineState,
                                                            clearance_height,
                                                            rapid_safety_space,
                                                            start_depth,
															CContourParams::ePlunge );
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

    if (pMachineState->Location().Z() < (m_depth_op_params.ClearanceHeight() / theApp.m_program->m_units))
    {
        // Move up above workpiece to relocate to the start of the next operation.
        python << _T("rapid(z=") << m_depth_op_params.ClearanceHeight() / theApp.m_program->m_units << _T(")\n");

        CNCPoint where(pMachineState->Location());
        where.SetZ(m_depth_op_params.ClearanceHeight() / theApp.m_program->m_units);
        pMachineState->Location(where);
    }

    if (number_of_bad_sketches > 0)
    {
        wxString message;
        message << _("Failed to create contours around ") << number_of_bad_sketches << _(" sketches");
        wxMessageBox(message);
    }

	return(python);
}




Python CChamfer::AppendTextToProgram(CMachineState *pMachineState)
{
	Python python;

	// Look at the child operations objects and generate a toolpath as appropriate.
	python << CDepthOp::AppendTextToProgram( pMachineState );

	// Whatever underlying sharp edges we're going to cleanup, we need to know how deep
	// we need to plunge the chamfering bit into the work before we can get the chamfering
	// width required.
	// NOTE: At this stage, we won't allow multiple passes to produce an overly wide
	// chamfer.  i.e. if the chamfering width is longer than the chamfering bit's cutting
	// edge length then we're out of business.

	CTool *pChamferingBit = CTool::Find( m_tool_number );
	if (pChamferingBit == NULL)
	{
		// No socks, no shirt, no service.
		return(python);
	} // End if - then

	if (pChamferingBit->m_params.m_type != CToolParams::eChamfer)
	{
		// We need to make the various radius and angle calculations based on the
		// assumption that it's a chamfering bit.  If not, we can't handle the
		// mathematics (at least I can't).

		printf("Only chamfering bits are supported for chamfer operations\n");
		return(python);
	}

	if (m_params.m_chamfer_width > pChamferingBit->m_params.m_cutting_edge_height)
	{
		// We don't support multiple passes to chamfer the edge (yet).  Just don't try.
		printf("Chamfer width %lf is too large for a single pass of the chamfering bit's edge (%lf)\n",
				m_params.m_chamfer_width / theApp.m_program->m_units,
				pChamferingBit->m_params.m_cutting_edge_height / theApp.m_program->m_units );
		return(python);
	}

	// How deep do we have to plunge in order to cut this width of chamfer?
	double theta = pChamferingBit->m_params.m_cutting_edge_angle / 360.0 * 2.0 * PI;	// in radians.




	for (HeeksObj *child = GetFirstChild(); child != NULL; child = GetNextChild())
	{
		switch (child->GetType())
		{
        case DrillingType:
		case CounterBoreType:
			python << AppendTextForCircularChildren(pMachineState, theta, child, pChamferingBit);
			break;

		case ProfileType:
		case ContourType:
		case PocketType:
		case SketchType:
			python << AppendTextForProfileChildren(pMachineState, theta, child, pChamferingBit);
			break;

		default:
			break;
		} // End switch
	} // End for

	return(python);
}

void CChamfer::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element = heeksCAD->NewXMLElement( "Chamfer" );
	heeksCAD->LinkXMLEndChild( root,  element );

	m_params.WriteXMLAttributes(element);

	WriteBaseXML(element);
}

// static member function
HeeksObj* CChamfer::ReadFromXMLElement(TiXmlElement* element)
{
	CChamfer* new_object = new CChamfer;

	std::list<TiXmlElement *> elements_to_remove;

	// read point and circle ids
	for(TiXmlElement* pElem = heeksCAD->FirstXMLChildElement( element ) ; pElem; pElem = pElem->NextSiblingElement())
	{
		std::string name(pElem->Value());
		if(name == "params"){
			new_object->m_params.ReadParametersFromXMLElement(pElem);
			elements_to_remove.push_back(pElem);
		}
		else if(name == "symbols"){
			for(TiXmlElement* child = heeksCAD->FirstXMLChildElement( pElem ) ; child; child = child->NextSiblingElement())
			{
				if (child->Attribute("type") && child->Attribute("id"))
				{
					new_object->AddSymbol( atoi(child->Attribute("type")), atoi(child->Attribute("id")) );

					// We need to convert these type/id pairs into HeeksObj pointers but we want them to
					// come from the right source.  If we're importing data then they need to come from the
					// data we're importing.  If we're updating the main model then the main tree
					// will do.  We don't want to just use heeksCAD->GetIDObject() here as it will always
					// look in the main tree.  Perhaps we can force a recursive 'ReloadPointers()' call
					// so that these values are reset when necessary.  Eventually we will be storing the
					// child elements as real XML elements rather than just references.  Until time passes
					// a little longer, we need to support this type/id version.  Otherwise old HeeksCNC files
					// won't read in correctly.
				}
			} // End for
			elements_to_remove.push_back(pElem);
		} // End if
	}

	for (std::list<TiXmlElement*>::iterator itElem = elements_to_remove.begin(); itElem != elements_to_remove.end(); itElem++)
	{
		heeksCAD->RemoveXMLChild( element, *itElem);
	}

	new_object->ReadBaseXML(element);

	return new_object;
}


/**
	The old version of the CChamfer object stored references to graphics as type/id pairs
	that get read into the m_symbols list.  The new version stores these graphics references
	as child elements (based on ObjList).  If we read in an old-format file then the m_symbols
	list will have data in it for which we don't have children.  This routine converts
	these type/id pairs into the HeeksObj pointers as children.
 */
void CChamfer::ReloadPointers()
{
	for (Symbols_t::iterator symbol = m_symbols.begin(); symbol != m_symbols.end(); symbol++)
	{
		HeeksObj *object = heeksCAD->GetIDObject( symbol->first, symbol->second );
		if (object != NULL)
		{
			Add( object, NULL );
		}
	}

	m_symbols.clear();	// We don't want to convert them twice.

	CDepthOp::ReloadPointers();
}

bool CChamfer::operator== ( const CChamfer & rhs ) const
{
	if (m_params != rhs.m_params) return(false);

	return(CDepthOp::operator==(rhs));
}

#endif