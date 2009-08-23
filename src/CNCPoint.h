// CNCPoint.h
/*
 * Copyright (c) 2009, Dan Heeks, Perttu Ahola
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#pragma once

#include <list>
#include <vector>

#include "gp_Pnt.hxx"


/**
	This is simply a wrapper around the gp_Pnt class from the OpenCascade library
	that allows objects of this class to be used with methods such as std::sort() etc.
 */
class CNCPoint : public gp_Pnt {
public:
	CNCPoint() : gp_Pnt(0.0, 0.0, 0.0) { }
	CNCPoint( const double *xyz ) : gp_Pnt(xyz[0], xyz[1], xyz[2]) { }
	CNCPoint( const double &x, const double &y, const double &z ) : gp_Pnt(x,y,z) { }
	CNCPoint( const gp_Pnt & rhs ) : gp_Pnt(rhs) { }

	CNCPoint & operator+= ( const CNCPoint & rhs )
	{
		SetX( X() + rhs.X() );
		SetY( Y() + rhs.Y() );
		SetZ( Z() + rhs.Z() );

		return(*this);
	}

	bool operator==( const CNCPoint & rhs ) const
	{
		if (X() != rhs.X()) return(false);
		if (Y() != rhs.Y()) return(false);
		if (Z() != rhs.Z()) return(false);

		return(true);
	} // End equivalence operator

	bool operator!=( const CNCPoint & rhs ) const
	{
		return(! (*this == rhs));
	} // End not-equal operator

	bool operator<( const CNCPoint & rhs ) const
	{
		if (X() > rhs.X()) return(false);
		if (X() < rhs.X()) return(true);
		if (Y() > rhs.Y()) return(false);
		if (Y() < rhs.Y()) return(true);
		if (Z() > rhs.Z()) return(false);
		if (Z() < rhs.Z()) return(true);

		return(false);	// They're equal
	} // End equivalence operator
}; // End CNCPoint class definition.



/**
	By defining a structure that inherits from std::binary_function and has an operator() method, we
	can use this class to sort lists or vectors of CNCPoint objects.  We will do this, initially, to
	sort points of NC operations so as to minimize rapid travels.

	The example code to call this would be;
	    std::vector<CNCPoint> points;		// Some container of CNCPoint objects
		points.push_back(CNCPoint(3,4,5));	// Populate it with good data
		points.push_back(CNCPoint(6,7,8));

		for (std::vector<CNCPoint>::iterator l_itPoint = points.begin(); l_itPoint != points.end(); l_itPoint++)
		{
			std::vector<CNCPoint>::iterator l_itNextPoint = l_itPoint;
			l_itNextPoint++;

			if (l_itNextPoint != points.end())
			{
				sort_points_by_distance compare( *l_itPoint );
				std::sort( l_itNextPoint, points.end(), compare );
			} // End if - then
		} // End for
 */
struct sort_points_by_distance : public std::binary_function< const CNCPoint &, const CNCPoint &, bool >
{
	sort_points_by_distance( const CNCPoint & reference_point )
	{
		m_reference_point = reference_point;
	} // End constructor

	CNCPoint m_reference_point;

	// Return true if dist(lhs to ref) < dist(rhs to ref)
	bool operator()( const CNCPoint & lhs, const CNCPoint & rhs ) const
	{
		return( lhs.Distance( m_reference_point ) < rhs.Distance( m_reference_point ) );
	} // End operator() overload
}; // End sort_points_by_distance structure definition.




