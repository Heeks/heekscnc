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
#include "gp_Vec.hxx"

extern CHeeksCADInterface* heeksCAD;

/**
	This is simply a wrapper around the gp_Pnt class from the OpenCascade library
	that allows objects of this class to be used with methods such as std::sort() etc.
 */
class CNCPoint : public gp_Pnt {
public:
	CNCPoint() : gp_Pnt(0.0, 0.0, 0.0)
	{
		tolerance = heeksCAD->GetTolerance();
	}
	CNCPoint( const double *xyz ) : gp_Pnt(xyz[0], xyz[1], xyz[2])
	{
		tolerance = heeksCAD->GetTolerance();
	}
	CNCPoint( const double &x, const double &y, const double &z ) : gp_Pnt(x,y,z)
	{
		tolerance = heeksCAD->GetTolerance();
	}
	CNCPoint( const gp_Pnt & rhs ) : gp_Pnt(rhs)
	{
		tolerance = heeksCAD->GetTolerance();
	}

	double X(const bool in_drawing_units = false) const
	{
		if (in_drawing_units == false) return(gp_Pnt::X());
		else return(gp_Pnt::X() / theApp.m_program->m_units);
	}

	double Y(const bool in_drawing_units = false) const
	{
		if (in_drawing_units == false) return(gp_Pnt::Y());
		else return(gp_Pnt::Y() / theApp.m_program->m_units);
	}

	double Z(const bool in_drawing_units = false) const
	{
		if (in_drawing_units == false) return(gp_Pnt::Z());
		else return(gp_Pnt::Z() / theApp.m_program->m_units);
	}

	CNCPoint & operator+= ( const CNCPoint & rhs )
	{
		SetX( X() + rhs.X() );
		SetY( Y() + rhs.Y() );
		SetZ( Z() + rhs.Z() );

		return(*this);
	}

	CNCPoint operator- ( const CNCPoint & rhs ) const
	{
		CNCPoint result(*this);
		result.SetX( X() - rhs.X() );
		result.SetY( Y() - rhs.Y() );
		result.SetZ( Z() - rhs.Z() );

		return(result);
	}

	bool operator==( const CNCPoint & rhs ) const
	{
		// We use the sum of both point's tolerance values.
		return(Distance(rhs) < (tolerance + rhs.tolerance));
	} // End equivalence operator

	bool operator!=( const CNCPoint & rhs ) const
	{
		return(! (*this == rhs));
	} // End not-equal operator

	bool operator<( const CNCPoint & rhs ) const
	{
		if (*this == rhs) return(false);

		if (fabs(X() - rhs.X()) > tolerance)
		{
			if (X() > rhs.X()) return(false);
			if (X() < rhs.X()) return(true);
		}

		if (fabs(Y() - rhs.Y()) > tolerance)
		{
			if (Y() > rhs.Y()) return(false);
			if (Y() < rhs.Y()) return(true);
		}

		if (fabs(Z() - rhs.Z()) > tolerance)
		{
			if (Z() > rhs.Z()) return(false);
			if (Z() < rhs.Z()) return(true);
		}

		return(false);	// They're equal
	} // End equivalence operator

	void ToDoubleArray( double *pArrayOfThree ) const
	{
		pArrayOfThree[0] = X();
		pArrayOfThree[1] = Y();
		pArrayOfThree[2] = Z();
	} // End ToDoubleArray() method

private:
	double tolerance;
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



struct sort_points_by_z : public std::binary_function< const CNCPoint &, const CNCPoint &, bool >
{
	bool operator()( const CNCPoint & lhs, const CNCPoint & rhs ) const
	{
		return( lhs.Z() < rhs.Z() );
	} // End operator() overload
}; // End sort_points_by_z structure definition.



/**
	This is simply a wrapper around the gp_Pnt class from the OpenCascade library
	that allows objects of this class to be used with methods such as std::sort() etc.
 */
class CNCVector : public gp_Vec {
public:
	CNCVector() : gp_Vec(0.0, 0.0, 0.0)
	{
		tolerance = heeksCAD->GetTolerance();
	}

	CNCVector( const double *xyz ) : gp_Vec(xyz[0], xyz[1], xyz[2])
	{
		tolerance = heeksCAD->GetTolerance();
	}
	CNCVector( const double &x, const double &y, const double &z ) : gp_Vec(x,y,z)
	{
		tolerance = heeksCAD->GetTolerance();
	}

	CNCVector( const gp_Vec & rhs ) : gp_Vec(rhs)
	{
		tolerance = heeksCAD->GetTolerance();
	}

	bool operator==( const CNCVector & rhs ) const
	{
		return(this->IsEqual(rhs, tolerance, tolerance) == Standard_True);
	} // End equivalence operator

	bool operator!=( const CNCVector & rhs ) const
	{
		return(! (*this == rhs));
	} // End not-equal operator

	bool operator<( const CNCVector & rhs ) const
	{
		for (int offset=1; offset <=3; offset++)
		{
			if (fabs(Coord(offset) - rhs.Coord(offset)) < tolerance) continue;

			if (Coord(offset) > rhs.Coord(offset)) return(false);
			if (Coord(offset) < rhs.Coord(offset)) return(true);
		}

		return(false);	// They're equal
	} // End equivalence operator

private:
	double tolerance;
}; // End CNCVector class definition.






