// Fixture.cpp
/*
 * Copyright (c) 2009, Dan Heeks, Perttu Ahola
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "stdafx.h"
#include <math.h>
#include "Fixture.h"
#include "Fixtures.h"
#include "CNCConfig.h"
#include "ProgramCanvas.h"
#include "interface/HeeksObj.h"
#include "interface/PropertyInt.h"
#include "interface/PropertyDouble.h"
#include "interface/PropertyLength.h"
#include "interface/PropertyChoice.h"
#include "interface/PropertyString.h"
#include "interface/PropertyVertex.h"
#include "interface/PropertyCheck.h"
#include "tinyxml/tinyxml.h"
#include "CNCPoint.h"
#include "PythonStuff.h"
#include "interface/Tool.h"
#include "MachineState.h"
#include "Program.h"
#include "interface/strconv.h"

#include <gp_Pnt.hxx>
#include <gp_Ax1.hxx>
#include <gp_Trsf.hxx>

#include <sstream>
#include <string>
#include <algorithm>

class CProgram;

extern CHeeksCADInterface* heeksCAD;

void CFixtureParams::set_initial_values()
{
	CNCConfig config(ConfigScope());

	config.Read(_T("m_yz_plane"), &m_yz_plane, 0.0);
	config.Read(_T("m_xz_plane"), &m_xz_plane, 0.0);
	config.Read(_T("m_xy_plane"), &m_xy_plane, 0.0);

	double pivot_point_x, pivot_point_y, pivot_point_z;
	config.Read(_T("pivot_point_x"), &pivot_point_x, 0.0);
	config.Read(_T("pivot_point_y"), &pivot_point_y, 0.0);
	config.Read(_T("pivot_point_z"), &pivot_point_z, 0.0);

	m_pivot_point = gp_Pnt( pivot_point_x, pivot_point_y, pivot_point_z );

	config.Read(_T("safety_height_defined"), &m_safety_height_defined, false);
	config.Read(_T("safety_height"), &m_safety_height, 0.0);

	config.Read(_T("touch_off_point_defined"), &m_touch_off_point_defined, false);
	double touch_off_point_x, touch_off_point_y, touch_off_point_z;
	config.Read(_T("touch_off_point_x"), &touch_off_point_x, 0.0);
	config.Read(_T("touch_off_point_y"), &touch_off_point_y, 0.0);
	config.Read(_T("touch_off_point_z"), &touch_off_point_z, 0.0);
	m_touch_off_point = gp_Pnt( touch_off_point_x, touch_off_point_y, touch_off_point_z );

	config.Read(_T("touch_off_description"), &m_touch_off_description, _T(""));
}

void CFixtureParams::write_values_to_config()
{
	// We ALWAYS write the parameters into the configuration file in mm (for consistency).
	// If we're now in inches then convert the values.
	// We're in mm already.
	CNCConfig config(ConfigScope());

	config.Write(_T("m_yz_plane"), m_yz_plane);
	config.Write(_T("m_xz_plane"), m_xz_plane);
	config.Write(_T("m_xy_plane"), m_xy_plane);

	config.Write(_T("pivot_point_x"), m_pivot_point.X());
	config.Write(_T("pivot_point_y"), m_pivot_point.Y());
	config.Write(_T("pivot_point_z"), m_pivot_point.Z());

	config.Write(_T("safety_height_defined"), m_safety_height_defined);
	config.Write(_T("safety_height"), m_safety_height);

	config.Write(_T("touch_off_point_defined"), m_touch_off_point_defined);
	config.Write(_T("touch_off_point_x"), m_touch_off_point.X());
	config.Write(_T("touch_off_point_y"), m_touch_off_point.Y());
	config.Write(_T("touch_off_point_z"), m_touch_off_point.Z());

	config.Write(_T("touch_off_description"), m_touch_off_description);
}

static void on_set_yz_plane(double value, HeeksObj* object)
{
	((CFixture*)object)->m_params.m_yz_plane = value;
	 ((CFixture*)object)->ResetTitle();
}

static void on_set_xz_plane(double value, HeeksObj* object)
{
	((CFixture*)object)->m_params.m_xz_plane = value;
	((CFixture*)object)->ResetTitle();
}

static void on_set_xy_plane(double value, HeeksObj* object)
{
	((CFixture*)object)->m_params.m_xy_plane = value;
	((CFixture*)object)->ResetTitle();
}

static void on_set_pivot_point(const double *vt, HeeksObj* object){
	((CFixture *)object)->m_params.m_pivot_point.SetX( vt[0] );
	((CFixture *)object)->m_params.m_pivot_point.SetY( vt[1] );
	((CFixture *)object)->m_params.m_pivot_point.SetZ( vt[2] );
}

static void on_set_touch_off_point(const double *vt, HeeksObj* object){
	((CFixture *)object)->m_params.m_touch_off_point.SetX( vt[0] );
	((CFixture *)object)->m_params.m_touch_off_point.SetY( vt[1] );
	// ((CFixture *)object)->m_params.m_touch_off_point.SetZ( vt[2] ); /* We're not using the Z value for this */

	if (fabs(vt[2]) > heeksCAD->GetTolerance())
	{
	    wxMessageBox(_("Warning: The 'Z' coordinate is not used for this.  Set the 'z' in the 'safety height' property instead (Don't forget to express it in machine - G53 - coordinates)"));
	}
}

static void on_set_touch_off_point_defined(const bool value, HeeksObj *object)
{
    ((CFixture *)object)->m_params.m_touch_off_point_defined = value;
    heeksCAD->Changed();
}

static void on_set_touch_off_description(const wxChar *value, HeeksObj* object){
	((CFixture *)object)->m_params.m_touch_off_description = value;
}

static void on_set_safety_height_defined(const bool value, HeeksObj *object)
{
    ((CFixture *)object)->m_params.m_safety_height_defined = value;
    heeksCAD->Changed();
}

static void on_set_safety_height(const double value, HeeksObj *object)
{
    ((CFixture *)object)->m_params.m_safety_height = value;
    heeksCAD->Changed();
}

void CFixtureParams::GetProperties(CFixture* parent, std::list<Property *> *list)
{
	list->push_back(new PropertyDouble(_("YZ plane (around X) rotation"), m_yz_plane, parent, on_set_yz_plane));
	list->push_back(new PropertyDouble(_("XZ plane (around Y) rotation"), m_xz_plane, parent, on_set_xz_plane));
	list->push_back(new PropertyDouble(_("XY plane (around Z) rotation"), m_xy_plane, parent, on_set_xy_plane));

	double pivot_point[3];
	pivot_point[0] = m_pivot_point.X();
	pivot_point[1] = m_pivot_point.Y();
	pivot_point[2] = m_pivot_point.Z();

	list->push_back(new PropertyVertex(_("Pivot Point"), pivot_point, parent, on_set_pivot_point));

    list->push_back(new PropertyCheck(_("Safety Height Defined"), m_safety_height_defined, parent, on_set_safety_height_defined));

    if (m_safety_height_defined)
    {
        list->push_back(new PropertyLength(_("Safety Height (in G53 - Machine - coordinates)"), m_safety_height, parent, on_set_safety_height));
    }

	list->push_back(new PropertyCheck(_("Touch Off Point Defined"), m_touch_off_point_defined, parent, on_set_touch_off_point_defined));
	if (m_touch_off_point_defined)
	{
		double touch_off_point[3];
		touch_off_point[0] = m_touch_off_point.X();
		touch_off_point[1] = m_touch_off_point.Y();
		touch_off_point[2] = m_touch_off_point.Z();

		wxString title;
		title << _("Touch-off Point (in ") << parent->m_coordinate_system_number << _T(" coordinates)");
		list->push_back(new PropertyVertex(title, touch_off_point, parent, on_set_touch_off_point));
		list->push_back(new PropertyString(_("Touch-off Description"), m_touch_off_description, parent, on_set_touch_off_description));
	}

}

void CFixtureParams::WriteXMLAttributes(TiXmlNode *root)
{
	TiXmlElement * element;
	element = new TiXmlElement( "params" );
	root->LinkEndChild( element );

	element->SetDoubleAttribute("yz_plane", m_yz_plane);
	element->SetDoubleAttribute("xz_plane", m_xz_plane);
	element->SetDoubleAttribute("xy_plane", m_xy_plane);

	element->SetDoubleAttribute("pivot_point_x", m_pivot_point.X());
	element->SetDoubleAttribute("pivot_point_y", m_pivot_point.Y());
	element->SetDoubleAttribute("pivot_point_z", m_pivot_point.Z());

	element->SetAttribute("safety_height_defined", m_safety_height_defined);
	element->SetDoubleAttribute("safety_height", m_safety_height);

	element->SetAttribute("touch_off_point_defined", m_touch_off_point_defined);
	element->SetDoubleAttribute("touch_off_point_x", m_touch_off_point.X());
	element->SetDoubleAttribute("touch_off_point_y", m_touch_off_point.Y());
	element->SetDoubleAttribute("touch_off_point_z", m_touch_off_point.Z());

	element->SetAttribute("touch_off_description", Ttc(m_touch_off_description.c_str()));
}

void CFixtureParams::ReadParametersFromXMLElement(TiXmlElement* pElem)
{
	set_initial_values();

	if (pElem->Attribute("yz_plane")) m_yz_plane = atof(pElem->Attribute("yz_plane"));
	if (pElem->Attribute("xz_plane")) m_xz_plane = atof(pElem->Attribute("xz_plane"));
	if (pElem->Attribute("xy_plane")) m_xy_plane = atof(pElem->Attribute("xy_plane"));

	if (pElem->Attribute("pivot_point_x")) m_pivot_point.SetX( atof(pElem->Attribute("pivot_point_x")) );
	if (pElem->Attribute("pivot_point_y")) m_pivot_point.SetY( atof(pElem->Attribute("pivot_point_y")) );
	if (pElem->Attribute("pivot_point_z")) m_pivot_point.SetZ( atof(pElem->Attribute("pivot_point_z")) );

	int flag = 0;
	if (pElem->Attribute("safety_height_defined")) pElem->Attribute("safety_height_defined", &flag);
	m_safety_height_defined = (flag != 0);
	if (pElem->Attribute("safety_height")) m_safety_height = atof(pElem->Attribute("safety_height"));

	flag = 0;
	if (pElem->Attribute("touch_off_point_defined")) pElem->Attribute("touch_off_point_defined", &flag);
	m_touch_off_point_defined = (flag != 0);
	if (pElem->Attribute("touch_off_point_x")) m_touch_off_point.SetX( atof(pElem->Attribute("touch_off_point_x")) );
	if (pElem->Attribute("touch_off_point_y")) m_touch_off_point.SetY( atof(pElem->Attribute("touch_off_point_y")) );
	if (pElem->Attribute("touch_off_point_z")) m_touch_off_point.SetZ( atof(pElem->Attribute("touch_off_point_z")) );

	if (pElem->Attribute("touch_off_description")) m_touch_off_description = Ctt(pElem->Attribute("touch_off_description"));
}

const wxBitmap &CFixture::GetIcon()
{
	static wxBitmap* icon = NULL;
	if(icon == NULL)icon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/fixture.png")));
	return *icon;
}

/**
	This method is called when the CAD operator presses the Python button.  This method generates
	Python source code whose job will be to generate RS-274 GCode.  It's done in two steps so that
	the Python code can be configured to generate GCode suitable for various CNC interpreters.

	This class should just select the appropriate coordinate system.
 */
Python CFixture::AppendTextToProgram() const
{
	Python python;

	if (m_title.size() > 0)
	{
		python << _T("comment(") << PythonString(m_title) << _T(")\n");
	} // End if - then

	python << _T("workplane(") << int(m_coordinate_system_number) << _T(")\n");

	return(python);
}


// 1 = G54, 2 = G55 etc.
static void on_set_coordinate_system_number(const int zero_based_choice, HeeksObj* object)
{
	if (zero_based_choice < 0) return;	// An error has occured.
	CFixture *pFixture = (CFixture *) object;

	pFixture->m_coordinate_system_number = CFixture::eCoordinateSystemNumber_t(zero_based_choice + 1);	// Change from zero-based to one-based offset
	pFixture->ResetTitle();

	// See if we already have a fixture for this coordinate system.  If so, merge with it.
	CFixture *pExistingFixture = theApp.m_program->Fixtures()->Find( pFixture->m_coordinate_system_number );
	if ((pExistingFixture != NULL) && (pExistingFixture != pFixture))
	{
        // There is a pre-existing fixture for this coordinate system.  Use the pre-existing one and
        // throw this one away.

        for (HeeksObj *parent = pFixture->GetFirstOwner(); parent != NULL; parent = pFixture->GetNextOwner())
        {
            parent->Remove(pFixture);
            parent->Add( pExistingFixture, NULL );
        } // End for
	}
} // End on_set_coordinate_system_number

/**
	NOTE: The m_title member is a special case.  The HeeksObj code looks for a 'GetShortString()' method.  If found, it
	adds a Property called 'Object Title'.  If the value is changed, it tries to call the 'OnEditString()' method.
	That's why the m_title value is not defined here
 */
void CFixture::GetProperties(std::list<Property *> *list)
{
	std::list< wxString > choices;

	{
		choices.push_back(_T("G54"));
		choices.push_back(_T("G55"));
		choices.push_back(_T("G56"));
		choices.push_back(_T("G57"));
		choices.push_back(_T("G58"));
		choices.push_back(_T("G59"));
		choices.push_back(_T("G59.1"));
		choices.push_back(_T("G59.2"));
		choices.push_back(_T("G59.3"));
	} // End for
	list->push_back(new PropertyChoice(_("Coordinate System"), choices, int(m_coordinate_system_number) - 1, this, on_set_coordinate_system_number));

	m_params.GetProperties(this, list);
	HeeksObj::GetProperties(list);
}


HeeksObj *CFixture::MakeACopy(void)const
{
	return new CFixture(*this);
}

void CFixture::CopyFrom(const HeeksObj* object)
{
	operator=(*((CFixture*)object));
}

bool CFixture::CanAddTo(HeeksObj* owner)
{
	return ((owner->GetType() == FixturesType) || (COp::IsAnOperation(owner->GetType())));
}

void CFixture::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element = new TiXmlElement( "Fixture" );
	root->LinkEndChild( element );
	element->SetAttribute("title", Ttc(m_title.c_str()));

	std::ostringstream l_ossValue;
	l_ossValue.str(""); l_ossValue << m_coordinate_system_number;
	element->SetAttribute("coordinate_system_number", l_ossValue.str().c_str() );

	m_params.WriteXMLAttributes(element);
	WriteBaseXML(element);
}

// static member function
HeeksObj* CFixture::ReadFromXMLElement(TiXmlElement* element)
{
	int coordinate_system_number = 1;
	if (element->Attribute("coordinate_system_number")) coordinate_system_number = atoi(element->Attribute("coordinate_system_number"));

	if (element->Attribute("title"))
	{
		wxString title(Ctt(element->Attribute("title")));
		CFixture* new_object = new CFixture( title.c_str(), CFixture::eCoordinateSystemNumber_t(coordinate_system_number));

		for(TiXmlElement* pElem = TiXmlHandle(element).FirstChildElement().Element(); pElem; pElem = pElem->NextSiblingElement())
		{
			std::string name(pElem->Value());
			if(name == "params"){
				new_object->m_params.ReadParametersFromXMLElement(pElem);
			}
		}

		new_object->ReadBaseXML(element);
		return new_object;
	} // End if - then

	return(NULL);
}


void CFixture::OnEditString(const wxChar* str){
        m_title.assign(str);
	heeksCAD->Changed();
}



/**
 * This method uses the various attributes of the cutting tool to produce a meaningful name.
 * eg: with diameter = 6, units = 1 (mm) and type = 'drill' the name would be '6mm Drill Bit".  The
 * idea is to produce a m_title value that is representative of the cutting tool.  This will
 * make selection in the program list easier.
 *
 * NOTE: The ResetTitle() method looks at the m_title value for strings that are likely to
 * have come from this method.  If this method changes, the ResetTitle() method may also
 * need to change.
 */
wxString CFixture::GenerateMeaningfulName() const
{
    wxString name;

    name << m_coordinate_system_number;

	if (fabs(m_params.m_yz_plane) > heeksCAD->GetTolerance())
	{
		name << _(" rotated ") << m_params.m_yz_plane << _(" degrees in YZ plane");
	} // End if - then

	if (fabs(m_params.m_xz_plane) > heeksCAD->GetTolerance())
	{
		name << _(" rotated ") << m_params.m_xz_plane << _(" degrees in XZ plane");
	} // End if - then

	if (fabs(m_params.m_xy_plane) > heeksCAD->GetTolerance())
	{
		name << _(" rotated ") << m_params.m_xy_plane << _(" degrees in XY plane");
	} // End if - then

	return(name);
} // End GenerateMeaningfulName() method


/**
	Reset the m_title value with a meaningful name ONLY if it does not look like it was
	automatically generated in the first place.  If someone has reset it manually then leave it alone.

	Return a verbose description of what we've done (if anything) so that we can pop up a
	warning message to the operator letting them know.
 */
wxString CFixture::ResetTitle()
{
	if ( (m_title == GetTypeString()) ||
	     ((m_title.Find( _T("G5") ) != -1)))
	{
		// It has the default title.  Give it a name that makes sense.
		m_title = GenerateMeaningfulName();
		heeksCAD->Changed();

        wxString change;
		change << _("Changing name to ") << m_title << _T("\n");
		return( change );
	} // End if - then

	// Nothing changed, nothing to report
	return(_T(""));
} // End ResetTitle() method



/**
        This is the Graphics Library Commands (from the OpenGL set).  This method calls the OpenGL
        routines to paint the cutting tool in the graphics window.  The graphics is transient.
 */
void CFixture::glCommands(bool select, bool marked, bool no_color)
{

} // End glCommands() method


/**
	The Adjustment() method is the workhorse of this class.  This is where the coordinate
	data that's used to generate GCode is rotated to its new position based on the
	fixture's settings.  All the NC Operations classes should use this method to rotate
	their values immediately prior to generating GCode.
 */

gp_Pnt CFixture::Adjustment( const gp_Pnt & point ) const
{
	gp_Pnt transformed_point(point);

	gp_Trsf matrix = GetMatrix();

	transformed_point.Transform( matrix );

	return(transformed_point);
} // End Adjustment() method

gp_Pnt CFixture::Adjustment( double *point ) const
{
	gp_Pnt ref( point[0], point[1], point[2] );
	ref = Adjustment( ref );
	point[0] = ref.X();
	point[1] = ref.Y();
	point[2] = ref.Z();
	return(ref);
} // End Adjustment() method


void CFixture::extract(const gp_Trsf& tr, double *m)
{
	m[0] = tr.Value(1, 1);
	m[1] = tr.Value(1, 2);
	m[2] = tr.Value(1, 3);
	m[3] = tr.Value(1, 4);
	m[4] = tr.Value(2, 1);
	m[5] = tr.Value(2, 2);
	m[6] = tr.Value(2, 3);
	m[7] = tr.Value(2, 4);
	m[8] = tr.Value(3, 1);
	m[9] = tr.Value(3, 2);
	m[10] = tr.Value(3, 3);
	m[11] = tr.Value(3, 4);
	m[12] = 0;
	m[13] = 0;
	m[14] = 0;
	m[15] = 1;
}

#ifndef PI
#define PI 3.14159265358979323846
#endif

/**
	This is a single rotation matrix that allows transformations around the pivot point
	about the x,y and z axes by the various angles of rotation.
 */
gp_Trsf CFixture::GetMatrix() const
{
	gp_Dir x_direction( 1, 0, 0 );
	gp_Dir y_direction( 0, 1, 0 );
	gp_Dir z_direction( 0, 0, 1 );

	double yz_plane_angle_in_radians = (m_params.m_yz_plane / 360) * (2 * PI);	// degrees expressed in radians
	double xz_plane_angle_in_radians = (m_params.m_xz_plane / 360) * (2 * PI);	// degrees expressed in radians
	double xy_plane_angle_in_radians = (m_params.m_xy_plane / 360) * (2 * PI);	// degrees expressed in radians

	gp_Trsf yz_plane_rotation_matrix;
	yz_plane_rotation_matrix.SetRotation( gp_Ax1( m_params.m_pivot_point, x_direction), yz_plane_angle_in_radians );

	gp_Trsf xz_plane_rotation_matrix;
	xz_plane_rotation_matrix.SetRotation( gp_Ax1( m_params.m_pivot_point, y_direction), xz_plane_angle_in_radians );

	gp_Trsf xy_plane_rotation_matrix;
	xy_plane_rotation_matrix.SetRotation( gp_Ax1(m_params.m_pivot_point, z_direction), xy_plane_angle_in_radians );

	gp_Trsf matrix = yz_plane_rotation_matrix * xz_plane_rotation_matrix * xy_plane_rotation_matrix;

	return(matrix);
} // End GetMatrix() method


/**
	This class compares the angles between the vector formed by connecting the two points with the
	second vector passed in.  It returns the smallest angle by joining the two points in both orders.
 */
double CFixture::AxisAngle( const gp_Pnt & one, const gp_Pnt & two, const gp_Vec & pivot, const gp_Vec & axis ) const
{
	gp_Pnt lhs(one);
	gp_Pnt rhs(two);

	if (pivot.Angle( gp_Vec( gp_Pnt(0,0,0), gp_Pnt(0,0,1) ) ) < 0.0001)
	{
		// We're pivoting around the Z axis

		if (axis.Angle( gp_Vec( gp_Pnt(0,0,0), gp_Pnt(1,0,0) ) ) < 0.0001)
		{
			// We're comparing it with the X axis.
			if (lhs.X() > rhs.X())
			{
				gp_Pnt temp(lhs);
				lhs = rhs;
				rhs = temp;
			}

			double angle = axis.Angle( gp_Vec( lhs, rhs ) );
			if (lhs.Y() > rhs.Y()) angle *= -1.0;
			return((angle / (2 * PI)) * 360.0);
		}
		else if (axis.Angle( gp_Vec( gp_Pnt(0,0,0), gp_Pnt(0,1,0) ) ) < 0.0001)
		{
			// We're comparing it with the Y axis.
			if (lhs.Y() > rhs.Y())
			{
				gp_Pnt temp(lhs);
				lhs = rhs;
				rhs = temp;
			}

			double angle = axis.Angle( gp_Vec( lhs, rhs ) );
			if (lhs.X() < rhs.X()) angle *= -1.0;
			return((angle / (2 * PI)) * 360.0);
		}
	} // End if - then
	else if (pivot.Angle( gp_Vec( gp_Pnt(0,0,0), gp_Pnt(0,1,0) ) ) < 0.0001)
	{
		// We're pivoting around the Y axis

		if (axis.Angle( gp_Vec( gp_Pnt(0,0,0), gp_Pnt(1,0,0) ) ) < 0.0001)
		{
			// We're comparing it with the X axis.
			if (lhs.X() > rhs.X())
			{
				gp_Pnt temp(lhs);
				lhs = rhs;
				rhs = temp;
			}

			double angle = axis.Angle( gp_Vec( lhs, rhs ) );
			if (lhs.Z() > rhs.Z()) angle *= -1.0;
			return((angle / (2 * PI)) * 360.0);
		}
		else if (axis.Angle( gp_Vec( gp_Pnt(0,0,0), gp_Pnt(0,0,1) ) ) < 0.0001)
		{
			// We're comparing it with the Z axis.
			if (lhs.Y() > rhs.Y())
			{
				gp_Pnt temp(lhs);
				lhs = rhs;
				rhs = temp;
			}

			double angle = axis.Angle( gp_Vec( lhs, rhs ) );
			if (lhs.X() < rhs.X()) angle *= -1.0;
			return((angle / (2 * PI)) * 360.0);
		}
	} // End if - then

	return(-1);
}

void CFixture::SetRotationsFromProbedPoints( const wxString & probed_points_xml_file_name )
{
	TiXmlDocument xml;
	if (! xml.LoadFile( Ttc(probed_points_xml_file_name.c_str()) ))
	{
		printf("Failed to load XML file '%s'\n", Ttc(probed_points_xml_file_name.c_str()) );
	} // End if - then
	else
	{
		TiXmlElement *root = xml.RootElement();
		if (root != NULL)
		{
			std::vector<CNCPoint> points;

			for(TiXmlElement* pElem = TiXmlHandle(root).FirstChildElement().Element(); pElem; pElem = pElem->NextSiblingElement())
			{
				CNCPoint point(0,0,0);
				for(TiXmlElement* pPoint = TiXmlHandle(pElem).FirstChildElement().Element(); pPoint; pPoint = pPoint->NextSiblingElement())
				{
					std::string name(pPoint->Value());

					if (name == "X") point.SetX( atof(pPoint->GetText()) );
					if (name == "Y") point.SetY( atof(pPoint->GetText()) );
					if (name == "Z") point.SetZ( atof(pPoint->GetText()) );
				} // End for

				points.push_back(point);
			} // End for

			while (points.size() > 0)
			{
				if (points.size() >= 3)
				{
					double reference_rise;
					double reference_run;

					// rotation around Z
					reference_rise = points[2].Y();
					reference_run = points[2].X();

					double xy_reference = (atan2( reference_rise, reference_run ) / (2 * PI)) * 360.0;

					// rotation around Y
					reference_rise = points[2].Z();
					reference_run = points[2].X();

					double xz_reference = (atan2( reference_rise, reference_run ) / (2 * PI)) * 360.0;

					// rotation around X
					reference_rise = points[2].Z();
					reference_run = points[2].Y();

					double yz_reference = (atan2( reference_rise, reference_run ) / (2 * PI)) * 360.0;
					double measured_rise;
					double measured_run;

					// rotation around Z
					if ((points[2].X() != 0.0) || (points[2].Y() != 0.0))
					{
						measured_rise = points[1].Y() - points[0].Y();
						measured_run = points[1].X() - points[0].X();

						m_params.m_xy_plane = (atan2( measured_rise, measured_run ) / (2 * PI)) * 360.0;
						m_params.m_xy_plane -= xy_reference;
					}

					// rotation around Y
					if ((points[2].X() != 0.0) || (points[2].Z() != 0.0))
					{
						measured_rise = points[1].Z() - points[0].Z();
						measured_run = points[1].X() - points[0].X();

						m_params.m_xz_plane = (atan2( measured_rise, measured_run ) / (2 * PI)) * 360.0;
						m_params.m_xz_plane -= xz_reference;
					}

					// rotation around X
					if ((points[2].Y() != 0.0) || (points[2].Z() != 0.0))
					{
						measured_rise = points[1].Z() - points[0].Z();
						measured_run = points[1].Y() - points[0].Y();

						m_params.m_yz_plane = (atan2( measured_rise, measured_run ) / (2 * PI)) * 360.0;
						m_params.m_yz_plane -= yz_reference;
					}

					// Erase these three points
					points.erase(points.begin());
					points.erase(points.begin());
					points.erase(points.begin());
				} // End if - then
				else
				{
					points.erase(points.begin(), points.end());	// They're not in a group of three so ignore them.
				}
			} // End while
		} // End if - then
	} // End if - else
} // End SetRotationsFromProbedPoints() method


class Fixture_ImportProbeData: public Tool
{

CFixture *m_pThis;

public:
	Fixture_ImportProbeData() { m_pThis = NULL; }

	// Tool's virtual functions
	const wxChar* GetTitle(){return _("Import probe data");}

	void Run()
	{
		// Prompt the user to select a file to import.
		wxFileDialog fd(heeksCAD->GetMainFrame(), _T("Select a file to import"), _T("."), _T(""),
				wxString(_("Known Files")) + _T(" |*.xml;*.XML;")
					+ _T("*.Xml;"),
					wxOPEN | wxFILE_MUST_EXIST );
		fd.SetFilterIndex(1);
		if (fd.ShowModal() == wxID_CANCEL) return;
		m_pThis->SetRotationsFromProbedPoints( fd.GetPath().c_str() );
	}
	wxString BitmapPath(){ return _T("import");}
	wxString previous_path;
	void Set( CFixture *pThis ) { m_pThis = pThis; }
};

static Fixture_ImportProbeData import_probe_data;

void CFixture::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{

	import_probe_data.Set( this );

	t_list->push_back( &import_probe_data );
}


bool CFixture::operator== ( const CFixture & rhs ) const
{
    return(m_coordinate_system_number == rhs.m_coordinate_system_number);
}

bool CFixture::operator!= ( const CFixture & rhs ) const
{
    return(! (m_coordinate_system_number == rhs.m_coordinate_system_number));
}


