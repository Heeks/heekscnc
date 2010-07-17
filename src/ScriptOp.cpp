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
#include "tinyxml/tinyxml.h"
#include "PythonStuff.h"
#include "MachineState.h"

#include <sstream>
#include <iomanip>

#include <BRepAdaptor_Curve.hxx>
#include <gp_Circ.hxx>
#include <BRepTools.hxx>
#include <BRep_Tool.hxx>
#include <BRepMesh.hxx>
#include <Poly_Polygon3D.hxx>

CScriptOp::CScriptOp( const CScriptOp & rhs ) : COp(rhs)
{
	m_str = rhs.m_str;
}

CScriptOp & CScriptOp::operator= ( const CScriptOp & rhs )
{
	if (this != &rhs)
	{
		COp::operator=( rhs );
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



/* static */ Python CScriptOp::OpenCamLibDefinition(TopoDS_Edge edge)
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

			python << _T("ocl.Line(ocl.Point(") << PS.X(false) << _T(",") << PS.Y(false) << _T(",") << PS.Z(false) << _T("), ")
					<< _T("ocl.Point(") << PE.X(false) << _T(",") << PE.Y(false) << _T(",") << PE.Z(false) << _T("))");
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

			python << _T("ocl.Arc(")
					<< _T("ocl.Point(") << PS.X(false) << _T(",") << PS.Y(false) << _T(",") << PS.Z(false) << _T("),") // Start
					<< _T("ocl.Point(") << PE.X(false) << _T(",") << PE.Y(false) << _T(",") << PE.Z(false) << _T("),") // End
					<< _T("ocl.Point(") << centre.X(false) << _T(",") << centre.Y(false) << _T(",") << centre.Z(false) << _T("),") // Centre
					<< (l_bClockwise?_T("True"):_T("False")) << _T(")"); // Direction
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
				python << _T("ocl.Path(");
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
						python << _T("ocl.Line(")
								<< _T("ocl.Point(") << previous.X(false) << _T(",") << previous.Y(false) << _T(",") << previous.Z(false) << _T(")")
								<< _T("ocl.Point(") << p.X(false) << _T(",") << p.Y(false) << _T(",") << p.Z(false) << _T(")")
								<< _T(")");
					}
				} // End for
				python << _T(")");	// End ocl.Path() definition
			} // End if - then
		}
		break;
	} // End switch

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

		python << _T("\tsketches = []\n");
		python << _T("\tpoints = []\n");
	}

    for (std::list<HeeksObj *>::iterator itObject = objects.begin(); itObject != objects.end(); itObject++)
    {
		std::vector<TopoDS_Edge> edges;
		switch ((*itObject)->GetType())
		{
		case PointType:
			double p[3];
			memset( p, 0, sizeof(p) );
			heeksCAD->VertexGetPoint( *itObject, p );
			python << _T("\tpoints.append(ocl.Point(") << p[0] << _T(",") << p[1] << _T(",") << p[2] << _T("))\n");
			break;

		case SketchType:
			if (! heeksCAD->ConvertSketchToEdges( *itObject, edges ))
			{
				Python empty;
				return(empty);
			}
			else
			{
				// The edges will already have been sorted.  We need to traverse them in order and separate
				// connected sequences of them.  As we start new connected sequences, we should check to see
				// if they connect back to their beginning and mark them as 'periodic'.
				if (edges.size() > 0)
				{
					python << _T("\tsketch_id_") << (int) (*itObject)->m_id << _T(" = ocl.Path()\n");
				}

				for (std::vector<TopoDS_Edge>::size_type offset = 0; offset < edges.size(); offset++)
				{
					python << _T("\tsketch_id_") << (int) (*itObject)->m_id << _T(".append(") << OpenCamLibDefinition(edges[offset]) << _T(")\n");
				}

				if (edges.size() > 0)
				{
					python << _T("\tsketches.append(")
							<< _T("sketch_id_") << (int) (*itObject)->m_id << _T(")\n");
				}
			} // End if - else
			break;
		} // End switch
    } // End for

    return(python);

}

Python CScriptOp::AppendTextToProgram(CMachineState *pMachineState)
{
	Python python;

	std::list<HeeksObj *> children;
	for (HeeksObj *child = GetFirstChild(); child != NULL; child = GetNextChild())
	{
	    children.push_back(child);
	}

    if (children.size() > 0)
    {
        Python object_title;
        object_title << _T("script_op_id_") << (int) this->m_id;

        python << OpenCamLibDefinition(children, object_title);
        python << _T("\n");
        python << _T("graphics = ") << object_title << _T("()\n\n");
    }

	python << m_str.c_str();

	if(python.Last() != _T('\n'))python.Append(_T('\n'));

	return python;
} // End AppendTextToProgram() method

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
	TiXmlElement * element = new TiXmlElement( "ScriptOp" );
	root->LinkEndChild( element );
	element->SetAttribute("script", Ttc(m_str.c_str()));

	COp::WriteBaseXML(element);
}

// static member function
HeeksObj* CScriptOp::ReadFromXMLElement(TiXmlElement* element)
{
	CScriptOp* new_object = new CScriptOp;

	new_object->m_str = wxString(Ctt(element->Attribute("script")));

	// read common parameters
	new_object->ReadBaseXML(element);

	return new_object;
}

bool CScriptOp::operator==( const CScriptOp & rhs ) const
{
	if (m_str != rhs.m_str) return(false);

	return(COp::operator==(rhs));
}
