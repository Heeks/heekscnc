
#include "stdafx.h"
#include "Chamfer.h"
#include "interface/PropertyLength.h"
#include "CNCPoint.h"
#include "Drilling.h"
#include "CounterBore.h"
#include "ProgramCanvas.h"
#include "MachineState.h"

void CChamferParams::set_initial_values()
{
	CNCConfig config(ConfigScope());

	config.Read(_T("m_chamfer_width"), &m_chamfer_width, 1.0);	// 1.0 mm
}

void CChamferParams::write_values_to_config()
{
	// We always want to store the parameters in mm and convert them back later on.

	CNCConfig config(ConfigScope());

	// These values are in mm.
	config.Write(_T("m_chamfer_width"), m_chamfer_width);
}

static void on_set_chamfer_width(double value, HeeksObj* object)
{
	((CChamfer*)object)->m_params.m_chamfer_width = value;
	((CChamfer*)object)->m_params.write_values_to_config();
}


void CChamferParams::GetProperties(CChamfer * parent, std::list<Property *> *list)
{
	list->push_back(new PropertyLength(_("Chamfer Width"), m_chamfer_width, parent, on_set_chamfer_width));
}

void CChamferParams::WriteXMLAttributes(TiXmlNode *root)
{
	TiXmlElement * element;
	element = new TiXmlElement( "params" );
	root->LinkEndChild( element );

	element->SetDoubleAttribute("m_chamfer_width", m_chamfer_width);
}

void CChamferParams::ReadParametersFromXMLElement(TiXmlElement* pElem)
{
	if (pElem->Attribute("m_chamfer_width")) pElem->Attribute("m_chamfer_width", &m_chamfer_width);
}


CChamfer::CChamfer(	const Symbols_t &symbols,
			const int cutting_tool_number )
		: CDepthOp(GetTypeString(), NULL, cutting_tool_number, ChamferType), m_symbols(symbols)
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
		case PocketType:
		case CounterBoreType:
		case DrillingType:
		case FixtureType:
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

	CCuttingTool *pChamferingBit = CCuttingTool::Find( m_cutting_tool_number );
	if (pChamferingBit == NULL)
	{
		// No socks, no shirt, no service.
		return(python);
	} // End if - then

	if (pChamferingBit->m_params.m_type != CCuttingToolParams::eChamfer)
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
		// See what the maximum possible depth is for this chamfering bit.  We want to figure
		// out whether we can cut with the middle part of the chamfering bit rather than
		// cutting using the very tip (we don't want to break it off).  In fact, we should
		// really cut with the top-most cutting edge so that it's as strong as it can be.  This
		// depends on the area available for fitting the chamfering bit.

		double min_chamfer_diameter = pChamferingBit->m_params.m_flat_radius * 2.0;

		Circles_t circles;

		if (child->GetType() == DrillingType)
		{
			// Get the size of the drilled holes.  We need to know whether we need to just plunge
			// the chamfering bit directly down into the hole or whether we need to run
			// around the edge.

			CDrilling *pDrilling = (CDrilling *) child;
			CCuttingTool *pDrillBit = CCuttingTool::Find( pDrilling->m_cutting_tool_number );
			if (pDrillBit == NULL)
			{
				// It's difficult to drill a hole without a drill bit but apparently this file does.
				printf("Ignoring drilling operation (id=%d) with no cutting tool defined\n", pDrilling->m_id );
				continue;
			}

			double hole_diameter = pDrillBit->CuttingRadius(false) * 2.0;

			if (hole_diameter < min_chamfer_diameter)
			{
				printf("Ignoring chamfer for drilled hole due to geometry of selected chamfering bit\n");
				continue;
			}

			std::vector<CNCPoint> locations = pDrilling->FindAllLocations(pMachineState);
			for (std::vector<CNCPoint>::const_iterator l_itLocation = locations.begin(); l_itLocation != locations.end(); l_itLocation++)
			{
				CNCPoint point = pMachineState->Fixture().Adjustment( *l_itLocation );
				circles.push_back( Circle( point, hole_diameter, pDrilling->m_params.m_depth ) );
			} // End for
		} // End if - then

		if (child->GetType() == CounterBoreType)
		{
			CCounterBore *pCounterBore = ((CCounterBore *) child);

			std::list<int> unused;
			std::vector<CNCPoint> locations = pCounterBore->FindAllLocations(&unused, pMachineState);
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
				continue;
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
					<< _T("standoff=") << drawing_units(m_depth_op_params.m_clearance_height) << _T(", ")
					<< _T("dwell=") << 0.0 << _T(", ")
					<< _T("peck_depth=") << 0.0 // << ", "
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
							<< _T("z=") << m_depth_op_params.m_clearance_height/theApp.m_program->m_units << _T(")\n");

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
							_T("i=") << drawing_units(centre.X(false) - point.X(false)) << _T(", ") <<
							_T("j=") << drawing_units(centre.Y(false) - point.Y(false)) << _T(")\n");
				point.SetX( centre.X(false) - radius_of_spiral );
				point.SetY( centre.Y(false) );

				// Second quadrant (9 O'Clock to 6 O'Clock)
				python << _T("arc_ccw( x=") << centre.X(true) << _T(", ") <<
							_T("y=") << drawing_units(centre.Y(false) - radius_of_spiral) << _T(", ") <<
							_T("z=") << drawing_units(cutting_depth) << _T(", ") <<	// full depth now
							_T("i=") << drawing_units(centre.X(false) - point.X(false)) << _T(", ") <<
							_T("j=") << drawing_units(centre.Y(false) - point.Y(false)) << _T(")\n");
				point.SetX( centre.X(false) );
				point.SetY( centre.Y(false) - radius_of_spiral );

				// Third quadrant (6 O'Clock to 3 O'Clock)
				python << _T("arc_ccw( x=") << drawing_units(centre.X(false) + radius_of_spiral) << _T(", ") <<
							_T("y=") << centre.Y(true) << _T(", ") <<
							_T("z=") << drawing_units(cutting_depth) << _T(", ") <<	// full depth now
							_T("i=") << drawing_units(centre.X(false) - point.X(false)) << _T(", ") <<
							_T("j=") << drawing_units(centre.Y(false) - point.Y(false)) << _T(")\n");
				point.SetX( centre.X(false) + radius_of_spiral );
				point.SetY( centre.Y(false) );

				// Fourth quadrant (3 O'Clock to 12 O'Clock)
				python << _T("arc_ccw( x=") << centre.X(true) << _T(", ") <<
							_T("y=") << drawing_units(centre.Y(false) + radius_of_spiral) << _T(", ") <<
							_T("z=") << drawing_units(cutting_depth) << _T(", ") <<	// full depth now
							_T("i=") << drawing_units(centre.X(false) - point.X(false)) << _T(", ") <<
							_T("j=") << drawing_units(centre.Y(false) - point.Y(false)) << _T(")\n");
				point.SetX( centre.X(false) );
				point.SetY( centre.Y(false) + radius_of_spiral );

				python << _T("rapid( z=") << drawing_units(m_depth_op_params.m_clearance_height) << _T(")\n");
			}
		} // End for
	} // End for

	return(python);
}

void CChamfer::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element = new TiXmlElement( "Chamfer" );
	root->LinkEndChild( element );

	m_params.WriteXMLAttributes(element);

	WriteBaseXML(element);
}

// static member function
HeeksObj* CChamfer::ReadFromXMLElement(TiXmlElement* element)
{
	CChamfer* new_object = new CChamfer;

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
		element->RemoveChild(*itElem);
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

