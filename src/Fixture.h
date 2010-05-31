
// Fixture.h
/*
 * Copyright (c) 2009, Dan Heeks, Perttu Ahola
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

 #pragma once

#include "tinyxml/tinyxml.h"
#include "HeeksCNCTypes.h"
#include "interface/Property.h"

#include <gp_Pnt.hxx>

#include <list>
#include <vector>
#include <algorithm>

class CFixture;
class Python;

class CFixtureParams {
public:
	double m_yz_plane;	// i.e. rotation angle around x axis - in degrees
	double m_xz_plane;	// i.e. rotation angle around y axis - in degrees
	double m_xy_plane;	// i.e. rotation angle around z axis - in degrees

	gp_Pnt m_pivot_point;	// Fixture's pivot point for rotation.

	bool m_safety_height_defined;
	double m_safety_height;

	bool m_touch_off_point_defined;	// Is the m_touch_off_point valid?
	gp_Pnt m_touch_off_point;	// Coordinate in the local coordinate system for safe starting point
								// when switching to this fixture.
	wxString m_touch_off_description;	// Tell the operator what to do when setting up this fixture.
										// eg: "touch off 0,0,0 at bottom left corner".

	CFixtureParams()
	{
		m_yz_plane = 0.0;
		m_xz_plane = 0.0;
		m_xy_plane = 0.0;

		m_pivot_point = gp_Pnt( 0.0, 0.0, 0.0 );

		m_safety_height_defined = false;
		m_safety_height = 0.0;

		m_touch_off_point = gp_Pnt(0.0, 0.0, 0.0);
		m_touch_off_description = _T("");
	} // End constructor.

	void set_initial_values(const bool safety_height_defined, const double safety_height);
	void write_values_to_config();
	void GetProperties(CFixture* parent, std::list<Property *> *list);
	void WriteXMLAttributes(TiXmlNode* pElem);
	void ReadParametersFromXMLElement(TiXmlElement* pElem);

	wxString ConfigScope() const { return(_("Fixture")); }
};

/**
	The Fixture class represents a vice or similar on the physical milling machine.  It has
	an origin in terms of a local coordinate system (G54, G55 etc.) and it is sometimes
	bolted down crooked.  This class supports the definition of which coordinate system
	the fixture is using as well as how crooked it is with respect to the three axes.  The
	adjustment is made in the A, B and C axes.  These are defined as rotations around
	X, Y and Z respectively.  To this end, the most common use of this class will be to
	assign a rotation angle to the C axis.  This represents that, although the part is
	nicely lined up with the Z axis, it has been rotated such that it doesn't lay along
	the X and Y axes.

	Eventually, I would like to add a pyVCP (Python Visual Control Panel) script that
	helps to drive a touch probe.  The coordinates from the touch probe are stored in an
	ASCII file.  I would like this class to read in this file in order to determine the
	axis rotation values without the operator having to do the calculations themselves.  A
	programmer can dream anyway.

	It should be noted that this class does not produce any rotational axis GCode.  It
	is really meant to align the GCode with a workpiece that is not straight.
 */
class CFixture: public HeeksObj {
public:
	CFixtureParams m_params;
        wxString m_title;

	typedef enum {
		G54 = 1,
		G55,
		G56,
		G57,
		G58,
		G59,
		G59_1,
		G59_2,
		G59_3
	} eCoordinateSystemNumber_t;

	friend wxString & operator << ( wxString & ss, const eCoordinateSystemNumber_t & coordinate_system )
	{
		switch (coordinate_system)
		{
		case G54:	ss << _T("G54");
			break;

		case G55:	ss << _T("G55");
			break;

		case G56:	ss << _T("G56");
			break;

		case G57:	ss << _T("G57");
			break;

		case G58:	ss << _T("G58");
			break;

		case G59:	ss << _T("G59");
			break;

		case G59_1:	ss << _T("G59.1");
			break;

		case G59_2:	ss << _T("G59.2");
			break;

		case G59_3:	ss << _T("G59.3");
			break;
		} // End switch()

		return(ss);
	}

	eCoordinateSystemNumber_t m_coordinate_system_number;

	//	Constructors.
	CFixture(const wxChar *title, 
			const eCoordinateSystemNumber_t coordinate_system_number,
			const bool safety_height_defined, 
			const double safety_height )
				: m_coordinate_system_number(coordinate_system_number)
	{
		m_params.set_initial_values(safety_height_defined, safety_height);
		if (title != NULL)
		{
			m_title = title;
		} // End if - then
		else
		{
			m_title = GenerateMeaningfulName();
		} // End if - else
	} // End constructor

	bool operator< ( const CFixture & rhs ) const
	{
	    return(m_coordinate_system_number < rhs.m_coordinate_system_number);
	}

	 // HeeksObj's virtual functions
        int GetType()const{return FixtureType;}
	const wxChar* GetTypeString(void) const{ return _T("Fixture"); }
        HeeksObj *MakeACopy(void)const;

        void WriteXML(TiXmlNode *root);
        static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);

	// program whose job is to generate RS-274 GCode.
	Python AppendTextToProgram() const;

	void GetProperties(std::list<Property *> *list);
	void CopyFrom(const HeeksObj* object);
	bool CanAddTo(HeeksObj* owner);
	const wxBitmap &GetIcon();
    const wxChar* GetShortString(void)const{return m_title.c_str();}
	void glCommands(bool select, bool marked, bool no_color);

    bool CanEditString(void)const{return true;}
    void OnEditString(const wxChar* str);

	// static CFixture *Find( const eCoordinateSystemNumber_t coordinate_system_number );
	// static int GetNextFixture();

	wxString GenerateMeaningfulName() const;
	wxString ResetTitle();

	gp_Pnt Adjustment( const gp_Pnt & point ) const;
	gp_Pnt Adjustment( double *point ) const;

	static void extract(const gp_Trsf& tr, double *m);
	gp_Trsf GetMatrix() const;

	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);
	void SetRotationsFromProbedPoints( const wxString & probed_points_xml_file_name );
	double AxisAngle( const gp_Pnt & one, const gp_Pnt & two, const gp_Vec & pivot, const gp_Vec & axis ) const;

	bool operator== ( const CFixture & rhs ) const;
	bool operator!= ( const CFixture & rhs ) const;

}; // End CFixture class definition.


