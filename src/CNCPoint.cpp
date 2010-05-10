// CNCPoint.cpp
/*
 * Copyright (c) 2009, Dan Heeks, Perttu Ahola
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "stdafx.h"
#include "CNCPoint.h"
#include "interface/HeeksCADInterface.h"
#include "Program.h"

class CProgram;

extern CHeeksCADInterface* heeksCAD;


CNCPoint::CNCPoint() : gp_Pnt(0.0, 0.0, 0.0)
{
    tolerance = heeksCAD->GetTolerance();
}

CNCPoint::CNCPoint( const double *xyz ) : gp_Pnt(xyz[0], xyz[1], xyz[2])
{
    tolerance = heeksCAD->GetTolerance();
}

CNCPoint::CNCPoint( const double &x, const double &y, const double &z ) : gp_Pnt(x,y,z)
{
    tolerance = heeksCAD->GetTolerance();
}
CNCPoint::CNCPoint( const gp_Pnt & rhs ) : gp_Pnt(rhs)
{
    tolerance = heeksCAD->GetTolerance();
}

double CNCPoint::X(const bool in_drawing_units /* = false */) const
{
    if (in_drawing_units == false) return(gp_Pnt::X());
    else return(gp_Pnt::X() / theApp.m_program->m_units);
}

double CNCPoint::Y(const bool in_drawing_units /* = false */) const
{
    if (in_drawing_units == false) return(gp_Pnt::Y());
    else return(gp_Pnt::Y() / theApp.m_program->m_units);
}

double CNCPoint::Z(const bool in_drawing_units /* = false */) const
{
    if (in_drawing_units == false) return(gp_Pnt::Z());
    else return(gp_Pnt::Z() / theApp.m_program->m_units);
}

CNCPoint & CNCPoint::operator+= ( const CNCPoint & rhs )
{
    SetX( X() + rhs.X() );
    SetY( Y() + rhs.Y() );
    SetZ( Z() + rhs.Z() );

    return(*this);
}

CNCPoint CNCPoint::operator- ( const CNCPoint & rhs ) const
{
    CNCPoint result(*this);
    result.SetX( X() - rhs.X() );
    result.SetY( Y() - rhs.Y() );
    result.SetZ( Z() - rhs.Z() );

    return(result);
}

bool CNCPoint::operator==( const CNCPoint & rhs ) const
{
    // We use the sum of both point's tolerance values.
    return(Distance(rhs) < (tolerance + rhs.tolerance));
} // End equivalence operator

bool CNCPoint::operator!=( const CNCPoint & rhs ) const
{
    return(! (*this == rhs));
} // End not-equal operator

bool CNCPoint::operator<( const CNCPoint & rhs ) const
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

void CNCPoint::ToDoubleArray( double *pArrayOfThree ) const
{
    pArrayOfThree[0] = X();
    pArrayOfThree[1] = Y();
    pArrayOfThree[2] = Z();
} // End ToDoubleArray() method




CNCVector::CNCVector() : gp_Vec(0.0, 0.0, 0.0)
{
    tolerance = heeksCAD->GetTolerance();
}

CNCVector::CNCVector( const double *xyz ) : gp_Vec(xyz[0], xyz[1], xyz[2])
{
    tolerance = heeksCAD->GetTolerance();
}
CNCVector::CNCVector( const double &x, const double &y, const double &z ) : gp_Vec(x,y,z)
{
    tolerance = heeksCAD->GetTolerance();
}

CNCVector::CNCVector( const gp_Vec & rhs ) : gp_Vec(rhs)
{
    tolerance = heeksCAD->GetTolerance();
}

bool CNCVector::operator==( const CNCVector & rhs ) const
{
    return(this->IsEqual(rhs, tolerance, tolerance) == Standard_True);
} // End equivalence operator

bool CNCVector::operator!=( const CNCVector & rhs ) const
{
    return(! (*this == rhs));
} // End not-equal operator

bool CNCVector::operator<( const CNCVector & rhs ) const
{
    for (int offset=1; offset <=3; offset++)
    {
        if (fabs(Coord(offset) - rhs.Coord(offset)) < tolerance) continue;

        if (Coord(offset) > rhs.Coord(offset)) return(false);
        if (Coord(offset) < rhs.Coord(offset)) return(true);
    }

    return(false);	// They're equal
} // End equivalence operator








