// ScriptOp.cpp
/*
 * Copyright (c) 2010, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "stdafx.h"
#include <TopoDS_Edge.hxx>
#include "ScriptOp.h"
#include "ProgramCanvas.h"
#include "Program.h"
#include "interface/HeeksObj.h"
#include "interface/PropertyList.h"
#include "interface/PropertyCheck.h"
#include "tinyxml/tinyxml.h"
#include "PythonStuff.h"

#ifdef HEEKSCNC
#define FIND_FIRST_TOOL CTool::FindFirstByType
#define FIND_ALL_TOOLS CTool::FindAllTools
#else
#define FIND_FIRST_TOOL heeksCNC->FindFirstToolByType
#define FIND_ALL_TOOLS heeksCNC->FindAllTools
#endif

#include <sstream>
#include <iomanip>

#include <BRepAdaptor_Curve.hxx>
#include <gp_Circ.hxx>
#include <BRepTools.hxx>
#include <BRep_Tool.hxx>
#include <BRepMesh.hxx>
#include <Poly_Polygon3D.hxx>

CScriptOp::CScriptOp( const CScriptOp & rhs ) : CDepthOp(rhs)
{
	m_str = rhs.m_str;
}

CScriptOp & CScriptOp::operator= ( const CScriptOp & rhs )
{
	if (this != &rhs)
	{
		CDepthOp::operator=( rhs );
		m_str = rhs.m_str;
	}

	return(*this);
}

const wxBitmap &CScriptOp::GetIcon()
{
	if(!m_active)return GetInactiveIcon();
	static wxBitmap* icon = NULL;
	if(icon == NULL)icon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/scriptop.png")));
	return *icon;
}



/* static */ Python CScriptOp::OpenCamLibDefinition(TopoDS_Edge edge, Python prefix, Python suffix)
{
    Python python;

	BRepAdaptor_Curve curve(edge);
	switch (curve.GetType())
	{
		case GeomAbs_Line:
		{
			CNCPoint PS;
			gp_Vec VS;
			curve.D1(curve.FirstParameter(), PS, VS);
			CNCPoint PE;
			gp_Vec VE;
			curve.D1(curve.LastParameter(), PE, VE);

			python << prefix << _T("ocl.Line(ocl.Point(") << PS.X(true) << _T(",") << PS.Y(true) << _T(",") << PS.Z(true) << _T("), ")
					<< _T("ocl.Point(") << PE.X(true) << _T(",") << PE.Y(true) << _T(",") << PE.Z(true) << _T("))") << suffix;
		}
		break;

		case GeomAbs_Circle:
		{
			CNCPoint PS;
			gp_Vec VS;
			curve.D1(curve.FirstParameter(), PS, VS);
			CNCPoint PE;
			gp_Vec VE;
			curve.D1(curve.LastParameter(), PE, VE);
			gp_Circ circle = curve.Circle();

            // Arc towards PE
            CNCPoint point(PE);
            CNCPoint centre( circle.Location() );
            bool l_bClockwise = (circle.Axis().Direction().Z() <= 0);

			python << prefix << _T("ocl.Arc(")
					<< _T("ocl.Point(") << PS.X(true) << _T(",") << PS.Y(true) << _T(",") << PS.Z(true) << _T("),") // Start
					<< _T("ocl.Point(") << PE.X(true) << _T(",") << PE.Y(true) << _T(",") << PE.Z(true) << _T("),") // End
					<< _T("ocl.Point(") << centre.X(true) << _T(",") << centre.Y(true) << _T(",") << centre.Z(true) << _T("),") // Centre
					<< (l_bClockwise?_T("True"):_T("False")) << _T(")") // Direction
					<< suffix;
			break;
		}

		default:
		{
			// make lots of small lines
			gp_Pnt PS;
			gp_Vec VS;
			curve.D1(curve.FirstParameter(), PS, VS);
			gp_Pnt PE;
			gp_Vec VE;
			curve.D1(curve.LastParameter(), PE, VE);

			BRepTools::Clean(edge);
			double max_deviation_for_spline_to_arc = 0.001;
			BRepMesh::Mesh(edge, max_deviation_for_spline_to_arc);

			TopLoc_Location L;
			Handle(Poly_Polygon3D) Polyg = BRep_Tool::Polygon3D(edge, L);
			if (!Polyg.IsNull()) {
				const TColgp_Array1OfPnt& Points = Polyg->Nodes();
				Standard_Integer po;
				int i = 0;
				std::list<CNCPoint> interpolated_points;
				CNCPoint previous;
				for (po = Points.Lower(); po <= Points.Upper(); po++, i++) {
					CNCPoint p = (Points.Value(po)).Transformed(L);
					interpolated_points.push_back(p);

					if (interpolated_points.size() == 1)
					{
						previous = p;
					}
					else
					{
						python << prefix << _T("ocl.Line(")
								<< _T("ocl.Point(") << previous.X(true) << _T(",") << previous.Y(true) << _T(",") << previous.Z(true) << _T("),")
								<< _T("ocl.Point(") << p.X(true) << _T(",") << p.Y(true) << _T(",") << p.Z(true) << _T(")")
								<< _T(")")
								<< suffix;
                        previous = p;
					}
				} // End for
			} // End if - then
		}
		break;
	} // End switch

	return(python);
}

/*
This is a simple way to insert datum parameters for translating gcode around later
*/
/* static */ Python CScriptOp::MiscDefs(std::list<HeeksObj *> objects, Python object_title )
{
    Python python;

    for (std::list<HeeksObj *>::iterator itObject = objects.begin(); itObject != objects.end(); itObject++)
    {
		std::vector<TopoDS_Edge> edges;
		switch ((*itObject)->GetType())
		{


		case CoordinateSystemType:			
			// you can paste in new datum/coordinate system parameters with this
			python << _T("translate(")<< heeksCAD->GetDatumPosX(*itObject)/ theApp.m_program->m_units << _T(",") << heeksCAD->GetDatumPosY(*itObject)/ theApp.m_program->m_units << _T(",") << heeksCAD->GetDatumPosZ(*itObject)/ theApp.m_program->m_units<< _T(")\n");

			//python << _T("datumX_dir(")<< heeksCAD->GetDatumDirx_X(*itObject)/ theApp.m_program->m_units << _T(",") << heeksCAD->GetDatumDirx_Y(*itObject)/ theApp.m_program->m_units << _T(",") << heeksCAD->GetDatumDirx_Z(*itObject)/ theApp.m_program->m_units<< _T(")\n");
			
			//python << _T("datumY_dir(")<< heeksCAD->GetDatumDiry_X(*itObject)/ theApp.m_program->m_units << _T(",") << heeksCAD->GetDatumDiry_Y(*itObject)/ theApp.m_program->m_units << _T(",") << heeksCAD->GetDatumDiry_Z(*itObject)/ theApp.m_program->m_units<< _T(")\n");

			break;

			
			} // End switch
    } // End for

    return(python);

}





/**
    This method converts the list of objects into a set of nested classes that
    describe the geometry.  These classes will include definitions of these objects
    in terms of lines, arcs and points.  Any BSpline (or other) objects that are
 */
/* static */ Python CScriptOp::OpenCamLibDefinition(std::list<HeeksObj *> objects, Python object_title )
{
    Python python;

	if (objects.size() > 0)
	{
		python << _T("class ") << object_title << _T(":\n");

		python << _T("\040\040\040\040sketches = []\n");
		python << _T("\040\040\040\040points = []\n");
		python << _T("\040\040\040\040circles = []\n");
	}

    for (std::list<HeeksObj *>::iterator itObject = objects.begin(); itObject != objects.end(); itObject++)
    {
		std::list< std::vector<TopoDS_Edge> > edges_list;
		switch ((*itObject)->GetType())
		{
			double p[3];

		case CircleType:
			memset( p, 0, sizeof(p) );
			double radius;
			heeksCAD->GetArcCentre( *itObject, p );
			radius = heeksCAD->CircleGetRadius( *itObject);
			// start and end at north pole, ccw
			python << _T("\040\040\040\040circles.append(ocl.Arc(ocl.Point(") << p[0] / theApp.m_program->m_units << _T(",") << (p[1]+radius) / theApp.m_program->m_units << _T(",") << p[2] / theApp.m_program->m_units;
			python << _T("), ocl.Point(") << p[0] / theApp.m_program->m_units << _T(",") << (p[1]+radius) / theApp.m_program->m_units << _T(",") << p[2] / theApp.m_program->m_units << _T("),");
			python << _T(" ocl.Point(") << p[0] / theApp.m_program->m_units << _T(",") <<  p[1] / theApp.m_program->m_units << _T(",") << p[2] / theApp.m_program->m_units << _T("), True))\n");
			break;


		case PointType:
			memset( p, 0, sizeof(p) );
			heeksCAD->VertexGetPoint( *itObject, p );
			python << _T("\040\040\040\040points.append(ocl.Point(") << p[0] / theApp.m_program->m_units << _T(",") << p[1] / theApp.m_program->m_units << _T(",") << p[2] / theApp.m_program->m_units << _T("))\n");
			break;


		case SketchType:
			if (! heeksCAD->ConvertSketchToEdges( *itObject, edges_list ))
			{
				Python empty;
				return(empty);
			}
			else
			{
				// The edges will already have been sorted.  We need to traverse them in order and separate
				// connected sequences of them.  As we start new connected sequences, we should check to see
				// if they connect back to their beginning and mark them as 'periodic'.
				unsigned int num_edges = 0;

				for(std::list< std::vector<TopoDS_Edge> >::iterator It = edges_list.begin(); It != edges_list.end(); It++)
				{
					std::vector<TopoDS_Edge> &edges = *It;
					num_edges += edges.size();
				}

				if (num_edges > 0)
				{
					python << _T("\040\040\040\040sketch_id_") << (int) (*itObject)->m_id << _T(" = ocl.Path()\n");
				}

				for(std::list< std::vector<TopoDS_Edge> >::iterator It = edges_list.begin(); It != edges_list.end(); It++)
				{
					std::vector<TopoDS_Edge> &edges = *It;
					for (std::vector<TopoDS_Edge>::size_type offset = 0; offset < edges.size(); offset++)
					{
						Python prefix;
						Python suffix;
						prefix << _T("\040\040\040\040sketch_id_") << (int) (*itObject)->m_id << _T(".append(");
						suffix << _T(")\n");
						python << OpenCamLibDefinition(edges[offset], prefix, suffix);
					}
				}

				if (num_edges > 0)
				{
					python << _T("\040\040\040\040sketches.append(")
						<< _T("sketch_id_") << (int) (*itObject)->m_id << _T(")\n");
				}
			} // End if - else
			break;
		} // End switch
    } // End for

    return(python);

}


Python CScriptOp::AppendTextToProgram()
{
	Python python;

	if (m_emit_depthop_params)
	{
		python << CDepthOp::AppendTextToProgram();
	} else 	{
		if(m_comment.Len() > 0)
		{
		  python << _T("comment(") << PythonString(m_comment) << _T(")\n");
		}
	}

	std::list<HeeksObj *> children;
	for (HeeksObj *child = GetFirstChild(); child != NULL; child = GetNextChild())
	{
	    children.push_back(child);
	}

    if (children.size() > 0)
    {
        Python object_title;
        object_title << _T("script_op_id_") << (int) this->m_id;
        python << MiscDefs(children, object_title);
        python << OpenCamLibDefinition(children, object_title);
        python << _T("\n");
        python << _T("graphics = ") << object_title << _T("()\n\n");
    }

	python << m_str.c_str();

	if(python.Last() != _T('\n'))python.Append(_T('\n'));

	return python;
} // End AppendTextToProgram() method




static void on_set_emit_depthop_params(bool value, HeeksObj* object)
{
	((CScriptOp*)object)->m_emit_depthop_params = (value ? 1:0);
	((CScriptOp*)object)->WriteDefaultValues();
}

void CScriptOp::GetProperties(std::list<Property *> *list)
{
    list->push_back(new PropertyCheck(_("emit_depthop_params"), m_emit_depthop_params != 0, this, on_set_emit_depthop_params));
    CDepthOp::GetProperties(list);
}

ObjectCanvas* CScriptOp::GetDialog(wxWindow* parent)
{
	static TextCanvas* object_canvas = NULL;
	if(object_canvas == NULL)object_canvas = new TextCanvas(parent, &m_str);
	else
	{
		object_canvas->m_str = &m_str;
		object_canvas->SetWithObject(this);
	}
	return object_canvas;
}

HeeksObj *CScriptOp::MakeACopy(void)const
{
	return new CScriptOp(*this);
}

void CScriptOp::CopyFrom(const HeeksObj* object)
{
	if (object->GetType() == GetType())
	{
		operator=(*((CScriptOp*)object));
	}
}

bool CScriptOp::CanAddTo(HeeksObj* owner)
{
	return ((owner != NULL) && (owner->GetType() == OperationsType));
}

void CScriptOp::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element = heeksCAD->NewXMLElement( "ScriptOp" );
	heeksCAD->LinkXMLEndChild( root,  element );
	element->SetAttribute( "script", Ttc(m_str.c_str()));
	element->SetAttribute( "emit_depthop_params", int(m_emit_depthop_params) );

	CDepthOp::WriteBaseXML(element);
}

// static member function
HeeksObj* CScriptOp::ReadFromXMLElement(TiXmlElement* element)
{
	CScriptOp* new_object = new CScriptOp;

	new_object->m_str = wxString(Ctt(element->Attribute("script")));
	if (element->Attribute("emit_depthop_params")) element->Attribute("emit_depthop_params", &new_object->m_emit_depthop_params);

	// read common parameters
	new_object->ReadBaseXML(element);

	return new_object;
}

bool CScriptOp::operator==( const CScriptOp & rhs ) const
{
	if (m_str != rhs.m_str) return(false);

	return(CDepthOp::operator==(rhs));
}
