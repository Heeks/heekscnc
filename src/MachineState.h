// MachineState.h
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#pragma once

#include "PythonStuff.h"
#include "CNCPoint.h"

#include <map>

class Python;
class CNCPoint;
class CAttachOp;

/**
    The CMachineState class stores information about the machine for use
    in the GCode generation routines.  An object of this class will be passed
    both into and back out of each gcode generation routine so that
    subsequent routines can know whether tool changes or fixture changes
    are either necessary or have occured.

    Location values returned also allow subsequent gcode generation routines
    to sort their objects so that starting points can be selected based on
    previous operation's ending points.

	This class also keeps track of which objects have had their gcode generated
	for which fixtures.  We need to know this so that we don't double-up while
	we are handling the various private and public fixture settings.
 */
class CMachineState
{
private:
	/**
		This class remembers an individual machine operation along with
		the fixture used for gcode generation.  It's really just a placeholder
		for a tuple of three values so we can keep track of what we've already
		processed.
	 */
public:
	CAttachOp* m_attached_to_surface;

	CMachineState();
    ~CMachineState();

    CMachineState(const CMachineState & rhs);
    CMachineState & operator= ( const CMachineState & rhs );

    int Tool() const { return(m_tool_number); }
    Python Tool( const int new_tool );

    CNCPoint Location() const { return(m_location); }
    void Location( const CNCPoint rhs ) { m_location = rhs; }

    bool operator== ( const CMachineState & rhs ) const;
    bool operator!= ( const CMachineState & rhs ) const { return(! (*this == rhs)); }

private:
    int         m_tool_number;
    CNCPoint      m_location;
	bool m_drag_knife_on;

}; // End CMachineState class definition
